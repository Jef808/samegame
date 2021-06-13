// mcts.cpp
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <random>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/stopwatch.h>
#include <sstream>
#include "mcts.h"
#include "samegame.h"
//#include "dsu.h"
//#include "debug.h"

using namespace sg;

namespace mcts {


std::ostream& operator<<(std::ostream& _out, const mcts::Agent& agent) {
    return _out << Agent::debug_tree_stats(agent);
}

std::ostream& operator<<(std::ostream& _out, const mcts::Edge& edge) {
    return _out << Agent::debug_edge_stats(edge);
}

std::ostream& operator<<(std::ostream& _out, const ::mcts::Node& node) {
    return _out << Agent::debug_node_stats(node);
}

//************************** Node Lookup Table****************************/

MCTSLookupTable MCTS {};
const Action ACTION_NONE = ClusterData();
Edge EDGE_NONE = Edge();


Node* get_node(const sg::State& state)
{
    Key state_key = state.key();

    //auto [it, success] = MCTS.insert(std::make_pair(new_node.key, new_node));

    auto node_it = MCTS.find(state_key);
    if (node_it != MCTS.end()) {
        return &(node_it->second);
    }

    Node new_node = { .key = state_key, };
    auto new_node_it = MCTS.insert(std::make_pair(state_key, new_node)).first;

    return &(new_node_it->second);
}

// Used for choosing actions during the random_simulations.
namespace Random {

    std::random_device rd;
    std::mt19937 e { rd() }; // or std::default_random_engine e{rd()};
    std::uniform_int_distribution<int> dist { 0, MAX_CHILDREN };

