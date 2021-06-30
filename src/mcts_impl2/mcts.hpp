#ifndef __MCTS_HPP_
#define __MCTS_HPP_

#include "mcts_impl2/mcts_tree.h"
#include "mcts_impl2/mcts.h"

#include <cassert>
#include <cmath>
#include <functional>
#include <optional>
#include <numeric>
#include <vector>
#include <iostream>

namespace mcts_impl2 {

template < typename ActionT >
std::pair<int, int> to_coords(const ActionT& action) {
    int x = action.rep % 15;
    int y = action.rep / 14;
    return std::pair{ x, 14 - y };
}


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
    return best_traversal(method);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::run() {
    p_current_node = m_tree.get_root();
    if (p_current_node->n_visits > 0 && p_current_node->children.size() == 0) {
        return;
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
    ++iteration_cnt;
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::select_leaf() {
    return_to_root();
    while (p_current_node->n_visits > 0 && p_current_node->children.size() > 0) {
        ++p_current_node->n_visits;
        edge_pointer edge = get_best_edge(ActionSelectionMethod::by_ucb);
        assert(edge != nullptr);
        bool traversal_check = traverse_edge(edge);
        if (!traversal_check) {
            auto [x, y] = to_coords(edge->action);
            std::cerr << "WARNING! In select_leaf: Tried to traverse with action " << edge->action
                      << " which is (" << x << ", " << y << ")"
                      << " on state\n" << m_state << std::endl;
            return;
        }
    }
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, MAX_DEPTH>::edge_pointer
Mcts<StateT, ActionT, MAX_DEPTH>::get_best_edge(ActionSelectionMethod method) {
    auto cmp = make_edge_cmp(method);
    return &*std::max_element(p_current_node->children.begin(), p_current_node->children.end(), cmp);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
std::pair<typename StateT::reward_type, typename StateT::reward_type>
Mcts<StateT, ActionT, MAX_DEPTH>::simulate_playout(const ActionT& action, unsigned int reps) {
    reward_type avg_reward = 0;
    reward_type best_reward = 0;
    const StateT state_backup = m_state.clone();
    ActionT _action = action;

    for (auto i=0; i<reps; ++i) {
        reward_type score = 0;
        while (!m_state.is_trivial(_action)) {
            score += evaluate(_action);
            _action = m_state.apply_random_action();
        }
        score += evaluate_terminal();
        
        avg_reward += avg_reward + (score - avg_reward) / (i+1);
        best_reward = std::max(best_reward, score);
        m_state.reset(state_backup);
    }
    return std::pair{ avg_reward, best_reward };
}
    
template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::expand_current_node() {
    if (p_current_node->n_visits > 1) {
        ++p_current_node->n_visits;
        return;
    }
    // auto init_child = [&](const auto& action) {
    //     edge_type new_edge { .action = action, };
    //     auto [avg_val, best_val] = simulate_playout(action);
    //     new_edge.avg_val = avg_val;
    //     new_edge.best_val = best_val;
    //     return new_edge;
    // };
    auto valid_actions = m_state.valid_actions_data();

    for (auto a : valid_actions) {
        if (a.size != m_state.get_cd(a.rep).size) {
            std::cerr << "WARNING! Corrupted Action: " << a
                      << " which is (" << a.rep % 15 << ", " << 14 - (a.rep / 14)
                      << ") on state\n" << m_state << std::endl;
        }
        edge_type new_edge { .action = a, };
        auto [avg_val, best_val] = simulate_playout(a);
        new_edge.avg_val = avg_val;
        new_edge.best_val = best_val;
        p_current_node->children.push_back(new_edge);
    }

    ++p_current_node->n_visits;

    // std::transform(valid_actions.begin(), valid_actions.end(),
    //                std::back_inserter(p_current_node->children), init_child);
}


template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::backpropagate() {
    using BackpropagationStrategy::avg_value;
    using BackpropagationStrategy::avg_best_value;
    using BackpropagationStrategy::best_value;

    auto cmp_best_value = [](const auto& a, const auto& b) {
        return a.best_val < b.best_val;
    };

    reward_type val = [&](){
        if (p_current_node->children.empty()) {
            return evaluate_terminal();
        }
        if (backpropagation_strategy == best_value) {
            return std::max_element(p_current_node->children.begin(), p_current_node->children.end(),
                                    cmp_best_value)->best_val;
        }
        auto get_value = [&](const auto& edge) {
            return backpropagation_strategy == avg_value ? edge.avg_val : edge.best_val;
        };
        double total = std::transform_reduce(p_current_node->children.begin(), p_current_node->children.end(),
                                             0, std::plus<double>(), get_value);

        return total / p_current_node->children.size();
    }();

    m_tree.backpropagate(val);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
bool
Mcts<StateT, ActionT, MAX_DEPTH>::traverse_edge(edge_pointer edge) {
    m_tree.traversal_push(edge);
    bool success = m_state.apply_action(edge->action);
    if (success) {
        p_current_node = m_tree.get_node(m_state.key());
    }
    return success;
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, MAX_DEPTH>::ActionSequence
Mcts<StateT, ActionT, MAX_DEPTH>::best_traversal(ActionSelectionMethod method) {
    return_to_root();
    edge_pointer p_nex_edge;
    while (p_current_node->n_visits > 0 && p_current_node->children.size() > 0) {
        p_nex_edge = get_best_edge(method);
        bool traverse_success = traverse_edge(p_nex_edge);
        if (!traverse_success) {
            auto [x, y] = to_coords(p_nex_edge->action);
            std::cerr << "WARNING! in best_traversal:\nTried to traverse with action " << p_nex_edge->action
                      << " which is (" << x << ", " << y << ")"
                      << " on state\n" << m_state << std::endl;
            break;
        }
        m_actions_done.push_back(p_nex_edge->action);
    }

    if (p_current_node->n_visits > 1) {
        return m_actions_done;
    }

    ActionT action = m_state.apply_random_action();
    while (!m_state.is_trivial(action)) {
        m_actions_done.push_back(action);
        action = m_state.apply_random_action();
    }
    return m_actions_done;
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::return_to_root() {
    p_current_node = m_tree.get_root();
    m_state.reset(m_root_state);
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
void
Mcts<StateT, ActionT, MAX_DEPTH>::apply_root_action(const edge_type& edge) {
    m_root_state.apply_action(edge.action);
    m_actions_done.push_back(edge.action);
    p_current_node = m_tree.set_root(m_state.key());
}

template < typename StateT, typename ActionT, size_t MAX_DEPTH>
struct
Mcts<StateT, ActionT, MAX_DEPTH>::UCB {
    double m_C;
    unsigned int m_N;
    UCB(double C, unsigned int N) :
        m_C(C), m_N(N) { }
    auto operator()(const auto& a) {
        return a.avg_val + m_C * sqrt(log(m_N) / (a.n_visits + 1));
    }
};

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
auto constexpr
Mcts<StateT, ActionT, MAX_DEPTH>::make_edge_cmp(ActionSelectionMethod method) const {
    auto cmp = [m=method, C=exploration_constant, N=p_current_node->n_visits](const auto& a, const auto& b) {
        if (m == ActionSelectionMethod::by_ucb) {
            auto ucb = UCB(C, N);
            return ucb(a) < ucb(b);
        }
        if (m == ActionSelectionMethod::by_n_visits)
            return a.n_visits < b.n_visits;

        return a.best_val < b.best_val;

    };
    return cmp;
}

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
bool
Mcts<StateT, ActionT, MAX_DEPTH>::computation_resources() {
    return iteration_cnt < max_iterations;
}


} // namespace mcts_impl2

#endif // MCTS_HPP_
