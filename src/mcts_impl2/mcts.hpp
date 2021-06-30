#ifndef __MCTS_HPP_
#define __MCTS_HPP_

#include "mcts_impl2/mcts.h"

#include <optional>
#include <vector>

namespace mcts_impl2 {


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::traverse_edge(EdgeSelectionMethod method) {
     auto result = edge.avg_val;

}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::select_leaf() {

}


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
std::pair<typename StateT::reward_type, typename StateT::reward_type>
Mcts<StateT, ActionT, MAX_DEPTH>::simulate_playout(const ActionT& action, unsigned int reps, RewardAggregation agg) {
    reward_type avg_reward = 0;
    reward_type best_reward = 0;
    std::optional<StateT> state_backup = std::optional<StateT>(reps > 1 ? r_state.clone() : std::nullopt);
    ActionT _action = action;

    for (auto i=0; i<reps; ++i) {
        reward_type score = 0;
        while (!r_state.is_terminal()) {
            score += evaluate(_action);
            _action = r_state.apply_random_action();
        }
        score += evaluate_terminal();
        
        avg_reward += avg_reward + (score - avg_reward) / (i+1);
        best_reward = std::max(best_reward, score);
        if (i < reps-1) {
                r_state.reset(state_backup);
        }
    }
    return std::pair{ avg_reward, best_reward };
}
    
template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::expand_current_node() {
    auto init_child = [&](const auto& action) {
        edge_type new_edge { .action = action, };
        auto [avg_val, best_val] = simulate_playout(action);
        new_edge.avg_val = avg_val;
        new_edge.best_val = best_val;
        return new_edge;
    };
    auto valid_actions = r_state.valid_actions();

    std::transform(std::execution::par_unseq,
                   valid_actions.begin(), valid_actions.end(),
                   std::back_inserter(current_node.children), init_child);

    auto By_best_value = [](const auto& a, const auto& b)
        { return a.best_val < b.best_val; };
    // std::sort(std::execution::par_unseq,
    //           current_node.children.begin(),
    //           current_node.children.end(),
    //           By_best_value);
}


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::backpropagate(BackpropagationStrategy strat) {
    using BackpropagationStrategy::avg_value;
    using BackpropagationStrategy::avg_best_value;
    using BackpropagationStrategy::best_value;
    auto get_best_value = [](const auto& edge) { return edge.best_val; };
    auto get_avg_value  = [](const auto& edge) { return edge.avg_val; }; 

    reward_type val = [&](){
        if (current_node.children.empty()) {
            return evaluate_terminal();
        }
        if (strat == best_value) {
            return *std::max_element(current_node.children.begin(), current_node.children.end(),
                                     get_best_value);
        }
        auto get_value = (strat == avg_value ? get_avg_value : get_best_value);
        return std::accumulate(current_node.children.begin(), current_node.children.end(),
                               get_value) / current_node.children.size();
    }();

    m_tree.backpropagate(val);    
}


} // namespace mcts_impl2

#endif // MCTS_HPP_
