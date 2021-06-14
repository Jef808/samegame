#ifndef MCTS_ROLLOUT_H_
#define MCTS_ROLLOUT_H_

#include <functional>
#include <limits>

namespace mctsimpl {


auto static constexpr DEFAULT_MAX_CHILDREN = 128;


template < typename StateT, typename MctsEval, typename EdgeT, typename RewardT=double >
RewardT random_simulation(StateT& r_state, EdgeT& initial_edge, int n_simuls=1) {
    auto action_descriptor = initial_edge->action_descriptor;
    RewardT total = 0;
    RewardT best = std::numeric_limits<RewardT>::min();
    auto init_state_descriptor = r_state.descriptor();

    for (auto i=0; i<n_simuls; ++i) {
        RewardT score = 0;
        while (!action_descriptor.is_trivial()) {
            score += MctsEval()(action_descriptor);
            r_state.apply(action_descriptor);
        }
        score += MctsEval()(r_state);
        r_state.reset(init_state_descriptor);

        best = score > best ? score : best;
    }

    initial_edge->best_val = best > initial_edge->best_val ? best : initial_edge->best_val;
    return total / n_simuls;
}


template < typename StateT, typename NodeT, typename EdgeT >
void populate_edges(StateT& state, NodeT* node, int n_simuls=1) {
    auto valid_actions_descriptors = state.valid_actions_descriptors();

    for (auto action_it = valid_actions_descriptors.begin();
         action_it != valid_actions_descriptors.end();
         ++action_it) {
        auto new_edge = EdgeT{ .action_descriptor=*action_it, };
        new_edge.val_best = random_simulation(state, new_edge, n_simuls);
        node->children.push_back(new_edge);
    }

    auto n_children = node->children.size() > DEFAULT_MAX_CHILDREN ? DEFAULT_MAX_CHILDREN : node->children.size();

    std::partial_sort(begin(node->children), begin(node->children) + n_children, end(node->children),
        [](const auto& a, const auto& b) {
            return a.val_best > b.val_best;
        });

    node->children.resize(n_children);
}


} //namespace mctsimpl

#endif // MCTS_ROLLOUT_H_
