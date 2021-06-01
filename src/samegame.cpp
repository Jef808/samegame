// samegame.cpp
//#include "grid.h"
#include "samegame.h"
#include "clusterhelper.h"
#include "sghash.h"
#include "types.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace sg {

State::State(StateData& sd)
    : p_data(&sd)
{
}

/**
 * Populates the grid with the input and computes the key and
 * color counter in one pass.
 */
State::State(std::istream& _in, StateData& sd)
    : p_data(&sd)
 {
    int _in_color = 0;
    Grid& cells = p_data->cells;
    cells.n_empty_rows = 0;
    ColorsCounter cnt_colors = p_data->cnt_colors;
    Key key = 0;
    bool row_empty;

    for (int row = 0; row < HEIGHT; ++row) {
        row_empty = true;

        for (int col = 0; col < WIDTH; ++col) {
            _in >> _in_color;
            const Color color = to_enum<Color>(_in_color + 1);
            cells[col + row * WIDTH] = color;

            // Generate the helper data at the same time
            if (color != Color::Empty) {
                row_empty = false;
                key ^= ZobristKey(col + row * WIDTH, color);
                ++colors[color];
            }
        }

        // Count the number of empty rows (we're going from top to down)
        if (row_empty) {
            ++p_data->n_empty_rows;
        }
    }

    p_data->key = key;
}

Color State::get_color(const Cell cell) const
{
    return p_data->cells[cell];
}

void State::set_color(const Cell cell, const Color color)
{
    p_data->cells[cell] = color;
}

void State::move_data(StateData* _sd)
{
    *_sd = *p_data;
    p_data = _sd;
}

int State::n_empty_rows() const
{
    return p_data->n_empty_rows;
}


//****************************************** Actions methods ***************************************/

// NOTE: Can return empty vector
ClusterDataVec State::valid_actions_data() const
{
    clusters::generate_clusters(p_data->cells);
    ClusterDataVec ret {};
    ret.reserve(MAX_CELLS);

    for (auto it = grid_dsu.begin(); it != grid_dsu.end(); ++it) {
        if (it->size() > 1) {// && get_color(it->rep) != Color::Empty) {
            ret.push_back(ClusterData { it->rep, get_color(it->rep), it->size() });
        }
    }

    return ret;
}

bool key_uninitialized(const Grid& grid, Key key)
{
    return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
}

Key State::key() const
{
    if (key_uninitialized(p_data->cells, p_data->key)) {
        p_data->key = zobrist::get_key(p_data->cells);
    }

    return p_data->key;
}

/**
 * First check if the key contains the answer, check for clusters but return
 * false as soon as it finds one instead of computing all clusters.
 */
bool State::is_terminal() const
{
    Key key = p_data->key;

    // If the first bit is on, then it has been computed and stored in the second bit.
    if (key & 1) {
        return key & 2;
    }
    return !clusters::has_nontrivial_cluster(p_data->cells);
}

bool State::is_empty() const
{
    return p_data->cells[((HEIGHT - 1) * WIDTH)] == Color::Empty;
}

//******************************** Apply / Undo actions **************************************/

/**
 * Apply an action to a state in a persistent way: the new data is
 * recorded on the provided StateData object.
 */
ClusterData State::apply_action(const ClusterData& cd, StateData& sd) const
{
    sd.cells = p_data->cells;
    sd.previous = p_data; // Provide info of current state.

    ClusterData cd_ret = clusters::apply_action(sd.cells, cd.rep);
    sd.cnt_colors[cd_ret.color] -= cd_ret.size;

    return cd_ret;
}

ClusterData State::apply_random_action()
{
    ClusterData cd = clusters::apply_random_action(p_data->cells);
    p_data->cnt_colors[cd.color] -= cd.size;
    return cd;
}

void State::undo_action(const ClusterData& cd)
{
    p_data = p_data->previous;
    p_data->cnt_colors[cd.color] += cd.size;
}



//*************************** Display *************************/

template <>
sg::State_Action<Cluster>::State_Action(const State& _state, const Cluster& _cluster)
    : r_state(_state)
    , m_cluster(_cluster)
{
}

template <>
sg::State_Action<Cluster>::State_Action(const State& _state, const Cell _cell)
    : r_state(_state)
    , m_cluster(CLUSTER_NONE)
{
    if (_cell != CELL_NONE)
        m_cluster = _state.get_cluster_blind(_cell);
}

namespace display {

    enum class Color_codes : int {
        BLACK = 30,
        RED = 31,
        GREEN = 32,
        YELLOW = 33,
        BLUE = 34,
        MAGENTA = 35,
        CYAN = 36,
        WHITE = 37,
        B_BLACK = 90,
        B_RED = 91,
        B_GREEN = 92,
        B_YELLOW = 93,
        B_BLUE = 94,
        B_MAGENTA = 95,
        B_CYAN = 96,
        B_WHITE = 97,
    };

    enum class Shape : int {
        SQUARE,
        DIAMOND,
        B_DIAMOND,
        NONE
    };

    std::map<Shape, string> Shape_codes {
        { Shape::SQUARE, "\u25A0" },
        { Shape::DIAMOND, "\u25C6" },
        { Shape::B_DIAMOND, "\u25C7" },
    };

    std::string shape_unicode(Shape s)
    {
        return Shape_codes[s];
    }

    std::string color_unicode(Color c)
    {
        return std::to_string(to_integral(Color_codes(to_integral(c) + 90)));
    }

