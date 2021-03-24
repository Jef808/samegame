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
#include <spdlog/fmt/ostr.h>
#include "mcts.h"
//#include "debug.h"


using namespace sg;

namespace mcts {

std::ostream& operator<<(std::ostream& _out, const mcts::Agent& agent) {
  return _out << Agent::debug_tree_stats(agent);
}

std::ostream& operator<<(std::ostream& _out, const mcts::Edge& edge) {
    return _out << Agent::debug_edge_stats(edge);
    // std::stringstream ss;
    // ss << "Rep=" << std::to_string(edge.cd.rep) << ", n=" << edge.n_visits << ", val_best=" << edge.val_best << ", val_avg=" << std::fixed << std::setprecision(2) << edge.reward_avg_visit;
    // return _out << ss.str();
}

std::ostream& operator<<(std::ostream& _out, const ::mcts::Node& node) {
    return _out << Agent::debug_node_stats(node);
    // std::stringstream ss;
    // ss << "key = " << std::to_string(node.key) << ", n_visits = " << node.n_visits << ", n_children = " << node.n_children;
    // return _out << ss.str();
}

//************************** Node Lookup Table****************************/

MCTSLookupTable MCTS;

Edge EDGE_NONE = { };


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
    new_node.parent = actions[ply-1];    // We've set actions[0] to ACTION_NONE so okay

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

