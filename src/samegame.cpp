// samegame.cpp
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "samegame.h"
#include "dsu.h"
#include "types.h"
//#include "debug.h"

namespace sg {

// ************************  Constants for sentinel values ********************* //

const Cluster CLUSTER_NONE = {};
const std::vector<Cell> CLUSTER_VEC_NONE = { CELL_NONE };
const ClusterData CLUSTERD_NONE = { CELL_NONE, Color::Empty, 0 };

// **************************  DSU  ********************************************* //

namespace details {

    DSU dsu = DSU { };

} // namespace details


// ***************************  Randomization  ********************************** //

namespace Random {

    namespace {

        static std::random_device rd;
        static std::mt19937 gen;

    void init()
    {
        gen = std::mt19937(rd());
    }

    struct get {
        template < typename T >
        T operator()(std::uniform_int_distribution<T>& _dist)
        {
            return _dist(gen);
        }
    };

    } // namespace

    /**
     * Generate a random vector of integers between _beg and _end.
     */
    auto ordering(const int _beg, const int _end)
    {
        const size_t n = _end - _beg;
        std::vector<int> ret(n);
        std::iota(begin(ret), end(ret), _beg);
        //std::transform(_ret.begin(), _ret.end(), ret.begin(), [](auto i){ return to_enum<Cell>(i); });

        for (auto i = 0; i < n; ++i) {
            std::uniform_int_distribution<int> dist(i, n - 1);
            int j = get()(dist);
            std::swap(ret[i], ret[j]);
        }

        return ret;
    }

    void shuffle(std::vector<Cell>& v)
    {
        size_t n = v.size();

        for (auto i = 0; i < n; ++i) {
            std::uniform_int_distribution<Cell> dist(i, n - 1);
            int j = get()(dist);
            std::swap(v[i], v[j]);
        }
    }

} // namespace Random

//*************************************  Zobrist  ***********************************/

namespace Zobrist {

    using ClusterMembers = std::vector<Cell>;

    std::array<Key, MAX_CELLS * MAX_COLORS> ndx_keys { 0 };

    Key cellColorKey(const Cell cell, const Color color) {
        return ndx_keys[cell * to_integral(color)];
    }

    /**
     * For every cell in the cluster, XOR with the key corresponding to that
     * position in the grid.
     */
    // Key clusterKey(const ClusterMembers& members, const Color color)
    // {
    //     Key key = 0;

    //     std::for_each(members.cbegin(), members.cend(),
    //                   [&key, color](auto cell) mutable
    //                   { key ^= cellColorKey(cell, color); });

    //     return key;
    // }

    // Key rarestColorKey(const State& state) {
    //     auto rarest_color = *std::min_element(state.colors.begin() + 1, state.colors.end());
    //     return rarest_color << 1;
    // }
    // Key terminalKey(const State& state) {
    //     return state.is_terminal();
    // }

} // namespace Zobrist

//*************************************** Game logic **********************************/


void State::init()
{
    using Zobrist::ndx_keys;

    // Generate the Random engines
    Random::init();

    // Generate the random keys for the Zobrist hash
    std::uniform_int_distribution<unsigned long long> dis(
        std::numeric_limits<std::uint64_t>::min(),
        std::numeric_limits<std::uint64_t>::max()
    );

    for (auto it = ndx_keys.begin(); it != ndx_keys.end(); ++it) {

        auto prekey = Random::get()(dis);
        // Reserve two bits. The first one indicates if is_terminal() has been checked,
        // the second stores the result if so.
        prekey = prekey >> 2;
        *it = (prekey << 2);
    }
}

State::State(std::istream& _in, StateData& sd)
    : p_data(&sd)
    , colors { 0 }
{
    int _in_color = 0;
    Grid& cells = p_data->cells;

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            _in >> _in_color;
            const Color color = to_enum<Color>(_in_color + 1);
            cells[j + i * WIDTH] = color;
            ++colors[color];
        }
    }

    p_data->key = compute_key();
}

void State::populate_color_counter()
{
    const auto& grid = cells();
    for (auto it = grid.cbegin(); it != grid.cend(); ++it) {
        ++colors[*it];
    }
}

Color State::get_color(const Cell cell) const
{
    return p_data->cells[cell];
}

void State::set_color(const Cell cell, const Color color)
{
    p_data->cells[cell] = color;
    // Grid& m_cells = p_data->cells;
    // m_cells[cell] = color;
}

