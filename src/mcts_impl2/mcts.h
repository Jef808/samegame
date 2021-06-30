#ifndef __MCTS_H_
#define __MCTS_H_

#include "mcts_impl2/mcts_tree.h"

namespace mcts_impl2 {

template < typename StateT,
           typename ActionT,
           size_t MAX_DEPTH >
class Mcts
{
public:
    using reward_type = typename StateT::reward_type;
    using ActionSequence = typename std::vector<ActionT>;

    Mcts(StateT& _r_state) :
        r_state(_r_state), m_tree() { }

    ActionSequence best_action_sequence();

private:
    using Tree = MctsTree<StateT, ActionT, MAX_DEPTH>;
    using node_type = typename Tree::Node;
    using edge_type = typename Tree::Edge;
    using node_pointer = typename Tree::node_pointer;

    StateT& r_state;
    Tree m_tree;
    node_type& current_node;

    enum class EdgeSelectionMethod
        { by_ucb, by_n_visits, by_avg_value, by_best_value };
    enum class BackpropagationStrategy
        { avg_value, avg_best_value, best_value };

    reward_type evaluate(const ActionT& action)
        { return r_state.evaluate(action); }
    reward_type evaluate_terminal()
        { return r_state.evaluate_terminal(); }

    /**
     * Select the best edge from the current node according to the given criterion
     * and update the current_node pointer to the resulting node.
     */
    void traverse_edge(EdgeSelectionMethod);

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
     */
    void expand_current_node();

    /**
     * After expanding the leaf node, update the statistics of all edges connecting it
     * to the root according to the results obtained from the simulated playouts.
     *
     * @NOTE One can specify to backpropagate the best result, the average result,
     * or the average of the best results.
     */
    void backpropagate(BackpropagationStrategy = BackpropagationStrategy::Best_value);
};



} // namespace mcts_impl2

#endif
