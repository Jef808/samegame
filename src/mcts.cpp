// mcts.cpp
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <random>
#include <spdlog/spdlog.h>
#include "mcts.h"
//#include "debug.h"


using namespace sg;

namespace mcts {

std::ostream& operator<<(std::ostream& _out, const mcts::Agent& agent) {
  return _out << agent.debug_tree_stats();
}

//************************** Node Lookup Table****************************/

MCTSLookupTable MCTS;

// get_node queries the Lookup Table until it finds the node with given position,
// or creates a new entry in case it doesn't find it.
Node* Agent::get_node()
{
    //state.generate_clusters();
    Key state_key = state.key();

    auto node_it = MCTS.find(state_key);
    if (node_it != MCTS.end())
        return &(node_it->second);

    // Insert the new node in the Hash table if it wasn't found.
    Node new_node { };
    new_node.key = state_key;
    if (ply > 1) {
        new_node.parent = actions[ply-1];
    }

    auto new_node_it = MCTS.insert(std::make_pair(state_key, new_node)).first;
    return &(new_node_it->second);
}

// Used for choosing actions during the random_simulations.
namespace Random {

    std::random_device rd;
    std::mt19937 e{rd()}; // or std::default_random_engine e{rd()};
    std::uniform_int_distribution<int> dist{0, MAX_CHILDREN};

    Action choose(const ActionVec& choices)
    {
        if (choices.empty())
            return ACTION_NONE;
        return choices[dist(rd) % choices.size()];
    }

    ClusterData choose(const ActionDVec& choices)
    {
        if (choices.empty())
            return ClusterData { ACTION_NONE, COLOR_NONE, 0 };
        return choices[dist(rd) % choices.size()];
    }
}

std::ostream& operator<<(std::ostream& _out, const mcts::Edge& edge)
{
    std::stringstream ss;
    ss << "Rep = " << std::to_string(edge.cd.rep) << ", n_visits = " << edge.n_visits << ", value_assured = " << std::to_string(edge.value_assured) << ", avg_value = " << edge.reward_avg_visit;
    return _out << ss.str();
}

std::ostream& operator<<(std::ostream& _out, const ::mcts::Node& node)
{
    std::stringstream ss;
    ss << "key = " << std::to_string(node.key) << ", n_visits = " << node.n_visits << ", n_children = " << node.n_children;
    return _out << ss.str();
}

//******************************* Time management *************************/

using TimePoint = std::chrono::milliseconds::rep;

std::chrono::time_point<std::chrono::steady_clock> search_start;

void init_time()
{
    search_start = std::chrono::steady_clock::now();
}

TimePoint time_elapsed()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now() - search_start).count();
}


//******************************** Ctor(s) *******************************/

Agent::Agent(State& state)
    : state(state)
    , value_global_max(std::numeric_limits<double>::min())
    , value_global_min(std::numeric_limits<double>::max())
{
    //create_root();
}


//******************************** Main methods ***************************/

ClusterData Agent::MCTSBestAction()
{
    init_time();
    create_root();

    while (computation_resources())
    {
        Node* node = tree_policy();
        Reward reward = rollout_policy(node);
        backpropagate(node, reward);
        ++iteration_cnt;

        // if (Debug::debug_main_methods) {
        //     std::cerr << iteration_cnt << ": tree_policy chose\n";
        //     std::cerr << state.display<std::ostream>(std::cerr);
        //     std::cerr << "\nAnd rollout gave reward of " << std::fixed << reward << std::endl;
        // }
    }

    // if (Debug::debug_main_methods) {
    //     std::cerr << iteration_cnt << " number of iterations hit. Wrapping up..." << std::endl;
    // }

    // if (Debug::debug_counters)
    // {
    //     std::cerr << "Iterations: " << iteration_cnt << '\n';
    //     std::cerr << "Descent count: " << descent_cnt << '\n';
    //     std::cerr << "Rollout count: " << cnt_simulations << '\n';
    //     std::cerr << "Exploration count: " << explored_nodes_cnt << '\n';
    //     std::cerr << "Number of nodes in table: " << MCTS.size() << std::endl;
    // }

    auto* choice = best_visits(root);

    // if (Debug::debug_main_methods)
    //     std::cerr << "returning from main method" << std::endl;

    return choice->cd;
}

