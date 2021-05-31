// mcts.cpp
#include "mcts.h"
#include "dsu.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <random>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>
#include <sstream>
//#include "debug.h"

using namespace sg;

namespace mcts {

const Action ACTION_NONE = CELL_NONE;

std::ostream& operator<<(std::ostream& _out, const mcts::Agent& agent)
{
    return _out << Agent::debug_tree_stats(agent);
}

std::ostream& operator<<(std::ostream& _out, const mcts::Edge& edge)
{
    return _out << Agent::debug_edge_stats(edge);
    // std::stringstream ss;
    // ss << "Rep=" << std::to_string(edge.cd.rep) << ", n=" << edge.n_visits << ", val_best=" << edge.val_best << ", val_avg=" << std::fixed << std::setprecision(2) << edge.reward_avg_visit;

    // return _out << ss.str();
}

std::ostream& operator<<(std::ostream& _out, const ::mcts::Node& node)
{
    return _out << Agent::debug_node_stats(node);
    // std::stringstream ss;
    // ss << "key = " << std::to_string(node.key) << ", n_visits = " << node.n_visits << ", n_children = " << node.n_children;
    // return _out << ss.str();
}

//************************** Node Lookup Table****************************/

MCTSLookupTable MCTS {};

Edge EDGE_NONE = {};

// get_node queries the Lookup Table until it finds the node with given position,
// or creates a new entry in case it doesn't find it.
Node* get_node(const State& state)
{
    Key state_key = state.key();

    auto node_it = MCTS.find(state_key);
    if (node_it != MCTS.end())
        return &(node_it->second);

    // Insert the new node in the Hash table if it wasn't found.
    Node new_node {};
    new_node.key = state_key;
    //new_node.parent = state.p_data->parent_action;    // We've set actions[0] to ACTION_NONE so okay

    auto new_node_it = MCTS.insert(std::make_pair(state_key, new_node)).first;
    return &(new_node_it->second);
}

// A more direct version that interprets a 0 key as an empty state.
///////
// NOTE: The new insert methods probably do what I'm doing without so many redundant instructions.
///////
// Node* get_node_or_create_it(State& state)
// {
//     Key state_key = state.key();

//     auto node_it = MCTS.find(state_key);
//     if (node_it != MCTS.end())
//         return &(node_it->second);

//     Node new_node { };
//     new_node.key = state_key;
//     auto new_node_it = MCTS.insert(std::make_pair(state_key, new_node)).first;
//     return &(new_node_it->second);
// }

// Used for choosing actions during the random_simulations.
namespace Random {

    std::random_device rd;
    std::mt19937 e { rd() }; // or std::default_random_engine e{rd()};
    std::uniform_int_distribution<int> dist { 0, MAX_CHILDREN };

    Action choose(const ActionVec& choices)
    {
        if (choices.empty())
            return ACTION_NONE;
        return choices[dist(rd) % choices.size()];
    }

