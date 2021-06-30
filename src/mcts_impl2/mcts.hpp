#ifndef __MCTS_HPP_
#define __MCTS_HPP_

#include "mcts_impl2/mcts.h"

#include <cmath>
#include <functional>
#include <optional>
#include <vector>

namespace mcts_impl2 {


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
ActionT
Mcts<StateT, ActionT, MAX_DEPTH>::best_action(ActionSelectionMethod method) {
    run();
    auto edge = get_best_edge(method);
    apply_root_action(edge);
    return edge.action;
}


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, MAX_DEPTH>::ActionSequence
Mcts<StateT, ActionT, MAX_DEPTH>::best_action_sequence(ActionSelectionMethod method) {
    run();
    return best_traversal();
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::run() {
    m_current_node = m_tree.get_root();
    if (m_current_node.n_visits > 0 && m_current_node.children.size() == 0) {
        return m_actions_done;
    }
    while (computation_resources()) {
        step();
    }
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::step() {
    select_leaf();
    expand_current_node();
    backpropagate();
    return_to_root();
    ++iteration_cnt;
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::select_leaf() {
    while (m_current_node.n_visits > 0 && m_current_node.children.size() > 0) {
        edge_type& edge = get_best_edge(ActionSelectionMethod::ucb);
        traverse_edge(edge);
    }
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, MAX_DEPTH>::edge_type&
Mcts<StateT, ActionT, MAX_DEPTH>::get_best_edge(ActionSelectionMethod method) {
    auto cmp = make_edge_cmp(method);
    return *std::max_element(m_current_node.children.begin(), m_current_node.children.end(), cmp);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
std::pair<typename StateT::reward_type, typename StateT::reward_type>
Mcts<StateT, ActionT, MAX_DEPTH>::simulate_playout(const ActionT& action, unsigned int reps) {
    reward_type avg_reward = 0;
    reward_type best_reward = 0;
    std::optional<StateT> state_backup = std::optional<StateT>(reps > 1 ? m_state.clone() : std::nullopt);
    ActionT _action = action;

    for (auto i=0; i<reps; ++i) {
        reward_type score = 0;
        while (!m_state.is_terminal()) {
            score += evaluate(_action);
            _action = m_state.apply_random_action();
        }
        score += evaluate_terminal();
        
        avg_reward += avg_reward + (score - avg_reward) / (i+1);
        best_reward = std::max(best_reward, score);
        if (i < reps-1) {
                m_state.reset(state_backup);
        }
    }
    return std::pair{ avg_reward, best_reward };
}
    
template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::expand_current_node() {
    if (++m_current_node.n_visits > 1) {
        return;
    }
    auto init_child = [&](const auto& action) {
        edge_type new_edge { .action = action, };
        auto [avg_val, best_val] = simulate_playout(action);
        new_edge.avg_val = avg_val;
        new_edge.best_val = best_val;
        return new_edge;
    };
    auto valid_actions = m_state.valid_actions();

    std::transform(std::execution::par_unseq,
                   valid_actions.begin(), valid_actions.end(),
                   std::back_inserter(m_current_node.children), init_child);
}


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::backpropagate() {
    using BackpropagationStrategy::avg_value;
    using BackpropagationStrategy::avg_best_value;
    using BackpropagationStrategy::best_value;
    auto get_best_value = [](const auto& edge) { return edge.best_val; };
    auto get_avg_value  = [](const auto& edge) { return edge.avg_val; }; 

    reward_type val = [&](){
        if (m_current_node.children.empty()) {
            return evaluate_terminal();
        }
        if (backpropagation_strategy == best_value) {
            return *std::max_element(m_current_node.children.begin(), m_current_node.children.end(),
                                     get_best_value);
        }
        auto get_value = (backpropagation_strategy == avg_value ? get_avg_value : get_best_value);
        return std::accumulate(m_current_node.children.begin(), m_current_node.children.end(),
                               get_value) / m_current_node.children.size();
    }();

    m_tree.backpropagate(val);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::traverse_edge(edge_type& edge) {
    m_tree.traversal_push(&edge);
    ++m_current_node.n_visits;
    m_state.apply_action(edge.action);
    m_current_node = m_tree.get_node(m_state.key());
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, MAX_DEPTH>::ActionSequence
Mcts<StateT, ActionT, MAX_DEPTH>::best_traversal(ActionSelectionMethod method) {
    while (m_current_node.n_visits > 0) {
        edge_type& nex_edge = get_best_edge(method);
        traverse_edge(nex_edge);
        m_actions_done.push_back(nex_edge.action);
    }
    while (!m_state.is_terminal()) {
        ActionT action = m_state.apply_random_action();
        m_actions_done.push_back(action);
    }
    return m_actions_done;
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::return_to_root() {
    m_current_node = m_tree.get_root();
    m_state.reset(m_root_state);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::apply_root_action(const edge_type& edge) {
    m_root_state.apply_action(edge.action);
    m_actions_done.push_back(edge.action);
    m_current_node = m_tree.set_root(m_state.key());
}

template < typename StateT, typename ActionT, size_t MAX_DEPTH>
struct
Mcts<StateT, ActionT, MAX_DEPTH>::UCB {
    auto operator()(double C, unsigned int N) {
        return [C, N](const auto& a) {
            return a.avg_value + C * sqrt(log(N) / (a.n_visits + 1));
        };
    }
};

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
auto constexpr
Mcts<StateT, ActionT, MAX_DEPTH>::make_edge_cmp(ActionSelectionMethod method) const {
    switch(method)
    {
    case ActionSelectionMethod::by_ucb:
        return [ucb = UCB(exploration_constant, m_current_node.n_visits)](const auto& a, const auto& b) {
            return ucb(a) < ucb(b);
        };
    case ActionSelectionMethod::by_n_visits:
        return [](const auto& a, const auto& b) {
            return a.n_visits < b.n_visits;
        };
    case ActionSelectionMethod::by_best_value:
        return [](const auto& a, const auto& b) {
            return a.best_val < b.best_val;
        };
    }
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
bool
Mcts<StateT, ActionT, MAX_DEPTH>::computation_resources() {
    return iteration_cnt < max_iterations;
}


} // namespace mcts_impl2

#endif // MCTS_HPP_
