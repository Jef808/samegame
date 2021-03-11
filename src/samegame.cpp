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

//********************************************* DSU UTILITY ********************************/

// Utility datastructure used to compute the valid actions from a state,
// i.e. form the clusters.
namespace DSU {

using repCluster = std::pair<Cell, Cluster>;
typedef std::array<repCluster, MAX_CELLS> ClusterList;

std::array<Cluster, MAX_CELLS> _Defaults;
ClusterList cl;

void reset()
{
    for (int i=0; i<MAX_CELLS; ++i) {

        cl[i].second = _Defaults[i];
        cl[i].first  = i;
    }
}

Cell find_rep(Cell cell)
{
    // Condition for cell to be a representative
    if (cl[cell].first == cell) {

        return cell;
    }

    return cl[cell].first = find_rep(cl[cell].first);
}

void unite(Cell a, Cell b)
{
    a = find_rep(a);
    b = find_rep(b);

    if (a != b) {

        if (cl[a].second.size() < cl[b].second.size()) {       // Make sure the cluster at b is the smallest one

            std::swap(a, b);
        }

        cl[b].first = cl[a].first;                             // Update the representative

        cl[a].second.splice(cl[a].second.end(), cl[b].second); // Merge the cluster of b into that of a
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
    for (auto i=0; i<MAX_CELLS; ++i) {

        DSU::_Defaults[i].push_back(i);
    }

    std::random_device rd;
    std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max()
    );

    for (int i=1; i<19; ++i) {

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
    int i = 0;
    std::deque<int> zero_cols {};

    while (i < WIDTH - 1)
    {
        if (m_cells[i + (HEIGHT - 1) * WIDTH] == COLOR_NONE)
        {
            zero_cols.push_back(i);
        }
        else
        {
            // move right column into the empty left column
            if (!zero_cols.empty())
            {
                auto x = zero_cols.front();
                zero_cols.pop_front();

                for (int j = 0; j < HEIGHT; ++j)
                {
                    m_cells[x + j * WIDTH] = m_cells[i + j *  WIDTH];
                }
            }
        }

        ++i;
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

    // First row
    for (int i=0; i < WIDTH - 1; ++i) {

        // Compare right
        if (m_cells[i + 1] != 0 && m_cells[i + 1] == m_cells[i])

            DSU::unite(i + 1, i);
    }

    // Other rows
    for (int i=WIDTH; i<MAX_CELLS; ++i) {

        // Compare up
        if (m_cells[i - WIDTH] != 0 && m_cells[i - WIDTH] == m_cells[i])

            DSU::unite(i - WIDTH, i);

        // Compare right
        if ((i + 1) % WIDTH != 0 && m_cells[i + 1] != 0 && m_cells[i + 1] == m_cells[i])

            DSU::unite(i + 1, i);
    }

    // Store non-trivial clusters and count colors
    for (int i=0; i<MAX_CELLS; ++i) {

        auto& cluster = DSU::cl[i].second;

        if (cluster.size() > 1) {

            auto color = m_cells[cluster.back()];
            m_colors[color] += cluster.size();

            m_clusters.push_back(&cluster);
        }
    }

    return m_clusters;
}

Cluster* State::find_cluster(Cell cell)
{
    auto ret = std::find_if(m_clusters.begin(), m_clusters.end(),

                        [cell](auto* cluster) {

                    return std::find(cluster->begin(), cluster->end(), cell) != cluster->end();
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

    if (cluster->size() > 1) {

        for (auto cell : *cluster) {

            m_cells[cell] = COLOR_NONE;
            ++n_removed;
        }
    }

    return std::make_pair(n_removed, color);
}

void State::apply_action(Action action, StateData& sd)
{
    auto* cluster = find_cluster(action);

    auto [sz, color] = kill_cluster(cluster);

    // Update the state physically
    pull_down();
    pull_left();

    // Update the StateData objects
    sd.ply = m_ply + 1;
    sd.previous = data;
    data = &sd;

    // Make necessary computations and update the key.
    ++m_ply;
    m_colors[color] -= sz;
}

//*******************************  DEBUGGING  ***********************************/

int x_labels[15] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

std::ostream& _DEFAULT(std::ostream& _out) {

    return _out << "\033[0m";
}
std::ostream& _RED(std::ostream& _out) {

    return _out << "\033[32m";
}

void State::display(std::ostream& _out, Cell ndx, bool labels) const
{
    // For every row
    for (int y = 0; y < HEIGHT; ++y)
    {
        if (labels) {
            _out << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }

        // Print row except last entry
        for (int x = 0; x < WIDTH - 1; ++x)
        {
            // Check for colored entry
            if (x + y * WIDTH == ndx)
            {
                _RED(_out) << (int)m_cells[x + y * WIDTH];
                _DEFAULT(_out) << ' ';
            }
            else
            {
                _out << (int)m_cells[x + y * WIDTH] << ' ';
            }
        }

        // Print last entry,
        // checking for colored entry
        if (WIDTH - 1 + y * WIDTH == ndx)
        {
            _RED(_out) << (int)m_cells[WIDTH - 1 + y * WIDTH];
            _DEFAULT(_out) << ' ';
        }
        else
        {
            _out << (int)m_cells[WIDTH - 1 + y * WIDTH] << '\n';
        }
    }

    if (labels)
    {
        _out << std::string(34, '_') << '\n' << std::string(5, ' ');

        for (int x : x_labels) {

            _out << x << ((x < 10) ? " " : "");
        }

        _out << std::endl;
    }
}


} //namespace sg
