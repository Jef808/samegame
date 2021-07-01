#include "clusterhelper.h"
#include "dsu.h"
#include "rand.h"
#include "types.h"
#include <deque>
#include <iostream>
#include <set>


namespace sg::clusters {

namespace {

/**
 * Data structure to partition the grid into clusters by colors
 */
DSU<sg::Cluster, sg::MAX_CELLS> grid_dsu {};

/**
 * Utility class initializing a random number generator and implementing
 * the methods we need for the random actions.
 */
Rand::Util<Cell> rand_util { };


//************************************** Grid manipulations **********************************/

/**
 * Populate the disjoint data structure grid_dsu with all adjacent clusters
 * of cells sharing a same color.
 */
// FIXME This returns some clusters that are way too large...
void generate_clusters(const Grid& _grid)
{
    grid_dsu.reset();

    auto n_empty_rows = _grid.n_empty_rows;
    bool row_empty = true;
    bool found_empty_cell = false;
    Cell empty_cell = CELL_NONE;

    auto row = HEIGHT - 1;
    // Iterate from bottom row upwards so we can stop at the first empty row.
    while (row > n_empty_rows) {
        row_empty = true;
        // All the row except last cell
        for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell) {
            if (_grid[cell] == Color::Empty) {
                if (found_empty_cell) {
                    grid_dsu.unite(cell, empty_cell);
                } else {
                    empty_cell = cell;
                    found_empty_cell = true;
                }
                continue;
            }
            row_empty = false;
            // compare up
            if (_grid[cell] == _grid[cell - WIDTH]) {
                grid_dsu.unite(cell, cell - WIDTH);
            }
            // compare right
            if (_grid[cell] == _grid[cell + 1]) {
                grid_dsu.unite(cell, cell + 1);
            }
        }
        // If the last cell of the row is empty
        Cell cell = (row + 1) * WIDTH - 1;
        if (_grid[cell] == Color::Empty) {
            if (row_empty) {
                // Unite all the (empty) cells above with the `empty_cell` cell,
                // then return.
                grid_dsu.unite(empty_cell, cell);
                for (auto e_cell = cell - WIDTH; e_cell > 0; --e_cell) {
                    grid_dsu.unite(e_cell, e_cell - 1);
                }
                return;
            }
            if (found_empty_cell) {
                grid_dsu.unite(cell, empty_cell);
            } else {
                empty_cell = cell;
                found_empty_cell = true;
            }
        }
        // If it is not empty, compare up
        else if (_grid[cell] == _grid[cell - WIDTH]) {
            grid_dsu.unite(cell, cell - WIDTH);
        }
        --row;
    }
    // The upmost non-empty row: only compare right
    for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell) {
        //row_empty = true;
        if (_grid[cell] == Color::Empty) {
            if (found_empty_cell) {
                grid_dsu.unite(cell, empty_cell);
            } else {
                empty_cell = cell;
                found_empty_cell = true;
            }
            continue;
        }
        //row_empty = false;
        if (_grid[cell] == _grid[cell + 1]) {
            grid_dsu.unite(cell, cell + 1);
        }
    }
}

/**
 * Make cells drop down if they lie above empty cells.
 */