int State::n_empty_rows() const
{
    return p_data->n_empty_rows;
}

// TODO: Only check columns intersecting the killed cluster
void State::pull_cells_down()
{
    Grid& m_cells = p_data->cells;
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        // stack the non-zero colors going up
        std::array<Color, HEIGHT> new_column { Color::Empty };
        int new_height = 0;

        for (int j = 0; j < HEIGHT; ++j) {
            auto bottom_color = m_cells[i + (HEIGHT - 1 - j) * WIDTH];

            if (bottom_color != Color::Empty) {
                new_column[new_height] = bottom_color;
                ++new_height;
            }
        }
        // pop back the value (including padding with 0)
        for (int j = 0; j < HEIGHT; ++j) {
            m_cells[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
        }
    }
}

// TODO: Only check columns (before finding one) intersecting the killed cluster
void State::pull_cells_left()
{
    Grid& m_cells = p_data->cells;
    int i = 0;
    std::deque<int> zero_col;

    while (i < WIDTH) {
        if (zero_col.empty()) {
            // Look for empty column
            while (i < WIDTH - 1 && m_cells[i + (HEIGHT - 1) * WIDTH] != Color::Empty) {
                ++i;
            }
            zero_col.push_back(i);
            ++i;

        } else {
            int x = zero_col.front();
            zero_col.pop_front();
            // Look for non-empty column
            while (i < WIDTH && m_cells[i + (HEIGHT - 1) * WIDTH] == Color::Empty) {
                zero_col.push_back(i);
                ++i;
            }
            if (i == WIDTH)
                break;
            // Swap the non-empty column with the first empty one
            for (int j = 0; j < HEIGHT; ++j) {
                std::swap(m_cells[x + j * WIDTH], m_cells[i + j * WIDTH]);
            }
            zero_col.push_back(i);
            ++i;
        }
    }
}

// NOTE: Can return empty vector
ClusterDataVec State::valid_actions_data() const
{
    generate_clusters();
    ClusterDataVec ret {};
    ret.reserve(MAX_CELLS);

    for (auto it = details::dsu.begin(); it != details::dsu.end(); ++it) {
        if (get_color(it->rep) == Color::Empty || details::dsu.find_rep(it->rep) != it->rep) {
            continue;
        }
        ret.emplace_back(it->rep, get_color(it->rep), it->size());
        //ClusterData cd { it->rep, get_color(it->rep), it->size() };
        //ret.push_back(cd);
    }
    return ret;
}

//******************************* MAKING MOVES ***********************************/

void State::generate_clusters() const
{
    DSU& dsu = details::dsu;
    dsu.reset();

    Grid& m_cells = p_data->cells;
    p_data->n_empty_rows = 0;
    bool row_empty = false;
    int j = HEIGHT - 1;

    // Iterate from bottom row to the second row
    while (j > 0) {
        row_empty = true;
        // All row except last cell
        for (int i = j * WIDTH; i < j * WIDTH + WIDTH - 1; ++i) {
            // compare up
            if (m_cells[i] != Color::Empty && m_cells[i] == m_cells[i - WIDTH]) {
                details::dsu.unite(i, i - WIDTH);
                row_empty = false;
            }
            // compare right
            if (m_cells[i] != Color::Empty && m_cells[i] == m_cells[i + 1]) {
                details::dsu.unite(i, i + 1);
                row_empty = false;
            }
        }
        // Last cell, compare up
        if (m_cells[j * WIDTH + WIDTH - 1] != Color::Empty && m_cells[j * WIDTH + WIDTH - 1] == m_cells[j * WIDTH - 1]) {
            details::dsu.unite(j * WIDTH + WIDTH - 1, j * WIDTH - 1);
            row_empty = false;
        }
        // If we saw an empty row, there is nothing above either
        if (row_empty) {
            p_data->n_empty_rows = j+1;
            break;
        }
        --j;
    }
    // First row
    if (!row_empty) {
        row_empty = true;
        for (int i = 0; i < WIDTH - 1; ++i) {
            if (m_cells[i] != Color::Empty && m_cells[i] == m_cells[i + 1]) {
                details::dsu.unite(i, i + 1);
                row_empty = false;
            }
        }

        // Can cast the bool to an int to record whether or not the first row is empty.
        p_data->n_empty_rows = int(row_empty);
    }
}

