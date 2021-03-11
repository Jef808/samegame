#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <deque>
#include <list>
#include <random>
#include <string>
#include <sstream>
#include <vector>
#include "samegame.h"
#include "types.h"

namespace sg {

//********************************************* Utilities ********************************/

// Utility datastructure used to compute the valid actions from a state,
// i.e. form the clusters.
namespace DSU {

std::array<Cluster, MAX_CELLS> _Defaults;

using repCluster = std::pair<Cell, Cluster>;
typedef std::array<repCluster, MAX_CELLS> ClusterList;

ClusterList cl;

void reset()
{
    for (int i=0; i<MAX_CELLS; ++i)
    {
        cl[i].second = _Defaults[i];
        cl[i].first  = i;
    }
}

Cell find_rep(Cell cell)
{
    // Condition for cell to be a representative
    if (cl[cell].first == cell)
        return cell;

    return cl[cell].first = find_rep(cl[cell].first);
}

void unite(Cell a, Cell b)
{
    a = find_rep(a);
    b = find_rep(b);

    if (a != b)
    {
        // Make sure to move the smallest of the two clusters
        if (cl[a].second.size() < cl[b].second.size())
            std::swap(a, b);
        // Set the representative of b to the one of a
        cl[b].first = cl[a].first;
        // Merge the (smaller) Cluster at b into the Cluster at a
        cl[a].second.splice(cl[a].second.end(), cl[b].second);
    }
}

} //namespace DSU


//*******************************  ZOBRIST  ***********************************/

namespace Zobrist {

   std::array<Key, 19> ndx_keys { 0 };    // First entry = 0 for a trivial action

    Key moveKey(Action action) {
       return ndx_keys[action];
    }
    Key colorKey(Color color) {
       return ndx_keys[color];
    }
    Key clustersKey(Cluster& cluster) {
       return 0;
    }
    Key terminalKey = 1;

} // namespace Zobrist


//*************************************** Game logic **********************************/

void State::init()
{
    for (auto i=0; i<MAX_CELLS; ++i)
    {
        DSU::_Defaults[i].push_back(i);
    }

    std::random_device rd;
    std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max()
    );
    for (int i=1; i<19; ++i)
    {
        Zobrist::ndx_keys[i] = ((dis(rd) >> 1) << 1);    // Reserve first bit for terminal.
    }
}

State::State(std::istream& _in)
    : m_cells { }
    , m_colors { }
    , m_ply(1)
{
    int _in_color = 0;

    for (int i=0; i<HEIGHT; ++i) {
        for (int j=0; j<WIDTH; ++j) {

            _in >> _in_color;
            m_cells[j + i * WIDTH] = Color(_in_color + 1);
        }
    }

    auto& clusters = cluster_list();
    auto data = new StateData();
    data->key = 0;     // TODO implement keys
    data->ply = m_ply;
}