    // Return a random choice, or a NULL cd
    ClusterData choose(const ActionDVec& choices)
    {
        if (choices.empty())
            return ClusterData { CELL_NONE, COLOR_NONE, 0 };
        return choices[dist(rd) % choices.size()];
    }
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
    , cnt_new_roots(0)
{
    MCTS.reserve(MAX_PLY);
    //reset();
}

//******************************** Main methods ***************************/

void Agent::reset()
{
    MCTS.clear();
    // Reset configs
    max_iterations = MAX_ITER;
    exploration_cst = EXPLORATION_CST;
}

void Agent::set_root()
{
    if (use_time) {
        init_time();
    }

    // Reset the search buffers (or 0 initialize)
    nodes = {};
    actions = {};
    stack = {};

    actions[0] = &EDGE_NONE;    // Will be assigned to root->parent

    // NOTE: Since state.p_data will point to states[1] when making a 'root move', we store the state's
    // p_data below, in states[0].
    // (Otherwise, the first call to apply_action after a root move would do state[1].previous = &state[1];
    // then modify state[1].grid according to the action, and the original (now root) state would be lost.
    states[0] = std::move(*state.p_data);
    state.p_data = &states[0];

    // Reset the counters
    ply                 = 1;
    global_max_depth    = ply;
    cnt_iterations      = 0;
    cnt_simulations     = 0;
    cnt_descent         = 0;
    cnt_explored_nodes  = 0;
    cnt_rollout         = 0;
    cnt_new_nodes       = 0;
    ++cnt_new_roots;

    // NOTE : states[ply=1] will contain the result of applying action stack[ply].action, on calling
    // apply_action for the first time, (and subsequently etc...), the state points state[ply=1]'s
    // previous* pointer to the initial state, before applying the action.
    // NOTE : It is the state's responsibility to update states[ply], we only pass it
    // a (ref to a) StateData object when asking for an apply_action(), and the undo_action() reverts the
    // state to state[ply]->previous

    // NOTE: No need to generate clusters, a state constructs its key on initialization and apply_move so it
    // always has a valid key to return.
    // TODO: However, we need to implement tree_policy without calls to state.apply_action... only then can we
    // defer state.generate_clusters() to the expansion phase (init_childrene).
    state.generate_clusters();
    root = nodes[ply] = get_node();

    // Reset stats
    value_global_max = std::numeric_limits<double>::min();
    value_global_min = std::numeric_limits<double>::max();

    if (root->n_visits == 0) {
        init_children();
    }
}

bool Agent::computation_resources()
{
    bool res = cnt_iterations < max_iterations;

    if (use_time)
        res &= (time_elapsed() < MAX_TIME - 100);    // NOTE: Clock keeps running during I/O when testing

    return res;
}

ClusterData Agent::MCTSBestAction()
{
    init_time();
    set_root();

    if (is_terminal(root))
    {
        return {CELL_NONE, COLOR_NONE, 0};
    }

    while (computation_resources())
    {
        step();
    }



    auto* choice = best_visits(root);

    return choice->cd;
}

void Agent::step()
{
    Node* node = tree_policy();

    Reward reward = rollout_policy(node);

    backpropagate(node, reward);

    ++cnt_iterations;
}

Node* Agent::tree_policy()
{
    assert(is_root(current_node()));

    // TODO Should only check this once, not every time we make a descent...
    if (is_terminal(root)) {
       return root;
    }

    state.generate_clusters();    // NOTE: For subsequent iterations of the MCTS algorithm,
                                  // when a bunch of undo_actions() have just been done.
                                  // TODO: Since this is always starting at the root, we might
                                  // as well save the clusters configuration at the root for the
                                  // whole thing. (It's already kind of encoded into the root's children... )

    // Navigate to an unexplored node, selecting the path carefully to maximize rewards and minimize regret.
    // NOTE: with the is_terminal check in the while loop condition, we can increment the n_visit of terminal nodes
    // equally in the rollout_policy is_terminal check, and the cnt_descent is always incremented!
    while (!is_terminal(current_node()) && current_node()->n_visits > 0 && ply < MAX_PLY)
    {
        // The strategy for balancing exploration and exploitaion
        actions[ply] = best_ucb(current_node());

        ++nodes[ply]->n_visits;
        // spdlog::debug("\nStep={} ply={}\n", cnt_iterations, ply);
        // for (int i=0; i<current_node()->n_children; ++i)
        // {
        //     auto* child = &current_node()->children[i];
        //     if (child->cd.rep == actions[ply]->cd.rep)
        //         spdlog::debug("  \033[35m{}, UCB={:.2f}\033[m", current_node()->children[i], ucb(current_node(), *child));
        //     else
        //         spdlog::debug("  {}, UCB={:.2f}", current_node()->children[i], ucb(current_node(), *child));
        // }


        if (actions[ply]->cd.size < 2) {

            spdlog::debug("best_ucb returned {} on state \n{}\nThe state had children\n", actions[ply]->cd.rep, state);
            for (auto& c : current_node()->children_list()) {
                spdlog::debug("{}\n", c);
            }

        }

        // if (actions[ply]->cd.size == 0)

        //     return current_node();

        apply_action(actions[ply]->cd);    // TODO: Replace by a LookupTable query (we've seen the state/action pairs during tree_policy)

        nodes[ply] = get_node();    // Either the node has been seen and is associated to a state key,
                                    // or not and get_node creates a record of it.

        // TODO How to deal with different paths (different nodes in the tree) leading
        // to equivalent state position? The state key will be the same but not the nodes.
        // Have to make sure to only keep the best path and reorganize the tree without losing information.
        // NOTE in this direction : currently the last_move field of a Node object isn't used! it can have
        // multiple parents. This most likely messes with the algorithm by favoring positions that arise
        // more frequenty.
    }

    ++cnt_descent;

    //assert (current_node()->n_visits == 0);  // Could have terminal node, or reached MAX_PLY...

    if (ply > global_max_depth)
        global_max_depth = ply;

    return current_node();
}

// Once we have the next unexplored node (or terminal), we expand it and do a random
// playout starting with every one of its children.
Reward Agent::rollout_policy(Node* node)
{
    assert(node == current_node());

    Reward rollout_reward = 0;

    if (is_terminal(current_node())) {
        ++current_node()->n_visits;
        rollout_reward += evaluate_terminal();
    }

    // NOTE: It is possible that we want to "sample" the same path to a terminal node a few times during
    // the algorithm, then start selecting another path after a few iterations...
    // if (is_terminal(node))
    // {
    //     spdlog::debug("Move {} Step {}, is_terminal() returned true.", cnt_new_roots, cnt_iterations);
    //     if (state.key() != current_node()->key)
    //         spdlog::debug("Move {} Step {}, keys don't match after call to is_terminal.", cnt_new_roots, cnt_iterations);
    //     ++node->n_visits;
    //     rollout_reward = node->parent->sg_value_from_root + evaluate_terminal();
    // }
    else
    {
        assert(node->n_visits == 0);

        // if (state.key() != current_node()->key)
        //     spdlog::debug("Move {} Step {}, keys don't match entering rollout_policy", cnt_new_roots, cnt_iterations);
        // Expand the node and do a rollout on each child (if any) and sets node->n_visits to 1
        // The newly intialized children's val_best also includes the sg_value_from_root of parent.
        init_children();

        // if (state.key() != current_node()->key)
        //     spdlog::debug("Move {} Step {}, keys don't match after init_child", cnt_new_roots, cnt_iterations);
        rollout_reward = node->children.back().val_best;

        // NOTE: Only after init_children() can we can tell that the node is terminal, since
        //       is_terminal() returns n_visits > 1 && n_children == 0;
        //
        // NOTE: With Samegame, it's probaby better to initialize nodes with average value of their child (maybe do more
        // than 1 simul in init_children too?)


        // if (node->n_children == 0) {
        //     rollout_reward = node->parent->sg_value_from_root + evaluate_terminal();
        // } else {
        //     for (int i=0; i < node->n_children; ++i)
        //     {
        //         rollout_reward += node->children[i].val_best;
        //     }
        //     rollout_reward /= node->n_children;
        // }

        if (node->children[0].val_best < value_global_min) {
        value_global_min = node->children[0].val_best;
        }
    }

    if (rollout_reward > value_global_max) {
        value_global_max = rollout_reward;
    }

    ++cnt_rollout;

    return rollout_reward;
}

// Backpropagate the rollout reward r, that is the best value seen after running a
// random playout from each child of node.
//
// Let's start by rescaling the reward linearly before using the sharper curves above
void Agent::backpropagate(Node* node, Reward r)
{
    assert(node == current_node());

    while (!is_root(current_node()))
    {
        assert(stack[ply-1].cd.rep != CELL_NONE);
        undo_action();                        // NOTE: Fetches last action on the search stack.

        ++actions[ply]->n_visits;
        actions[ply]->reward_avg_visit += (r - actions[ply]->reward_avg_visit) / (actions[ply]->n_visits);

        if (r > actions[ply]->val_best)
           actions[ply]->val_best = r;
    }
}

// Initialize some or all edges from a node with the (three long) accumulated reward of a
// simulated playthrough.
void Agent::init_children()
{
    assert(!is_terminal(current_node()));

    // state.generate_clusters();    // NOTE: Clusters are generated from tree_policy or set_root already.
    // Make a local copy because the state's valid_actions container will change during the random simulations.
    auto valid_actions_data = state.valid_actions_data();
    auto nb_actions = valid_actions_data.size();

    // TODO Maybe only store this once in the parent Node since it is used for all its children
    // NOTE: Below is okay since root->parent is EDGE_NONE now
    auto parent_sg_value_from_root = current_node()->parent->sg_value_from_root;    //current_node() == root ? 0 : current_node()->parent->sg_value_from_root;

    // We start by loading all the children on the Edges vector, then we can sort/filter etc... in place, and then
    // if needed resizing the vector back down.
    auto& children_cont = current_node()->children_list();
    children_cont.reserve(nb_actions);

    for (auto& cd : valid_actions_data)
    {
        assert(cd.rep != CELL_NONE);
        // cd is passed by copy so only copy so the local ref cd won't be invalidated.

        Reward simulation_reward = random_simulation(cd);

        ++cnt_simulations;

        children_cont.emplace_back(cd, parent_sg_value_from_root + sg_value(std::move(cd)), simulation_reward);
        //new_action.cd = std::move(cd);
        //new_action.n_visits = 0;
        //new_action.sg_value_from_root =
        //new_action.val_best = simulation_reward;
        //new_action.reward_avg_visit = 0;
    }

    int n_children = children_cont.size() > MAX_CHILDREN ? MAX_CHILDREN : children_cont.size();
    std::partial_sort(begin(children_cont), begin(children_cont) + n_children, end(children_cont),
        [](const auto& a, const auto& b) {
            return a.val_best < b.val_best;
        });

    if (n_children < children_cont.size())
        children_cont.resize(n_children);

    current_node()->n_children = n_children;
    ++cnt_explored_nodes;
    ++current_node()->n_visits;
}

// First idea would be to not save or lookup anything, but save the next-to-last state
// (really, state-before-known-win/lose) to start building a table of alphas and betas.
Reward Agent::random_simulation(ClusterData _cd)
{
    stack[ply].r = 0;
    int depth=0;
    // Do until we hit a terminal state or the maximum depth
    while (!State::is_terminal(state.key()) && ply < MAX_PLY)
    {
        stack[ply+1].r = stack[ply].r + sg_value(_cd);  // Accumulate the rewards

        assert(_cd.rep != CELL_NONE && state.cells()[_cd.rep] != COLOR_NONE);    // It should be CELL_NONE iff cd.size == 0
        apply_action(_cd);
        ++depth;

        _cd = Random::choose(state.valid_actions_data());   // TODO Really wasting a lot of ressource by computing all clusters just to return 1.
    }

    Reward res = stack[ply].r;    // Contains the sum of all rewards through the simulation.

    if (ply < MAX_PLY)
        res += evaluate_terminal();

    while (depth > 0) {
        undo_action();
        --depth;
    }

    // while (stack[ply-1].current_action != ACTION_NONE) {            // Use the sentinel we set up for this
    //     undo_action();                                           // Simply reverts the state's grid to the previous one, using stack[ply-1].cd to restore the number of colors
    //     --cnt;
    // }

    // Restore the changed value
    //stack[ply-1].current_action = last_action;

    return res;
}

// ***************************************** Selection of Nodes ************************************//

// "Upper Confidence Bound" of the edge
Reward Agent::ucb(Node* node, const Edge& edge)
{
    Reward result = edge.n_visits ? edge.reward_avg_visit : edge.val_best;

    result = result / 5000.0;

    // The UCT exploration term
    result += exploration_cst * sqrt( (double)(log(node->n_visits)) / (double)(edge.n_visits+1) );

    return result;
}

// TODO Refactor the "best___" methods into one.
//
Edge* Agent::best_ucb(Node* node)
{
    Edge* best = &EDGE_NONE;
    auto best_val = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list())
    {
        if (auto r = ucb(current_node(), c); r > best_val)
            best = &c;
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        return &node->children_list().back();
    }

