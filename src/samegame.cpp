// samegame.cpp
//
// TODO Don't store as much data for the states. Maybe declare the
//
//
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <functional>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "samegame.h"
#include "debug.h"


namespace sg {


//**************************  Disjoint Union Structure  ********************************/

// Utility datastructure used to compute the valid actions from a state,
// i.e. form the clusters.
namespace DSU {

    // using ClusterList = std::array<Cluster, MAX_CELLS>;
    //using repCluster = std::pair<Cell, Cluster>;
    //typedef std::array<repCluster, MAX_CELLS> ClusterList;

    ClusterList cl { };

    void reset()
    {
        Cell cell = 0;
        for (auto& c : cl) {
            c.rep = cell;
            c.members = { cell };
            ++cell;
        }
    }

    Cell find_rep(Cell cell)
    {
        // Condition for cell to be a representative
        if (cl[cell].rep == cell) {
            return cell;
        }
        return  cl[cell].rep = find_rep(cl[cell].rep);
    }

    void unite(Cell a, Cell b)
    {
        a = find_rep(a);
        b = find_rep(b);

        if (a != b) {
             // Make sure the cluster at b is the smallest one
            if (cl[a].members.size() < cl[b].members.size()) {
                std::swap(a, b);
            }

            cl[b].rep = cl[a].rep; // Update the representative
            cl[a].members.splice(cl[a].members.end(), cl[b].members); // Merge the cluster of b into that of a
        }
    }

} //namespace DSU

//*************************************  Zobrist  ***********************************/

namespace Zobrist {

    std::array<Key, 1126> ndx_keys { 0 }; // First entry = 0 for a trivial action

    Key clusterKey(const ClusterData& cd) {
        return ndx_keys[cd.rep * cd.color];
    }
    Key rarestColorKey(const State& state) {
        auto rarest_color = *std::min_element(state.colors.begin() + 1, state.colors.end());
        return rarest_color << 1;
    }
    Key terminalKey(const State& state) {
        return state.is_terminal();
    }

} // namespace Zobrist


//********************************  Bitwise methods  ***********************************/

// NOTE It is expected that the r_cluster member is a reference to the
// current list of clusters when calling generate_key(). Otherwise it will
// generate the key of some other state. Note also that the key doesn't completely
// determine the state, but most likely is good enough. We could have a 'next state
// lookup table' taking current_key ^ cluster_key to the key of of the state we get
// when applying that cluster (storing the cluster's known value in there?)
//
// Generate a Zobrist hash from the list of ClusterData objects
Key State::generate_key() const
{
    Key key = 0;

    for (const auto& cluster : r_clusters)
    {
        if (cluster.members.size() > 1)
            key ^= Zobrist::clusterKey({cluster.rep, p_data->cells[cluster.rep]});
    }

    //key ^= Zobrist::rarestColorKey(*this);
    //key ^= Zobrist::terminalKey(*this);

    return key;
}

Key State::key() const
{
    return p_data->key;
}

//*************************************** Game logic **********************************/

void State::init()
{
    std::random_device rd;
    std::uniform_int_distribution<unsigned long long> dis(
        std::numeric_limits<std::uint64_t>::min(),
        std::numeric_limits<std::uint64_t>::max());

    for (int i = 1; i < 1126; ++i) {
        Zobrist::ndx_keys[i] = ((dis(rd) >> 4) << 4); // Reserve first bit for terminal,
                                                      // next 3 bits for rarest color
    }
}

State::State(std::istream& _in, StateData* sd)
    : p_data(sd)
    , colors{0}
    , r_clusters(DSU::cl)
{
    int _in_color = 0;
    Grid& cells = p_data->cells;

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            _in >> _in_color;
            cells[j + i * WIDTH] = Color(_in_color + 1);
        }
    }

    generate_clusters();
    p_data->key = generate_key();
    p_data->ply = 0;
    init_ccounter();
}

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
    auto _beg = r_clusters.begin();
    auto _end = r_clusters.end();

    return std::none_of(_beg, _end, [](const auto& cluster) { return cluster.members.size() > 1; });
}

bool State::is_empty() const
{
    return cells()[(HEIGHT - 1) * WIDTH] == COLOR_NONE;
}

//******************************* MAKING MOVES ***********************************/


// TODO start from bottom left corner and try to stop early when most of the grid is empty.
void State::generate_clusters() const
{
    DSU::reset();
    Grid& m_cells = p_data->cells;
    // First row
    for (int i = 0; i < WIDTH - 1; ++i) {
        if (m_cells[i] == COLOR_NONE)
            continue;
        // Compare right
        if (m_cells[i + 1] == m_cells[i])
            DSU::unite(i + 1, i);
    }
    // Other rows
    for (int i = WIDTH; i < MAX_CELLS; ++i) {
        if (m_cells[i] == COLOR_NONE)
            continue;
        // Compare up
        if (m_cells[i - WIDTH] == m_cells[i])
            DSU::unite(i - WIDTH, i);
        // Compare right if not at end of row
        if ((i + 1) % WIDTH != 0 && m_cells[i + 1] == m_cells[i])
            DSU::unite(i + 1, i);
    }
    // NOTE instead of the following, we now store a ref to DSU::cl and will filter out the trivial
    //      clusters in the call to valid_actions()
    //
    // for (int i = 0; i < MAX_CELLS; ++i) {
    //     auto& cluster = DSU::cl[i].members;

    //     if (cluster.size() > 1) {
    //         auto color = m_cells[cluster.back()];
    //         colors[color] += cluster.size();
    //         m_clusters.push_back(&cluster);
    //     }
    // }
}

