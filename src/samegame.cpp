#include <array>
#include <iostream>
#include <deque>
#include <list>
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
        if (cl[a].second.size() < cl[b].second.size())
            std::swap(a, b);

        // Set the representative of b to the one of a.
        cl[b].first = cl[a].first;
        // Merge the (smaller) Cluster at b into the Cluster at a
        cl[a].second.splice(cl[a].second.end(), cl[b].second);

        // cl[a].second.sort();
        // cl[b].second.sort();
        // cl[a].second.merge(cl[b].second);
        // while(!clusters[b].empty())
        // {
        //     Cell v = clusters[b].back();
        //     clusters[b].pop_back();

        //     parent[v] = a;
        //     clusters[a].push_back(v);
        // }
    }
}



} //namespace DSU

//*************************************** Game logic **********************************/

void State::init()
{
    for (auto i=0; i<MAX_CELLS; ++i)
    {
        DSU::_Defaults[i].push_back(i);
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
}

void State::pull_down()
{
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        int bottom = i + WIDTH * (HEIGHT - 1);
        // look for an empty cell
        while (bottom > WIDTH - 1 && m_cells[bottom] != 0) {
            bottom -= WIDTH;
        }
        // make non-empty m_cells above drop
        int top = bottom - WIDTH;
        while (top > -1) {
            // if top is non-empty, swap its color with empty color and move both pointers up
            if (m_cells[top] != 0) {
                std::swap(m_cells[top], m_cells[bottom]);
                bottom -= WIDTH;
            }
            // otherwise move top pointer up
            top -= WIDTH;
        }
    }
}

void State::pull_left()
{
    int left_col = WIDTH * (HEIGHT - 1);
    // look for an empty column
    while (left_col < MAX_CELLS && m_cells[left_col] != 0) {
        ++left_col;
    }
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

    // Retrieve the non-empty clusters in the list.
    for (int i=0; i<MAX_CELLS; ++i)
    {
        if (DSU::cl[i].second.size() > 0)
            m_clusters.push_back(&DSU::cl[i].second);
        // if (DSU::cl[i].first == i)
        //     m_clusters.push_back(&DSU::cl[i].second);
    }

    return m_clusters;
}

void State::kill_cluster(const Cluster& c)
{
    for (int i=0; i<c.size(); ++i)
    {
        m_cells[i] = COLOR_NONE;
    }
}


//*******************************  DEBUGGING  ***********************************/

void State::display(std::ostream& _out, bool labels) const
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        if (labels) {
            _out << HEIGHT - 1 - y << ((HEIGHT - 1 - y < 10) ? "  " : " ") << "| ";
        }

        for (int x = 0; x < WIDTH - 1; ++x) {
            _out << (int)m_cells[x + y * WIDTH] << ' ';
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
