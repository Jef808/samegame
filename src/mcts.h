#ifndef __MCTS_H_
#define __MCTS_H_

#include <chrono>
#include <unordered_map>
#include <iostream>   // TODO move the debug stuff to a debug class and forward declare Agent there
#include "types.h"
#include "samegame.h"

namespace mcts {

struct ActionNode;
struct Node;

// FIXME Why SearchStack keeps complaining but ActionNode and Node are fine???
struct SearchStack {

    sg::Action* pv;
    sg::Action currentAction;
    int ply;
    Reward r;
};

// template<class Entry, int Size>
// struct HashTable {
//   Entry* operator[](sg::Key key) { return &table[(uint32_t)key & (Size - 1)]; }
//   private:
//  std::vector<Entry> table = std::vector<Entry>(Size);
//    };

// Holds the information known about the value of moves independantly from the
// mcts tree structure. To be used for extra quick searches.
//struct SearchStack;


class Agent {

public:
    using StateData = sg::StateData;
    using State = sg::State;
    using Action= sg::Action;

    Agent(State& state);

    Action MCTSBestAction();

    void create_root();
    bool computation_resources();
    Node* tree_policy();
    Reward rollout_policy(Node* node);
    void backpropagate(Node* node, Reward r);

    ActionNode* best_uct(Node* node);
    ActionNode* best_visits(Node* node);
    ActionNode* best_avg_val(Node* node);

    Node* current_node();
    bool is_root(Node* root);
    bool is_terminal(Node* node);
    void apply_action(Action move);
    void apply_action(Action move, StateData& sd);
    void undo_action();
    void undo_action(Action move);
    void init_children();

    Reward random_simulation(Action move);
    Reward evaluate_terminal();

    bool is_terminal(const StateData&);
    Reward evaluate_terminal(const StateData&);

    // Wether to backpropagate the minmax value of nodes or the rollout reward.
    static void set_backpropagate_minimax(bool);

    // Debugging
    void print_node(std::ostream&, Node*) const;
    void print_tree(std::ostream&, int depth) const;
    static void set_exp_c(double c);
    static void set_max_time(int t);
    static void set_max_iter(int i);
    static inline bool debug_counters      = false;
    static inline bool debug_main_methods  = false;
    static inline bool debug_tree          = false;
    static inline bool debug_best_visits   = false;
    static inline bool debug_init_children = false;
    static inline bool debug_random_sim    = false;

private:
    State& state;
    Node* root;

    int ply;
    int iteration_cnt;

    int rollout_cnt;
    int descent_cnt;
    int explored_nodes_cnt;

    // To keep track of nodes during the search (indexed by ply)
    std::array<Node*, MAX_PLY>       nodes;       // The nodes.
    std::array<ActionNode*, MAX_PLY> actions;     // The actions.
    std::array<StateData, MAX_PLY>   states;      // Utility allowing state to do and undo actions.
    std::array<SearchStack, MAX_PLY> stackBuf;  // Allows to perform independant without creading nodes.
};

struct ActionNode {
    sg::Action          action;
    int                 n_visits;
    Reward              prior_value;
    Reward              action_value;
    double              avg_action_value;
    bool                decisive;                // If it is a known win etc...
};

struct Node {
    // Careful when iterating through children, an ActionNode object doesn't have
    // a 'zero' value (so may be initialized with random noise).
    // NOTE: after the init_children() method, the children will be ordered by
    // their à priori value `prior_value`.
    using cont_children = std::array<ActionNode, MAX_CHILDREN>;

    Key                 key                              = 0;           // Zobrist Hash of the state
    int                 n_visits                         = 0;
    int                 n_children                       = 0;
    int                 n_expanded_children              = 0;
    sg::Action          last_action                      = sg::ACTION_NONE;
    bool                best_known;
    cont_children       children;

    cont_children& children_list() { return children; }
};

inline bool operator==(const Node& a, const Node& b)
{
    return a.key == b.key;
}




typedef std::unordered_map<Key, Node> MCTSLookupTable;

extern MCTSLookupTable MCTS;


} // namespace mcts

#endif // __MCTS_H_