    std::string print_cell(const sg::Grid& grid, Cell ndx, Output output_mode, const sg::Cluster& cluster = sg::Cluster())
    {
        bool ndx_in_cluster = find(cluster.cbegin(), cluster.cend(), ndx) != cluster.cend();
        std::stringstream ss;

        if (output_mode == Output::CONSOLE) {
            Shape shape = ndx == cluster.rep ? Shape::B_DIAMOND : ndx_in_cluster ? Shape::DIAMOND
                                                                                 : Shape::SQUARE;

            ss << "\033[1;" << color_unicode(grid[ndx]) << "m" << shape_unicode(shape) << "\033[0m";

            return ss.str();
        } else {
            // Underline the cluster representative and output the cluster in bold
            std::string formatter = "";
            if (ndx_in_cluster)
                formatter += "\e[1m]";
            if (ndx == cluster.rep)
                formatter += "\033[4m]";

            ss << formatter << std::to_string(to_integral(grid[ndx])) << "\033[0m\e[0m";

            return ss.str();
        }
    }

} // namespace display

const int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

std::string to_string(const State_Action<Cluster>& sa, sg::Output output_mode)
{
    bool labels = true;

    const auto& grid = sa.r_state.cells();

    std::stringstream ss { "\n" };

    // For every row
    for (int y = 0; y < HEIGHT; ++y) {
        if (labels) {
            ss << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }
        // Print row except last entry
        for (int x = 0; x < WIDTH - 1; ++x) {
            ss << display::print_cell(grid, x + y * WIDTH, output_mode, sa.m_cluster) << ' ';
        }
        // Print last entry,
        ss << display::print_cell(grid, WIDTH - 1 + y * WIDTH, output_mode, sa.m_cluster) << '\n';
    }

    if (labels) {
        ss << std::string(34, '_') << '\n'
           << std::string(5, ' ');

        for (int x : x_labels) {

            ss << x << ((x < 10) ? " " : "");
        }
        ss << '\n';
    }

    return ss.str();
}

template <typename _Cluster>
std::ostream& operator<<(ostream& _out, const State_Action<_Cluster>& sa);

template <>
std::ostream& operator<<(ostream& _out, const State_Action<Cluster>& state_action)
{
    return _out << to_string(state_action, Output::CONSOLE);
}

std:;ostream& operator<<(ostream& _out, const ClusterData& _cd)
{
    return _out << _cd.rep << ' ' << to_string(_cd.color) << ' ' << std::to_string(_cd.size);
}

std::ostream& operator<<(ostream& _out, const State& state)
{
    auto sa = State_Action<Cluster>(state, CELL_NONE);
    return _out << to_string(sa, Output::CONSOLE);
    //return _out << state.to_string(CELL_NONE, Output::CONSOLE);
}

void State::enumerate_clusters(ostream& _out) const
{
    clusters::generate_clusters(p_data->cells);

    for (auto it = grid_dsu.begin();
         it != grid_dsu.end();
         ++it) {
        spdlog::info("Rep={}, size={}", it->rep, it->size());
    }
}

void State::view_clusters(ostream& _out) const
{
    clusters::generate_clusters(p_data->cells);

    for (auto it = grid_dsu.cbegin();
         it != grid_dsu.cend();
         ++it) {
        assert(it->rep < MAX_CELLS && it->rep > -1);

        if (it->size() > 1 && it->rep == grid_dsu.find_rep(it->rep) && get_color(it->rep) != Color::Empty) { //size() > 1) {
            auto sa = State_Action<Cluster>(*this, *it);
            spdlog::info("\n{}\n", sa);
            this_thread::sleep_for(500.0ms);
        }
    }
}

bool operator==(const StateData& a, const StateData& b)
{
    if (a.ply != b.ply) {
        return false;
    }
    if (a.key != 0 && b.key != 0) {
        return a.key == b.key;
    }
    for (int i = 0; i < MAX_CELLS; ++i) {
        if (a.cells[i] != b.cells[i]) {
            return false;
        }
    }
    return true;
}

bool operator==(const ClusterData& a, const ClusterData& b)
{
    return a.rep == b.rep && a.color == b.color && a.size == b.size;
}

} //namespace sg


// ***************************** SCRAP ****************************************
//
//
// ClusterData State::find_random_cluster_medium_grid()
// {
//     ClusterData ret { };
//     // Prepare a random order to use for traversing the grid row by row
//     static std::array<uint8_t, 15> row_ndx;
//     Random::shuffle_rows(row_ndx);
//     // Then traverse rows until our conditions are met.
//     static std::array<ClusterData, 16> cluster_choices;
//     cluster_choices.fill(ClusterData { });
//     int _nb = 0;

//     static uint8_t ymax;

//     Grid& cells = p_data->cells;

//     for (int _y = 0; _y < 15; ++_y)
//     {
//         int x = 0, y = row_ndx[_y];
//         while (x < 15)
//         {

//     std::find_if(begin(cells) + y * 15, begin(cells) + (y + 1) * 15,
//                 [&](auto c){ return c != Color::Empty; });

//             bool row_empty = true;

//             if (cells[x + y * 15] != Color::Empty)
//             {
//                 row_empty = false;
//                 ret = kill_cluster_blind(Cell(x + y * 15));
//                 if (ret.size > 2)
//                 {
//                     cluster_choices[_nb] = ret;
//                 }
//             }

//             ++x;

//             if (!cluster_choices.empty()) {
//                 break;
//             }
//             if (row_empty && y < ymax)    {
//                 ymax = y;
//             }
//         }
//     }
// }
