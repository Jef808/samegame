// main.cpp
#include <iostream>
#include <fstream>
#include <thread>
#include <sstream>
#include "debug.h"
#include "samegame.h"
#include "mcts.h"


using namespace std;
using namespace literals::chrono_literals;
using namespace sg;
using namespace mcts;


int main(int argc, char *argv[])
{
    State::init();
    StateData sd_root = {};

    ifstream _if;
    _if.open("../data/input.txt", ios::in);
    State state { State(_if, &sd_root) };
    _if.close();

    Debug::mcts::set_debug_init_children(true);
    Debug::mcts::set_debug_counters(true);
    Debug::mcts::set_debug_final_scores(true);
    Debug::mcts::set_debug_tree(true);
    Debug::mcts::set_debug_main_methods(true);

    Agent agent(state);

    cout << (int)state.key();

    ClusterData cd = agent.MCTSBestAction();

    agent.print_debug_cnts(cout);
    // string buf;

    //while (!state.is_terminal())
    // {
    //     while (Cell(action) > MAX_CELLS - 1 || state.cells()[Cell(action)] == COLOR_NONE)
    //     {
    //         stringstream ss;
    //         int x, y;
    //         buf.clear();

    //         cout << "Choose a cell (row column) to play a move, or U to undo a move.\n" << endl;
    //         Debug::display(cout, state);

    //         getline(cin, buf);

    //         if (buf[0] == 'U') {
    //             state.undo_action(action);
    //         }
    //         else {
    //             ss << buf;
    //             ss >> x >> y ;
    //             action = Action(x + (14 - y) * WIDTH);
    //         }
    //     }

    //     Cluster* cluster = state.get_cluster(Cell(action));
    //     cout << "Chosen cluster: " << endl;
    //     Debug::display(cout, state, Cell(action), cluster);

    //     cout << "Effect of apply_action_blind: " << endl;
    //     agent.apply_action_blind(action);

    //     //this_thread::sleep_for(500ms);

    //     //agent.apply_action(action);

    //     action = ACTION_NONE;
    // }

    Debug::display(cout, state);
    cout << "Game over!" << endl;

    return 0;
}