void State::pull_down()
{

    // For all columns
    for (int i = 0; i < WIDTH; ++i) {

        // stack the non-zero colors going up
        std::array<Color, HEIGHT> new_column { COLOR_NONE };
        int new_height = 0;

        for (int j = 0; j < HEIGHT; ++j)
        {
            auto bottom_color = m_cells[i + (HEIGHT - 1 - j) * WIDTH];

            if (bottom_color != COLOR_NONE)
            {
                new_column[new_height] = bottom_color;
                ++new_height;
            }
        }
        // pop back the value (including padding with 0)
        for (int j = 0; j < HEIGHT; ++j)
        {
            {
                m_cells[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
            }
        }
    }
}

void State::pull_left()
{
    int left_col = WIDTH * (HEIGHT - 1);
    // look for an empty column
    while (left_col < MAX_CELLS && m_cells[left_col] != COLOR_NONE) {
        ++left_col;
    }

    if (left_col == MAX_CELLS)
        return;

    // pull columns on the right to the empty column
    int right_col = left_col + 1;
    while (right_col < MAX_CELLS) {
        // if the column is non-empty, swap all its colors with the empty column and move both pointers right
        if (m_cells[right_col] != 0) {
            for (int i = 0; i < HEIGHT; ++i) {
                std::swap(m_cells[right_col - i * WIDTH], m_cells[left_col - i * WIDTH]);
            }
            ++left_col;
        }
        // otherwise move right pointer right
        ++right_col;
    }
}

bool State::is_terminal() const
{
    auto _beg = m_clusters.begin();
    auto _end = m_clusters.end();

    return std::none_of(_beg, _end, [](auto* cluster){ return cluster->size() > 1; });
}

//******************************* MAKING MOVES ***********************************/

ClusterVec& State::cluster_list()
{
    DSU::reset();

    // Traverse first row of grid and merge with same colors to the right
    for (int i=0; i < WIDTH - 1; ++i)
    {
        if (m_cells[i + 1] != 0 && m_cells[i + 1] == m_cells[i])
            DSU::unite(i + 1, i);
    }

    // Traverse the rest of the rows
    for (int i=WIDTH; i<MAX_CELLS; ++i)
    {
        // Merge with same colors above
        if (m_cells[i - WIDTH] != 0 && m_cells[i - WIDTH] == m_cells[i])
            DSU::unite(i - WIDTH, i);

        // Merge with same colors to the right
        if ((i + 1) % WIDTH != 0 && m_cells[i + 1] != 0 && m_cells[i + 1] == m_cells[i])
            DSU::unite(i + 1, i);
    }

    // Retrieve the non-trivial clusters in the list.
    for (int i=0; i<MAX_CELLS; ++i)
    {
        auto& cluster = DSU::cl[i].second;
        if (cluster.size() > 1)
        {
            // Compute the color counter in passing
            auto color = m_cells[cluster.back()];
            m_colors[color] += cluster.size();

            // Save the cluster.
            m_clusters.push_back(&cluster);
        }
    }

    return m_clusters;
}

// Cell get_repr(Cluster* cluster)
// {
//     assert (!cluster->empty());
//     return cluster->front();
// }

// Action action_from_cell(Cell cell)
// {
//     return get_repr(find_cluster(cell));
// }

Cluster* State::find_cluster(Cell cell)
{
    auto ret = std::find_if(m_clusters.begin(), m_clusters.end(),
        [cell](auto* cluster) {
            return std::find(cluster->begin(),
                             cluster->end(),
                             cell) != cluster->end();
        });
    assert (ret != m_clusters.end());
    return *ret;
}

// Remove the cells in the chosen cluster, return
// the number of cells removed and their color.
std::pair<int, Color> State::kill_cluster(Cluster* cluster)
{
    assert(cluster != nullptr);
    assert(!cluster->empty());

    int n_removed = 0;
    Color color = m_cells[cluster->front()];

    if (cluster->size() > 1)
    {
        for (auto cell : *cluster)
        {
            m_cells[cell] = COLOR_NONE;
            ++n_removed;
        }
    }
    return std::make_pair(n_removed, color);
}

void State::apply_action(Action action, StateData& sd)
{
    // Get the whole group to remove.
    auto* cluster = find_cluster(action);
    auto [sz, color] = kill_cluster(cluster);
    // Update the state
    pull_down();
    pull_left();

    // Update the StateData object with a new one
    sd.ply = m_ply + 1;
    sd.previous = data;
    data = &sd;

    // Make necessary computations and update the key.
    ++m_ply;
    m_colors[color] -= sz;
}

//*******************************  DEBUGGING  ***********************************/

std::ostream& _DEFAULT(std::ostream& _out) {
    return _out << "\033[0m";
}
std::ostream& _RED(std::ostream& _out) {
    return _out << "\033[32m";
}

void State::display(std::ostream& _out, Cell ndx, bool labels) const
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        if (labels) {
            _out << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }

        for (int x = 0; x < WIDTH - 1; ++x)
        {
            if (x + y * WIDTH == ndx)
            {
                _RED(_out) << (int)m_cells[x + y * WIDTH];
                _DEFAULT(_out) << ' ';
            }
            _out << (int)m_cells[x + y * WIDTH] << ' ';
        }
        if (WIDTH - 1 + y * WIDTH == ndx)
        {
            _RED(_out) << (int)m_cells[WIDTH - 1 + y * WIDTH];
            _DEFAULT(_out) << ' ';
        }
        _out << (int)m_cells[WIDTH - 1 + y * WIDTH] << std::endl;
    }

    if (labels)
    {
        _out << std::string(34, '_') << '\n';

        int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
        _out << std::string(5, ' ');
        for (int x : x_labels) {
            _out << x << ((x < 10) ? " " : "");
        }

        _out << std::endl;
    }
}
} //namespace sg