    return best;

    // for (int i=0; i<node->n_children; ++i)
    // {
    //     auto c = node->children[i];

    //     Reward r = ucb(current_node(), c);
    //     if (r > best_val)
    //     {
    //         best_val = r;
    //         best_ndx = i;
    //     }

        // if (Debug::debug_ucb) {
        //     std::cerr << "Edge " << (int)c.cd.rep << ":\n";
        //     std::cerr << "    best: " << int(c.val_best) << " avg: " << std::fixed << c.reward_avg_visit << " ucb-expl: " << std::fixed << r - c.reward_avg_visit << '\n';
        //     std::cerr << "    ucb-value: " << r << std::endl;
        // }
    // }


    // return &(node->children[best_ndx]);
}

Edge* Agent::best_visits(Node* node)
{
    // if (Debug::debug_best_visits)
    //     std::cerr << "Choosing best visits. choices are :";

    Edge* best = &EDGE_NONE;
    auto cur_best = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list())
    {
        if (auto r = c.val_best; r > cur_best)
            best = &c;
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        return &node->children_list().back();
    }

    return best;

    // if (Debug::debug_best_visits)
    //     std::cerr << "Returning with action " << (int)node->children[best].cd.rep << std::endl;

    //return &(node->children[best]);
}

Edge* Agent::best_avg_val(Node* node)
{
    Edge* best = &EDGE_NONE;
    auto cur_best = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list())
    {
        if (auto r = c.reward_avg_visit; r > cur_best)
            best = &c;
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        return &node->children_list().back();
    }

    return best;

    // if (Debug::debug_best_visits)
    //     std::cerr << "Choosing best visits. choices are :";

    // int best = 0;
    // auto best_val = -std::numeric_limits<int>::max();

    // auto& children = node->children_list();

    // for (int i=0; i<node->n_children; ++i)
    // {
    //     // if (node->children[i].action == ACTION_NONE)
    //     //     continue;
    //     auto v = children[i].reward_avg_visit;

    //     // if (Debug::debug_best_visits)
    //     //     std::cerr << "Action " << children[i].action << " with " << v << " visits and mean value " << children[i].reward_avg_visit << std::endl;

    //     if (v > best_val)
    //     {
    //         best_val = v;
    //         best = i;
    //     }
    // }

    // if (Debug::debug_best_visits)
    //     std::cerr << "Returning with action " << node->children[best].action << std::endl;

    //return &(node->children[best]);
}