void Agent::print_final_score() const
{
for (int i=0; i<root->n_children; ++i)
    {
        auto edges = root->children_list();
        std::cerr << "Action " << (int)edges[i].cd.rep << ":\n";
        std::cerr << "Average value " << (int)edges[i].reward_avg_visit << '\n';
        std::cerr << "Best value " << (int)edges[i].value_assured << '\n';
        std::cerr << "Number of visits " << edges[i].n_visits << '\n';
    }
}
void Agent::create_root()
{
    if (use_time) {
        init_time();
    }

    // Reset the search buffers (or 0 initialize)
    nodes = {};
    actions = {};
    states = {};
    stack = {};

    // Reset the counters
    ply                 = 1;
    iteration_cnt       = 0;
    cnt_simulations     = 0;
    descent_cnt         = 0;
    explored_nodes_cnt  = 0;
    cnt_new_nodes       = 0;

    // NOTE : states[ply=1] will contains the result of applying action stack[ply].action, on calling
    // apply_action for the first time, (and subsequently etc...), the state points state[ply=1]'s
    // previous* pointer to the initial state, before applying the action.
    // NOTE : It is the state's responsibility to update states[ply], we only pass it
    // a (ref to a) StateData object when asking for an apply_action(), and the undo_action() reverts the
    // state to state[ply]->previous (not states[ply-1]... the root state is not even stored in there!)

    state.generate_clusters();
    root = nodes[ply] = get_node();
    root->key = state.key();

    if (root->n_visits == 0) {
        init_children();
    }
}

bool Agent::computation_resources()
{
    bool res = iteration_cnt < MAX_ITER;

    if (use_time)
        res &= (time_elapsed() < MAX_TIME - 100);    // NOTE: Clock keeps running during I/O when testing

    return res;
}

Node* Agent::tree_policy()
{
    assert(current_node() == root);

    // TODO Should only check this once, not every time we make a descent...
    if (root->n_children == 0)
    {
        return root;
    }

    state.generate_clusters();    // NOTE: For subsequent iterations of the MCTS algorithm,
                                  // when a bunch of undo_actions() have just been done.
                                  // TODO: Since this is always starting at the root, we might
                                  // as well save the clusters configuration at the root for the
                                  // whole thing. (It's already kind of encoded into the root's children... )

    // At each previously explored node, choose an action in the direction that
    // is "most important" to sample in the tree. (i.e. minimizing regret in long
    // or short term)
    while (current_node()->n_visits > 0)
    {
        if (is_terminal(current_node()))
        {
            // if (Debug::debug_tree) {
            //     std::cerr << "Terminal node hit.\n";
            // }

            return current_node();
        }

        // The choice of Edge (action) at each node is driven by the uct policy
        actions[ply] = best_ucb(current_node());

        ++nodes[ply]->n_visits;

        // Keep record of number of times each part of the tree has been sampled.

        //++current_node()->n_visits;

        ClusterData cd = actions[ply]->cd;  // Keep record of the path we're tracing to go back along it.
        assert(cd.rep != ACTION_NONE);       // TODO Remove this

        apply_action(cd);

        nodes[ply] = get_node();    // Either the node has been seen and is associated to a state key,
                                         // or not and get_node creates a record of it.


        // if (nodes[ply]->n_visits == 0) {
        //     ++cnt_new_nodes;
        //     nodes[ply]->parent = actions[ply];
        // }

        // if (Debug::debug_tree) {
        //     std::cerr << "Descent " << descent_cnt << '\n';
        //     auto& children = nodes[ply-1]->children_list();
        //     std::cerr << "Choices of actions: ";
        //     for (int i=0; i<current_node()->n_children; ++i) {
        //         std::cerr << children[i];
        //     }

        //     std::cerr << "Action chosen: " << (int)actions[ply]->cd.rep;
        //     std::cerr << std::endl;
        // }

        // TODO How to deal with different paths (different nodes in the tree) leading
        // to equivalent state position? The state key will be the same but not the nodes.
        // Have to make sure to only keep the best path and reorganize the tree without losing information.
        // NOTE in this direction : currently the last_move field of a Node object isn't used! it can have
        // multiple parents. This most likely messes with the algorithm by favoring positions that arise
        // more frequenty.
    }


    assert (current_node()->n_visits == 0);

    current_node()->parent = actions[ply-1];
    // At this point, we reached an unexplored node and it is time to explore the game tree from
    // there.

    ++descent_cnt;
    return current_node();
}

