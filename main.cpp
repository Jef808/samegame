#include "samegame.h"
#include "mcts.h"
#include "policies.h"

#include <algorithm>
#include <iomanip>
#include <thread>
#include <fstream>
#include <iostream>

#include "display.h"

using namespace sg;
using namespace mcts;
using namespace sg::display;
using namespace std::literals::chrono_literals;


const int time_cst = 20;

auto now() { return std::chrono::steady_clock::now(); }

auto time(auto start, auto end)
{
  std::chrono::duration<double> elapsed = end - start;
  return elapsed.count();
}

/**
 * Remove the contribution from the log term after a given number of visits.
 *
 * NOTE This really improves the run-time and average score!
 */
template<int N>
struct TimeCutoff_UCB_Func
{
  auto operator()(double expl_cst, unsigned int n_parent_visits)
  {
    return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge) {
      double ret =
          (n_parent_visits < N
               ? expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0))
               : 0.00001);
      return ret + edge.avg_val;
    };
  }
};

/**
 * Weight the colors by the following rule:
 *
 * weight[i] = 1 / (proportion of cells of color i amongst the remaining cells)
 */
struct ColorWeighted_UCB_Func
{

  ColorWeighted_UCB_Func(const sg::State& _state) : r_state(_state) {}

  auto operator()(double expl_cst, unsigned int n_parent_visits)
  {
    update_weights();
    return [&, expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge) {
      auto color = std::underlying_type_t<sg::Color>(edge.action.color);
      return m_weights[color] * edge.avg_val
             + expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0));
    };
  }

  void update_weights()
  {
    double total = std::accumulate(
        r_state.color_counter().begin(), r_state.color_counter().end(), 0.0);
    for (int i = 0; i < 5; ++i)
    {
      m_weights[i] = total / (5.0 * r_state.color_counter()[i]);
    }
  }
  policies::Default_UCB_Func UCB_Func{};
  std::array<double, 5> m_weights;
  /** Don't store the counter itself since the state's address isn't fixed.  */
  const State& r_state;
};

/**
 * Applies a random action but tries to go for the color which has the least
 * number of cells in the grid first.
 */
struct SmallColor_Playout_Func
{
  SmallColor_Playout_Func(State& _state)
    : state(_state)
  {
  }

  ClusterData operator()()
  {
    const ColorCounter& colors = state.color_counter();
    const Color target = Color(std::distance(colors.begin(), std::min_element(colors.begin(), colors.end())));
    return state.apply_random_action(target);
  }

  State& state;
};


/** Vizualize an action sequence in the console. */
void vizualize(const State& state,
               const auto& actions,
               int delay_in_ms = 150)
{
  state.view_action_sequence(actions, delay_in_ms);
}

auto run_test(const State& state, int n_iterations, int max_time_in_ms, double expl_cst)
{
  // Make a copy of the state to run the algorithm.
  State _state(state);

  // Initialize the Mcts agent with our UCB function.
  using MctsAgent = Mcts<sg::State,
                         sg::ClusterData,
                         //policies::Default_UCB_Func,
                         //ColorWeighted_UCB_Func,
                         TimeCutoff_UCB_Func<time_cst>,
                         SmallColor_Playout_Func,
                         128>;
  //MctsAgent mcts(_state, ColorWeighted_UCB_Func(state));
  //MctsAgent mcts(_state, policies::Default_UCB_Func{});
  MctsAgent mcts(_state, TimeCutoff_UCB_Func<time_cst>{});

  // Configure it.
  mcts.set_exploration_constant(expl_cst);
  mcts.set_max_iterations(n_iterations);
  mcts.set_max_time(max_time_in_ms);
  mcts.set_backpropagation_strategy(
      MctsAgent::BackpropagationStrategy::best_value);

  // Get the resulting action sequence.
  std::vector<ClusterData> action_seq =
      mcts.best_action_sequence(MctsAgent::ActionSelection::by_n_visits);
  return std::make_tuple(action_seq,
                         mcts.get_iterations_cnt(),
                         mcts.get_n_nodes() );
}

int n_iterations = 0;
int max_time_in_ms = 20000;
double expl_cst = 1.0;
const int delay_in_ms = 200;
const int n_runs = 1;

int main()
{
  PRINT("Reading input file...");

  std::ifstream _if;
  _if.open("../data/input.txt", std::ios::in);
  if (!_if)
  {
    PRINT("Could not open input file");
    return EXIT_FAILURE;
  }
  // Read the initial data into the state.
  State state(_if);
  _if.close();

  char t;

  for (int i = 0; i < n_runs; ++i)
  {
    // Run mcts (with timer)
    PRINT("Computing best action sequence (time set at ",
          max_time_in_ms / 1000.0, " seconds...)");
    auto tik = now();
    auto [action_seq,
          res_n_iterations,
          res_n_nodes] = run_test(state, n_iterations, max_time_in_ms, expl_cst);
    auto tok = now();
    auto time_taken = time(tik, tok);

    // Vizualize it.
    state.view_action_sequence(action_seq, delay_in_ms);

    PRINT(std::setprecision(2),
          "Max time set: ",                   max_time_in_ms / 1000.0, " seconds",
          "\nMax number of iterations set: ", n_iterations,
          "\nNumber of iterations: ",         res_n_iterations,
          "\nNumber of nodes created ",       res_n_nodes,
          "\nTime taken: ",                   time_taken, " seconds",
          "\nExploration constant: ",         expl_cst,
          "\nTime cutoff constant: ",         time_cst);

    if (i < n_runs-1)
    {
      std::cin.ignore();
      PRINT("Input any character to continue...");
      std::cin >> t;
    }

  }

    return EXIT_SUCCESS;
}
