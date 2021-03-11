// debug.cpp

#include <string>
#include "samegame.h"
#include "debug.h"


namespace Debug {

using namespace sg;

//****************************** PRINTING BOARDS ************************/

int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

std::ostream& _DEFAULT(std::ostream& _out)
{

    return _out << "\033[0m";
}
std::ostream& _RED(std::ostream& _out)
{

    return _out << "\033[32m";
}

void display(std::ostream& _out, const State& state, Cell ndx, bool labels)
{
    // For every row
    for (int y = 0; y < HEIGHT; ++y) {
        if (labels) {
            _out << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }

        // Print row except last entry
        for (int x = 0; x < WIDTH - 1; ++x) {
            // Check for colored entry
            if (x + y * WIDTH == ndx) {
                _RED(_out) << (int)state.m_cells[x + y * WIDTH];
                _DEFAULT(_out) << ' ';
            } else {
                _out << (int)state.m_cells[x + y * WIDTH] << ' ';
            }
        }

        // Print last entry,
        // checking for colored entry
        if (WIDTH - 1 + y * WIDTH == ndx) {
            _RED(_out) << (int)state.m_cells[WIDTH - 1 + y * WIDTH];
            _DEFAULT(_out) << ' ';
        } else {
            _out << (int)state.m_cells[WIDTH - 1 + y * WIDTH] << '\n';
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