/**
 * Method to kill a cluster in case the whole cluster is
 * known.
 */
void State::kill_cluster(const Cluster& cluster)
{
    const Color color = get_color(cluster.rep);

    // Kill the cells
    for (const auto cell : cluster.members) {
        set_color(cell, Color::Empty);
    }

    // Update the color counter
    colors[color] -= cluster.size();
}

// Tries to kill the cluster represented by the ClusterData object.
// If the cell is empty or isolated, don't do anything.
// Return true if the cluster was killed, false otherwise.
ClusterData State::kill_cluster_blind(const Cell cell, const Color color)
{
    ClusterData cd { cell, color, 0 };

    if (cell == CELL_NONE || color == Color::Empty) {
        return cd;
    }

    Grid& grid = p_data->cells;

    std::deque<Cell> queue { cd.rep };
    Cell cur = CELL_NONE;

    grid[cd.rep] = Color::Empty;
    ++cd.size;

    while (!queue.empty()) {
        cur = queue.back();
        queue.pop_back();
        // We remove the cells adjacent to `cur` with the target color
        // Look right
        if (cur % WIDTH < WIDTH - 1 && grid[cur + 1] == color) {
            queue.push_back(cur + 1);
            grid[cur + 1] = Color::Empty;
            ++cd.size;
        }
        // Look down
        if (cur < (HEIGHT - 1) * WIDTH && grid[cur + WIDTH] == color) {
            queue.push_back(cur + WIDTH);
            grid[cur + WIDTH] = Color::Empty;
            ++cd.size;
        }
        // Look left
        if (cur % WIDTH > 0 && grid[cur - 1] == color) {
            queue.push_back(cur - 1);
            grid[cur - 1] = Color::Empty;
            ++cd.size;
        }
        // Look up
        if (cur > WIDTH - 1 && grid[cur - WIDTH] == color) {
            queue.push_back(cur - WIDTH);
            grid[cur - WIDTH] = Color::Empty;
            ++cd.size;
        }
    }
    // If only the rep was killed (cluster of size 1), restore it
    if (cd.size == 1) {
        grid[cd.rep] = color;
    }

    return cd;
}

bool State::kill_cluster_blind(const ClusterData& cd)
{
    ClusterData res = kill_cluster_blind(cd.rep, cd.color);

    bool is_nontrivial = cd.rep != CELL_NONE && cd.color != Color::Empty && cd.size > 1;

    return is_nontrivial;
}

/**
 * Apply an action to a state in a persistent way: the new data is
 * recorded on the provided StateData object.
 */
bool State::apply_action(const ClusterData& cd, StateData& sd)
{
    sd.previous = p_data; // Provide info of current state.
    sd.cells = cells(); // Copy the current grid on the provided StateData object
    p_data = &sd;

    bool action_nontrivial = kill_cluster_blind(cd);

    // Clean up grid if successfull
    if (action_nontrivial)
    {
        pull_cells_down();
        pull_cells_left();
    }
    // Restore the state's data if not
    else
    {
        p_data = p_data->previous;
    }

    return action_nontrivial;
}

/**
 * Apply an action to a state in a mutable way: the state's data
 * is overwritten.
 */
ClusterData State::apply_action_blind(const Cell _cell)
{
    auto cd = kill_cluster_blind(_cell, get_color(_cell));

    if (cd.size > 1) {
        pull_cells_down();
        pull_cells_left();
    }

    return cd;
}

/**
 * Apply an action to a state in a mutable way: the state's data
 * is overwritten.
 */
bool State::apply_action_blind(const ClusterData& _cd)
{
    bool res = kill_cluster_blind(_cd);

    if (res) {
        pull_cells_down();
        pull_cells_left();
    }

    return res;
}

ClusterData State::apply_random_action()
{
    ClusterData cd = kill_random_valid_cluster();
    if (cd.color == Color::Empty) {
        return CLUSTERD_NONE;
    }
    if (cd.size > 1) {
        pull_cells_down();
        pull_cells_left();
    }
    return cd;
}

// ClusterData State::apply_random_action_generating()
// {
//     auto cd = kill_random_valid_cluster_generating();
//     if (cd.color == Color::Empty) {
//         return CLUSTERD_NONE;
//     }
//     if (cd.size > 1) {
//         pull_cells_down();
//         pull_cells_left();
//     }

