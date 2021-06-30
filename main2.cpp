#include "mcts_impl2/mcts.h"
#include "samegame.h"
#include "display.h"

#include <algorithm>
#include <iomanip>
#include <thread>
#include <fstream>
#include <iostream>

using namespace sg;
using namespace mcts_impl2;
using namespace std;
using namespace std::literals::chrono_literals;

template < typename... Args>
void PRINT(Args... args) {
    ((cout << args << ", "), ...);
    cout << endl;
}

template < typename T >
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec) {
    for (auto t : vec) {
        out << t << ", ";
    }
    return out;
}

int main()
{
    sg::StateData sd;
        std::ifstream _if;
        _if.open("../data/input.txt", std::ios::in);
        if (!_if)
        {
            cerr << "Could not open input file" << endl;
            return EXIT_FAILURE;
        }

    auto state = State(_if, sd);
    _if.close();

    sg::StateData sd_backup;
    state.copy_data_to(sd_backup);

    Mcts<State, ClusterData, 128> mcts(state);

    mcts.set_exploration_constant(1);
    mcts.set_max_iterations(15000);

    auto action_seq = mcts.best_action_sequence(Mcts<State, ClusterData, 128>::ActionSelectionMethod::by_n_visits);


    PRINT("Got the sequence!");

    state.copy_data_from(sd_backup);


    double score = 0;

    for (auto a : action_seq) {
        state.display(a.rep);
        state.apply_action(a);
        score += std::max(0UL, (a.size - 2) * (a.size - 2));
        PRINT("SCORE : ", score);
        //std::this_thread::sleep_for(200.0ms);
    }

    score += 1000 * (state.is_empty());

    PRINT("FINAL SCORE: ", score);

}