void pull_cells_down(Grid& _grid)
{
    // For all columns
    for (int i = 0; i < WIDTH; ++i) {
        // stack the non-zero colors going up
        std::array<Color, HEIGHT> new_column { Color::Empty };
        int new_height = 0;

        for (int j = 0; j < HEIGHT; ++j) {
            auto bottom_color = _grid[i + (HEIGHT - 1 - j) * WIDTH];

            if (bottom_color != Color::Empty) {
                new_column[new_height] = bottom_color;
                ++new_height;
            }
        }
        // pop back the value (including padding with 0)
        for (int j = 0; j < HEIGHT; ++j) {
            _grid[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
        }
    }
}

/**
 * Stack the non-empty columns towards the left, leaving empty columns
 * only at the right side of the grid.
 */
void pull_cells_left(Grid& _grid)
{
    int i = 0;
    std::deque<int> zero_col;

    while (i < WIDTH) {
        if (zero_col.empty()) {
            // Look for empty column
            while (i < WIDTH - 1 && _grid[i + (HEIGHT - 1) * WIDTH] != Color::Empty) {
                ++i;
            }
            zero_col.push_back(i);
            ++i;

        } else {
            int x = zero_col.front();
            zero_col.pop_front();
            // Look for non-empty column
            while (i < WIDTH && _grid[i + (HEIGHT - 1) * WIDTH] == Color::Empty) {
                zero_col.push_back(i);
                ++i;
            }
            if (i == WIDTH)
                break;
            // Swap the non-empty column with the first empty one
            for (int j = 0; j < HEIGHT; ++j) {
                std::swap(_grid[x + j * WIDTH], _grid[i + j * WIDTH]);
            }
            zero_col.push_back(i);
            ++i;
        }
    }
}

/**
 * Empty the cells of the cluster to which the given cell belongs, if
 * that cluster is valid.
 *
 * @Return The cluster descriptor of the given cell.
 */
ClusterData kill_cluster(Grid& _grid, const Cell _cell)
{
    const Color color = _grid[_cell];
    ClusterData cd { _cell, color, 0 };

    if (_cell == CELL_NONE || color == Color::Empty) {
        return cd;
    }

    std::deque<Cell> queue { _cell };
    Cell cur = CELL_NONE;

    _grid[_cell] = Color::Empty;
    ++cd.size;

    while (!queue.empty()) {
        cur = queue.back();
        queue.pop_back();
        // We remove the cells adjacent to `cur` with the target color
        // Look right
        if (cur % WIDTH < WIDTH - 1 && _grid[cur + 1] == color) {
            queue.push_back(cur + 1);
            _grid[cur + 1] = Color::Empty;
            ++cd.size;
        }
        // Look down
        if (cur < (HEIGHT - 1) * WIDTH && _grid[cur + WIDTH] == color) {
            queue.push_back(cur + WIDTH);
            _grid[cur + WIDTH] = Color::Empty;
            ++cd.size;
        }
        // Look left
        if (cur % WIDTH > 0 && _grid[cur - 1] == color) {
            queue.push_back(cur - 1);
            _grid[cur - 1] = Color::Empty;
            ++cd.size;
        }
        // Look up
        if (cur > WIDTH - 1 && _grid[cur - WIDTH] == color) {
            queue.push_back(cur - WIDTH);
            _grid[cur - WIDTH] = Color::Empty;
            ++cd.size;
        }
    }
    // If only the rep was killed (cluster of size 1), restore it
    if (cd.size == 1) {
        _grid[_cell] = color;
    }

    return cd;
}

/**
 * Kill a random cluster
 *
 * @Return The cluster that was killed, or some cluster of size 0 or 1.
 */
ClusterData kill_random_cluster(Grid& _grid)
{
    using CellnColor = std::pair<Cell, Color>;
    ClusterData ret {};

    // Random ordering for non-empty rows
    auto rows = rand_util.gen_ordering(_grid.n_empty_rows, HEIGHT);

    // Vector to store the non-empty cells and their color per row
    std::vector<CellnColor> non_empty_cols {};
    std::vector<int> ordering {};
    non_empty_cols.reserve(15);
    ordering.reserve(15);

    for (const auto row : rows) {
        // In case more empty rows were discovered in previous iteration of this for loop
        if (row < _grid.n_empty_rows)
            continue;

        // Collect the non-empty cells in this row along with their colors
        auto row_begin_ndx = row * WIDTH;
        auto row_begin_it = _grid.begin() + row_begin_ndx;
        auto row_end_it = row_begin_it + WIDTH;
        for (auto it = row_begin_it;
             it != row_end_it;
             ++it) {
            if (const Color color = *it; color != Color::Empty) {
                const Cell ndx = row_begin_ndx + std::distance(row_begin_it, it);
                non_empty_cols.push_back(std::make_pair(ndx, color));
            }
        }

        //Adjust the number of empty rows if one was just discovered
        if (non_empty_cols.empty() && row > _grid.n_empty_rows) {
            _grid.n_empty_rows = row;
            continue;
        }

        // Choose a random ordering to go along the above non-empties
        ordering = rand_util.gen_ordering(0, non_empty_cols.size());

        // Traverse the non-empties along that ordering and try to kill the cluster at
        // the corresponding cell.
        for (auto it = begin(ordering);
             it != end(ordering);
             ++it) {
            const auto [_cell, _color] = non_empty_cols[*it];
            ret = kill_cluster(_grid, _cell);//, _color);
            if (ret.size > 1) {
                return ret;
            }
        }

        // Reset the non-empty vector in preparation for the next row
        non_empty_cols.clear();
    }

    return ret;
}


} // namespace