//     return cd;
// }

// Need a ClusterData object if undoing multiple times in a row!!!!
// void State::undo_action(Action action)
// {
//     // Simply revert the data
//     p_data = p_data->previous;

//     // Use the action to reincrement the color counter
//     auto cluster = get_cluster_blind(action);
//     const Color color = get_color(cluster.rep);
//     colors[color] += cluster.size();
// }

void State::undo_action(const ClusterData& cd)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    colors[cd.color] += cd.size;
}

/**
 * Builds the cluster to which the given cell belongs
 * to directly from the grid.
 */
Cluster State::get_cluster_blind(const Cell cell) const
{
    Color color = get_color(cell);

    if (color == Color::Empty) {
        return CLUSTER_NONE;
        spdlog::warn("WARNING! Called get_cluster_blind with an empty color");
    }

    // Copy the grid
    Grid grid = p_data->cells;

    std::vector<Cell> ret { cell };
    grid[cell] = Color::Empty;

    std::deque<Cell> queue { cell };
    int cnt = 1;

    Cell cur;

    while (!queue.empty()) {
        cur = queue.back();
        queue.pop_back();
        // remove boundary cells with color equal to current cell's color
        // Look right
        if (cur % WIDTH < WIDTH - 1 && grid[cur + 1] == color) {
            grid[cur + 1] = Color::Empty;
            ret.push_back(cur + 1);
            queue.push_back(cur + 1);
            ++cnt;
        }
        // Look down
        if (cur < (HEIGHT - 1) * WIDTH && grid[cur + WIDTH] == color) {
            grid[cur + WIDTH] = Color::Empty;
            ret.push_back(cur + WIDTH);
            queue.push_back(cur + WIDTH);
            ++cnt;
        }
        // Look left
        if (cur % WIDTH > 0 && grid[cur - 1] == color) {
            grid[cur - 1] = Color::Empty;
            ret.push_back(cur - 1);
            queue.push_back(cur - 1);
            ++cnt;
        }
        // Look up
        if (cur > WIDTH - 1 && grid[cur - WIDTH] == color) {
            grid[cur - WIDTH] = Color::Empty;
            ret.push_back(cur - WIDTH);
            queue.push_back(cur - WIDTH);
            ++cnt;
        }
    }

    // NOTE: now that we copy the grid, no need for this (not ideal but oh well)
    // // Replace the colors
    // for (auto cell : ret) {
    //     grid[cell] = color;
    // }

    auto cl_ret = Cluster(cell, std::move(ret));
    return cl_ret;

}

//********************************  Bitwise methods  ***********************************/

/**
 * Iterate through the cells like in the generate_clusters() method,
 * but returns false as soon as it identifies a cluster.
 */
bool State::has_nontrivial_cluster() const
{
    const Grid& m_cells = cells();
    p_data->n_empty_rows = 0;
    int j = HEIGHT - 1;

    // Iterate from bottom row to the second row
    while (j > 0) {
        // All row except last cell
        for (int i = j * WIDTH; i < j * WIDTH + WIDTH - 1; ++i) {
            // compare up
            if (m_cells[i] != Color::Empty && m_cells[i] == m_cells[i - WIDTH]) {
                return false;
            }
            // compare right
            if (m_cells[i] != Color::Empty && m_cells[i] == m_cells[i + 1]) {
                return false;
            }
        }
        // Last cell, compare up
        if (m_cells[j * WIDTH + WIDTH - 1] != Color::Empty && m_cells[j * WIDTH + WIDTH - 1] == m_cells[j * WIDTH - 1]) {
            return false;
        }
        --j;
    }
    // First row

    for (int i = 0; i < WIDTH - 1; ++i) {
        if (m_cells[i] != Color::Empty && m_cells[i] == m_cells[i + 1]) {
            return false;
        }
    }

    return true;
}

/**
 * Check if a cell has some neighbor with the same color
 * on its right or upwards.
 *
 * NOTE: If called with an empty cell, it will return true if a
 * neighbor is also empty.
 */
bool same_color_adjacent(const Grid& grid, auto cell_it)
{
    const Color color = *cell_it;
    const auto ndx = std::distance(grid.cbegin(), cell_it);

    // check right
    bool right_nbh = ndx % WIDTH != 0 && grid[ndx + 1] == color;
    // check down
    bool down_nbh = ndx < CELL_BOTTOM_LEFT && grid[ndx + WIDTH] == color;

    return right_nbh || down_nbh;
}

