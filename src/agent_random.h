#ifndef __AGENT_RANDOM_H_
#define __AGENT_RANDOM_H_

#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <tuple>
#include <utility>
#include <vector>
#include <iomanip>

namespace mcts {

template<typename StateT,
         typename ActionT>
class Agent_random {
public:
Agent_random(StateT& state) :
    m_state(state),
    best_variation{},
    best_variation_length(0),
    best_reward(0),
    action_stack{},
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

    std::chrono::milliseconds::rep time_elapsed()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
            .count();
    }

    void get_stats()
         { explore_statespace(); }

    unsigned int get_simul_cnt() const
        { return simul_cnt; }

    float get_avg_depth() const
        { return avg_depth; }

    std::vector<std::pair<float, unsigned int>> get_avg_n_children() const
        { return avg_n_children; }

    void set_n_iterations(unsigned int n)
        { n_iterations = n; }

    void set_time_limit(unsigned int n)
        { time_limit = n; }

    std::pair<unsigned int, unsigned int> view_expansion_rate()
    {
        StateT tmp_state = m_state;

        start = std::chrono::steady_clock::now();

        unsigned int cnt = 0;
        unsigned int leaf_cnt = 0;

        while (computation_resources())
        {
            ActionT action = tmp_state.apply_random_action();
            ++cnt;

            while (!tmp_state.is_trivial(action))
            {
                action = tmp_state.apply_random_action();
                ++cnt;
            }
            ++leaf_cnt;
            tmp_state = m_state;
        }
        return std::pair<unsigned int, unsigned int>{ cnt, leaf_cnt };
    }

    double complete_search_near_leaf(int threshold)
    {
        StateT tmp_state = m_state;

        auto actions = tmp_state.valid_actions_data();
        auto depth = 0;
        auto score = 0.0;

        while (actions.size() > threshold)
        {
            std::cout << "Depth " << depth << " num actions: " << actions.size() << std::endl;
            auto action = actions[rand() % actions.size()];
            score += (action.size - 2) * (action.size - 2);
            tmp_state.apply_action(action);
            ++depth;
            actions = tmp_state.valid_actions_data();
        }
        std::cout << "Depth " << depth << " num actions: " << actions.size()
                  << " Score: " << score
                  << ". Attempting complete search..." << std::endl;

         start = std::chrono::steady_clock::now();

        auto best_score = 0;

        for (auto a : actions)
        {
            auto _score = (a.size - 2.0) * (a.size - 2.0);
            auto cnt = 0;
            auto tmp_tmp_state = tmp_state;
            tmp_tmp_state.apply_action(a);
            auto res = do_dfs(tmp_tmp_state);
            cnt = res.first;
            _score += res.second;
            if (_score > best_score)
                best_score = _score;
            std::cout << "Score: " << _score << " for " << cnt << " States in branch...\n Total time taken to explore: " << std::setprecision(2) << time_elapsed() << std::endl;
        }

        return score + best_score;
    }

    std::tuple<int, int, int> generating_valid_actions_rate()
    {
        StateT tmp_state = m_state;

        start = std::chrono::steady_clock::now();

        int cnt = 0;
        int children_cnt = 0;
        int leaf_cnt = 0;

        while (computation_resources())
        {
            auto actions = tmp_state.valid_actions_data();
            auto depth = 0;

            while (!actions.empty())
            {
                ++depth;
                std::cout << actions.size() << " children at depth " << depth << std::endl;
                children_cnt += actions.size();
                auto action = actions[rand() % actions.size()];
                tmp_state.apply_action(action);
                ++cnt;
                actions = tmp_state.valid_actions_data();
            }
            ++leaf_cnt;
        }
        return std::make_tuple(cnt, children_cnt, leaf_cnt);
    }

    static auto do_dfs(StateT& _state)
    {
        int cnt = 0;
        auto actions = _state.valid_actions_data();

        if (actions.empty())
            return std::pair{ 0, 1000.0 * _state.is_empty() };

        cnt += actions.size();
        double best_score = 0.0;

        for (auto a : actions)
        {
            auto _score = (a.size - 2.0) * (a.size - 2.0);
            StateT tmp_state = _state;
            tmp_state.apply_action(a);
            auto res = do_dfs(tmp_state);
            _score += res.second;
            cnt += res.first;
            if (_score > best_score)
                best_score = _score;
        }

        return std::pair{ cnt, best_score };
    }

private:
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;

    StateT& m_state;
    std::array<ActionT, 128> best_variation;
    int best_variation_length;
    reward_type best_reward;
    std::array<ActionT, 128> action_stack;

    unsigned int time_limit;
    unsigned int n_iterations;

    time_point start;
    unsigned int simul_cnt;
    float avg_depth;
    std::vector<std::pair<float, unsigned int>> avg_n_children;  // { avg, n_visits }


    bool computation_resources()
    {
        bool time_ok = time_limit == 0 || time_elapsed() < time_limit;
        bool iters_ok = n_iterations == 0 || simul_cnt < n_iterations;
        return time_ok && iters_ok;
    }

    void explore_statespace()
    {
        reward_type score = 0;
        // Make a copy since the `apply_action()` methods mutate the state.
        StateT tmp_state = m_state;
        avg_depth = 0;

        std::vector<ActionT> children;
        avg_n_children.push_back(std::pair{ float(children.size()), 1 });
        simul_cnt = 0;
        ActionT action = tmp_state.apply_random_action();

        while (computation_resources())
        {
            unsigned int depth = 0;
            tmp_state = m_state;
            children = tmp_state.valid_actions_data();

            while (!tmp_state.is_trivial(action))
            {
                ++depth;
                if (depth > avg_n_children.size())
                {
                    avg_n_children.push_back(std::pair{ children.size(), 1 });
                }
                else
                {
                    ++avg_n_children[depth].second;
                    avg_n_children[depth].first += (children.size() - avg_n_children[depth].first) / avg_n_children[depth].second;
                }
                children = tmp_state.valid_actions_data();
            }
            ++simul_cnt;
            avg_depth += (depth - avg_depth) / simul_cnt;
        }
    }

    void random_simul() {
      reward_type score = 0;
      // Make a copy since the `apply_action()` methods mutate the state.
      StateT tmp_state = m_state;

      simul_cnt = 0;
      avg_depth = 0;
      unsigned int depth = 0;
      action_stack[depth] = tmp_state.apply_random_action();

      while (!tmp_state.is_trivial(action_stack[depth]))
      {
        score += tmp_state.evaluate(action_stack[depth]);
        action_stack[++depth] = tmp_state.apply_random_action();
      }
      score += tmp_state.evaluate_terminal();

      if (score > best_reward)
      {
        best_reward = score;
        best_variation = action_stack;
        best_variation_length = depth;
      }
      ++simul_cnt;
    }
};

} // namespace mcts

#endif // AGENT_RANDOM_H_
