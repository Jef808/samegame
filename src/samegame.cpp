// samegame.cpp
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <functional>
#include <list>
#include <map>
#include <random>
#include <utility>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include "samegame.h"
#include "dsu.h"

//#include "debug.h"


namespace sg {

const Cluster CLUSTER_NONE = {CELL_NONE, {}};
const ClusterData CLUSTERD_NONE = {CELL_NONE, COLOR_NONE, 0};

template < typename CellContainer >
std::string to_string(const CellContainer& cl_members) {
    std::stringstream ss;
    ss << "{ ";
    for (const auto& m : cl_members) {
        ss << (int)m << ", ";
    }
    return ss.str();
}

//**************************  Disjoint Union Structure  ********************************/

// Utility datastructure used to compute the valid actions from a state,
// i.e. form the clusters.
//namespace DSU {

    // // using ClusterList = std::array<Cluster, MAX_CELLS>;
    // //using repCluster = std::pair<Cell, Cluster>;
    // //typedef std::array<repCluster, MAX_CELLS> ClusterList;

    // ClusterList cl { };

    // void reset()
    // {
    //     Cell cell = 0;
    //     for (auto& c : cl) {
    //         c.rep = cell;
    //         c.members = { cell };
    //         ++cell;
    //     }
    // }

    // Cell find_rep(Cell cell)
    // {
    //     // Condition for cell to be a representative
    //     if (cl[cell].rep == cell) {
    //         return cell;
    //     }
    //     return  cl[cell].rep = find_rep(cl[cell].rep);
    // }

    // // void unite(Cell a, Cell b)
    // // {
    // //     a = find_rep(a);
    // //     b = find_rep(b);

    // //     if (a != b) {
    // //          // Make sure the cluster at b is the smallest one
    // //         if (cl[a].members.size() < cl[b].members.size()) {
    // //             std::swap(a, b);
    // //         }

    // //         cl[b].rep = cl[a].rep; // Update the representative
    // //         cl[a].members.splice(cl[a].members.end(), cl[b].members); // Merge the cluster of b into that of a
    // //     }
    // // }

    // void unite(Cell a, Cell b)
    // {
    //     a = find_rep(a);
    //     b = find_rep(b);

    //     if (a != b) {
    //          // Make sure the cluster at b is the smallest one
    //         if (cl[a].members.size() < cl[b].members.size()) {
    //             std::swap(a, b);
    //         }

    //         cl[b].rep = cl[a].rep; // Update the representative
    //         // auto& members_a = cl[a].members;
    //         // const auto& members_b = cl[b].members;
    //         // members_a.insert(end(members_a),
    //         //                  begin(members_b),
    //         //                  end(members_b));
    //         cl[a].members.insert(cl[a].members.cend(),
    //                              cl[b].members.cbegin(),
    //                              cl[b].members.cend());
    //         cl[b].members.clear();
    //     }
    // }

//} //namespace DSU

namespace details {

    DSU dsu = DSU();

} // namespace details

//*************************************  Zobrist  ***********************************/

namespace Zobrist {

    std::array<Key, 1126> ndx_keys { 0 }; // First entry = 0 for a trivial action

    Key cellColorKey(Cell cell, Color color) {
        return ndx_keys[cell * color];
    }

    Key clusterKey(const ClusterData& cd) {
        return ndx_keys[cd.rep * cd.color];
    }
    // Key rarestColorKey(const State& state) {
    //     auto rarest_color = *std::min_element(state.colors.begin() + 1, state.colors.end());
    //     return rarest_color << 1;
    // }
    Key terminalKey(const State& state) {
        return state.is_terminal();
    }

} // namespace Zobrist

//*************************************** Game logic **********************************/

/**
 * Check if a cluster is valid as an action
 *
 * @Note If cluster.members was implemented with
 * static memory size (say an std::array) with some
 * 0-initialization by default, checking that
 * members.size() > 1 means nothing!
 * Add a test for that.
 */
bool State::is_valid(const Cluster& cluster) const
{
    return cluster.rep != CELL_NONE && cluster.members.size() > 1;
}

void State::init()
{
    std::random_device rd;
    std::uniform_int_distribution<unsigned long long> dis(
        std::numeric_limits<std::uint64_t>::min(),
        std::numeric_limits<std::uint64_t>::max());

    for (int i = 1; i < 1126; ++i) {
        Zobrist::ndx_keys[i] = ((dis(rd) >> 1) << 1); // Reserve first bit for terminal
    }
}

State::State(std::istream& _in, StateData& sd)
    : p_data(&sd)
    , colors{0}
    , p_dsu(&details::dsu)

{
    int _in_color = 0;
    Grid& cells = p_data->cells;

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            _in >> _in_color;
            cells[j + i * WIDTH] = Color(_in_color + 1);
        }
    }

    p_data->n_empty_rows = 0;
    generate_clusters();
    init_ccounter();
    p_data->key = generate_key();
    p_data->ply = 0;

}

