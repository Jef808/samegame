// debug.cpp
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include "debug.h"
#include "samegame.h"

using namespace std;
using namespace sg;

namespace Debug {
namespace mcts {

bool debug_random_sim    = false;
bool debug_counters      = false;
bool debug_main_methods  = false;
bool debug_tree          = false;
bool debug_best_visits   = false;
bool debug_init_children = false;
bool debug_final_scores  = false;
bool debug_ucb           = false;

std::vector<::mcts::Reward> rewards;

} // namespace mcts

namespace sg {
bool debug_key           = false;
} // namespace sg

//****************************  COLORS **********************************/

namespace {

enum class COLOR : int {
    BLACK     = 30,
    RED       = 31,
    GREEN     = 32,
    YELLOW    = 33,
    BLUE      = 34,
    MAGENTA   = 35,
    CYAN      = 36,
    WHITE     = 37,
    B_BLACK   = 90,
    B_RED     = 91,
    B_GREEN   = 92,
    B_YELLOW  = 93,
    B_BLUE    = 94,
    B_MAGENTA = 95,
    B_CYAN    = 96,
    B_WHITE   = 97,
};

enum class SHAPE : int {
    SQUARE,
    DIAMOND,
    B_DIAMOND
};

COLOR sg_colored(Color c)
{
    return COLOR(c + 30);
}

template<SHAPE shape>
string print_cell(const Grid& cells, Cell cell)
{
    // Square, or circle, or circle with boundary.
    string sh = (shape == SHAPE::SQUARE ? "\u25A0" :
                 shape == SHAPE::DIAMOND ? "\u25C6" : "\u25C7");
    stringstream ss;
    ss<<"\033[1;"<<static_cast<int>(sg_colored(cells[cell]))<<"m"<<sh<<"\033[0m";
    return ss.str();
}

} // namespace

//****************************** PRINTING BOARDS ************************/



int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

void display(std::ostream& _out, const State& state, Cell ndx, const Cluster* cluster)
{
    bool labels = true;
    const auto& cells = state.cells();

    std::vector<Cell> cl;
    if (cluster != nullptr) {
        for (auto c : cluster->members) {
            cl.push_back(c);
        }
    }

    auto in_cluster = [&cl](Cell c){
        return find(cl.begin(), cl.end(), c) != cl.end();
    };

    // For every row
    for (int y = 0; y < HEIGHT; ++y) {
        if (labels) {
            _out << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }

        // Print row except last entry
        for (int x = 0; x < WIDTH - 1; ++x) {

            // Check for colored entry
            if (x + y * WIDTH == ndx) {
                _out << print_cell<SHAPE::B_DIAMOND>(cells, x + y * WIDTH) << ' ';
            } else if (!cl.empty() && in_cluster(x + y * WIDTH)) {
                _out << print_cell<SHAPE::DIAMOND>(cells, x + y * WIDTH) << ' ';
            } else {
                _out << print_cell<SHAPE::SQUARE>(cells, x + y * WIDTH) << ' ';
            }
        }
        // Print last entry,
        if (WIDTH - 1 + y * WIDTH == ndx) {
                _out << print_cell<SHAPE::B_DIAMOND>(cells, WIDTH - 1 + y * WIDTH) << '\n';
        } else if (!cl.empty() && in_cluster(WIDTH - 1 + y * WIDTH)) {
              _out << print_cell<SHAPE::DIAMOND>(cells, WIDTH - 1 + y * WIDTH) << '\n';
        } else {
                _out << print_cell<SHAPE::SQUARE>(cells, WIDTH - 1 + y * WIDTH) << '\n';
        }
    }

    if (labels) {
        _out << std::string(34, '_') << '\n'
             << std::string(5, ' ');

        for (int x : x_labels) {

            _out << x << ((x < 10) ? " " : "");
        }

        _out << std::endl;
    }
}


} // namespace Debug
