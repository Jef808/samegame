// mcts.h
#ifndef __MCTS_H_
#define __MCTS_H_

#include <unordered_map>
#include "samegame.h"
#include "types.h"


// template<class Entry, int Size>
// struct HashTable {
//   Entry* operator[](Key key) { return &table[(uint32_t)key & (Size - 1)]; }
//   private:
//  std::vector<Entry> table = std::vector<Entry>(Size);
//    };

namespace mcts {

struct Edge;
struct Node;

// Used to hold some information about actions independantly of the
// mcts tree structure. Allows sampling playouts without creating nodes.
struct SearchData {
  int ply;
  Action* pv;
  sg::ClusterData cd;
  Reward r;
};

class Agent {

public:
  using State = sg::State;
  using StateData = sg::StateData;
  using ClusterData = sg::ClusterData;

  //static void init();
  Agent(State& state);

  ClusterData MCTSBestAction();

  void create_root();
  bool computation_resources();
  Node* tree_policy();
  Reward rollout_policy(Node* node);    // TODO: If the playout hits a branch that clears the grid, return it from the whole algorithm.
  void backpropagate(Node* node, Reward r);

  Reward ucb(Node* node, Edge& edge);

  Edge* best_ucb(Node* node);     // TODO : refactor into only one method taking a parameter
  Edge* best_visits(Node* node);
  Edge* best_avg_val(Node* node);
  Edge* best_value_assured(Node* node);

  Node* current_node();
  bool is_root(Node* root);
  bool is_terminal(Node* node);
  void apply_action(Action action);
  void apply_action_blind(Action action);
  void apply_action(Action action, StateData& sd);
  void apply_action(const ClusterData& cd);
  void undo_action();
  void undo_action(const ClusterData& cd);
  void init_children();

  Reward random_simulation(const ClusterData& cd);
  Reward sg_value(const ClusterData& cd);
  Reward evaluate_terminal();

  // Use MinMax to evaluate and backpropagate when it is a 2players game.
  //const GameNbPlayers nb_players = GAME_1P;

  // Debugging
  void print_node(std::ostream&, Node*) const;
  void print_tree(std::ostream&, int depth) const;
  static void set_exp_c(double c);
  static void set_max_time(int t);
  static void set_max_iter(int i);
  void print_debug_cnts(std::ostream&) const;

private:
  State& state;
  Node* root;

  Reward value_global_max;

  int ply;
  int iteration_cnt;
  int cnt_simulations;
  int descent_cnt;
  int explored_nodes_cnt;
  int cnt_new_nodes;

  // To keep track of the data during the search (indexed by ply)
  // They are all 0-value initialized in the create_root() method
  // NOTE It might be too much to store all of that on the stack...
  std::array<Node*, MAX_PLY>        nodes;       // Elements are stored in the MCTSLookupTable
  std::array<Edge*, MAX_PLY>        actions;     // Elements are stored in their parent node
  std::array<StateData, MAX_PLY>    states;      // To keep history of states along a branch (stored on the heap)
  std::array<SearchData, MAX_PLY>   stack;       // To perform the playout samplings without creating new nodes
};

struct Edge {
  sg::ClusterData  cd;
  int     value_sg_from_root;
  int     n_visits;
  Reward  value_assured;
  Reward  reward_avg_visit;
  bool    decisive;                              // If it is a known win etc...
};

struct Node {
  // NOTE: Careful when iterating through edges, an Edge object doesn't have
  // a 'zero' value (so may be initialized with random noise).
  // NOTE: after the init_children() method, the children will be ordered by
  // their à priori value `prior_value`.
  using Edges = std::array<Edge, MAX_CHILDREN>;

  Key      key;
  int      n_visits;
  int      n_children;
  int      n_expanded_children;
  Edge*    parent;
  Edges    children;

  Edges& children_list() { return children; }
};

inline bool operator==(const Node& a, const Node& b) {
  return a.key == b.key;
}


typedef std::unordered_map<Key, Node> MCTSLookupTable;

extern MCTSLookupTable MCTS;


} // namespace mcts

#endif // __MCTS_H_