    // Return a random choice, or a NULL cd
    ClusterData choose(const sg::State::ClusterDataVec& choices)
    {
        if (choices.empty())
            return ClusterData { CELL_NONE, Color::Empty, 0 };
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
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - search_start).count();
}

bool Agent::computation_resources()
{
    bool res = cnt_iterations < max_iterations;

    if (use_time)
        res &= (time_elapsed() < MAX_TIME - 100); // NOTE: Clock keeps running during I/O when testing

    return res;
}

//******************************** Ctor(s) *******************************/

Agent::Agent(State& state)
    : state(state)
    , root(nullptr)
    //, root_sd()
{
    reset();
    states[0] = *state.p_data;
    state.p_data = &(states[0]);
    root = nodes[1] = get_node(state);
}

//******************************** Main methods ***************************/

/////////////////////////////////////////
// 'ZERO-INIT' OF A MCTS SEARCH
//     THINK: (RE)BASING OF AN INITIAL STATE,
//             NOT MEMORY ALLOCATION
/////////////////////////////////////////

StateData copy_sd(const State& state)
{
    return { state.cells(), state.key(), state.n_empty_rows() };
    //return ret;
    // std::copy(state.cells().begin(), state.cells().end(), ret.cells.begin());
    // ret.cells = state.cells();
    // ret.key = state.key();
    // ret.n_empty_rows = state.p_data->n_empty_rows;
}

void Agent::set_root()
{
    if (use_time) {
        init_time();
    }

    // Reset the search buffers (0 initialize our memory pool)
    nodes = { {} };
    actions = { {} };
    stack = { {} };

    states[0] = *(state.p_data);
    state.p_data = &(states[0]);

    actions[0] = &EDGE_NONE;

    // Set the pointer to root node
    root = nodes[1] = get_node(state);

    // Reset the counters
    ply = 1;
    global_max_depth = ply;
    cnt_iterations = 0;
    cnt_simulations = 0;
    cnt_descent = 0;
    cnt_explored_nodes = 0;
    cnt_rollout = 0;
    cnt_new_nodes = 0;

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

    //root = nodes[ply] = get_node(state);

    // Reset stats
    //value_global_max = std::numeric_limits<double>::min();
    //value_global_min = std::numeric_limits<double>::max();

    if (root->n_visits == 0) {
        init_children();
    }

    spdlog::debug("After call to init_children, root has {}", root->n_children);
}

///////////////////////////////////////////////
// THE 'USER INTERFACE' TO THE MCTS ALGORITHM
///////////////////////////////////////////////

ClusterData Agent::MCTSBestAction()
{
    //set_root();

    if (is_terminal(root)) {
        spdlog::debug("root is terminal??");
        return { CELL_NONE, Color::Empty, 0 };
    }

    while (computation_resources()) {
        step();
    }

    auto* choice = best_visits(root);

    return choice->cd;
}

void Agent::step()
{
    auto logger = spdlog::get("m_logger");

    // TODO: During tree_policy, when computing a state's key, keep the data
    // of the state's clusters until we know that it's node isn't the one returned.
    logger->debug("before tree_policy");
    Node* node = tree_policy();

    // TODO: Use the result of the above TODO to initialize the edges and start the
    // random simulations.
    logger->debug("before rollout_policy");
    Reward reward = rollout_policy(node);

    logger->debug("before backpropagate");
    backpropagate(node, reward);

    // TODO: Maybe I need a cleaning step here to make sure the root = node[1] corresponds
    // to the current state and nothing has been lost?
    assert(ply == 1);
    assert(current_node() == nodes[ply]);
    assert(is_root(current_node()));
    ++cnt_iterations;
}

/////////////////////////////////////////////////////
// IMPLEMENTATION OF THE MAIN METHODS
// OF THE MCTS ALGORITHM
/////////////////////////////////////////////////////

// Navigate to an unexplored node, selecting the path carefully to maximize rewards and minimize regret.
Node* Agent::tree_policy()
{
    assert(ply == 1);
    assert(current_node() == nodes[ply]);
    assert(is_root(current_node()));

    // TODO Should only check this once, not every time we make a descent...
    if (is_terminal(root)) {
        return root;
    }

    while (!is_terminal(current_node()) && current_node()->n_visits > 0 && ply < MAX_PLY) {
        actions[ply] = best_ucb(current_node());
        ++nodes[ply]->n_visits;

        // NOTE: Must be a better way to phrase this if statement...
        if (actions[ply]->cd.size < 2) {
            spdlog::error("best_ucb returned invalid action {} on non-terminal state \n{}\n", actions[ply]->cd.rep, state);
        }

        ///////////////////
        // Here, we 1) apply the action (compute the cluster, reorganize the cells
        //          2) lookup the key of the new state in the hash table
        //                    !!!!! So we need a quick way to compute keys, or
        //                    !!!!! at worst regenerate the clusters and compute the key.
        //////////////////

        // We go up one ply here
        apply_action(actions[ply]->cd); // TODO: Replace by a LookupTable query (we've seen the state/action pairs during tree_policy)

        nodes[ply] = get_node(state);
    }

    // NOTE: nodes[ply] is now 1) new, 2) terminal, 3) too deep
    ++cnt_descent;

    // Keep track of how deep we've been to date (acts like a sentinel value for our stacks)
    if (ply > global_max_depth)
        global_max_depth = ply;

    return current_node();
}

/**
 * NOTE: The Rollout phase is started at a node that's not explored yet but has all its valid
 * actions freshly computed to get its hash key.
 */
Reward Agent::rollout_policy(Node* node)
{
    assert(node == current_node());

    Reward rollout_reward = 0;

    if (is_terminal(current_node())) {
        ++current_node()->n_visits;
        rollout_reward += actions[ply - 1]->sg_value_from_root + evaluate_terminal();
        return rollout_reward;
    }

    assert(node->n_visits == 0);

    // Initialize the new node (includes running simulations on its children)
    // The children (edges) of current_node will be sorted by those simulation scores
    init_children();

    // TODO: Test the impact of returning rollout_reward = average of the node->children[i].val_best instead
    //       of the best result as below.
    //       Also check if doing a certain amount of simulations instead of just one helps out.
    rollout_reward += node->children[0].val_best;

    // TODO: Decide if the global max/min values are actually needed (was initially for addressing
    //       the problem of normalizing rewards between 0 and 1).
    //        if (node->children.back().val_best < value_global_min) {
    //            value_global_min = node->children.back().val_best;
    //        }

    //    if (rollout_reward > value_global_max) {
    //        value_global_max = rollout_reward;
    //    }

    ++cnt_rollout;

    return rollout_reward;
}

// This is the function that initializes a node.
// Initialize some or all edges from a node with the (three long) accumulated reward of a
// simulated playthrough.
void Agent::init_children()
{
    spdlog::debug("Before valid_actions");

    auto valid_actions = state.valid_actions_data();

    spdlog::debug("after valid_actions: ");

    for (auto act : valid_actions) {
        spdlog::debug("{}", act);
    }

    // assert(state.key() == current_node()->key);
    // assert(!is_terminal(current_node()));

    auto& children_edges = current_node()->children_list();
    children_edges.reserve(valid_actions.size());

    for (const auto& cd : valid_actions) {
        assert(cd.size > 1 && cd.color != Color::Empty && cd.rep != CELL_NONE);

        Reward simulation_reward = random_simulation(cd, 100);

        ++cnt_simulations;
        // children_edges.emplace_back(cd,
        //                             actions[ply-1]->sg_value_from_root + sg_value(cd),   // sg_value_from_root
        //                             simulation_reward,                                   // val_best
        //                             0,                                                   // reward_avg_visit
        //                             0);                                                  // n_visits
        Edge new_action {};

        new_action.cd = cd;
        new_action.n_visits = 0;
        new_action.sg_value_from_root = actions[ply - 1]->sg_value_from_root + evaluate_valid_action(cd);
        new_action.val_best = simulation_reward;
        new_action.reward_avg_visit = 0;

        spdlog::debug("Pushing back {}", new_action);
        children_edges.push_back(new_action);
    }

    auto n_children = valid_actions.size() > MAX_CHILDREN ? MAX_CHILDREN : valid_actions.size();
    std::partial_sort(begin(children_edges), begin(children_edges) + n_children, end(children_edges),
        [](const auto& a, const auto& b) {
            return a.val_best > b.val_best;
        });

    if (n_children < children_edges.size())
        children_edges.resize(n_children);

    current_node()->n_children = n_children;
    ++cnt_explored_nodes;
    ++current_node()->n_visits;
}

// Backpropagate the rollout reward r, (the best value seen after running a
// random playout from each child of the expanded node).
void Agent::backpropagate(Node* node, Reward r)
{
    assert(node == current_node());

    while (!is_root(current_node())) {
        assert(stack[ply - 1].cd.rep != CELL_NONE);
        undo_action(); // NOTE: Fetches last action on the search stack.

        ++actions[ply]->n_visits;
        actions[ply]->reward_avg_visit += (r - actions[ply]->reward_avg_visit) / (actions[ply]->n_visits);

        if (r > actions[ply]->val_best)
            actions[ply]->val_best = r;
    }

    assert(ply == 1);
}

//////////////////////////////////////
//     RANDOM SIMULATION:
//
// 1) Store the initial data for when we'll return
// 2) Generate random actions and apply them without computing anything else
// 3) Keep track of the score
// 4) Restore the initial state and return the total score.
/////////////////////////////////////

/**
 * Returns the average reward after running `n_simuls` starting with `_cd`
 */
Reward Agent::random_simulation(const ClusterData& _cd, std::size_t n_simuls)
{
    auto action_trivial = [](const ClusterData& cd) {
        return cd.rep == CELL_NONE || cd.color == Color::Empty || cd.size < 2;
    };

    // If the provided action is trivial or invalid, return early
    if (action_trivial(_cd)) {
        return 0;
    }

    // Backup the StateData for restoring the state before returning
    const StateData sd_backup = *state.p_data;

    // The StateData object that will be used along the simulation.
    StateData sd_simul {};

    // The ActionData object that will be used along the simulation.
    ClusterData cd = _cd;

    // Apply the provided action but write the result on sd_simul
    bool action_valid = state.apply_action(cd, sd_simul);
    assert(action_valid);

    // Keep a copy of this first state to initialize all the simulations faster
    const StateData sd_simul_root = sd_simul;

    Reward total_reward = 0, score = evaluate_valid_action(cd);

    for (int i = 0; i < n_simuls; ++i) {
        // Apply random actions and accumulate rewards until the state is terminal
        while (1) {
            cd = state.apply_random_action();
            // Break when there are no more valid actions to perform.
            if (action_trivial(cd)) {
                break;
            }
            score += evaluate_valid_action(cd);
        }

        // Add the value of the terminal state to the score
        score += evaluate_terminal();
        // Accumulate
        total_reward += score;

        // Reset score and restore sd_simul to sd_simul_root for the next simulation.
        *state.p_data = sd_simul_root;
        score = 0;
    }

    Reward avg_reward = total_reward / n_simuls;

    // Restore the initial state
    *state.p_data = sd_backup;

    return avg_reward;
}

// ***************************************** Selection of Nodes ************************************//

//////////////////////////////////////////////////////
// SELECTION OF NODES:
//
// Here, we only search through the populated children edges of a node.
//
// NOTE: This should not be part of the same class since no use of any
//       actual state is done. Only the populated tree statistics.
///////////////////////////////////////////////////////

// "Upper Confidence Bound" of the edge
Reward Agent::ucb(Node* node, const Edge& edge)
{
    Reward result = edge.n_visits ? edge.reward_avg_visit : edge.val_best;

    result = result / 5000.0;

    // The UCT exploration term
    result += exploration_cst * sqrt((double)(log(node->n_visits)) / (double)(edge.n_visits + 1));

    return result;
}

// TODO Refactor the "best___" methods into one.
//      This way, we can call generate_clusters() only at the beginning of it.
//
Edge* Agent::best_ucb(Node* node)
{
    Edge* best = &EDGE_NONE;
    Reward best_val = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list()) {
        if (Reward r = ucb(current_node(), c); r > best_val) {
            best_val = r;
            best = &c;
        }
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        spdlog::warn("Why would I do that??");
        return &(node->children_list()[0]);
    }