    template < typename Cont, typename Cont::value_type _Default >
    typename Cont::value_type choose (const Cont& choices)
    {
        if (choices.empty())
            return typename Cont::value_type();
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

void init_logger()
{
    auto logging_lvl = (use_logging) ? spdlog::level::trace : spdlog::level::off;
    auto mcts_logger = spdlog::get("mcts_logger");
    if (mcts_logger.get() == nullptr) {
        mcts_logger = spdlog::rotating_logger_mt("mcts_logger", "logs/mcts.txt", 1048576 * 5, 3);
    }

    mcts_logger->set_level(logging_lvl);

    // if (!use_logging) {
    //     mcts_logger->set_level(spdlog::level::off);
    // }

    // mcts_logger->set_level(spdlog::level::trace);
}

// TODO: The problem right now is with the actions stack not getting initialized
Agent::Agent(sg::State& state)
    : state(state)
    , states()
    , stack()
{
    init_logger();
    init_data();
}

void Agent::init_data()
{
    ply                 = 1;
    cnt_iterations      = 0;
    cnt_simulations     = 0;
    cnt_descent         = 0;
    cnt_explored_nodes  = 0;
    cnt_rollout         = 0;
    cnt_new_nodes       = 0;
    value_global_max    = std::numeric_limits<double>::min();
    value_global_min    = std::numeric_limits<double>::max();
    value_global_avg    = 0;
    global_max_depth    = 0;
}

void Agent::init_search()
{
    // Backup the state descriptor in case it is stored higher up on the states stack
    // states[0] = *state.p_data;
    // state.p_data = &(states[0]);
    states[0] = state.clone_data();

    state.redirect_data_to(states[0]);

    nodes[1] = get_node(state);

    root = nodes[1];

    actions[0] = &EDGE_NONE;

    if (nodes[1]->n_visits == 0) {
        init_children();
    }
}

//******************************** Main methods ***************************/

//////////////////////////////////////////////////////////////////////////////////
// AT EVERY APPLY_ACTION, I WILL USE A StateData object COMING
// FROM THE STATES stack... AND AT EVERY STEP OF
// TREE_POLICY(), WE ASSIGN A POINTER TO AN EDGE TO ACTION[ply]...
////////////////////////////////////////////////////////////////////////////////////

// void Agent::set_root()
// {
//     if (use_time) {
//         init_time();
//     }

//     // Reset the search buffers (0 initialize our memory pool)
//     nodes = { {} };
//     actions = { {} };
//     stack = { {} };

//     // states[0] = *(state.p_data);
//     // state.p_data = &(states[0]);
//     states[0] = state.clone_data();
//     state.redirect_data_to(states[0]);

//     actions[0] = &EDGE_NONE;

//     // Set the pointer to root node
//     root = nodes[1] = get_node(state);

//     // Reset the counters
//     ply = 1;
//     global_max_depth = ply;
//     cnt_iterations = 0;
//     cnt_simulations = 0;
//     cnt_descent = 0;
//     cnt_explored_nodes = 0;
//     cnt_rollout = 0;
//     cnt_new_nodes = 0;

//     // NOTE : states[ply=1] will contain the result of applying action stack[ply].action, on calling
//     // apply_action for the first time, (and subsequently etc...), the state points state[ply=1]'s
//     // previous* pointer to the initial state, before applying the action.
//     // NOTE : It is the state's responsibility to update states[ply], we only pass it
//     // a (ref to a) StateData object when asking for an apply_action(), and the undo_action() reverts the
//     // state to state[ply]->previous

//     // NOTE: No need to generate clusters, a state constructs its key on initialization and apply_move so it
//     // always has a valid key to return.
//     // TODO: However, we need to implement tree_policy without calls to state.apply_action... only then can we
//     // defer state.generate_clusters() to the expansion phase (init_childrene).

//     //root = nodes[ply] = get_node(state);

//     // Reset stats
//     //value_global_max = std::numeric_limits<double>::min();
//     //value_global_min = std::numeric_limits<double>::max();

//     if (root->n_visits == 0) {
//         init_children();
//     }

//     spdlog::debug("After call to init_children, root has {}", root->n_children);
// }

///////////////////////////////////////////////
// THE 'USER INTERFACE' TO THE MCTS ALGORITHM
///////////////////////////////////////////////

void Agent::get_pv()
{
    assert(is_root(current_node()));
    stack[ply].reward = 0;
    ClusterData* p_cd;

    while (!is_terminal(current_node())) {
        p_cd = &(best_visits(current_node())->cd);
        apply_action(*p_cd);
    }
}

void Agent::display_pv()
{
    auto logger = spdlog::get("mcts_logger");

    int final_ply = ply;
    state.copy_data_from(states[0]);

    std::pair<State&, Cell> sa { state, CELL_NONE };
    logger->trace("\n{}\n\n", sa);

    Reward score = 0;

    for (int i=1; i<final_ply; ++i) {
        state.apply_action(stack[i].cd);
        //sa.first.apply_action(stack[i]);
        score += stack[i-1].reward;
        logger->trace("\n{}\nScore: {}", sa, score);
        sa.second = stack[i-1].cd.rep;
    }

    logger->trace("Final score: {}", score + evaluate_terminal());
}

ClusterData Agent::MCTSBestActions()
{
    auto logger = spdlog::get("mcts_logger");

    init_search();
    if (is_terminal(root)) {
        logger->trace("root is terminal??");
        return { CELL_NONE, Color::Empty, 0 };
    }

    while (computation_resources()) {
        step();
    }

    logger->trace("Finished {} steps, the best line of actions is\n", cnt_iterations);

    auto* choice = best_visits(root);

    logger->trace("Best action found: {}", *choice);

    return choice->cd;
}

void Agent::step()
{
    // TODO: This function is called in a loop, it's a bit ridiculous to reload this logger
    // every time...
    auto logger = spdlog::get("mcts_logger");

    logger->trace("Step {}", cnt_iterations);

    // TODO: During tree_policy, when computing a state's key, keep the data
    // of the state's clusters until we know that it's node isn't the one returned.
    Node* node = tree_policy();

    logger->trace("Chosen node: {}", *node);
    // TODO: Use the result of the above TODO to initialize the edges and start the
    // random simulations.
    Reward reward = rollout_policy(node);

    logger->trace("Rollout reward: {}", reward);

    backpropagate(node, reward);

    if (ply != 1) {
        logger->warn("ply is {} after backpropagate!", ply);
    }
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
    auto logger = spdlog::get("mcts_logger");

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
            logger->error("best_ucb returned invalid action {} on non-terminal state \n{}\n", actions[ply]->cd.rep, state);
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

        logger->trace("Applied action {}, now at node {}", *actions[ply-1], *nodes[ply]);
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
    auto logger = spdlog::get("mcts_logger");
    if (!logger) {
        spdlog::warn("Cannot find logger");
    }

    auto valid_actions = state.valid_actions_data();

    spdlog::debug("Entered init_children");
    //logger->trace("Init_children for node {}\nValid actions:\n    ", *current_node());

    assert(current_node()->n_visits == 0);

    spdlog::debug("Found current_node");

    for (auto act : valid_actions) {
         logger->trace("{}", act);
    }

    // assert(state.key() == current_node()->key);
    // assert(!is_terminal(current_node()));

    auto& children_edges = current_node()->children_list();
    children_edges.reserve(valid_actions.size());

    for (const auto& cd : valid_actions) {
        assert(cd.size > 1 && cd.color != Color::Empty && cd.rep != CELL_NONE);

        Reward simulation_reward = random_simulation(cd, 10);

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

        logger->trace("Pushing back action {}", new_action);
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
        auto const cd = stack[ply-1].cd;
        assert(cd.size > 1 && cd.rep != CELL_NONE && cd.color != Color::Empty);
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
Reward Agent::random_simulation(const ClusterData& _cd, size_t n_simuls)
{
    auto action_trivial = [](const ClusterData& cd) {
        return cd.rep == CELL_NONE || cd.color == Color::Empty || cd.size < 2;
    };

    // If the provided action is trivial or invalid, return early
    if (action_trivial(_cd)) {
        return evaluate_terminal();
    }

    // NOTE: No need to store a backup here, there is one at
    // states[ply-1]

    //const StateData sd_backup = state.clone_data();
    // The ActionData object that will be used along the simulation.
    auto cd = apply_action(_cd);

    // Temporary hack... if cd is trivial then nothing has been done in apply_action
    if (action_trivial(cd)) {
        spdlog::warn("argument to apply_action wasn't trivial but return value was");
        return evaluate_terminal();
    }
    // This time we store a backup because the subsequent apply_action calls
    // don't save any data (or increment the ply) for performance reasons.

    // TODO: In view of the above, the random_simulation method shouldn't be
    // using the same API as the tree_policy() method... it's just confusing.
    // Here I should just pass the state to a 'random_simulator' and not touch it
    // more.
    const StateData sd_simul_backup = state.clone_data();

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
        state.copy_data_from(sd_simul_backup);

        score = 0;
    }

    // undo the initial action we applied when entering the function
    undo_action();

    return total_reward / n_simuls;
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
    // if (node->key & 1) {
    //     return node->key & 2;
    // }

    return node->n_visits > 0 && node->n_children == 0;
}

////////////////////////////////////////
// APPLYING AND UNDOING ACTIONS
////////////////////////////////////////

// Sends a cluster as the action
// NOTE: After a call to Agent::apply_action(cd), the state's data
// points to the agent's states[ply-1]
//
// In other words, we use the agent's states[ply] to record the new state.
ClusterData Agent::apply_action(const ClusterData& _cd)
{
    assert(ply < MAX_PLY);

    // The state will store its new data on the passed StateData object
    ClusterData cd = state.apply_action(_cd, states[ply]);

    if (cd.size < 2 || cd.rep == CELL_NONE || cd.color == Color::Empty) {
        spdlog::debug("apply_action called with invalid action");
        return ClusterData {.rep = CELL_NONE, .color = Color::Empty, .size = 0};
    }

    // So at (this) ply, the action is given by stack[ply]
    stack[ply].cd = cd;
    stack[ply].ply = ply;
    stack[ply].reward = evaluate_valid_action(cd);

     ++ply;
    return cd;

 }

void Agent::undo_action()
{
    // If apply_action can return early and not increment ply... undo_action() should
    // do the same in exactly the same way
    if (stack[ply-1].cd.size < 2 || stack[ply-1].cd.rep == CELL_NONE || stack[ply-1].cd.color == Color::Empty) {
        spdlog::debug("undo_action called with invalid action on stack");
        return;
    }
    state.undo_action(stack[ply - 1].cd); // NOTE: State holds a pointer to its previous StateData object
    --ply;
}

//***************************** Evaluation of nodes **************************/

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

void Agent::reset()
{
    MCTS.clear();
    // Reset configs
    max_iterations = MAX_ITER;
    exploration_cst = EXPLORATION_CST;

    ply = 1;
    global_max_depth = ply;
    cnt_iterations = 0;
    cnt_simulations = 0;
    cnt_descent = 0;
    cnt_explored_nodes = 0;
    cnt_rollout = 0;
    cnt_new_nodes = 0;

    actions[0] = { };
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

} // namespace mcts