// Once we have the next unexplored node, we expand it and do a random
// playout starting with every one of its children.
Reward Agent::rollout_policy(Node* node)
{
    assert(node == current_node());

    if (node->n_visits > 0 && is_terminal(node))
    {
        return node->parent->value_sg_from_root + evaluate_terminal();
    }

    // Set the parent to the last action taken
    // node->parent = actions[ply-1];

    assert(node->n_visits == 0);

    init_children();         // Expand the node and do a rollout on each child (if any)

    Reward rollout_reward = 0;

    // NOTE: Only after init_children() can we can tell that the node is terminal, since
    //       is_terminal() returns n_visits > 1 && n_children == 0;
    if (node->n_children == 0) {
        rollout_reward = node->parent->value_sg_from_root + evaluate_terminal();
    } else {
        rollout_reward = node->children[0].value_assured;
    }

    // In order to keep rewards scaled between 0 and 1 while backpropagating
    if (rollout_reward > value_global_max)
    {
        value_global_max = rollout_reward;
    }
    if (rollout_reward < value_global_min)
    {
        value_global_min = rollout_reward;
    }

    return rollout_reward;
}

// Same as Stockfish's function but rescaled and translated to fit the observed values of this game.
Reward Agent::value_to_reward(double v)
{
    auto M = value_global_max;
    auto m = value_global_min;

    const double c = -0.00490739829861 * 2.0 / (M - m);

    auto res = 1.0 / ( 1+std::exp(c * (v-(M-m)/2.0))  );
    assert(res > -0.00001 && res < 1.00001);

    return res;
}

// The inverse
double Agent::reward_to_value(Reward r)
{
    auto M = value_global_max;
    auto m = value_global_min;

    const Reward g = 203.77396313709564 * (M - m) / 2.0;  // 1/k

    return g * std::log(r / (1.0 - r)) + (M - m)/2.0;
}

// Backpropagate the rollout reward r, that is the best value seen after running a
// random playout from each child of node.
//
// Let's start by rescaling the reward linearly before using the sharper curves above
void Agent::backpropagate(Node* node, Reward r)
{
    assert(node == current_node());

    while (ply > 1)
    {
        undo_action();                        // Ok because we also saved the actions in the search stack

        Edge* edge = actions[ply];

        // r = (nb_players - 1) * 1.0 - r;    // (For 2-player minimax : Undoing action changes player)

        if (r > edge->value_assured)
        {
            edge->value_assured = r;
        }

        auto& av = edge->reward_avg_visit;
        auto& n  = edge->n_visits;


        // Given a value to backpropagate (between value_global_min and value_global_max), we // add it to the
        // // "total reward" then rescale to [0, 1] linearly depending on observed upper & lower bounds to date
        av = ((av * n + r) - value_global_min) / (value_global_max - value_global_min);

        //av = av * ((value_global_max - value_global_min) + value_global_min) * n + r - value_global_min;

        ++n;
        //edge->reward_avg_visit = edge->reward_avg_visit * edge->n_visits * value_global_max + r) / ((edge->n_visits + 1) * value_global_max);    // In samegame, avg_val is gonna be too big

        ++edge->n_visits;

        // Adjust/change r here as wanted.

        // NOTE This seem to take care of my whole "action decisive" idea. I just need to make extremals rarer.
        // if (propagate_minimax)
        //     r = best_avg_val(current_node())->reward_avg_visit;
        // NOTE For one player games, could we still propagate the best avg_val?
        //
        // NOTE By saving the action value lower bound like we do now, we don't need to backpropagate
        // the max of children like that... Even if a region of the tree containing a critical branch
        // performs poorly on average, we will remember that critical branch until it gets beaten.
        //
        // TODO Maybe revisit that critical branch once it gets beaten?? (See "killer moves"?)
    }
}