void input(std::istream& _in, StateData& _sd)
{
    int _in_color { 0 };
    Color _color { Color::Empty };
    Grid& _grid = _sd.cells;
    _grid.n_empty_rows = { 0 };

    bool row_empty { true };

    for (auto row = 0; row < HEIGHT; ++row) {
        row_empty = true;

        for (auto col = 0; col < WIDTH; ++col) {
            _in >> _in_color;
            _color = to_enum<Color>(_in_color + 1);
            _grid[col + row * WIDTH] = _color;

            // Generate the helper data at the same time
            if (_color != Color::Empty) {
                row_empty = false;
                //key ^= ZobristKey(col + row * WIDTH, color);
                ++_sd.cnt_colors[to_integral(_color)];
            }
        }
        // Count the number of empty rows (we're going from top to down)
        if (row_empty) {
            ++_grid.n_empty_rows;
        }
    }
    //return key;
}

std::vector<Cluster> get_valid_clusters(const Grid& _grid)
{
    std::vector<Cluster> ret;
    ret.reserve(MAX_CELLS);
    generate_clusters(_grid);
    for (auto it = grid_dsu.cbegin(); it != grid_dsu.cend(); ++it) {
        if (auto ndx = std::distance(grid_dsu.cbegin(), it);
            _grid[ndx] != Color::Empty && it->size() > 1)
        {
            ret.emplace_back(*it);
        }
    }
    return ret;
}


bool same_as_right_nbh(const Grid& _grid, const Cell _cell)
{
    const Color color = _grid[_cell];
    // check right if not already at the right edge of the _grid
    if (_cell % (WIDTH - 1) != 0 && _grid[_cell + 1] == color) {
        return true;
    }
    return false;
}

/**
 * Check if the neighbor at the right or on top of a cell has the same color.
 *
 * NOTE: If called with an empty cell, it will return true if a
 * neighbor is also empty.
 */
bool same_as_right_or_up_nbh(const Grid& _grid, const Cell _cell)
{
    const Color color = _grid[_cell];
    // check right if not already at the right edge of the _grid
    if (_cell % (WIDTH - 1) != 0 && _grid[_cell + 1] == color) {
        return true;
    }
    // check up if not on the first row
    if (_cell > CELL_UPPER_RIGHT && _grid[_cell - WIDTH] == color) {
        return true;
    }
    return false;
}

/**
 * Iterate through the cells like in the generate_clusters() method,
 * but returns false as soon as it identifies a cluster.
 */
bool has_nontrivial_cluster(const Grid& _grid)
{
    auto n_empty_rows = _grid.n_empty_rows;
    bool row_empty = true;

    auto row = HEIGHT - 1;
    // Iterate from bottom row upwards so we can stop at the first empty row.
    while (row > n_empty_rows) {
        // All the row except last cell
        for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell) {
            if (_grid[cell] == Color::Empty) {
                continue;
            }
            row_empty = false;
            // compare up
            if (_grid[cell] == _grid[cell - WIDTH]) {
                return true;
            }
            // compare right
            if (_grid[cell] == _grid[cell + 1]) {
                return true;
            }
        }
        // If the last cell of the row is empty
        if (_grid[(row + 1) * WIDTH - 1] == Color::Empty) {
            if (row_empty) {
                return false;
            }
            continue;
        }
        // If not (compare up)
        if (_grid[(row + 1) * WIDTH - 1] == _grid[row * WIDTH]) {
            return true;
        }
        --row;
        row_empty = true;
    }
    // The upmost non-empty row: only compare right
    for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell) {
        if (_grid[cell] != Color::Empty && _grid[cell + 1] == _grid[cell]) {
            return true;
        }
    }

    return true;
}