Edge* Agent::best_val_best(Node* node)
{
    Edge* best = &EDGE_NONE;
    auto cur_best = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list())
    {
        if (auto r = c.val_best; r > cur_best)
            best = &c;
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        return &node->children_list().back();
    }

    return best;
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

    //return node->key & 1;
    // NOTE: To fake the node having had its "children initialized":
    return node->key & 1;
    // if (node->key & 1 || (node->n_visits > 1 && node->n_children == 0))
    //     return true;

    // if (node->key == 0)
    //     {
    //     state.generate_clusters();
    //     auto key = state.generate_key();
    //     if (key == 0)
    //     {
    //         spdlog::debug("Fix this nasty hack");
    //         return true;
    //         }
    //     }
}

//Sends a cluster as the action
void Agent::apply_action(const ClusterData& cd)
{
    assert (ply < MAX_PLY);

    stack[ply].ply = ply;
    stack[ply].cd = cd;              // This action dictates the next state, and current state (data) will go to stack[ply].previous
    //stack[ply].current_action = cd.rep;

    state.apply_action(cd, states[ply]);    // The state will store its new data on the passed StateData object
    ++ply;
}

// // Sends the representative of a cluster as the action
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
    state.undo_action(stack[ply-1].cd);   // NOTE: State holds a pointer to its previous StateData object
    --ply;
}