Reward Agent::evaluate_terminal()
{
    return state.is_empty() ? 1000 : 0;
}

Reward Agent::sg_value(const ClusterData& cd)
{
    return (cd.size - 2) * (cd.size - 2);
}

// NOTE: From StockFish's Monte Carlo implementation :
// /// MonteCarlo::value_to_reward() transforms a Stockfish value to a reward in [0..1].
// /// We scale the logistic function such that a value of 600 (about three pawns) is
// /// given a probability of win of 0.95, and a value of -600 is given a probability
// /// of win of 0.05
// Reward value_to_reward(Value v) {
//     const double k = -0.00490739829861;
//     double r = 1.0 / (1 + exp(k * int(v)));

//     assert(REWARD_MATED <= r && r <= REWARD_MATE);
//     return Reward(r);
// }
//
// enum Value : int {
//   VALUE_ZERO      = 0,
//   VALUE_DRAW      = 0,
//   VALUE_KNOWN_WIN = 10000,
//   VALUE_MATE      = 32000,
//   VALUE_INFINITE  = 32001,
//   VALUE_NONE      = 32002,

//   VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - 2 * MAX_PLY,
//   VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,

//   PawnValueMg   = 171,   PawnValueEg   = 240,
//   KnightValueMg = 764,   KnightValueEg = 848,
//   BishopValueMg = 826,   BishopValueEg = 891,
//   RookValueMg   = 1282,  RookValueEg   = 1373,
//   QueenValueMg  = 2526,  QueenValueEg  = 2646,

//   MidgameLimit  = 15258, EndgameLimit  = 3915
// };
// double MonteCarlo::UCB(Node node, Edge& edge) {

//     long fatherVisits = node->node_visits;
//     assert(fatherVisits > 0);

//     double result = 0.0;

//     if (edge.visits)
//         result += edge.meanActionValue;
//     else
//         result += UCB_UNEXPANDED_NODE;     // NOTE: default = 0.5

//     double C = UCB_USE_FATHER_VISITS ? exploration_constant() * sqrt(fatherVisits)
//                                      : exploration_constant();
//                                            // NOTE: default exp_constant = 10.0

//     double losses = edge.visits - edge.actionValue;
//     double visits = edge.visits;

//     double divisor = losses * UCB_LOSSES_AVOIDANCE + visits * (1.0 - UCB_LOSSES_AVOIDANCE);
//     result += C * edge.prior / (1 + divisor);

//     result += UCB_LOG_TERM_FACTOR * sqrt(log(fatherVisits) / (1 + visits));
//                                             // NOTE: default log_term_fact = 0.0

//     return result;
// }

// "Upper Confidence Bound" of the edge
Reward Agent::ucb(Node* node, Edge& edge)
{
    Reward result = 0;

    if (edge.n_visits) {
        result += edge.reward_avg_visit;
    } else {
        result += edge.value_assured/value_global_max;
    }

    // The UCT exploration term
    result += exploration_cst * sqrt( 2 * (double)(log(node->n_visits)) / (double)(edge.n_visits+1) );

    return result;
}

