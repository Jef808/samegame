#ifndef __MCTS_HPP_
#define __MCTS_HPP_

#include "mcts.h"
#include "mcts_tree.h"
#include "policies.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace mcts {
namespace timer {

std::chrono::time_point<std::chrono::steady_clock> start;

std::chrono::milliseconds::rep time_elapsed()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
      .count();
}

} // namespace timer

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
ActionT Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::best_action(
    ActionSelection method)
{
  run();
  auto* edge = get_best_edge(method);
  apply_root_action(*edge);
  return edge->action;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::ActionSequence
inline Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::best_action_sequence(
    ActionSelection method)
{
  run();
  return best_traversal(method);
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::run()
{
  init_counters();
  p_current_node = m_tree.get_root();
  if (p_current_node->n_visits > 0 && p_current_node->children.size() == 0)
  {
    return;
  }
  while (computation_resources())
  {
    step();
  }
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::step()
{
  return_to_root();
  select_leaf();
  expand_current_node();
  backpropagate();
  ++iteration_cnt;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::select_leaf()
{
  while (p_current_node->n_visits > 0 && p_current_node->children.size() > 0)
  {
    ++p_current_node->n_visits;
    edge_pointer edge = get_best_edge(ActionSelection::by_ucb);
    traverse_edge(edge);
  }
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::edge_pointer
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::get_best_edge(
    ActionSelection method)
{
  auto cmp = [&](const auto& a, const auto& b) {
    if (method == ActionSelection::by_ucb)
    {
      auto ucb = UCB_Func(std::move(exploration_constant),
                          std::move(p_current_node->n_visits));
      return ucb(a) < ucb(b);
    }
    if (method == ActionSelection::by_n_visits)
      return a.n_visits < b.n_visits;
    if (method == ActionSelection::by_avg_value)
      return a.avg_val < b.avg_val;
    return a.best_val < b.best_val;
  };

  return &*std::max_element(
      p_current_node->children.begin(), p_current_node->children.end(), cmp);
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
typename StateT::reward_type
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::simulate_playout(
    const ActionT& action)
{
  ActionT _action = action;
  reward_type score = 0.0;

  // Make a copy since the `apply_action()` methods mutate the state.
  StateT tmp_state = m_state;

  Playout_Functor Playout_Func(tmp_state);

  while (!tmp_state.is_trivial(_action))
  {
      score += tmp_state.evaluate(_action);
      _action = Playout_Func();
  }
  score += tmp_state.evaluate_terminal();

  return score;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::expand_current_node()
{
  if (p_current_node->n_visits > 0)
  {
    ++p_current_node->n_visits;
    return;
  }

  auto valid_actions = m_state.valid_actions_data();

  for (auto a : valid_actions)
    {
      edge_type new_edge{
         .action = a,
       };
      auto best_val = simulate_playout(a);
      new_edge.avg_val = new_edge.best_val = best_val;
      p_current_node->children.push_back(new_edge);
    }

  ++p_current_node->n_visits;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::backpropagate()
{
  using BackpropagationStrategy::avg_best_value;
  using BackpropagationStrategy::avg_value;
  using BackpropagationStrategy::best_value;

  auto cmp_best_value = [](const auto& a, const auto& b) {
    return a.best_val < b.best_val;
  };

  reward_type value_to_propagate = [&]() {
    if (p_current_node->children.empty())
    {
      return evaluate_terminal();
    }
    // BackpropagationStrategy::best_value
    if (backpropagation_strategy == best_value)
    {
      return std::max_element(p_current_node->children.begin(),
                              p_current_node->children.end(),
                              cmp_best_value)
          ->best_val;
    }
    // BackpropagationStrategy::avg_value and
    // BackpropagationStrategy::avg_best_value
    auto get_value = [&](const auto& edge) {
      return backpropagation_strategy == avg_value ? edge.avg_val
                                                   : edge.best_val;
    };
    double total = std::transform_reduce(p_current_node->children.begin(),
                                         p_current_node->children.end(),
                                         0,
                                         std::plus<double>(),
                                         get_value);

    return total / p_current_node->children.size();
  }();

  m_tree.backpropagate(value_to_propagate);
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::traverse_edge(
    edge_pointer edge)
{
  m_state.apply_action(edge->action);
  p_current_node = m_tree.get_node(m_state.key());
  m_tree.traversal_push(edge);
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::ActionSequence
Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::best_traversal(
    ActionSelection method)
{
  if (m_state != m_root_state)
  {
    return_to_root();
  }

  edge_pointer p_nex_edge;
  while (p_current_node->n_visits > 0 && p_current_node->children.size() > 0)
  {
    p_nex_edge = get_best_edge(method);
    traverse_edge(p_nex_edge);
    m_actions_done.push_back(p_nex_edge->action);
  }

  if (p_current_node->n_visits > 0)
  {
    return m_actions_done;
  }

  ActionT action = m_state.apply_random_action();
  while (!m_state.is_trivial(action))
  {
    m_actions_done.push_back(action);
    action = m_state.apply_random_action();
  }

  return m_actions_done;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
inline void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::return_to_root()
{
  p_current_node = m_tree.get_root();
  m_state = m_root_state;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::apply_root_action(
    const edge_type& edge)
{
  m_root_state.apply_action(edge.action);
  m_actions_done.push_back(edge.action);
  m_tree.set_root(m_root_state.key());
  return_to_root();
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
bool Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::computation_resources()
{
  bool time_ok = max_time > 0 ? timer::time_elapsed() < max_time : true;
  bool iterations_ok =
      max_iterations > 0 ? iteration_cnt < max_iterations : true;
  return time_ok && iterations_ok;
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
void Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::init_counters()
{
  iteration_cnt = 0;
  timer::start = std::chrono::steady_clock::now();
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::reward_type
inline Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::evaluate(const ActionT& action)
{
  return m_state.evaluate(action);
}

template<typename StateT,
         typename ActionT,
         typename UCB_Functor,
         typename Playout_Functor,
         size_t MAX_DEPTH>
typename Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::reward_type
inline Mcts<StateT, ActionT, UCB_Functor, Playout_Functor, MAX_DEPTH>::evaluate_terminal()
{
  return m_state.evaluate_terminal();
}

} // namespace mcts

#endif // MCTS_HPP_