/**
 * Xor with a unique random key for each (index, color) appearing in the grid.
 * Indicate that clusters have not yet been generated (so is_terminal() is not
 * checked) by switched the least significant bit on.
 */
Key State::compute_key() const
{
    Key& key = p_data->key = 0;

    const Grid& grid = cells();
    bool terminal_status_known = false;

    // XOR with each key corresponding to a (cell, color) pair in the grid
    for (auto cell_it = grid.cbegin(); cell_it != grid.cend(); ++cell_it) {
        if (*cell_it != Color::Empty) {
            key ^= Zobrist::cellColorKey(std::distance(grid.cbegin(), cell_it), *cell_it);
            // If the terminal status of the status is known, continue
            if (terminal_status_known) {
                continue;
            }
            // Otherwise keep checking for nontrivial cluster
            if (same_color_adjacent(grid, cell_it)) {
                key += 1;
                terminal_status_known = true;
            }
        }
    }
    // If by now terminal status is NOT known, the state has to be terminal
    if (!terminal_status_known) {
        key += 3;
    }

    return key;
}

bool key_uninitialized(const Grid& grid, Key key) {
        return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
};

Key State::key() const
{
    if (key_uninitialized(p_data->cells, p_data->key)) {
        p_data->key = compute_key();
    }

    return p_data->key;
}

/**
 * Check if there are any clusters of size at least 2. The non-const comes from the
 * fact that the result will be encoded in the state's key to avoid checking twice.
 */
bool State::is_terminal() const
{
    auto key_uninitialized = [](const Grid& grid, Key key) {
        return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
    };

    const Key& key = p_data->key;

    // If the key hasn't been computed, compute manually
    if (key_uninitialized(cells(), key)) {
        return !has_nontrivial_cluster();
    }
    // Otherwise, the result of is_terminal is encoded in the second bit.
    return key & 2;
}

// DEPRECATED
// ClusterData State::kill_random_valid_cluster_new()
// {
//     ClusterData ret {};

//     // Prepare a random order for the rows.
//     std::vector<int> rows(15 - p_data->n_empty_rows);
//     std::iota(begin(rows), end(rows), p_data->n_empty_rows);
//     Random::shuffle(rows);

//     std::vector<Cell> non_empties {};

//     for (auto row : rows) {
//         if (row < p_data->n_empty_rows) {
//             continue;
//         }
//         // Get the non-empty cells in that row
//         non_empties.clear();
//         for (auto i = row * WIDTH; i < row * WIDTH + WIDTH; ++i) {
//             if (p_data->cells[i] != Color::Empty)
//                 non_empties.push_back(i);
//         }
//         if (non_empties.empty()) {
//             p_data->n_empty_rows = row + 1;
//         }
//         // Randomize the order of those non-empty cells
//         Random::shuffle(non_empties);

//         for (auto cell : non_empties) {
//             ret = kill_cluster_blind(cell, p_data->cells[cell]);
//             if (ret.size > 1) {
//                 return ret;
//             }
//         }
//     }

//     return ret;
// }

ClusterData State::kill_random_valid_cluster()
{
    using CellnColor = std::pair<Cell, Color>;
    ClusterData ret {};

    // Reference to our grid.
    const auto& grid = p_data->cells;

    // Random ordering for non-empty rows
    auto rows = Random::ordering(p_data->n_empty_rows, 15);

    // Vector to store the non-empty cells and their color per row
    std::vector<CellnColor> non_empties {};
    std::vector<int> ordering {};
    non_empties.reserve(15);
    ordering.reserve(15);

    for (const auto row : rows) {
        // In case more empty rows were discovered in previous iteration of this for loop
        // if (row < p_data->n_empty_rows)
        //     continue;

        // Collect the non-empty cells in this row along with their colors
        auto row_begin_ndx = row * WIDTH;
        auto row_begin_it = grid.cbegin() + row_begin_ndx;
        auto row_end_it = row_begin_it + WIDTH;
        for (auto it = row_begin_it;
             it != row_end_it;
             ++it) {
            if (const Color color = *it; color != Color::Empty) {
                const Cell ndx = row_begin_ndx + std::distance(row_begin_it, it);
                non_empties.push_back(std::make_pair(ndx, color));
            }
        }

        //Adjust the number of empty rows if one was just discovered
        if (non_empties.empty() && row > p_data->n_empty_rows) {
            p_data->n_empty_rows = row;
            continue;
        }

        // Choose a random ordering to go along the above non-empties
        ordering = Random::ordering(0, non_empties.size());

        // Traverse the non-empties along that ordering and try to kill the cluster at
        // the corresponding cell.
        for (auto it = begin(ordering);
             it != end(ordering);
             ++it) {
            const auto [_cell, _color] = non_empties[*it];
            ret = kill_cluster_blind(_cell, _color);
            if (ret.size > 1) {
                return ret;
            }
        }

        // Reset the non-empty vector in preparation for the next row
        non_empties.clear();
    }

    return ret;
}

