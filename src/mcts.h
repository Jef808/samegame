// mcts.h
#ifndef __MCTS_H_
#define __MCTS_H_

#include <unordered_map>
#include <sstream>
#include <iosfwd>
#include <limits>
#include "samegame.h"
#include "types.h"


 namespace mcts {


const int MAX_PLY            = 128;
const int MAX_CHILDREN       = 128;
const int MAX_ITER           = 1500;
const int MAX_TIME           = 10000;   // In milliseconds.
//const int MAX_NODES        = 10000;
const double EXPLORATION_CST = 0.04;
const bool use_time          = false;
const bool use_logging       = true;

struct Edge;
struct Node;

// Used to hold some information about actions independantly of the
// mcts tree structure. Allows sampling playouts without creating nodes.
struct SearchData {
  int              ply { };
  sg::ClusterData  cd  { };//{ sg::CELL_NONE, sg::Color::Empty, 0 };    // TODO Get rid of this. The SearchStack idea is for TEMPORARY stacks so we don't wan't to make copies for no reason
  Reward           r   { };
};

class Agent {

public:
  using State = sg::State;
  using StateData = sg::StateData;
  using ClusterData = sg::ClusterData;

  Agent(State& state);
  void init_search();

  ClusterData MCTSBestAction();
  void step();
  void set_root();
  bool computation_resources();
  Node* tree_policy();
  Reward rollout_policy(Node* node);    // TODO: If the playout hits a branch that clears the grid, return it from the whole algorithm.
  void backpropagate(Node* node, Reward r);

  Reward ucb(Node* node, const Edge& edge);

  Edge* best_ucb(Node* node);     // TODO : refactor into only one method taking a parameter
  Edge* best_visits(Node* node);
  Edge* best_avg_val(Node* node);
  Edge* best_val_best(Node* node);

  Node* current_node();
  bool is_root(Node* root);
  bool is_terminal(Node* node);
  void apply_action(Action action);
  ClusterData apply_action_blind(Action action);
  bool apply_action_blind(const ClusterData& cd);
  void apply_action(Action action, StateData& sd);
  void apply_action(const ClusterData& cd);
  void undo_action();
  void undo_action(const ClusterData& cd);
  void init_children();
  //Node* get_node(State&);
  int get_ply() const;

  Reward random_simulation(const ClusterData& cd, size_t n=1);

  Reward evaluate_valid_action(const ClusterData& cd) const;
  Reward evaluate_terminal() const;
  Reward value_to_reward(double);
  double reward_to_value(Reward);

  std::vector<Action> get_edges_from_root() const;

  // Use MinMax to evaluate and backpropagate when it is a 2players game.
  //const GameNbPlayers nb_players = GAME_1P;

  // Options and Debugging
  void set_exploration_constant(double);
  void set_max_iterations(int);
  void print_node(std::ostream&, Node*) const;
  void print_tree(std::ostream&, int depth) const;
  void reset();

  friend std::ostream& operator<<(std::ostream&, const Agent&);
  static std::string debug_tree_stats(const Agent&);
  static std::string debug_edge_stats(const Edge&);
  static std::string debug_node_stats(const Node&);
  void print_final_score() const;
  bool debug_tree_policy  = false;
  double get_exploration_cst() const;

  // Data
  State& state;
  Node* root;
  // To keep track of the data during the search (indexed by ply)
  std::array<Node*, MAX_PLY>         nodes;       // Elements are stored in the MCTSLookupTable
  std::array<Edge*, MAX_PLY>         actions;     // Elements are stored in their parent node
  std::array<StateData, MAX_PLY>     states;      // To keep history of states along a branch (stored on the heap)
  std::array<SearchData, MAX_PLY>    stack;       // To perform the playout samplings without creating new nodes
    // Counters for bookkeeping
  int cnt_iterations      ;
  int cnt_simulations     ;
  int cnt_descent         ;
  int cnt_explored_nodes  ;
  int cnt_rollout         ;
  int cnt_new_nodes       ;
  // Tree statistics
  double value_global_max;
  double value_global_min;
  double value_global_avg;
  int    global_max_depth;

private:
  int    ply             = 1;
  double exploration_cst = EXPLORATION_CST;
  int    max_iterations  = MAX_ITER;
};

struct Edge {
  sg::ClusterData  cd { sg::CELL_NONE, sg::Color::Empty, 0 };
  int              sg_value_from_root { 0 };
  int              val_best { 0 };
  Reward           reward_avg_visit { 0 };
  int              n_visits { 0 };
};

struct Node {
  using Edges = std::vector<Edge>;
  Key              key { 0 };
  int              n_visits { 0 };
  int              n_children { 0 };
  Edges            children { {} };

  Edges& children_list() { return children; }
};

inline bool operator==(const Edge& a, const Edge& b) {
  return a.cd == b.cd && a.val_best == b.val_best;
}

inline bool operator==(const Node& a, const Node& b) {
  return a.key == b.key;
}

extern std::ostream& operator<<(std::ostream& _out, const mcts::Agent& agent);
extern std::ostream& operator<<(std::ostream& _out, const mcts::Edge& edge);
extern std::ostream& operator<<(std::ostream& _out, const mcts::Node& node);

typedef std::unordered_map<Key, Node> MCTSLookupTable;

extern MCTSLookupTable MCTS;


} // namespace mcts

#endif // __MCTS_H_
