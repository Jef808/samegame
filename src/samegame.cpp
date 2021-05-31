// samegame.cpp
#include "samegame.h"
#include "dsu.h"
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
//#include "debug.h"

namespace sg {

using Cluster = State::Cluster;
using DSU = DSU<Cluster, MAX_CELLS>;

// ************************  Constants for sentinel values ********************* //

const Cluster CLUSTER_NONE = Cluster(CELL_NONE);
const ClusterData CLUSTERD_NONE = { CELL_NONE, Color::Empty, 0 };

// **************************  DSU  ********************************************* //
namespace details {

    DSU dsu = sg::DSU();

} // namespace
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
            template <typename T>
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

    /**
     * NOTE: The upper left cell in the grid corresponds to 0,
     * so wee need to increment the cells when computing the key!
     */
    Key cellColorKey(const Cell cell, const Color color)
    {
        return ndx_keys[(cell + 1) * to_integral(color)];
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

//************************************ Utility functions ******************************/

// /**
//  * Check if a cell has some neighbor with the same color
//  * on its RIGHT or DOWNWARDS
//  *
//  * NOTE: If called with an empty cell, it will return true if a
//  * neighbor is also empty.
//  */
// bool same_color_bottom_right(const Grid& grid, const Cell cell)
// {
//     const Color color = grid[cell];
//     // check right
//     if (cell % WIDTH != 0 && grid[cell + 1] == color) {
//         return true;
//     }
//     // check down
//     if (cell < CELL_BOTTOM_LEFT && grid[cell + WIDTH] == color) {
//         return true;
//     }

//     return false;
// }

//*************************************** Game logic **********************************/

void State::init()
{
    using Zobrist::ndx_keys;

    // Generate the Random engines
    Random::init();

    // Generate the random keys for the Zobrist hash
    std::uniform_int_distribution<unsigned long long> dis(
        std::numeric_limits<std::uint64_t>::min(),
        std::numeric_limits<std::uint64_t>::max());

    for (auto it = ndx_keys.begin(); it != ndx_keys.end(); ++it) {

        auto prekey = Random::get()(dis);
        // Reserve two bits. The first one indicates if is_terminal() has been checked,
        // the second stores the result if so.
        prekey = prekey >> 2;
        *it = (prekey << 2);
    }
}

State::State(StateData& sd)
    : p_data(&sd)
    , colors { 0 }
{
}

/**
 * Populates the grid with the input and computes the key and
 * color counter in one pass.
 */
State::State(std::istream& _in, StateData& sd)
    : p_data(&sd)
    , colors { 0 }
{
    int _in_color = 0;
    Grid& cells = p_data->cells;
    Key key = 0;
    bool row_empty;
    p_data->n_empty_rows = 0;

    for (int row = 0; row < HEIGHT; ++row) {
        row_empty = true;

        for (int col = 0; col < WIDTH; ++col) {
            _in >> _in_color;
            const Color color = to_enum<Color>(_in_color + 1);
            cells[col + row * WIDTH] = color;

            // Generate the helper data at the same time
            if (color != Color::Empty) {
                row_empty = false;
                key ^= Zobrist::cellColorKey(col + row * WIDTH, color);
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

// NOTE: Can def be done at construction and subsequently adjusted when applying actions
// void State::populate_color_counter()
// {
//     const auto& grid = cells();
//     for (auto it = grid.cbegin(); it != grid.cend(); ++it) {
//         ++colors[*it];
//     }
// }

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

//************************************** Grid manipulations **********************************/

// TODO: Only check columns intersecting the killed cluster
void pull_cells_down(Grid& grid)
{
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        // stack the non-zero colors going up
        std::array<Color, HEIGHT> new_column { Color::Empty };
        int new_height = 0;

        for (int j = 0; j < HEIGHT; ++j) {
            auto bottom_color = grid[i + (HEIGHT - 1 - j) * WIDTH];

            if (bottom_color != Color::Empty) {
                new_column[new_height] = bottom_color;
                ++new_height;
            }
        }
        // pop back the value (including padding with 0)
        for (int j = 0; j < HEIGHT; ++j) {
            grid[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
        }
    }
}

// TODO: Only check columns (before finding one) intersecting the killed cluster
void pull_cells_left(Grid& grid)
{
    int i = 0;
    std::deque<int> zero_col;

    while (i < WIDTH) {
        if (zero_col.empty()) {
            // Look for empty column
            while (i < WIDTH - 1 && grid[i + (HEIGHT - 1) * WIDTH] != Color::Empty) {
                ++i;
            }
            zero_col.push_back(i);
            ++i;

        } else {
            int x = zero_col.front();
            zero_col.pop_front();
            // Look for non-empty column
            while (i < WIDTH && grid[i + (HEIGHT - 1) * WIDTH] == Color::Empty) {
                zero_col.push_back(i);
                ++i;
            }
            if (i == WIDTH)
                break;
            // Swap the non-empty column with the first empty one
            for (int j = 0; j < HEIGHT; ++j) {
                std::swap(grid[x + j * WIDTH], grid[i + j * WIDTH]);
            }
            zero_col.push_back(i);
            ++i;
        }
    }
}

void generate_clusters(const State& state)
{
    using details::dsu;

    dsu.reset();

    const Grid& grid = state.cells();
    auto n_empty_rows = state.n_empty_rows();
    bool row_empty = true;

    auto row = HEIGHT - 1;
    // Iterate from bottom row upwards so we can stop at the first empty row.
    while (row > n_empty_rows) {
        // All the row except last cell
        for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell) {
            if (grid[cell] == Color::Empty) {
                continue;
            }
            row_empty = false;
            // compare up
            if (grid[cell] == grid[cell - WIDTH]) {
                dsu.unite(cell, cell - WIDTH);
            }
            // compare right
            if (grid[cell] == grid[cell + 1]) {
                dsu.unite(cell, cell + 1);
            }
        }
        // If the last cell of the row is empty
        if (grid[(row + 1) * WIDTH - 1] == Color::Empty) {
            if (row_empty) {
                return;
            }
            continue;
        }
        // If it is not empty, compare up
        if (grid[(row + 1) * WIDTH - 1] == grid[row * WIDTH - 1]) {
            dsu.unite((row + 1) * WIDTH - 1, row * WIDTH - 1);
        }
        --row;
        row_empty = true;
    }
    // The upmost non-empty row: only compare right
    for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell) {
        if (grid[cell] == Color::Empty) {
            continue;
        }
        if (grid[cell] == grid[cell + 1]) {
            dsu.unite(cell, cell + 1);
        }
    }
}

//****************************************** Valid actions ***************************************/

// NOTE: Can return empty vector
State::ClusterDataVec State::valid_actions_data() const
{
    using details::dsu;

    generate_clusters(*this);
    ClusterDataVec ret {};
    ret.reserve(MAX_CELLS);

    for (auto it = dsu.begin(); it != dsu.end(); ++it) {
        if (it->size() > 1 && get_color(it->rep) != Color::Empty) {
            ret.push_back(ClusterData { it->rep, get_color(it->rep), it->size() });
        }
    }

    return ret;
}

// void State::generate_clusters() const //(State& state);
// {
//     using details::dsu;

//     dsu.reset();

//     const Grid& m_cells = p_data->cells;
//     int n_empty_rows = p_data->n_empty_rows;
//     bool row_empty = false;

//     // Iterate from bottom row to the second row
//     for (auto row = HEIGHT - 1; row > n_empty_rows; --row) {
//         row_empty = true;
//         // All row except last cell
//         for (int cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell) {
//             if (m_cells[cell] == Color::Empty) {
//                 continue;
//             }
//             // compare up
//             if (m_cells[cell - WIDTH] == m_cells[cell]) {
//                 dsu.unite(cell - WIDTH, cell);
//                 row_empty = false;
//             }
//             // compare right
//             if (m_cells[cell + 1] == m_cells[cell]) {
//                 dsu.unite(cell + 1, cell);
//                 row_empty = false;
//             }
//         }
//         // Last cell, if empty and rest of row was empty, we're done
//         if (m_cells[(row + 1) * WIDTH - 1] == Color::Empty) {
//             if (row_empty) {
//                 p_data->n_empty_rows = row + 1;
//                 return;
//             }
//         }
//         if (m_cells[(row + 1) * WIDTH - 1] == m_cells[row * WIDTH - 1]) {
//             dsu.unite(row * WIDTH + WIDTH - 1, row * WIDTH - 1);
//             row_empty = false;
//         }
//     }
//     row_empty = true;

//     for (auto cell = 0; cell < CELL_UPPER_RIGHT; ++cell) {
//         if (m_cells[cell] == Color::Empty) {
//             continue;
//         }
//         if (m_cells[cell] == m_cells[cell + 1]) {
//             dsu.unite(cell, cell + 1);
//             row_empty = false;
//         }
//     }

//     // We can cast the bool to an int to record whether or not the first row is empty.
//     p_data->n_empty_rows = int(row_empty);
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

///////////////////////////////////////////////////////////////////////////////
// TODO: I can scrap most of that... with the Zobrish scheme being to just
//       XOR the key of each cell, it's going to be easier to implemnt the
//       computation of the keys during calls to kill_cluster and pull_cells()
///////////////////////////////////////////////////////////////////////////////

/**
 * Iterate through the cells like in the generate_clusters() method,
 * but returns false as soon as it identifies a cluster.
 */
bool has_nontrivial_cluster(const State& state)
{
    const Grid& grid = state.cells();
    auto n_empty_rows = state.n_empty_rows();
    bool row_empty = true;

    auto row = HEIGHT - 1;
    // Iterate from bottom row upwards so we can stop at the first empty row.
    while (row > n_empty_rows) {
        // All the row except last cell
        for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell) {
            if (grid[cell] == Color::Empty) {
                continue;
            }
            row_empty = false;
            // compare up
            if (grid[cell] == grid[cell - WIDTH]) {
                return true;
            }
            // compare right
            if (grid[cell] == grid[cell + 1]) {
                return true;
            }
        }
        // If the last cell of the row is empty
        if (grid[(row + 1) * WIDTH - 1] == Color::Empty) {
            if (row_empty) {
                return false;
            }
            continue;
        }
        // If not (compare up)
        if (grid[(row + 1) * WIDTH - 1] == grid[row * WIDTH]) {
            return true;
        }
        --row;
        row_empty = true;
    }
    // The upmost non-empty row: only compare right
    for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell) {
        if (grid[cell] != Color::Empty && grid[cell + 1] == grid[cell]) {
            return true;
        }
    }

    return true;
}

/**
 * Check if the neighbor at the right or on top of a cell has the same color.
 *
 * NOTE: If called with an empty cell, it will return true if a
 * neighbor is also empty.
 */
bool same_color_right_up(const Grid& grid, const Cell cell)
{
    const Color color = grid[cell];
    // check right if not already at the right edge of the grid
    if (cell % (WIDTH - 1) != 0 && grid[cell + 1] == color) {
        return true;
    }
    // check up if not on the first row
    if (cell > CELL_UPPER_RIGHT && grid[cell - WIDTH] == color) {
        return true;
    }
    return false;
}

/**
 * Xor with a unique random key for each (index, color) appearing in the grid.
 * Also compute is_terminal() (Key will have first bit on once is_terminal() is known,
 * and it will be terminal iff the second bit is on).
 * Also records the number of empty rows in passing.
 */
Key compute_key(const State& state)
{
    const auto& grid = state.cells();

    auto n_empty_rows = state.n_empty_rows();
    Key key = 0;
    bool row_empty = false, terminal_status_known = false;

    for (auto row = HEIGHT - 1; row > n_empty_rows-1; --row) {
        row_empty = true;

        for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH; ++cell) {
            if (const Color color = grid[cell]; color != Color::Empty) {
                row_empty = false;

                key ^= Zobrist::cellColorKey(cell, color);

                // If the terminal status of the state is known, continue
                if (terminal_status_known) {
                    continue;
                }
                // Otherwise keep checking for nontrivial clusters upwards and forward
                if (same_color_right_up(grid, cell)) {
                    // Indicate that terminal status is known
                    terminal_status_known = true;
                }
            }
        }

        if (row_empty) {
            n_empty_rows = row + 1;
            break;
        }
    }

    // flip the first bit if we found out state was non-terminal earlier,
    // otherwise flip both the first and second bit.
    key += terminal_status_known ? 1 : 3;

    return key;
}

// Key State::compute_key() const
// {
//     Key& key = p_data->key = 0;

//     const Grid& grid = cells();
//     bool terminal_status_known = false;

//     // First row
//     // XOR with each key corresponding to a (cell, color) pair in the grid
//     for (int y = 0; y<HEIGHT; ++y) {
//         for (int x = 0; x < WIDTH; ++x) {

//         const Color color = grid[i];

//         if (color != Color::Empty) {
//             key ^= Zobrist::cellColorKey(i, color);
//             // If the terminal status of the status is known, continue
//             if (terminal_status_known) {
//                 continue;
//             }
//             // Otherwise keep checking for nontrivial cluster
//             if (same_color_right_up(grid, i)) {
//                 key += 1;
//                 terminal_status_known = true;
//             }
//         }
//     }
//     // If by now terminal status is NOT known, the state has to be terminal
//     if (!terminal_status_known) {
//         key += 3;
//     }

//     return key;
// }

bool key_uninitialized(const Grid& grid, Key key)
{
    return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
}

Key State::key() const
{
    if (key_uninitialized(p_data->cells, p_data->key)) {
        p_data->key = compute_key(*this);
    }

    return p_data->key;
}

/**
 * First check if the key contains the answer, check for clusters but return
 * false as soon as it finds one.
 */
bool State::is_terminal() const
{
    Key key = p_data->key;

    // If the first bit is on, then it has been computed and stored in the second bit.
    if (key & 1) {
        return key & 2;
    }

    return !has_nontrivial_cluster(*this);
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

//****************************** Implementation of apply_action ******************************/

/**
 * Function to kill a cluster in case the whole cluster is
 * known.
 */
void kill_cluster(State& state, const Cluster& cluster)
{
    const Color color = state.get_color(cluster.rep);

    // Kill the cells
    for (const auto cell : cluster.members) {
        state.set_color(cell, Color::Empty);
    }

    // Update the color counter
    state.colors[color] -= cluster.size();
}

// Tries to kill the cluster represented by the ClusterData object.
// If the cell is empty or isolated, don't do anything.
// Return true if the cluster was killed, false otherwise.
ClusterData kill_cluster_blind(State& state, const Cell cell, const Color color)
{
    ClusterData cd { cell, color, 0 };

    if (cell == CELL_NONE || color == Color::Empty) {
        return cd;
    }

    Grid& grid = state.p_data->cells;

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

bool kill_cluster_blind(State& state, const ClusterData& cd)
{
    ClusterData res = kill_cluster_blind(state, cd.rep, cd.color);

    assert(cd.color == res.color && cd.size == res.size);

    bool is_nontrivial = cd.rep != CELL_NONE && cd.color != Color::Empty && cd.size > 1;

    return is_nontrivial;
}

ClusterData kill_random_valid_cluster(State& state)
{
    using CellnColor = std::pair<Cell, Color>;
    ClusterData ret {};

    // Reference to our grid.
    const auto& grid = state.p_data->cells;

    // Random ordering for non-empty rows
    auto rows = Random::ordering(state.p_data->n_empty_rows, 15);

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
        if (non_empties.empty() && row > state.p_data->n_empty_rows) {
            state.p_data->n_empty_rows = row;
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
            ret = kill_cluster_blind(state, _cell, _color);
            if (ret.size > 1) {
                return ret;
            }
        }

        // Reset the non-empty vector in preparation for the next row
        non_empties.clear();
    }

    return ret;
}

//*************************************** Apply action **************************************/

/**
 * Apply an action to a state in a persistent way: the new data is
 * recorded on the provided StateData object.
 */
bool State::apply_action(const ClusterData& cd, StateData& sd)
{
    sd.previous = p_data; // Provide info of current state.
    sd.cells = cells(); // Copy the current grid on the provided StateData object
    p_data = &sd;

    bool action_nontrivial = kill_cluster_blind(*this, cd);

    // Clean up grid if successfull
    if (action_nontrivial) {
        pull_cells_down(p_data->cells);
        pull_cells_left(p_data->cells);
    }
    // Restore the state's data if not
    else {
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
    auto cd = kill_cluster_blind(*this, _cell, get_color(_cell));

    if (cd.size > 1) {
        pull_cells_down(p_data->cells);
        pull_cells_left(p_data->cells);
    }

    return cd;
}

/**
 * Apply an action to a state in a mutable way: the state's data
 * is overwritten.
 */
bool State::apply_action_blind(const ClusterData& _cd)
{
    bool res = kill_cluster_blind(*this, _cd);

    if (res) {
        pull_cells_down(p_data->cells);
        pull_cells_left(p_data->cells);
    }

    return res;
}

ClusterData State::apply_random_action()
{
    ClusterData cd = kill_random_valid_cluster(*this);
    if (cd.color == Color::Empty) {
        return CLUSTERD_NONE;
    }
    if (cd.size > 1) {
        pull_cells_down(p_data->cells);
        pull_cells_left(p_data->cells);
    }
    return cd;
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
//     pull_cells_down(p_data->cells);
//     pull_cells_left(p_data->cells);
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

// template < >
// ostream& operator<<(ostream& _out, const ClusterT<int, CELL_NONE>& cluster) {
//     return _out << cluster;
// }
// {
//     _out << "Rep= " << cluster.rep << " { ";
//     for (auto it = cluster.members.cbegin();
//          it != cluster.members.cend();
//          ++it)
//     {
//         _out << *it << ' ';
//     }
//     return _out << " }";
// }

template <typename _Cluster>
ostream& operator<<(ostream& _out, const State_Action<_Cluster>& sa);

template <>
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

void State::enumerate_clusters(ostream& _out) const
{
    generate_clusters(*this);

    for (auto it = details::dsu.begin();
         it != details::dsu.end();
         ++it) {
        spdlog::info("Rep={}, size={}", it->rep, it->size());
    }
}

void State::view_clusters(ostream& _out) const
{
    generate_clusters(*this);

    for (auto it = details::dsu.cbegin();
         it != details::dsu.cend();
         ++it) {
        assert(it->rep < MAX_CELLS && it->rep > -1);

        if (it->size() > 1 && it->rep == details::dsu.find_rep(it->rep) && get_color(it->rep) != Color::Empty) { //size() > 1) {
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

// /**
//  * When comparing clusters, only look at the (unordered) members,
//  * the representatives don't matter as long as the members are the same.
//  */
// template < typename _Index_T, _Index_T N >
// bool operator==(const ClusterT<_Index_T, N>& a, const ClusterT<_Index_T, N>& b) {
//     if (a.size() != b.size()) {
//         return false;
//     }

//     // Create copies so we can sort them
//     auto a_members = a.members;
//     auto b_members = b.members;

//     // Sort to make the two containers comparable
//     std::sort(a_members.begin(), a_members.end());
//     std::sort(b_members.begin(), b_members.end());

//     for (auto i = 0; i < a.members.size(); ++i) {
//         if (a_members[i] != b_members[i]) {
//             return false;
//         }
//     }
//     return true;
// }

// template < >
// bool operator==(const ClusterT<int, MAX_CELLS>& a, const ClusterT<int, MAX_CELLS>& b) {
//     if (a.size() != b.size()) {
//         return false;
//     }

//     // Create copies so we can sort them
//     auto a_members = a.members;
//     auto b_members = b.members;

//     // Sort to make the two containers comparable
//     std::sort(a_members.begin(), a_members.end());
//     std::sort(b_members.begin(), b_members.end());

//     for (auto i = 0; i < a.members.size(); ++i) {
//         if (a_members[i] != b_members[i]) {
//             return false;
//         }
//     }
//     return true;
// }

bool operator==(const ClusterData& a, const ClusterData& b)
{
    return a.rep == b.rep && a.color == b.color && a.size == b.size;
}

} //namespace sg