void State::init_ccounter()
{
    Grid& m_cells = p_data->cells;
    for (auto it = p_dsu->begin();
         it != p_dsu->end();
         ++it)
    {
        colors[m_cells[it->rep]] += it->members.size();
    }
}

// TODO: Only check columns intersecting the killed cluster
void State::pull_cells_down()
{
    Grid& m_cells = p_data->cells;
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        // stack the non-zero colors going up
        std::array<Color, HEIGHT> new_column { COLOR_NONE };
        int new_height = 0;

        for (int j = 0; j < HEIGHT; ++j) {
            auto bottom_color = m_cells[i + (HEIGHT - 1 - j) * WIDTH];

            if (bottom_color != COLOR_NONE) {
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

    while (i < WIDTH)
    {
        if (zero_col.empty())
        {
            // Look for empty column
            while (i < WIDTH - 1 && m_cells[i + (HEIGHT - 1) * WIDTH] != COLOR_NONE) {
                ++i;
            }
            zero_col.push_back(i);
            ++i;

        } else
        {
            int x = zero_col.front();
            zero_col.pop_front();
            // Look for non-empty column
            while (i < WIDTH && m_cells[i + (HEIGHT - 1) * WIDTH] == COLOR_NONE) {
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

bool State::is_terminal() const
{
    return std::none_of(p_dsu->begin(), p_dsu->end(), [](const auto& cluster) {
        return cluster.members.size() > 1;
    });
}

// bool State::check_terminal() const
// {
//     auto _beg = r_clusters.begin();
//     auto _end = r_clusters.end();

//     return std::none_of(_beg, _end, [](const auto& cluster) { return cluster.members.size() > 1; });
// }

bool State::is_empty() const
{
    return p_data->cells[(HEIGHT - 1) * WIDTH] == COLOR_NONE;
}


// NOTE: Can return empty vector
ActionDVec State::valid_actions_data() const
{
    generate_clusters();
    ActionDVec ret { };

    for (auto it = details::dsu.begin();
         it != details::dsu.end();
         ++it)
    {
        if (size_t sz = it->members.size(); sz > 1) {
            if (Cell rep = it->rep; rep != CELL_NONE) {
                if (Color color = p_data->cells[rep]; color != COLOR_NONE) {
                    ret.emplace_back(ClusterData{rep, color, sz});
                }
            }
        }
    }
    return ret;
}

//******************************* MAKING MOVES ***********************************/


// TODO start from bottom left corner and try to stop early when most of the grid is empty.
void State::generate_clusters_old() const
{
    // TODO: Implement something like that:
    // if (!need_refresh_clusters) {
    //     spdlog::debug("Unnecessary gen_clusters call");
    //     return;
    // }
    p_dsu->reset();
    Grid& m_cells = p_data->cells;
    // First row
    for (int i = 0; i < WIDTH - 1; ++i) {
        if (m_cells[i] == COLOR_NONE)
            continue;
        // Compare right
        if (m_cells[i + 1] == m_cells[i])
            p_dsu->unite(i + 1, i);
     }
    // Other rows
    for (int i = WIDTH; i < MAX_CELLS; ++i) {
        if (m_cells[i] == COLOR_NONE)
            continue;
        // Compare up
        if (m_cells[i - WIDTH] == m_cells[i])
            p_dsu->unite(i - WIDTH, i);
        // Compare right if not at end of row
        if ((i + 1) % WIDTH != 0 && m_cells[i + 1] == m_cells[i])
            p_dsu->unite(i + 1, i);
    }

    // NOTE: This doesn't work! We need to move the cluster members along with
    // the representative to be consistent with the data structure!
    // for (auto it = p_dsu->begin();
    //      it != p_dsu->end();
    //      ++it)
    // {
    //     if (it->members.size() > 1) {
    //         auto rep = *std::min_element(it->begin(), it->end());

    //     }
    //     // std::sort(begin(cluster.members), end(cluster.members));
    //     // cluster.rep = cluster.members.front();
    // }
}

void State::generate_clusters() const
{
    p_dsu->reset();
    Grid& m_cells = p_data->cells;
    p_data->n_empty_rows = 0;
    bool row_empty = false;
    int j = HEIGHT - 1;

    // Iterate from bottom row to the second row
    while (j > 0)
    {
        row_empty = true;
        // All row except last cell
        for (int i = j * WIDTH; i < j * WIDTH + WIDTH-1; ++i)
        {
            // compare up
            if (m_cells[i] && (m_cells[i] == m_cells[i-WIDTH])) {
                p_dsu->unite(i, i - WIDTH);
                row_empty = false;
            }
            // compare right
            if (m_cells[i] && (m_cells[i] == m_cells[i+1])) {
                p_dsu->unite(i, i+1);
                row_empty = false;
            }
        }
        // Last cell, compare up
        if (m_cells[j * WIDTH + WIDTH-1] && (m_cells[j * WIDTH + WIDTH-1] == m_cells[j * WIDTH - 1])) {
            p_dsu->unite(j * WIDTH + WIDTH-1, j * WIDTH - 1);
            row_empty = false;
        }
        if (row_empty) {
            p_data->n_empty_rows = j;
            break;
        }
        --j;
    }
    // First row
    if (!row_empty)
    {
        for (int i = 0; i < WIDTH - 1; ++i)
        {
            if (m_cells[i] && (m_cells[i] == m_cells[i+1])) {
                p_dsu->unite(i, i+1);
                row_empty = false;
            }
        }

        p_data->n_empty_rows = int(row_empty);
    }

    // NOTE: This doesn't work! We need to move the cluster members along with
    // the representative to be consistent with the data structure!
    // TODO: Make sure things are canonical while generating the clusters, or after when
    // actually sending them in response to a query?
    // for (auto it = p_dsu->begin();
    //      it != p_dsu->end();
    //      ++it)
    // {
    //     if (it->members.size() > 1) {
    //         it->rep = *std::min_element(it->begin(), it->end());
    //     }
    // }
}

// TODO Stop sending back naked pointers to data that's constantly changing maybe?
// TODO Use DSU::find_rep...
Cluster State::get_cluster(Cell cell) const
{
    generate_clusters();
    if (cell == MAX_CELLS || p_data->cells[cell] == COLOR_NONE) {
        spdlog::warn("Call made to get_cluster with invalid cell {}, returning CLUSTER_NONE", (int)cell);
        return Cluster { };
    }

    return p_dsu->get_cluster(cell);
    // Cell rep = p_dsu->find_rep(cell);
    // return p_dsu->get_cluster(rep);
    // auto it = std::find_if(DSU::cl.begin(), DSU::cl.end(), [c = cell](const auto& cluster) {
    //     return cluster.members.size() > 1 && std::find(begin(cluster.members), end(cluster.members), c) != end(cluster.members);
    // });

    // if (it != DSU::cl.end()) {
    //     return &(*it);
    // } else {
    //     spdlog::debug("Returning CLUSTER_NONE from get_cluster!\nState was \n{}\n", State_Action{*this});
    //     return const_cast<Cluster*>(&CLUSTER_NONE);
    // }
}

// TODO Stop sending back naked pointers to data that's constantly changing maybe?
// Cluster* State::get_cluster(const Cell& cell) const
// {
//     // TODO Skip these checks unless in debug mode
//     if (cell == CELL_NONE || cell == MAX_CELLS) {
//         spdlog::warn("Call made to get_cluster with invalid cell {}, returning CLUSTER_NONE", (int)cell);
//         return const_cast<Cluster*>(&CLUSTER_NONE);
//     }

//     if (p_data->cells[cell] == COLOR_NONE) {
//         spdlog::warn("Call made to get_cluster with empty cell {}, returning CLUSTER_NONE", (int)cell);
//         return const_cast<Cluster*>(&CLUSTER_NONE);
//     }

//     generate_clusters();
//     const auto& rep = DSU::find_rep(cell);
//     Cluster* ret = &DSU::cl[rep];

//     if (ret->members.empty()) {
//         spdlog::warn("returning pointer to cluster with empty members from call to get_clusters with cell {}", (int)cell);
//         return const_cast<Cluster*>(&CLUSTER_NONE);
//     }

//     return &DSU::cl[rep];
// }

// Remove the cells in the chosen cluster and adjusts the colors counter.
void State::kill_cluster(const Cluster& cluster)
{
    // if (cluster == nullptr)
    // {
    //     auto logger = spdlog::get("m_logger");
    //     //logger->set_level(spdlog::level::trace);
    //     spdlog::critical("kill_cluster called with nullptr. Current state is {}", State_Action{*this});
    //     return;
    // } else if (cluster == &CLUSTER_NONE)
    // {
    //     spdlog::warn("kill_cluster called with CLUSTER_NONE. Current state is {}", *this);
    //     return;
    // }

    //assert(cluster != nullptr);
    //assert(cluster->members.size() > 1);    // NOTE: if cluster is 0 initialized this doesn't check anything

    // if (cluster.members.size() < 2) {
    //     spdlog::warn("kill_cluster called with cluster {}", cluster);
    // }

    Grid& grid = p_data->cells;
        
    for (const auto cell : cluster.members) {
        Color color = grid[cell];
        --colors[color];
        grid[cell] = COLOR_NONE;
    }
}

void State::apply_action(const ClusterData& cd, StateData& sd)
{
    sd.previous = p_data;                    // Provide info of current state.
    sd.cells = cells();                      // Copy the current grid on the provided StateData object
    p_data = &sd;

    //generate_clusters();    // NOTE: There is a call to generate_clusters at start of get_cluster
    auto cluster = get_cluster(cd.rep);

    if (cluster.members.size() < 2)
        spdlog::warn("Call to State::apply_action made with cluster {}", cd);

    kill_cluster(cluster);     // The color counter is decremented here
    pull_cells_down();
    pull_cells_left();

    generate_clusters();
    p_data->key = generate_key();
}

// Need a ClusterData object if undoing multiple times in a row!!!!
void State::undo_action(Action action)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    auto cluster_cells = get_cluster(action).members;
    const Color color = p_data->cells[cluster_cells.back()];
    colors[color] += cluster_cells.size();
}

void State::undo_action(const ClusterData& cd)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    colors[cd.color] += cd.size;
}

// Tries to kill the cluster which `cell` is a member of.
// If the cell is empty or isolated, don't do anything.
// Return the ClusterData of `cell`.
ClusterData State::kill_cluster_blind(const Cell cell, const Color color)
{
    if (color == COLOR_NONE)
    {
        return {CELL_NONE, COLOR_NONE, 0};
        spdlog::warn("called kill_cluster_blind with an empty color! State was \n{}\n", *this);
    }

    std::deque<Action> queue{ cell };
    uint cnt = 1;
    Grid& grid = p_data->cells;
    Cell cur;

    grid[cell] = COLOR_NONE;
    while (!queue.empty())
    {
        cur = queue.back();
        queue.pop_back();
        // remove boundary cells with color equal to current cell's color
        // Look right
        if (cur % WIDTH < WIDTH - 1 && grid[cur+1] == color) {
            grid[cur+1] = COLOR_NONE;
            queue.push_back(cur+1);
            ++cnt;
        }
        // Look down
        if (cur < (HEIGHT - 1) * WIDTH && grid[cur + WIDTH] == color) {
            grid[cur + WIDTH] = COLOR_NONE;
            queue.push_back(cur+WIDTH);
            ++cnt;
        }
        // Look left
        if (cur % WIDTH > 0 && grid[cur-1] == color) {
            grid[cur - 1] = COLOR_NONE;
            queue.push_back(cur-1);
            ++cnt;
        }
        // Look up
        if (cur > WIDTH - 1 && grid[cur - WIDTH] == color) {
            grid[cur - WIDTH] = COLOR_NONE;
            queue.push_back(cur - WIDTH);
            ++cnt;
        }
    }
    //spdlog::debug("Killed {} cells", cnt);
    if (cnt < 2) {
        grid[cell] = color;
    }
    else {
        pull_cells_down();
        pull_cells_left();
    }

    return {cell, color, cnt};
}


//********************************  Bitwise methods  ***********************************/

// Generate a Zobrist hash from the grid
// TODO: As for generate_clusters(), can be more efficient
Key State::generate_key() const
{
    generate_clusters();

    Key key = 0;
    uint8_t ndx = 0;

    for (const auto& cell : p_data->cells) {
        if (cell != COLOR_NONE) {
            key ^= Zobrist::cellColorKey(ndx, cell);
        }
        ++ndx;
    }

    key ^= Zobrist::terminalKey(*this);

    return key;

    // for (const auto& cluster : r_clusters)
    // {
    //     if (cluster.members.size() > 1)
    //     {
    //         //auto rep = *std::min_element(begin(cluster.members), end(cluster.members));
    //         key ^= Zobrist::clusterKey({cluster.rep, p_data->cells[cluster.rep]});
    //     }
    // }

    //key ^= Zobrist::rarestColorKey(*this);
    //key ^= Zobrist::terminalKey(*this);
}

Key State::generate_key(const Cluster& cluster) const
{
    const auto& members = cluster.members;
    if (members.size() < 2)
        return 0;

    const Color cluster_color = p_data->cells[cluster.rep];
    Key key = 0;
    for (const auto& cell : members)
        key ^= Zobrist::cellColorKey(cell, cluster_color);

    return key;
}

// Key State::apply_action(Key state_key, Key action_key)
// {
//     return 0;
// }

Key State::key() const
{
    // if (p_data->key == 0)
    //     p_data->key = generate_key();
    return p_data->key;
}


// ***************************  Randomization  ********************************** //

namespace Random {

std::vector<uint8_t> ordering(const uint8_t _beg, const uint8_t _end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    const size_t n = _end - _beg;
    std::vector<Cell> ret(n);
    std::iota(begin(ret), end(ret), _beg);

    for (auto i = 0; i < n; ++i)
    {
        std::uniform_int_distribution<uint8_t> dist(i, n-1);
        int j = dist(gen);
        std::swap(ret[i], ret[j]);
    }

    return ret;
}

void shuffle(std::vector<uint8_t>& v)
{
    //constexpr auto min = std::chrono::high_resolution_clock::duration::min();
    static std::random_device rd;
    static std::mt19937 gen(rd());
    //std::vector<uint8_t> ret;
    //std::iota(begin(ret), end(ret), 0);
    size_t n = v.size();

    for (uint8_t i = 0; i < n; ++i)
    {
        std::uniform_int_distribution<uint8_t> dist(i, n-1);
        int j = dist(gen);
        std::swap(v[i], v[j]);
    }
}

} // namespace random

std::string display_rows(const std::vector<uint8_t>& v)
{
    std::stringstream ss;
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
    return ss.str();
}

void debug_rows(const std::vector<uint8_t>& v)
{
    spdlog::debug("Rows are: ");
    spdlog::debug(display_rows(v));
}

ClusterData State::kill_random_valid_cluster_generating()
{
    auto ordering = Random::ordering(0, MAX_CELLS);
    generate_clusters();
    ClusterData ret {};
    
    for (auto it = begin(ordering);
         it != end(ordering);
         ++it)
    {
        if (const Color color = p_data->cells[*it];
            color != COLOR_NONE)
        {
            auto cluster = p_dsu->get_cluster(*it);

            if (auto _size = cluster.members.size();
                _size > 1)
            {
                kill_cluster(cluster);
                auto rep = *std::min_element(cluster.begin(), cluster.end());
                return { rep, color, _size };
                break;
            }
        }
    }

    return ret;
}

ClusterData State::kill_random_valid_cluster()
{
    ClusterData ret { };

    // Prepare a random order for the rows.
    std::vector<Cell> rows(15-p_data->n_empty_rows);
    std::iota(begin(rows), end(rows), p_data->n_empty_rows);
    Random::shuffle(rows);

    for (auto row : rows)
    {
        // Get the non-empty cells in that row
        std::vector<Cell> non_empties;
        for (auto i = row * WIDTH; i < row * WIDTH + WIDTH; ++i) {
            if (p_data->cells[i] != COLOR_NONE)
                non_empties.push_back(Cell(i));
        }
        Random::shuffle(non_empties);

        for (auto cell : non_empties) {
            ret = kill_cluster_blind(cell, p_data->cells[cell]);
            if (ret.size > 1) {
                return ret;
            }
        }
    }

    return ret;
}

ClusterData State::kill_random_valid_cluster_new()
{
    using CellnColor = std::pair<Cell, Color>;
    ClusterData ret { };

    // Reference to our grid.
    const auto& grid = p_data->cells;

    // Random ordering for non-empty rows
    auto rows = Random::ordering(p_data->n_empty_rows, 15);

    // Vector to store the non-empty cells and their color per row
    std::vector<CellnColor> non_empties {};
    std::vector<uint8_t> ordering {};
    non_empties.reserve(15);
    ordering.reserve(15);

    for (const auto row : rows)
    {
        // In case more empty rows were discovered in previous iteration of this for loop
        // if (row < p_data->n_empty_rows)
        //     continue;

        // Collect the non-empty cells in this row along with their colors
        auto row_begin_ndx = row * WIDTH;
        auto row_begin_it = begin(grid) + row_begin_ndx;
        auto row_end_it = row_begin_it + WIDTH;
        for (auto it = row_begin_it;
             it != row_end_it;
             ++it)
        {
            if (const Color color = *it; color != COLOR_NONE)
            {
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
             ++it)
         {
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
//             if (cells[i] != COLOR_NONE)
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
//                 [&](auto c){ return c != COLOR_NONE; });

//             bool row_empty = true;

//             if (cells[x + y * 15] != COLOR_NONE)
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
//     ClusterData ret { CELL_NONE, COLOR_NONE, 0 };

//     std::vector<uint8_t> cols;
//     uint8_t x = 210;
//     while (p_data->cells[x] != COLOR_NONE) {
//         cols.push_back(x);
//         ++x;
//     }


//     std::vector<uint8_t> adj;
//     for (auto x : cols)
//     {
//         while (p_data->cells[x] != COLOR_NONE)
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
//     sd.cells[action] = COLOR_NONE;

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
//             sd.cells[cur+1] = COLOR_NONE;
//             queue.push_back(cur+1);
//             ++cnt;
//         }
//         // Look down
//         if (cur < (HEIGHT - 1) * WIDTH && sd.cells[cur + WIDTH] == color) {
//             sd.cells[cur + WIDTH] = COLOR_NONE;
//             queue.push_back(cur+WIDTH);
//             ++cnt;
//         }
//         // Look left
//         if (cur % WIDTH > 0 && sd.cells[cur-1] == color) {
//             sd.cells[cur - 1] = COLOR_NONE;
//             queue.push_back(cur-1);
//             ++cnt;
//         }
//         // Look up
//         if (cur > WIDTH - 1 && sd.cells[cur - WIDTH] == color) {
//             sd.cells[cur - WIDTH] = COLOR_NONE;
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


//*************************** DEBUG *************************/

using namespace std;

namespace display {

enum class Color_codes : int {
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

enum class Shape : int {
    SQUARE,
    DIAMOND,
    B_DIAMOND,
    NONE
};

map<Shape, string> Shape_codes {
    {Shape::SQUARE,    "\u25A0"},
    {Shape::DIAMOND,   "\u25C6"},
    {Shape::B_DIAMOND, "\u25C7"},
};

string shape_unicode(Shape s)
{
    return Shape_codes[s];
}

string color_unicode(Color c)
{
    return std::to_string((int)Color_codes(c + 90));
}

string print_cell(const sg::Grid& grid, Cell ndx, Output output_mode, const Cluster& cluster = CLUSTER_NONE)
{
    bool ndx_in_cluster = find(cluster.cbegin(), cluster.cend(), ndx) != cluster.cend();
    stringstream ss;

    if (output_mode == Output::CONSOLE)
    {
        Shape shape = ndx == cluster.rep ? Shape::B_DIAMOND :
            ndx_in_cluster ? Shape::DIAMOND : Shape::SQUARE;

        ss << "\033[1;" << color_unicode(grid[ndx]) << "m" << shape_unicode(shape) << "\033[0m";

        return ss.str();
    }
    else
    {
        // Underline the cluster representative and output the cluster in bold
        string formatter = "";
        if (ndx_in_cluster)
            formatter += "\e[1m]";
        if (ndx == cluster.rep)
            formatter += "\033[4m]";

        ss << formatter << std::to_string(grid[ndx]) << "\033[0m\e[0m";

        return ss.str();
    }
}

int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

} // namespace display

//****************************** PRINTING BOARDS ************************/

struct State_Action {
    const State& state;
    //Cell actionrep              = MAX_CELLS;
    Cluster cluster             = {};

    std::string display(Output = Output::CONSOLE) const;
};

string State_Action::display(sg::Output output_mode) const
{
    bool labels = true;

    const auto& grid = state.p_data->cells;

    stringstream ss {"\n"};

    // For every row
    for (int y = 0; y < HEIGHT; ++y) {
        if (labels) {
            ss << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }
        // Print row except last entry
        for (int x = 0; x < WIDTH - 1; ++x) {
            ss << display::print_cell(grid, Cell(x + y * WIDTH), output_mode, cluster) << ' ';
        }
        // Print last entry,
        ss << display::print_cell(grid, Cell(WIDTH - 1 + y * WIDTH), output_mode, cluster) << '\n';
    }

    if (labels) {
        ss << std::string(34, '_') << '\n'
             << std::string(5, ' ');

        for (int x : display::x_labels) {

            ss << x << ((x < 10) ? " " : "");
        }
        ss << '\n';
    }

    return ss.str();
}

void State::view_clusters(ostream& _out) const
{
    generate_clusters();
    std::vector<Cluster> clusters;

    for (auto it = p_dsu->begin();
         it != p_dsu->end();
         ++it)
    {
        if (it->members.size() > 1)
            clusters.push_back(*it);
    }
    for (const auto& c : clusters)
    {
        spdlog::info("\n{}\n", State_Action{*this, c});
        this_thread::sleep_for(1000.0ms);
    }
}

template <typename _Index>
ostream& operator<<(ostream& _out, const Cluster_T<_Index>& cluster);

template <>
ostream& operator<<(ostream& _out, const Cluster_T<Cell>& cluster)
{
    return _out << (cluster == CLUSTER_NONE ? "CLUSTER_NONE" : to_string(cluster.members));
}

ostream& operator<<(ostream& _out, const ClusterData& _cd)
{
    return _out << std::to_string(_cd.rep) << ' ' << std::to_string(_cd.color) << ' ' << (int)_cd.size;
}

ostream& operator<<(ostream& _out, const State_Action& state_action)
{
    return _out << state_action.display(Output::CONSOLE);

}

void State::display(ostream& _out, Output output_mode) const
{
    cout << State_Action({*this}).display(Output::CONSOLE);
}

ostream& operator<<(ostream& _out, const State& state)
{
    state.display(_out, Output::CONSOLE);
    return _out;
}

bool operator==(const Cluster& a, const Cluster& b) {
  return a.rep == b.rep && a.members == b.members;
}
bool operator==(const ClusterData& a, const ClusterData& b) {
  return a.color == b.color && a.size == b.size && a.rep == b.rep;
}


} //namespace sg
