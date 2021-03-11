#include <iostream>
#include <cassert>
#include <math.h>
#include <random>
#include "mcts.h"
#include "debug.h"


namespace mcts {

MCTSLookupTable MCTS;


using namespace sg;

//****************************** Utility functions ***********************/

// get_node queries the Hash Table until it finds the position,
// creating a new entry in case it doesn't find it.
Node* get_node(const State& state)
{
    Key state_key = state.key();

    auto node_it = MCTS.find(state_key);
    if (node_it != MCTS.end())
    {
        return &(node_it->second);
    }

    // Insert the new node in the Hash table if it wasn't found.
    Node new_node;
    new_node.key                    = state_key;
    //new_node.last_action              = actions[ply]->action;

    auto new_node_it = MCTS.insert(std::make_pair(state_key, new_node)).first;
    return &(new_node_it->second);
}

// Used for choosing actions during the random_simulations.
namespace Random {

    std::random_device rd;
    std::mt19937 e{rd()}; // or std::default_random_engine e{rd()};
    std::uniform_int_distribution<int> dist{0, MAX_CHILDREN};

    Action choose(const std::vector<Action>& choices)
    {
        if (choices.empty())
            return ACTION_NONE;
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
    , nodes{}
    , stackBuf{}
{
    create_root();
}

//******************************** Main methods ***************************/

Action Agent::MCTSBestAction()
{
    init_time();
    create_root();

    while (computation_resources())
    {
        Node* node = tree_policy();
        Reward reward = rollout_policy(node);
        backpropagate(node, reward);
        ++iteration_cnt;
    }

    if (debug_counters)
    {
        std::cerr << "Iterations: " << iteration_cnt << '\n';
        std::cerr << "Descent count: " << descent_cnt << '\n';
        std::cerr << "Rollout count: " << rollout_cnt << '\n';
        std::cerr << "Exploration count: " << explored_nodes_cnt << '\n';
        std::cerr << "Number of nodes in table: " << MCTS.size() << std::endl;
    }

    auto* choice = best_visits(root);

    if (debug_main_methods)
        std::cerr << "returning from main method" << std::endl;

    return choice->action;
}

void Agent::create_root()
{
    ply                 = state.data->ply;
    states[ply].ply = ply;
    states[ply].key     = state.key();

    // Reset the buffers
    nodes = {};

    // Reset the counters
    iteration_cnt       = 0;
    rollout_cnt         = 0;
    descent_cnt         = 0;
    explored_nodes_cnt  = 0;

    root = nodes[ply] = get_node(state);

    if (root->n_visits == 0)
    {
        init_children();
    }

    // TODO Clear the part of the tree that's dismissed in the cache
}

bool Agent::computation_resources()
{
    bool res =  iteration_cnt < MAX_ITER;

    if (use_time)
        res &= (time_elapsed() < MAX_TIME - 100);    // Clock keeps running when using gdb.

    return res;
}

Node* Agent::tree_policy()
{
    assert(current_node() == root);

    if (debug_tree)
    {
        Debug::display(std::cerr, state);
        std::cerr << "Initializing descent " << descent_cnt << std::endl;
    }

    if (root->n_children == 0)
        {
            if (debug_tree)
                std::cerr << "Root has no children" << std::endl;
            return root;
        }

    ++descent_cnt;

    // At each previously explored node, choose an action in the direction that
    // is "most important" to sample in the tree. (i.e. minimizing regret in long
    // or short term)
    while (current_node()->n_visits > 0)
    {
        if (is_terminal(current_node()))
        {
            if (debug_tree)
                std::cerr << "Terminal node hit." << std::endl;

            return current_node();
        }

        // The choice of Edge (action) at each node is driven by the uct policy
        actions[ply] = best_uct(current_node());

        // Keep record of number of times each part of the tree has been sampled.
        ++current_node()->n_visits;

        Action action = actions[ply]->action;  // Keep record of the path we're tracing to go back along it.
        assert(action != ACTION_NONE);       // TODO Remove this
        apply_action(action);

        nodes[ply] = get_node(state);    // Either the node has been seen and is associated to a state key,
                                         // or not and get_node creates a record of it.

        if (debug_tree)
        {
            Debug::display(std::cerr, state);
            std::cerr << "Continuing descent..." << std::endl;
        }

        // FIXME How to deal with different paths (different nodes in the tree) leading
        // to equivalent state position? The state key will be the same but not the nodes.
        // Have to make sure to only keep the best path and reorganize the tree without losing information.
        // NOTE in this direction : currently the last_move field of a Node object isn't used! it can have
        // multiple parents. This most likely messes with the algorithm by favoring positions that arise
        // more frequenty.
    }

    // At this point, we reached an unexplored node and it is time to explore the game tree from
    // there.
    if (debug_main_methods)
    {
        std::cerr << "Chosen node \n";
        Debug::display(std::cerr, state);
        std::cerr << std::endl;
    }

    return current_node();
}

// Once we have the next unexplored node, we do a random
// playout starting with every one of its children.
Reward Agent::rollout_policy(Node* node)
{
    if (debug_main_methods)
    {
        std::cerr << "Rollout on node with state \n";
        Debug::display(std::cerr, state);
    }

    assert(node == current_node());

    // NOTE: this doesn't seem to be necessary, we do
    // the same check when doing the random_simulation. Well, it would be
    // node->children will be { MOVE_END } and we would need to make a check before return
    // the prior_value. That would all be a waste of function calls for nothing.
    // NOTE: This is in big part why the algorithm is implemented by storing the values in the
    // edges. Maybe it is all taken cared of.
    //
    if (is_terminal(node))
    {
        ++node->n_visits;
        return 1 - evaluate_terminal();
    }

    assert(node->n_visits == 0);

    init_children();                      // Expand the node and do a rollout on each child, return max reward.

    return node->children[0].prior_value;
}

// Note: as in Stockfish's, we could backpropagate minimax of avg_value instead of rollout reward.
// (the more confident we are in our sampling, the more we want to propagate extremal results only?)
void Agent::backpropagate(Node* node, Reward r)
{
    assert(node == current_node());

    while (current_node() != root)
    {
        undo_action();

        r = 1.0 - r;    // Undoing action changes player.

        ActionNode* action = actions[ply];

        ++action->n_visits;

        action->action_value += r;
        action->avg_action_value = action->action_value / action->n_visits;

        //if (action->decisive)

        // This seem to take care of my whole "action decisive". I just need to make extremals rarer.
        if (propagate_minimax)
            r = best_avg_val(current_node())->avg_action_value;

        // Adjust/change r here as wanted.
    }
}

Reward Agent::evaluate_terminal()
{
    Reward r = evaluate_terminal(states[ply]);
    // record that the leading action is "decisive" if terminal state is a win.
    actions[ply-1]->decisive = r == 1;

    return r;
}

ActionNode* Agent::best_uct(Node* node)
{
    if (debug_tree)
    {
        std::cerr << "Choosing best uct\n";
        Debug::display(std::cerr, state);
        std::cerr << "Children are :\n";
        for (int i=0; i<node->n_children; ++i)
        {
            auto c = node->children[i];
            auto val = (double)c.avg_action_value + exploration_cst * sqrt( (double)(2 * log(node->n_visits)) / (double)(c.n_visits+1) );

            std::cerr << "Action " << c.action << " visits " << c.n_visits << " prior  " << c.prior_value << " avg_val " << c.avg_action_value << '\n';
            std::cerr << "    uct term : " << std::fixed << val;
            std::cerr << "    for uct value : " << std::fixed << c.avg_action_value + val << "\n\n";
        }

        std::cerr << "Press c" << std::endl;
    }
    auto best = 0;
    auto best_val = -std::numeric_limits<double>::max();

    for (int i=0; i<node->n_children; ++i)
    {
        auto* c = &(node->children[i]);

        if (c->n_visits < 1)
        {
            if (debug_tree)
                std::cerr << "\nChoosing " << c->action << std::endl;
            return c;    // children are already ordered by à priori value in this case.n
        }

        Reward r = (double)c->avg_action_value + exploration_cst * sqrt( (double)(2 * log(node->n_visits)) / (double)(c->n_visits+1) );
        if (r > best_val)
        {
            best_val = r;
            best = i;
        }
    }

    if (debug_tree)
        std::cerr << "\nChoosing " << node->children[best].action << std::endl;

    return &(node->children[best]);
}

ActionNode* Agent::best_visits(Node* node)
{
    if (debug_best_visits)
        std::cerr << "Choosing best visits. choices are :";

    int best = 0;
    auto best_val = -std::numeric_limits<int>::max();

    auto& children = node->children_list();

    for (int i=0; i<node->n_children; ++i)
    {
        // if (node->children[i].action == ACTION_NONE)
        //     continue;
        auto v = children[i].n_visits;

        if (debug_best_visits)
            std::cerr << "Action " << children[i].action << " with " << v << " visits and mean value " << children[i].avg_action_value << std::endl;

        if (v > best_val)
        {
            best_val = v;
            best = i;
        }
    }

    if (debug_best_visits)
        std::cerr << "Returning with action " << node->children[best].action << std::endl;

    return &(node->children[best]);
}

ActionNode* Agent::best_avg_val(Node* node)
{
    // if (debug_best_visits)
    //     std::cerr << "Choosing best visits. choices are :";

    int best = 0;
    auto best_val = -std::numeric_limits<int>::max();

    auto& children = node->children_list();

    for (int i=0; i<node->n_children; ++i)
    {
        // if (node->children[i].action == ACTION_NONE)
        //     continue;
        auto v = children[i].avg_action_value;

        // if (debug_best_visits)
        //     std::cerr << "Action " << children[i].action << " with " << v << " visits and mean value " << children[i].avg_action_value << std::endl;

        if (v > best_val)
        {
            best_val = v;
            best = i;
        }
    }

    // if (debug_best_visits)
    //     std::cerr << "Returning with action " << node->children[best].action << std::endl;

    return &(node->children[best]);
}

//***************************** Playing actions ******************************/

Node* Agent::current_node()
{
    return nodes[ply];
}

bool Agent::is_terminal(Node* node)
{
    assert(node == current_node());
    return states[ply].key & 1;
}

void Agent::init_children()
{
    //auto& children      = current_node()->children_list();
    assert(current_node()->n_children == 0);

    if (debug_init_children)
    {
        std::cerr << "Initializing children of node\n";
        Debug::display(std::cerr, state);
        std::cerr << std::endl;
    }

    // Make a local copy because the state's valid_actions
    // container will change during the random simulations.
    auto valid_actions = state.valid_actions();

    if (debug_init_children)
    {
        std::cerr << "With actions ";
        for (const auto& a : valid_actions) { std::cerr << a << ' '; }
        std::cerr << std::endl;
    }

    ++explored_nodes_cnt;

    for (auto action : valid_actions)
    {
        assert(action != ACTION_NONE);

        // std::cerr << "Starting random simulation at ply " << ply << " and state \n";
        // Debug::display(std::cerr, state);
        // std::cerr << std::endl;

        Reward prior = random_simulation(action);
        ++rollout_cnt;

        ActionNode new_action;
        new_action.action = action;
        new_action.n_visits = 0;
        new_action.prior_value = prior;
        new_action.action_value = 0;
        new_action.avg_action_value = 0;

        if (current_node()->n_children < MAX_CHILDREN)
        {
            current_node()->children_list()[current_node()->n_children] = new_action;
            ++(current_node()->n_children);
        }
    }

    if (debug_init_children)
        std::cerr << "initialized all children" << std::endl;

    ++current_node()->n_visits;

    std::sort(current_node()->children_list().begin(), current_node()->children_list().begin() + current_node()->n_children, [](const auto& a, const auto& b){
            return a.prior_value > b.prior_value;
        });

    // A sentinel to easily iterate through children.
    if (current_node()->n_children < MAX_CHILDREN)
    {
        ActionNode new_action;
        new_action.action = ACTION_NB;
        new_action.n_visits = 0;
        new_action.prior_value = 0;
        new_action.action_value = 0;
        new_action.avg_action_value = 0;

        current_node()->children_list()[current_node()->n_children] = new_action;
    }
}

void Agent::apply_action(Action action)
{
    ++ply;
    state.apply_action(action, states[ply]);
}

void Agent::undo_action()
{
    --ply;
    state.undo_action(actions[ply]->action);
}

// For using the search stack instead of the node stack.
void Agent::undo_action(Action action)
{
    --ply;
    state.undo_action(action);
}

//***************************** Evaluation of nodes **************************/

// TODO I need the search stack to record which actions I need to undo!!
//
// First idea would be to not save or lookup anything, but save the next-to-last state
// (really, state-before-known-win/lose) to start building a table of alphas and betas.
Reward Agent::random_simulation(Action action)
{
    if (debug_random_sim)
    {
        Debug::display(std::cerr, state);
        std::cerr << std::endl;
        std::cerr << "Random simulation, ply " << ply << ", next action is " << action << std::endl;
    }

    stackBuf[ply].ply = ply;
    stackBuf[ply].currentAction = action;
    stackBuf[ply].r = 0;

    apply_action(action);

    if (is_terminal(states[ply]))
    {
        if (debug_random_sim)
        {
            Debug::display(std::cerr, state);
            std::cerr << std::endl;
            std::cerr << "Reached terminal state at ply " << ply << std::endl;
            std::cerr << "Evaluation : " << std::fixed << evaluate_terminal(states[ply])  << std::endl;
        }

        stackBuf[ply-1].r += evaluate_terminal(states[ply]);
    } else
    {
        Action m = Random::choose(state.valid_actions());

        stackBuf[ply-1].r += 1 - random_simulation(m);
    }

    undo_action(stackBuf[ply-1].currentAction);

    if (debug_random_sim)
    {
        std::cerr << "After undoing action " << stackBuf[ply].currentAction << ", evaluation at ply " << ply << ": " << stackBuf[ply].r << std::endl;
        std::cerr << "State's ply : " << state.m_ply << std::endl;
    }

    return stackBuf[ply].r;
}


bool Agent::is_terminal(const StateData& sd)
{
    return sd.key & 1;
}
Reward Agent::evaluate_terminal(const StateData& sd)
{
    std::cerr << "EVALUATE(sd) NOT IMPLEMENTED YET!" << std::endl;
    return 0;
}


//************************************** DEBUGGING ***************************************/

// void Agent::set_exp_c(double c) { Agent::exploration_cst = c; }
// void Agent::set_max_time(int t) { Agent::MAX_TIME = t;  }
// void Agent::set_max_iter(int i) { Agent::MAX_ITER = i; }

void Agent::print_node(std::ostream& _out, Node* node) const
{
    _out << "Node: v=" << node->n_visits << std::endl;
    for (int i=0; i<node->n_children; ++i)
    {
        auto c = node->children[i];
        _out << "    Action " << c.action << ": v=" << c.n_visits << ", val=" << c.avg_action_value << std::endl;
    }
}

void Agent::print_tree(std::ostream& _out, int depth) const
{
    print_node(_out, root);
}

// Reward Agent::evaluate_terminal()
// {
//     Key key = states[ply].statusKey;

//     Reward r = key >> 2 & 1 ? 0
//                             : key >> 1 & 1 ? (1 - (ply & 1))
//                             : ply & 1;
//     return r;
// }

} // namespace mcts
