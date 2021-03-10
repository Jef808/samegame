#include <array>
#include <iostream>
#include <deque>
#include <string>
#include <vector>
#include "samegame.h"

//********************************************* Utilities ********************************/

// Utility datastructure used to compute the valid actions from a state,
// i.e. form the clusters.
namespace DSU {

typedef std::array<int, MAX_CELLS> ParentNdx;

constexpr ParentNdx DEFAULT_PN();

ParentNdx parent;
ClusterList clarr;

void reset()
{
    parent = DEFAULT_PN();
    clarr = ClusterList{ };
}

int find_rep(int ndx)
{
    if (parent[ndx] == ndx)
        return ndx;

    return parent[ndx] = find_rep(parent[ndx]);
}

void unite(int a, int b)
{
    a = find_rep(a);
    b = find_rep(b);

    if (a != b)
    {
        if (clarr[a].size < clarr[b].size)
            std::swap(a, b);

        int last_b = clarr[b].size - 1;
        int last_a = clarr[a].size - 1;

        // Merge the (smaller) Cluster at b into the Cluster at a
        while(last_b > 0)
        {
            int v = clarr[b][last_b];

            parent[v] = a;
            clarr[a][last_a] = v;

            ++clarr[a].size;
            --clarr[b].size;
        }
    }
}

constexpr ParentNdx DEFAULT_PN()
{
    ParentNdx pn { };
    for (int i=0; i<MAX_CELLS; ++i)
        pn[i] = i;
    return pn;
}

} //namespace DSU

//*************************************** Game logic **********************************/

void State::pull_down()
{
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        int bottom = i + WIDTH * (HEIGHT - 1);
        // look for an empty cell
        while (bottom > WIDTH - 1 && cells[bottom] != 0) {
            bottom -= WIDTH;
        }
        // make non-empty cells above drop
        int top = bottom - WIDTH;
        while (top > -1) {
            // if top is non-empty, swap its color with empty color and move both pointers up
            if (cells[top] != 0) {
                std::swap(cells[top], cells[bottom]);
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
    while (left_col < MAX_CELLS && cells[left_col] != 0) {
        ++left_col;
    }
    // pull columns on the right to the empty column
    int right_col = left_col + 1;
    while (right_col < MAX_CELLS) {
        // if the column is non-empty, swap all its colors with the empty column and move both pointers right
        if (cells[right_col] != 0) {
            for (int i = 0; i < HEIGHT; ++i) {
                std::swap(cells[right_col - i * WIDTH], cells[left_col - i * WIDTH]);
            }
            ++left_col;
        }
        // otherwise move right pointer right
        ++right_col;
    }
}

//******************************* MAKING MOVES ***********************************/

ClusterList& cluster_list()
{

}

void State::kill_cluster(const Cluster& c)
{
    for (int i=0; i<c.size; ++i)
    {
        cells[i] = COLOR_NONE;
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
            _out << (int)cells[x + y * WIDTH] << ' ';
        }

        _out << (int)cells[WIDTH - 1 + y * WIDTH] << std::endl;
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
