#ifndef __AGENT_RANDOM_H_
#define __AGENT_RANDOM_H_

#include <array>
#include <chrono>
#include <iostream>
#include <vector>

namespace mcts {

std::chrono::time_point<std::chrono::steady_clock> start;

std::chrono::milliseconds::rep time_elapsed()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - start)
      .count();
}

template<typename StateT,
         typename ActionT>
class Agent_random {
public:
Agent_random(StateT& state, unsigned int _time_limit) :
    m_state(state),
    best_variation{},
    best_variation_length(0),
    best_reward(0),
    action_stack{},
    time_limit(_time_limit),
    simul_cnt(0)
{ }
    using reward_type = typename StateT::reward_type;

    std::vector<ActionT> get_best_variation()
    {
      std::vector<ActionT> ret{};
      std::copy(best_variation.begin(), best_variation.begin() + best_variation_length,
                std::back_inserter(ret));
      return ret;
    }

    void run()
    {
      start = std::chrono::steady_clock::now();
      while (time_elapsed() < time_limit)
      {
        random_simul();
      }
    }

    unsigned int get_simul_cnt()
        { return simul_cnt; }

private:
    StateT& m_state;
    std::array<ActionT, 128> best_variation;
    int best_variation_length;
    reward_type best_reward;
    std::array<ActionT, 128> action_stack;
    unsigned int time_limit;
    unsigned int simul_cnt;

    void random_simul() {
      reward_type score = 0;
      // Make a copy since the `apply_action()` methods mutate the state.
      StateT tmp_state = m_state;

      int cnt = 0;
      action_stack[cnt] = tmp_state.apply_random_action();

      while (!tmp_state.is_trivial(action_stack[cnt]))
      {
        score += tmp_state.evaluate(action_stack[cnt]);
        action_stack[++cnt] = tmp_state.apply_random_action();
      }
      score += tmp_state.evaluate_terminal();

      if (score > best_reward)
      {
        best_reward = score;
        best_variation = action_stack;
        best_variation_length = cnt;
      }
      ++simul_cnt;
    }
};

} // namespace mcts

#endif // AGENT_RANDOM_H_