// TODO: Make this more efficient by not reinializing the random engine every time...
// ClusterData State::kill_random_valid_cluster()
// {
//     using CellnColor = std::pair<Cell, Color>;
//     ClusterData ret { };

//     // Prepare a random order for the rows.

//     std::vector<Cell> rows(15 - p_data->n_empty_rows);
//     std::iota(begin(rows), end(rows), p_data->n_empty_rows);
//     Random::shuffle(rows);

//     // Vector to store the non-empty cells with their color per row
//     std::vector<CellnColor> non_empties {};

//     // Reference to our grid.
//     for (const auto& row : rows)
//     {
//         non_empties.clear();
//         const auto row_begin = begin(p_data->cells) + row * WIDTH;
//         const auto row_end = row_begin + WIDTH;
//         // Get the non-empty cells in that row
//         for (auto it = begin(cells) + row_begin)
//         {

//         }
//         for (auto i = row * WIDTH; i < row * WIDTH + WIDTH; ++i) {
//             if (cells[i] != Color::Empty)
//                 non_empties.push_back(Cell(i));
//         }
//         // Shuffle them
//         Random::shuffle(non_empties);

//         // Try to kill a cluster using those non-empty cells
//         for (const auto& cell : non_empties) {
//             ret = kill_cluster_blind(cell, cells[cell]);
//             if (ret.size > 1) {
//                 return ret;
//             }
//         }
//     }

//     return ret;
// }

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

// Land somewhere randomly and start doing the 'generating_cluster' algorithm
// ClusterData State::apply_random_action()
// {
//     ClusterData ret { CELL_NONE, Color::Empty, 0 };

//     std::vector<uint8_t> cols;
//     uint8_t x = 210;
//     while (p_data->cells[x] != Color::Empty) {
//         cols.push_back(x);
//         ++x;
//     }

//     std::vector<uint8_t> adj;
//     for (auto x : cols)
//     {
//         while (p_data->cells[x] != Color::Empty)
//     }

//     while (ret.size < 2)
//     {
//         auto cell = random_cellv2();
//         ret = kill_cluster_blind(cell, p_data->cells[cell]);
//     }

//     return ret;
// }

// NOTE: An apply_action method without using cluster information
// void State::apply_action_blind(Action action, StateData& sd)
// {
//     //using pii = std::pair<uint8_t, uint8_t>;

//     sd.previous = p_data;                    // Provide info of current state.
//     sd.cells = cells();                      // Copy the current grid on the provided StateData object
//     p_data = &sd;

//     Color color = sd.cells[action];
//     int cnt = 1;
//     sd.cells[action] = Color::Empty;

//     // Manual 'kill_cluster method'
//     // TODO refactor into its own method, have the state manage which kill_cluster
//     // method to call depending on if the r_cluster member is up to date.
//     std::deque<Action> queue{ action };

//     while (!queue.empty())
//     {
//         Cell cur = queue.back();
//         queue.pop_back();
//         // remove boundary cells of same color as action
//         // Look right
//         if (cur % WIDTH < WIDTH - 1 && sd.cells[cur+1] == color) {
//             sd.cells[cur+1] = Color::Empty;
//             queue.push_back(cur+1);
//             ++cnt;
//         }
//         // Look down
//         if (cur < (HEIGHT - 1) * WIDTH && sd.cells[cur + WIDTH] == color) {
//             sd.cells[cur + WIDTH] = Color::Empty;
//             queue.push_back(cur+WIDTH);
//             ++cnt;
//         }
//         // Look left
//         if (cur % WIDTH > 0 && sd.cells[cur-1] == color) {
//             sd.cells[cur - 1] = Color::Empty;
//             queue.push_back(cur-1);
//             ++cnt;
//         }
//         // Look up
//         if (cur > WIDTH - 1 && sd.cells[cur - WIDTH] == color) {
//             sd.cells[cur - WIDTH] = Color::Empty;
//             queue.push_back(cur - WIDTH);
//             ++cnt;
//         }
//     }
//
//     colors[color] -= cnt;
//     pull_cells_down();
//     pull_cells_left();
//     generate_clusters();
//     p_data->key = generate_key();
// }