Edge* Agent::best_ucb(Node* node)
{
    auto best_ndx = 0;
    auto best_val = -std::numeric_limits<double>::max();

    for (int i=0; i<node->n_children; ++i)
    {
        auto c = node->children[i];

        Reward r = ucb(current_node(), c);
        if (r > best_val)
        {
            best_val = r;
            best_ndx = i;
        }

        // if (Debug::debug_ucb) {
        //     std::cerr << "Edge " << (int)c.cd.rep << ":\n";
        //     std::cerr << "    best: " << int(c.value_assured) << " avg: " << std::fixed << c.reward_avg_visit << " ucb-expl: " << std::fixed << r - c.reward_avg_visit << '\n';
        //     std::cerr << "    ucb-value: " << r << std::endl;
        // }
    }

    return &(node->children[best_ndx]);
}

Edge* Agent::best_visits(Node* node)
{
    // if (Debug::debug_best_visits)
    //     std::cerr << "Choosing best visits. choices are :";

    int best = 0;
    auto best_val = -std::numeric_limits<int>::max();

    auto& children = node->children_list();

    for (int i=0; i<node->n_children; ++i)
    {
        // if (node->children[i].action == ACTION_NONE)
        //     continue;
        auto v = children[i].n_visits;

        // if (Debug::debug_best_visits)
        //     std::cerr << "Action " << (int)children[i].cd.rep << " with " << v << " visits and mean value " << children[i].reward_avg_visit << std::endl;

        if (v > best_val)
        {
            best_val = v;
            best = i;
        }
    }

    // if (Debug::debug_best_visits)
    //     std::cerr << "Returning with action " << (int)node->children[best].cd.rep << std::endl;

    return &(node->children[best]);
}

Edge* Agent::best_avg_val(Node* node)
{
    // if (Debug::debug_best_visits)
    //     std::cerr << "Choosing best visits. choices are :";

    int best = 0;
    auto best_val = -std::numeric_limits<int>::max();

    auto& children = node->children_list();

    for (int i=0; i<node->n_children; ++i)
    {
        // if (node->children[i].action == ACTION_NONE)
        //     continue;
        auto v = children[i].reward_avg_visit;

        // if (Debug::debug_best_visits)
        //     std::cerr << "Action " << children[i].action << " with " << v << " visits and mean value " << children[i].reward_avg_visit << std::endl;

        if (v > best_val)
        {
            best_val = v;
            best = i;
        }
    }

    // if (Debug::debug_best_visits)
    //     std::cerr << "Returning with action " << node->children[best].action << std::endl;

    return &(node->children[best]);
}

Edge* Agent::best_value_assured(Node* node)
{
    int best = 0;
    auto best_val = -std::numeric_limits<int>::max();

    const auto& children = node->children_list();

    for (int i=0; i<node->n_children; ++i)
    {
        auto v = children[i].value_assured;

        if (v > best_val)
        {
            best_val = v;
            best = i;
        }
    }

    return &(node->children[best]);
}

//***************************** Playing actions ******************************/

Node* Agent::current_node()
{
    return nodes[ply];
}

int Agent::get_ply() const
{
    return ply;
}

bool Agent::is_root(Node* node)
{
    return ply == 1 && current_node() == node && node == root;
}

bool Agent::is_terminal(Node* node)
{
    assert(node == current_node());

    if (node->n_visits > 1 && node->n_children == 0)
        return true;

    return false;
}