// void Agent::undo_action(const ClusterData& cd)
// {
//     assert (ply > 1);
//     --ply;
//     state.undo_action(cd);    // State holds a pointer to its previous StateData object
// }

//***************************** Evaluation of nodes **************************/

// TODO If we hit an empty terminal state while searching, we stop everything and return that line!
Reward Agent::evaluate_terminal()
{
    return state.is_empty() ? 1000 : 0;
}

Reward Agent::sg_value(const ClusterData& cd)
{
    return (cd.size - 2) * (cd.size - 2);
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

void Agent::set_exploration_constant(double c)
{
    exploration_cst = c;
}

void Agent::set_max_iterations(int i)
{
    max_iterations = i;
}

std::string Agent::debug_tree_stats(const Agent& agent)
{
    std::stringstream ss;
    ss << " Iters=" << agent.cnt_iterations
       << " Selects=" << agent.cnt_descent
       << " Simuls=" << agent.cnt_simulations
       << " Children_inits=" << agent.cnt_explored_nodes
       << " Lookup_tb_sz=" << mcts::MCTS.size()
       << " Max depth=" << agent.global_max_depth;

    return ss.str();
}

std::string Agent::debug_edge_stats(const Edge& edge)
{
    std::stringstream ss;

    ss << std::fixed << std::setprecision(2)
       << "Rep=" << std::to_string(edge.cd.rep)
       << " n=" << edge.n_visits
       << " val_best=" << edge.val_best
       <<", val_avg=" << edge.reward_avg_visit;

    return ss.str();
}

std::string Agent::debug_node_stats(const Node& node)
{
    std::stringstream ss;

    ss << "key = " << std::to_string(node.key)
       << " n_visits=" << node.n_visits
       << " n_children=" << node.n_children;
    return ss.str();
}


void Agent::print_final_score() const
{
for (int i=0; i<root->n_children; ++i)
    {
        auto edges = root->children_list();
        std::cerr << "Action " << (int)edges[i].cd.rep << ":\n";
        std::cerr << "Average value " << (int)edges[i].reward_avg_visit << '\n';
        std::cerr << "Best value " << (int)edges[i].val_best << '\n';
        std::cerr << "Number of visits " << edges[i].n_visits << '\n';
    }
}

// void Agent::print_debug_cnts(std::ostream& _out) const
// {
//         std::cerr << "Iterations: " << cnt_iterations << '\n';
//         std::cerr << "Descent count: " << cnt_descent << '\n';
//         std::cerr << "Rollout count: " << cnt_simulations << '\n';
//         std::cerr << "Exploration count: " << cnt_explored_nodes << '\n';
//         std::cerr << "Number of nodes in table: " << mcts::MCTS.size() << '\n';
//         std::cerr << "Number of nodes not found with get_node: " << cnt_new_nodes << std::endl;
// }

// void Agent::print_node(std::ostream& _out, Node* node) const
// {
//     _out << "Node: v=" << node->n_visits << std::endl;
//     for (int i=0; i<node->n_children; ++i)
//     {
//         auto c = node->children[i];
//         _out << "    Action " << (int)c.cd.rep << ": v=" << c.n_visits << ", val=" << c.reward_avg_visit << std::endl;
//     }
// }

} // namespace mcts


// // Same as Stockfish's function but rescaled and translated to fit the observed values of this game.
// Reward Agent::value_to_reward(double v)
// {
//     auto M = value_global_max;
//     auto m = value_global_min;
//     auto avg = value_global_avg;

//     //const double c = -0.00490739829861;
//     const double c = -0.01;

//     auto res = 1.0 / ( 1+std::exp(c * (v - value_global_avg))  );    // Centered at the global average
//     //assert(res > -0.00001 && res < 1.00001);

//     return res;
// }

// // The inverse
// double Agent::reward_to_value(Reward r)
// {
//     auto M = value_global_max;
//     auto m = value_global_min;

//     const Reward g = 203.77396313709564 * (M - m) / 2.0;  // 1/k

//     return g * std::log(r / (1.0 - r)) + (M - m)/2.0;
// }
