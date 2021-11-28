// As things are, StateT has to implement the following methods:
//
// - is_trivial(const ActionT& action) determining if an action is trivial.
// - evaluate(const ActionT& action)
// - evaluate_terminal()
// - valid_actions_data() returning a vector containing all valid actions
// - apply_random_action()
// - apply_action(const ActionT& action)
// - key()

#ifndef __MCTS_H_
#define __MCTS_H_

#include "mcts_tree.h"
#include "policies.h"

namespace mcts {

template<typename StateT,
         typename ActionT,
         typename UCB_Functor = policies::Default_UCB_Func,
         typename Playout_Functor = typename policies::Default_Playout_Func<StateT, ActionT>,
         size_t MAX_DEPTH = 128>
class Mcts
{
 public:
  using state_type = StateT;
  using action_type = ActionT;
  using reward_type = typename StateT::reward_type;
  using ActionSequence = typename std::vector<ActionT>;
  enum class BackpropagationStrategy
  {
    avg_value,
    avg_best_value,
    best_value
  };
  enum class ActionSelection
  {
    by_ucb,
    by_n_visits,
    by_avg_value,
    by_best_value
  };

  Mcts(StateT& state,
       UCB_Functor ucb_func = policies::Default_UCB_Func{})
    : m_state(state),
      m_tree(state.key()),
      p_current_node(m_tree.get_root()),
      m_root_state(state),
      UCB_Func(ucb_func)
  {
  }

  ActionT best_action(ActionSelection);
  ActionSequence
      best_action_sequence(ActionSelection = ActionSelection::by_best_value);

 private:
  using Tree = MctsTree<StateT, ActionT, MAX_DEPTH>;
  using node_type = typename Tree::Node;
  using edge_type = typename Tree::Edge;
  using node_pointer = typename Tree::node_pointer;
  using edge_pointer = typename Tree::edge_pointer;

  StateT& m_state;
  Tree m_tree;
  node_pointer p_current_node;
  StateT m_root_state;
  ActionSequence m_actions_done;
  UCB_Functor UCB_Func;

  // Parameters
  double exploration_constant = 0.4;
  BackpropagationStrategy backpropagation_strategy =
      BackpropagationStrategy::avg_value;
  unsigned int max_iterations;
  unsigned int max_time = 20000;
  bool use_time = (max_time > 0);

  // Counters
  unsigned int iteration_cnt = 0;

  /**
     * Run the algorithm until the `computation_resources()` returns false.
     */
  void run();

  /**
     * Complete a full cycle of the algorithm.
     */
  void step();

  /**
     * Select the best edge from the current node according to the given method.
     */
  edge_pointer get_best_edge(ActionSelection);

  /**
   * The *Selection* phase of the algorithm.
   */
  edge_pointer Select_next_edge();

  /**
     * Traverse the tree to the next leaf to be expanded, using the ucb criterion
     * to select an edge at each node in the process.
     */
  void select_leaf();

  /**
     * Play a random actions from the current state starting at the given action, until
     * the state is terminal. Return the total score.
     *
     * The function is templated so that we can specify how to call state.apply_random_action()
     * depending on the context.
     */
  reward_type simulate_playout(const ActionT&);

  /**
     * For when the current node is a leaf, run `simulate_playout` on all the state's
     * valid actions and populate the current node with children edges corresponding
     * to those actions.
     *
     * @Note This increments the node's number of visits by 1 (and only does that when the
     * node had been visited before but is terminal).
     */
  void expand_current_node();

  /**
     * After expanding the leaf node, update the statistics of all edges connecting it
     * to the root with the results obtained from the simulated playouts, according to
     * the set BackpropagationStrategy.
     */
  void backpropagate();

  /**
     * Apply the edge's action to the state and update `m_current_node`.
     */
  void traverse_edge(edge_pointer);

  /**
     * Resets `m_current_node` with a reference to the root node, and reset the
     * state with the data from `m_root_state`.
     */
  void return_to_root();

  /**
     * Apply the edge's action to the state and change the root to be that
     * new state.
     *
     * @Note The action is pushed at the back of `m_actions_done`.
     */
  void apply_root_action(const edge_type& edge);

  /**
     * Using the given selection method, return the best path from root to leaf according to
     * the statistics collected thus far.
     *
     * @Note If the sequence reaches unvisited nodes, `best_traversal` completes it with
     * random choices of edges.
     */
  ActionSequence best_traversal(ActionSelection);

  /**
     * The evaluation of the states.
     */
  reward_type evaluate(const ActionT&);

  /**
   * Evaluation of terminal states.
   */
  reward_type evaluate_terminal();

  /**
   * Return true if the agent can continue with the algorithm.
   */
  bool computation_resources();

  /**
   * Initialize the counters and time to 0.
   */
  void init_counters();

 public:
  void set_exploration_constant(double c) { exploration_constant = c; }
  void set_backpropagation_strategy(BackpropagationStrategy strat)
  {
    backpropagation_strategy = strat;
  }
  void set_max_iterations(unsigned int n) { max_iterations = n; }
  void set_max_time(unsigned int t) { max_time = t; }

  unsigned int get_iterations_cnt() { return iteration_cnt; }
  size_t get_n_nodes() { return m_tree.size(); }
};

} // namespace mcts

#include "mcts.hpp"

#endif