/**
 * Builds the cluster to which the given cell belongs
 * to directly from the grid.
 */
Cluster get_cluster(const Grid& _grid, const Cell _cell)
{
    Color color = _grid[_cell];

    if (color == Color::Empty) {
        return Cluster();
        std::cerr << "WARNING! Called get_cluster with an empty color" << std::endl;
    }

    Cluster ret { _cell, };
    ret.members.reserve(MAX_CELLS);
    //std::vector<Cell> ret { _cell };
    //ret.reserve(MAX_CELLS);
    std::deque<Cell> queue { _cell };
    std::set<Cell> seen { _cell };

    Cell cur;

    while (!queue.empty()) {
        cur = queue.back();
        queue.pop_back();
        // remove boundary cells with color equal to current cell's color
        // Look right
        if (cur % WIDTH < WIDTH - 1 && seen.insert(cur+1).second) {//!seen.contains(cur + 1)) {
            //seen.insert(cur + 1);
            if (_grid[cur + 1] == color) {
                ret.push_back(cur + 1);
                queue.push_back(cur + 1);
            }
        }

        // Look down
        if (cur < (HEIGHT - 1) * WIDTH && seen.insert(cur + WIDTH).second) {
            //seen.insert(cur + WIDTH);
            if (_grid[cur + WIDTH] == color) {
                ret.push_back(cur + WIDTH);
                queue.push_back(cur + WIDTH);
            }
        }
        // Look left
        if (cur % WIDTH > 0 && seen.insert(cur - 1).second) {
            //seen.insert(cur - 1);
            if (_grid[cur - 1] == color) {
                ret.push_back(cur - 1);
                queue.push_back(cur - 1);
            }
        }
        // Look up
        if (cur > WIDTH - 1 && seen.insert(cur - WIDTH).second) {
            //seen.insert(cur - WIDTH);
            if (_grid[cur - WIDTH] == color) {
                ret.push_back(cur - WIDTH);
                queue.push_back(cur - WIDTH);
            }
        }
    }

    //auto cl_ret = Cluster(cell, std::move(ret));
    return ret;
}

ClusterData get_cluster_data(const Grid& _grid, const Cell _cell)
{
    const Cluster cluster = get_cluster(_grid, _cell);
    return ClusterData { .rep = cluster.rep, .color = _grid[_cell], .size = cluster.size() };
}


namespace {

   /**
    * @Return The descriptor associated to the given cluster.
    */
    ClusterData get_descriptor(const Grid& _grid, const Cluster& _cluster) {
        ClusterData ret { .rep = _cluster.rep, .color = _grid[_cluster.rep], .size = _cluster.size() };
        return ret;
    }
}  // namespace

std::vector<ClusterData> get_valid_clusters_descriptors(const Grid& _grid)
{
    std::vector<ClusterData> ret {};

    std::vector<Cluster> tmp = get_valid_clusters(_grid);
    ret.reserve(tmp.size());

    std::transform(tmp.begin(), tmp.end(), back_inserter(ret), [&_grid](const auto& cluster) {
        return get_descriptor(_grid, cluster);
    });

    return ret;
}

ClusterData apply_action(Grid& _grid, const Cell _cell)
{
    ClusterData cd_ret = kill_cluster(_grid, _cell);
    if (cd_ret.size > 1) {
        pull_cells_down(_grid);
        pull_cells_left(_grid);
    }
    return cd_ret;
}

ClusterData apply_random_action(Grid& _grid)
{
    ClusterData cd_ret = kill_random_cluster(_grid);
    if (cd_ret.size > 1) {
        pull_cells_down(_grid);
        pull_cells_left(_grid);
    }
    return cd_ret;
}



} // namespace sg::clusters
