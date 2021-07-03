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
{ std::cout << t << std::endl; }

auto now() {
    return std::chrono::steady_clock::now();
}

auto time(auto start, auto end) {
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count();
}


template < int N >
struct TimeCutoff_UCB_Func {
    auto operator()(double expl_cst, unsigned int n_parent_visits)
    {
        return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge)
        {
            double ret = (n_parent_visits < N ? expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0)) : 0.000000001);
            return ret + edge.avg_val;
        };
    }
};


std::pair<StateData, bool> input(const std::string& filename)
{
    StateData ret{};

    std::ifstream _if(filename);
    if (!_if) {
        std::cerr << "Could not open file!";
        return std::pair{ ret, false };
    }

    std::string buf;
    size_t n = std::string::npos;
    while (n == std::string::npos)
    {
        std::getline(_if, buf);
        n = buf.find("In");
    }

    std::string cut = buf.substr(n+6);

    std::istringstream iss{cut};

    for (int i=0; i<15; ++i) {
        std::transform(std::istream_iterator<int>(iss),
                   std::istream_iterator<int>(),
                   ret.cells.begin() + i * 15,
                   [](int i){
                       return sg::Color(i+1);
                   });
        iss.clear();
        iss.get();
        iss.get();
    }
    return std::pair{ ret, true };
}

auto generate_fns(int i)
{
    auto num = std::to_string(i);
    return "data/test" + num + ".json";
}

bool run_test(std::ofstream& ofs, int i) {
    std::string filename = generate_fns(i+1);

    auto [sd, file_opened] = input(filename);

    const auto sd_backup = sd;

    if (!file_opened) {
        return false;
    }

    State state(sd.key, sd.cells, sd.cnt_colors);
    State state_backup(state);

    using MctsAgent = Mcts<sg::State,
                           sg::ClusterData,
                           TimeCutoff_UCB_Func<30>,
                           128>;
    MctsAgent mcts(state, TimeCutoff_UCB_Func<30>{});

    mcts.set_exploration_constant(1.0);
    mcts.set_max_iterations(30000);
    mcts.set_backpropagation_strategy(MctsAgent::BackpropagationStrategy::best_value);

    auto tik = now();
    std::vector<ClusterData> action_seq = mcts.best_action_sequence(MctsAgent::ActionSelection::by_n_visits);
    auto tok = now();

    ofs << "Time taken for test " << i << ": " << time(tik, tok) << std::endl;

    // Log the sequence found in the output file.
    state_backup.log_action_sequence(ofs, action_seq);

    std::cout << "Done" << std::endl;
    return true;
}


int main()
{
    std::ofstream ofs("benchmark.log");
    if (!ofs) {
        std::cerr << "Could not open output file!" << std::endl;
        return EXIT_FAILURE;
    }

    ofs << "Running with TimeCutoff_UCB_Func<30>, 30000 iterations, exploration constant of 1.0\n\n" << std::endl;

    for (int i=0; i<50; ++i)
    {
        if (!run_test(ofs, i))
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