// Here of course we should hold on to a list of the actual clusters
// while we do the playouts (vector of cells for each action) (because otherwise we will
// call the apply_action_blind() method everytime)
void Agent::init_children()
{
    //auto& children      = current_node()->children_list();
    assert(current_node()->n_children == 0);

    auto parent_sg_value_from_root = current_node() == root ? 0 : current_node()->parent->value_sg_from_root;

    // if (Debug::debug_init_children)
    // {
    //     std::cerr << "Expansion " << explored_nodes_cnt;
    //     state.display<std::ostream>(std::cerr);
    //     std::cerr << "Cumulative reward from root : " << parent_sg_value_from_root << std::endl;
    // }

    // state.generate_clusters();    // NOTE: Clusters are generated from tree_policy or create_root already.
    // Make a local copy because the state's valid_actions
    // container will change during the random simulations.
    auto valid_actions_data = state.valid_actions_data();     // Have representatives, colors and sizes

    for (const auto& cd : valid_actions_data)
    {
        assert(cd.rep != ACTION_NONE);
        // assert(cd.p_cluster != nullptr);  // NOTE: No more p_cluster in a ClusterData

        // std::cerr << "Starting random simulation at ply " << ply << " and state \n";
        // Debug::display(std::cerr, state);
        // std::cerr << std::endl;

        // TODO: Implement option for state to save the cluster list for later...
        //       we already computed it before starting the for loop
        //
        // NOTE: My current idea of a p_cluster in the ClusterData object obviously
        //       doesn't work... we need to store the list of cells of the chosen cluster
        //state.generate_clusters();    // TODO Don't think we need that anymore

        Reward simulation_reward = random_simulation(cd);    // NOTE: random_simulation returns with clusters not up to date anymore!
        ++cnt_simulations;

        Edge new_action;
        new_action.cd = cd;
        new_action.n_visits = 0;
        new_action.value_sg_from_root = parent_sg_value_from_root + sg_value(cd);
        new_action.value_assured = new_action.value_sg_from_root + simulation_reward;
        new_action.reward_avg_visit = 0;

        if (current_node()->n_children < MAX_CHILDREN)
        {
            current_node()->children_list()[current_node()->n_children] = new_action;
            ++(current_node()->n_children);
        }
    }

    // if (Debug::debug_init_children) {
    //     std::cerr << "Initialized " << current_node()->n_children << " children, with values\n";
    //     for (int i=0; i<current_node()->n_children; ++i) {
    //         std::cerr << "Action " << (int)current_node()->children[i].cd.rep << ": Rollout reward " << current_node()->children[i].value_assured << '\n';
    //     }
    //     std::cerr << std::endl;
    // }

    ++explored_nodes_cnt;
    ++current_node()->n_visits;
    auto beg = current_node()->children_list().begin();
    auto nb  = current_node()->n_children;

    std::sort(beg, beg + nb, [](const auto& a, const auto& b){
            return a.value_assured > b.value_assured;
        });

    // NOTE : Edges now zero initialize to the empty edge below
    // A sentinel to easily iterate through children.
    // if (nb < MAX_CHILDREN)
    // {
    //     Edge new_action;
    //     new_action.action = ACTION_NONE;
    //     new_action.n_visits = 0;
    //     new_action.prior_value = 0;
    //     new_action.value_assured = 0;
    //     new_action.reward_avg_visit = 0;

    //     current_node()->children_list()[nb] = new_action;
    // }
}

//Sends a cluster as the action
void Agent::apply_action(const ClusterData& cd)
{
    assert (ply < MAX_PLY);

    stack[ply].ply = ply;
    stack[ply].cd = cd;              // This action dictates the next state, and current state (data) will go to stack[ply].previous

    state.apply_action(cd, states[ply]);    // The state will store its new data on the passed StateData object
    ++ply;

}

// // Sends the representative of a cluster as the action
// // NOTE: State must have the current list of clusters/representatives in memory
// void Agent::apply_action(Action action)
// {
//     assert (ply < MAX_PLY);

//     stack[ply].ply = ply;
//     stack[ply].cd.rep = action;          // This action dictates the next state, and current state (data) will go to stack[ply].previous

//     state.apply_action_gen_clusters(action, states[ply]);    // The state will store its new data on the passed StateData object
//     ++ply;
// }

