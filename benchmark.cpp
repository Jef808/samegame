// benchmark.cpp
#include "samegame.h"
#include "mcts.h"
#include "policies.h"

#include <algorithm>
#include <iomanip>
#include <thread>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>

#include "display.h"

using namespace sg;
using namespace mcts;

template<typename T>
void OUT(const T& t)
{
  std::cout << t << std::endl;
}

auto now() { return std::chrono::steady_clock::now(); }

auto time(auto start, auto end)
{
  std::chrono::duration<double> elapsed = end - start;
  return elapsed.count();
}

template<int N>
struct TimeCutoff_UCB_Func
{
  auto operator()(double expl_cst, unsigned int n_parent_visits)
  {
    return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge) {
      double ret =
          (n_parent_visits < N
               ? expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0))
               : 0.000000001);
      return ret + edge.avg_val;
    };
  }
};

std::pair<State, bool> input(const std::string& filename)
{
  std::ifstream _if(filename);
  if (!_if)
  {
    std::cerr << "Could not open file!";
    return std::pair{State(), false};
  }

  std::string buf;
  size_t n = std::string::npos;
  while (n == std::string::npos)
  {
    std::getline(_if, buf);
    n = buf.find("In");
  }

  std::string cut = buf.substr(n + 6);
  std::istringstream iss{cut};

  Grid grid{};
  ColorCounter ccolors{};
  for (int i = 0; i < 15; ++i)
  {
    std::transform(std::istream_iterator<int>(iss),
                   std::istream_iterator<int>(),
                   grid.begin() + i * 15,
                   [&ccolors](int i) { ++ccolors[i+1]; return sg::Color(i + 1); });
    iss.clear();
    iss.get();
    iss.get();
  }
  auto ret = State(std::move(grid), std::move(ccolors));
  return std::pair{ret, true};
}

auto generate_fns(int i)
{
  auto num = std::to_string(i);
  return "../data/test" + num + ".json";
}

bool run_test(std::ofstream& ofs,
              int i,
              int max_time,
              double expl_cst)
{
  std::string filename = generate_fns(i + 1);

  auto [state, file_opened] = input(filename);

  const auto state_backup = state;

  if (!file_opened)
  {
    return false;
  }

  using MctsAgent =
      Mcts<sg::State,
           sg::ClusterData,
           TimeCutoff_UCB_Func<30>,
           policies::Default_Playout_Func<sg::State, sg::ClusterData>,
           128>;
  MctsAgent mcts(state, TimeCutoff_UCB_Func<30>{});

  mcts.set_exploration_constant(expl_cst);
  mcts.set_max_iterations(0);
  mcts.set_max_time(max_time);
  mcts.set_backpropagation_strategy(
      MctsAgent::BackpropagationStrategy::best_value);

  //auto tik = now();
  std::vector<ClusterData> action_seq =
      mcts.best_action_sequence(MctsAgent::ActionSelection::by_n_visits);
  //auto tok = now();

  ofs << "With max time of " << max_time / 1000.0 << "secs:\n";

  ofs << "Number of iterations achieved for test " << i
      << ": " << mcts.get_iterations_cnt()
      << std::endl;

  // std::cout << "Time taken for test " << i
  //           << ": " << time(tik, tok) << " seconds."
  //           << std::endl;
  // Log the sequence found in the output file.
  state_backup.log_action_sequence(ofs, action_seq);

  return true;
}

const int max_time = 60000;
const int n_iterations = 10000;
const double expl_cst = 1.0;

int main()
{
  std::ofstream ofs("benchmark.log");
  if (!ofs)
  {
    std::cerr << "Could not open output file!" << std::endl;
    return EXIT_FAILURE;
  }

  ofs << "Running with TimeCutoff_UCB_Func<30>, "
      << max_time / 1000.0 << " seconds per test, "
      << "exploration constant " << expl_cst
      << std::endl;

  for (int i = 10; i < 21; ++i)
  {
    if (!run_test(ofs, i, max_time, 1.0))
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