    return best;
}

Edge* Agent::best_visits(Node* node)
{
    Edge* best = &EDGE_NONE;
    auto cur_best = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list()) {
        if (auto r = c.val_best; r > cur_best) {
            cur_best = r;
            best = &c;
        }
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        spdlog::warn("Why would I do that??");
        return &(node->children_list()[0]);
    }

    return best;
}

Edge* Agent::best_avg_val(Node* node)
{
    Edge* best = &EDGE_NONE;
    auto cur_best = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list()) {
        if (auto r = c.reward_avg_visit; r > cur_best) {
            cur_best = r;
            best = &c;
        }
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        spdlog::warn("Why would I do that??");
        return &(node->children_list()[0]);
    }

    return best;
}

Edge* Agent::best_val_best(Node* node)
{
    Edge* best = &EDGE_NONE;
    auto cur_best = std::numeric_limits<Reward>::min();

    for (auto& c : node->children_list()) {
        if (auto r = c.val_best; r > cur_best) {
            cur_best = r;
            best = &c;
        }
    }

    if (best == &EDGE_NONE && node->children_list().size() > 0) {
        spdlog::warn("Why would I do that??");
        return &(node->children_list()[0]);
    }

    return best;
}

//***************************** Playing actions ******************************/

//////////////////////////////
// NAVIGATION THROUGHOUT OUR DYNAMIC TREE
// (No use of the ref to state is done, only the 'snapshots'
//  that we use - e.g. the zobrist keys)
//////////////////////////////

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
    if (node->key & 1) {
        return node->key & 2;
    }

    return node->n_visits > 0 && node->n_children == 0;
}