void State::init_ccounter()
{
    Grid& m_cells = p_data->cells;
    for (const auto& cluster : r_clusters)
    {
        colors[m_cells[cluster.members.back()]] += cluster.members.size();
    }
}

Cluster* State::get_cluster(Cell cell) const
{
    return &r_clusters[DSU::find_rep(cell)];
}

// Remove the cells in the chosen cluster and adjusts the colors counter.
void State::kill_cluster(Cluster* cluster)
{
    assert(cluster != nullptr);
    assert(cluster->members.size() > 1);    // NOTE: if cluster is 0 initialized this doesn't check anything

    for (auto& cell : cluster->members) {
        Color& color = p_data->cells[cell];
        --colors[color];
        color = COLOR_NONE;
    }
}

// void State::kill_cluster(const Cluster* cluster)
// {
//     assert(cluster != nullptr);
//     assert(cluster->members.size() > 1);    // NOTE: if cluster is 0 initialized this doesn't check anything

//     for (auto& cell : cluster->members) {
//         Color& color = p_data->cells[cell];
//         --colors[color];
//         color = COLOR_NONE;
//     }
// }

// ActionVec State::valid_actions() const
// {
//     ActionVec ret { };
//     //generate_clusters();    // NOTE: This is done after apply_action calls and on construction of the state.
//     for (const auto& cluster : r_clusters) {
//         if (cluster.members.size() > 1)
//             ret.push_back(cluster.rep);
//     }

//     return ret;
// }

// NOTE: Can return empty vector
ActionDVec State::valid_actions_data() const
{
    ActionDVec ret { };
    if (r_clusters.empty())
        return ret;
    for (const auto& cluster : r_clusters) {
        uint8_t size = cluster.members.size();
        if (size > 1)
        {
            ClusterData cd = { cluster.rep, cells()[cluster.rep], size };
            ret.push_back(cd);
        }
    }

    return ret;
}

// VecClusterV State::valid_actions_clusters() const
// {
//     VecClusterV ret { };
//     for (const auto& cluster : r_clusters) {
//         ClusterV cv;

//         if (cluster.members.size() > 1)
//         {
//             std::copy(cluster.members.begin(), cluster.members.end(), back_inserter(cv.members));
//             ret.push_back(cv);
//         }
//     }
//     return ret;
// }

// // TODO Just call both apply_action methods the same but one takes a pointer to a cluster.
// //
// // NOTE: Applicable when we have just generated the clusters
// void State::apply_action_cluster(const Cluster* p_cluster, StateData& sd)
// {
//     sd.previous = p_data;                    // Provide info of current state.
//     sd.cells = cells();                      // Copy the current grid on the provided StateData object
//     //Key key = p_data->key;
//     p_data = &sd;

//     // Update the grid in the new StateData object
//     // FIXME I hate doing this but for now I need to recalculate the cluster
//     //
//     // generate_clusters();
//     //
//     // NOTE: This is to be used in the rollout phase. The agent will just have asked for
//     // all valid actions so they will be generated already.

//     kill_cluster(p_cluster);             // The color counter is decremented here
//     pull_cells_down();
//     pull_cells_left();

//     // TODO Make necessary computations and update the keys.
//     generate_clusters();
//     p_data->key = generate_key();
// }

// // NOTE: Applicable when we have just generated the clusters
// void State::apply_action_gen_clusters(Action action, StateData& sd)
// {
//     sd.previous = p_data;                    // Provide info of current state.
//     sd.cells = cells();                      // Copy the current grid on the provided StateData object
//     p_data = &sd;

//     auto* cluster = get_cluster(action);
//     kill_cluster(cluster);                   // The color counter is decremented here
//     pull_cells_down();
//     pull_cells_left();

//     // TODO Make necessary computations and update the keys.
//     generate_clusters();
//     p_data->key = generate_key();

//     // p_data->key = key();
// }

void State::apply_action(const ClusterData& cd, StateData& sd)
{
    sd.previous = p_data;                    // Provide info of current state.
    sd.cells = cells();                      // Copy the current grid on the provided StateData object
    //Key key = p_data->key;
    p_data = &sd;

    // Update the grid in the new StateData object
    // FIXME I hate doing this but for now I need to recalculate the cluster
    //
    generate_clusters();

    kill_cluster(get_cluster(cd.rep));     // The color counter is decremented here
    pull_cells_down();
    pull_cells_left();

    // TODO Make necessary computations and update the keys.
    generate_clusters();
    p_data->key = generate_key();
}

// // NOTE: To be used when we no longer have access to the DSU data
// // TODO implement more cleaverly, using bitboards say
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

//     colors[color] -= cnt;
//     pull_cells_down();
//     pull_cells_left();
//     generate_clusters();
//     p_data->key = generate_key();
// }

// Need a ClusterData object if undoing multiple times in a row!!!!
void State::undo_action(Action action)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    auto& cluster_cells = get_cluster(action)->members;
    Color color = p_data->cells[cluster_cells.back()];
    colors[color] += cluster_cells.size();
}

void State::undo_action(const ClusterData& cd)
{
    // Simply revert the data
    p_data = p_data->previous;

    // Use the action to reincrement the color counter
    colors[cd.color] += cd.size;
}


} //namespace sg