//******************************************* Small methods ************************************/

bool State::is_empty() const
{
    return p_data->cells[((HEIGHT - 1) * WIDTH)] == Color::Empty;
}


//*************************** DEBUG *************************/



using namespace std;

template < >
sg::State_Action<Cluster>::State_Action(const State& _state, const Cluster& _cluster)
    : r_state(_state)
    , m_cluster(_cluster)
  {}

template < >
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

    map<Shape, string> Shape_codes {
        { Shape::SQUARE, "\u25A0" },
        { Shape::DIAMOND, "\u25C6" },
        { Shape::B_DIAMOND, "\u25C7" },
    };

    string shape_unicode(Shape s)
    {
        return Shape_codes[s];
    }

    string color_unicode(Color c)
    {
        return std::to_string(to_integral(Color_codes(to_integral(c) + 90)));
    }

    string print_cell(const sg::Grid& grid, Cell ndx, Output output_mode, const sg::Cluster& cluster = CLUSTER_NONE)
    {
        bool ndx_in_cluster = find(cluster.cbegin(), cluster.cend(), ndx) != cluster.cend();
        stringstream ss;

        if (output_mode == Output::CONSOLE) {
            Shape shape = ndx == cluster.rep ? Shape::B_DIAMOND : ndx_in_cluster ? Shape::DIAMOND
                                                                         : Shape::SQUARE;

            ss << "\033[1;" << color_unicode(grid[ndx]) << "m" << shape_unicode(shape) << "\033[0m";

            return ss.str();
        } else {
            // Underline the cluster representative and output the cluster in bold
            string formatter = "";
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

string to_string(const State_Action<Cluster>& sa, sg::Output output_mode)
{
    bool labels = true;

    const auto& grid = sa.r_state.cells();

    stringstream ss { "\n" };

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

template < typename _Cluster >
ostream& operator<<(ostream& _out, const State_Action<_Cluster>& sa);

template < >
ostream& operator<<(ostream& _out, const State_Action<Cluster>& state_action)
{
    return _out << to_string(state_action, Output::CONSOLE);
}

ostream& operator<<(ostream& _out, const ClusterData& _cd)
{
    return _out << _cd.rep << ' ' << to_string(_cd.color) << ' ' << std::to_string(_cd.size);
}

ostream& operator<<(ostream& _out, const State& state)
{
    auto sa = State_Action<Cluster>(state, CELL_NONE);
    return _out << to_string(sa, Output::CONSOLE);
    //return _out << state.to_string(CELL_NONE, Output::CONSOLE);
}

//****************************** PRINTING BOARDS ************************/


const Cluster* State::dsu_cbegin() const
{
    return details::dsu.cbegin();
}
const Cluster* State::dsu_cend() const
{
    return details::dsu.cend();
}

void State::enumerate_clusters(ostream& _out) const
{
    generate_clusters();

    for (auto it = details::dsu.begin();
         it != details::dsu.end();
         ++it) {
        spdlog::info("Rep={}, size={}", it->rep, it->size());
    }
}

void State::view_clusters(ostream& _out) const
{
    generate_clusters();

    for (auto it = details::dsu.cbegin();
         it != details::dsu.cend();
         ++it) {
        if (it->size() > 1 && it->rep == details::dsu.find_rep(it->rep) && get_color(it->rep) != Color::Empty) {//size() > 1) {
            auto sa = State_Action<Cluster>(*this, *it);
            spdlog::info("\n{}\n", sa);
            this_thread::sleep_for(500.0ms);
        }
    }
}


bool operator==(const Cluster& a, const Cluster& b)
{
    return a.rep == b.rep && a.members == b.members;
}
bool operator==(const ClusterData& a, const ClusterData& b)
{
    return a.color == b.color && a.size == b.size && a.rep == b.rep;
}

} //namespace sg