////////////////////////////////////////
// INTERFACE BETWEEN THE STATE'S ACTION METHODS
// AND THE DYNAMIC TREE DATA
////////////////////////////////////////

//Sends a cluster as the action
void Agent::apply_action(const ClusterData& cd)
{
    assert(ply < MAX_PLY);

    stack[ply].ply = ply;
    stack[ply].cd = cd; // This action dictates the next state, and current state (data) will go to stack[ply].previous
    //stack[ply].current_action = cd.rep;

    //spdlog::debug("Before state apply action\n{}\n", state);

    state.apply_action(cd, states[ply]); // The state will store its new data on the passed StateData object
    ++ply;

    //spdlog::debug("After state apply action\n{}\n", state);
}

bool Agent::apply_action_blind(const ClusterData& cd)
{
    if (cd.color == Color::Empty || cd.size < 2 || cd.rep == CELL_NONE) {
        return false;
    }

    return state.apply_action_blind(cd);
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
    assert(ply > 1);
    state.undo_action(stack[ply - 1].cd); // NOTE: State holds a pointer to its previous StateData object
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

/////////////////////////////////////////////////////
// BUSINESS LOGIC FOR EVALUATION DURING MCTS
/////////////////////////////////////////////////////

/**
 * NOTE: This returns 1 for clusters consisting of a single cell
 * and 4 for empty clusters, hence the name of the function.
 */
Reward Agent::evaluate_valid_action(const ClusterData& cd) const
{
    return (cd.size - 2) * (cd.size - 2);
}

Reward Agent::evaluate_terminal() const
{
    return state.is_empty() ? 1000 : 0;
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

/////////////////////////////////////////////////////
// CONFIGURATION OF THE ALGORITHM
/////////////////////////////////////////////////////
void Agent::set_exploration_constant(double c)
{
    exploration_cst = c;
}

void Agent::set_max_iterations(int i)
{
    max_iterations = i;
}

///////////////////////////
// FOR BENCHMARKING AND RUNNING TESTS
//////////////////////////
void Agent::reset()
{
    MCTS.clear();
    // Reset configs
    max_iterations = MAX_ITER;
    exploration_cst = EXPLORATION_CST;
}

/////////////////////////////////////////////////////
// DEBUGGING UTILITIES
/////////////////////////////////////////////////////
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
       << ", val_avg=" << edge.reward_avg_visit;

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
    for (int i = 0; i < root->n_children; ++i) {
        auto edges = root->children_list();
        std::cerr << "Action " << (int)edges[i].cd.rep << ":\n";
        std::cerr << "Average value " << (int)edges[i].reward_avg_visit << '\n';
        std::cerr << "Best value " << (int)edges[i].val_best << '\n';
        std::cerr << "Number of visits " << edges[i].n_visits << '\n';
    }
}

double Agent::get_exploration_cst() const
{
    return exploration_cst;
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
