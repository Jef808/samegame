#ifndef __MCTS_H_
#define __MCTS_H_

#include "mcts_impl2/mcts_tree.h"
#include <memory>

namespace mcts_impl2 {

template < typename StateT,
           typename ActionT,
           size_t MAX_DEPTH >
class Mcts
{
public:
    using reward_type = typename StateT::reward_type;
    using ActionSequence = typename std::vector<ActionT>;
    enum class BackpropagationStrategy
        { avg_value, avg_best_value, best_value };
    enum class ActionSelectionMethod
        { by_ucb, by_n_visits, by_best_value };

    Mcts(StateT& _state) :
        m_state(_state),
        m_tree(_state.key()),
        p_current_node(m_tree.get_root()),
        m_root_state(m_state.clone())
    { }

    ActionT best_action(ActionSelectionMethod);
    ActionSequence best_action_sequence(ActionSelectionMethod = ActionSelectionMethod::by_best_value);

    void set_exploration_constant(double c)
        { exploration_constant = c; }
    void set_backpropagation_strategy(BackpropagationStrategy strat)
        { backpropagation_strategy = strat; }
    void set_max_iterations(unsigned int n)
        { max_iterations = n; }

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

    // Parameters
    double exploration_constant = 0.7;
    BackpropagationStrategy backpropagation_strategy = BackpropagationStrategy::best_value;
    unsigned int max_iterations;

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
    edge_pointer get_best_edge(ActionSelectionMethod);

    /**
     * Traverse the tree to the next leaf to be expanded, using the ucb criterion
     * to select an edge at each node in the process.
     */
    void select_leaf();

    /**
     * Play a random actions from the current state starting at the given action, until
     * the state is final. Repeat this process a number of times specified by the second (optional)
     * parameter and return the pair consisting of the average reward and the best reward, respectively.
     */
    std::pair<reward_type, reward_type> simulate_playout(const ActionT&, unsigned int = 1);

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
     *
     * @Note This increments the current node's number of visits by 1.
     */
    bool traverse_edge(edge_pointer);

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
     * Return true if the set computation resources (e.g. a set time) have not been depleted,
     * false otherwise.
     */
    bool computation_resources();

    /**
     * Using the given selection method, return the best path from root to leaf according to
     * the statistics collected thus far.
     *
     * @Note If the sequence reaches unvisited nodes, `best_traversal` completes it with
     * random choices of edges.
     */
    ActionSequence best_traversal(ActionSelectionMethod);

    reward_type evaluate(const ActionT& action)
        { return m_state.evaluate(action); }
    reward_type evaluate_terminal()
        { return m_state.evaluate_terminal(); }

    struct UCB;
    struct UCB2 { UCB2(double e, unsigned int n) {}};

    auto constexpr make_edge_cmp(ActionSelectionMethod) const;
};



// auto inline constexpr cmp_n_visits = [](const auto& a, const auto& b) {
//     return a.n_visits < b.n_visits;
// };
// auto inline constexpr cmp_best_value = [](const auto& a, const auto& b) {
//     return a.best_val < b.best_val;
// };
// auto inline constexpr get_ucb_cmp = []<typename UCB>(unsigned int node_n_visits, double expl_cst) {
//     return UCB(node_n_visits, expl_cst);
// };

} // namespace mcts_impl2

#include "mcts_impl2/mcts.hpp"

#endif