// void Agent::apply_action_blind(Action action)
// {
//     assert (ply < MAX_PLY);

//     stack[ply].ply = ply;
//     stack[ply].cd.rep = action;

//     state.apply_action_blind(action, states[ply]);
//     ++ply;
// }

void Agent::undo_action()
{
    assert (ply > 1);
    --ply;
    state.undo_action(stack[ply].cd);                  // State holds a pointer to its previous StateData object
}

// void Agent::undo_action(const ClusterData& cd)
// {
//     assert (ply > 1);
//     --ply;
//     state.undo_action(cd);    // State holds a pointer to its previous StateData object
// }

//***************************** Evaluation of nodes **************************/


// First idea would be to not save or lookup anything, but save the next-to-last state
// (really, state-before-known-win/lose) to start building a table of alphas and betas.
Reward Agent::random_simulation(const ClusterData& cd)
{
    // if (Debug::debug_random_sim)
    // {
    //     Debug::display(std::cerr, state, cd.rep, state.get_cluster(cd.rep));
    //     std::cerr << std::endl;
    //     std::cerr << "Random simulation, ply " << ply << ", next action is " << (int)cd.rep << std::endl;
    // }

    // NOTE this is done in `apply_action`
    // stack[ply].ply = ply;
    // stack[ply].currentAction = action;
    // stack[ply].r = 0;

    // NOTE: Start off with clusters generated from init_children, then from the
    //       previous ply's call to apply_action so OK
    apply_action(cd);

    if (state.is_terminal())
    {
        stack[ply].r = evaluate_terminal();

        // if (Debug::debug_random_sim)
        // {
        //     Debug::display(std::cerr, state);
        //     std::cerr << std::endl;
        //     std::cerr << "Reached terminal state at ply " << ply << '\n';
        //     std::cerr << "evaluate_terminal = " << std::fixed << stack[ply-1].r << '\n'<< std::endl;
        // }
    }

    else
    {
        ClusterData cd = Random::choose(state.valid_actions_data());

        stack[ply].r = sg_value(cd) + random_simulation(cd);
    }

    undo_action();

    return stack[ply+1].r;
}



// std::vector<Action> Agent::get_edges_from_root() const
// {
//     auto ret = std::vector<Edge*> { };

//     for (int hist_ply = 1; hist_ply < ply; ++hist_ply)
//     {
//         ret.push_back(actions[hist_ply]);
//     }
// }


//************************************** DEBUGGING ***************************************/


std::string Agent::debug_tree_stats() const
{
    std::stringstream ss;
    ss << "Iter_cnt = " << iteration_cnt <<
          " Select_cnt = " << descent_cnt <<
          " Simul_cnt = " << cnt_simulations <<
          " Node_init_cnt = " << explored_nodes_cnt <<
          " Node_Table_size = " << mcts::MCTS.size() << '\n';
    return ss.str();
}

// void Agent::print_debug_cnts(std::ostream& _out) const
// {
//         std::cerr << "Iterations: " << iteration_cnt << '\n';
//         std::cerr << "Descent count: " << descent_cnt << '\n';
//         std::cerr << "Rollout count: " << cnt_simulations << '\n';
//         std::cerr << "Exploration count: " << explored_nodes_cnt << '\n';
//         std::cerr << "Number of nodes in table: " << mcts::MCTS.size() << '\n';
//         std::cerr << "Number of nodes not found with get_node: " << cnt_new_nodes << std::endl;
// }

void Agent::print_node(std::ostream& _out, Node* node) const
{
    _out << "Node: v=" << node->n_visits << std::endl;
    for (int i=0; i<node->n_children; ++i)
    {
        auto c = node->children[i];
        _out << "    Action " << (int)c.cd.rep << ": v=" << c.n_visits << ", val=" << c.reward_avg_visit << std::endl;
    }
}

void Agent::print_tree(std::ostream& _out, int depth) const
{
    print_node(_out, root);
}

} // namespace mcts
