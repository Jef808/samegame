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
DSU<sg::Cluster, sg::MAX_CELLS> grid_dsu{};

/**
 * Utility class initializing a random number generator and implementing
 * the methods we need for the random actions.
 */
Rand::Util<Cell> rand_util{};

//************************************** Grid manipulations **********************************/

///TODO Remove the 'empty cel ls' stuff, or do something with it
/**
 * Populate the disjoint data structure grid_dsu with all adjacent clusters
 * of cells sharing a same color.
 */
void generate_clusters(const Grid& _grid)
{
  grid_dsu.reset();

  auto n_empty_rows = _grid.n_empty_rows;
  bool row_empty = true;
  bool found_empty_cell = false;
  Cell empty_cell = CELL_NONE;

  auto row = HEIGHT - 1;
  // Iterate from bottom row upwards so we can stop at the first empty row.
  while (row > n_empty_rows)
  {
    row_empty = true;
    // Iterate through all columns except last
    for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell)
    {
      if (_grid[cell] == Color::Empty)
        continue;

      // compare up
      if (_grid[cell] == _grid[cell - WIDTH])
        grid_dsu.unite(cell, cell - WIDTH);

      // compare right
      if (_grid[cell] == _grid[cell + 1])
        grid_dsu.unite(cell, cell + 1);
    }
    Cell _cell = (row + 1) * WIDTH - 1;
    // Last column
    // If the row was empty, we are done.
    if (row_empty = row_empty && _grid[_cell] == Color::Empty)
    {
      _grid.n_empty_rows = row;
      return;
    }
    // Compare up
    if (_grid[_cell] == _grid[_cell - WIDTH])
    {
      grid_dsu.unite(_cell, _cell - WIDTH);
    }
    --row;
  }

  row_empty = true;
  // The upmost non-empty row: only compare right
  for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell)
  {
    if (_grid[cell] == Color::Empty)
      continue;

    if (_grid[cell] == _grid[cell + 1])
      grid_dsu.unite(cell, cell + 1);
  }
  if (row_empty = row_empty && _grid[CELL_UPPER_RIGHT] == Color::Empty)
    ++_grid.n_empty_rows;
}

/**
 * Make cells drop down if they lie above empty cells.
 */
void pull_cells_down(Grid& _grid)
{
  // For all columns
  for (int i = 0; i < WIDTH; ++i)
  {
    // stack the non-zero colors going up
    std::array<Color, HEIGHT> new_column{Color::Empty};
    int new_height = 0;

    for (int j = 0; j < HEIGHT; ++j)
    {
      auto bottom_color = _grid[i + (HEIGHT - 1 - j) * WIDTH];
      if (bottom_color != Color::Empty)
      {
        new_column[new_height] = bottom_color;
        ++new_height;
      }
    }
    // pop back the value (including padding with 0)
    for (int j = 0; j < HEIGHT; ++j)
      _grid[i + j * WIDTH] = new_column[HEIGHT - 1 - j];
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

  while (i < WIDTH)
  {
    if (zero_col.empty())
    {
      // Look for empty column
      while (i < WIDTH - 1 && _grid[i + (HEIGHT - 1) * WIDTH] != Color::Empty)
        ++i;
      zero_col.push_back(i);
      ++i;
    }
    else
    {
      int x = zero_col.front();
      zero_col.pop_front();
      // Look for non-empty column
      while (i < WIDTH && _grid[i + (HEIGHT - 1) * WIDTH] == Color::Empty)
      {
        zero_col.push_back(i);
        ++i;
      }
      if (i == WIDTH)
        break;
      // Swap the non-empty column with the first empty one
      for (int j = 0; j < HEIGHT; ++j)
        std::swap(_grid[x + j * WIDTH], _grid[i + j * WIDTH]);
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
  ClusterData cd{_cell, color, 0};

  if (_cell == CELL_NONE || color == Color::Empty)
    return cd;

  std::array<Cell, sg::MAX_CELLS> queue;
  queue[0] = _cell;
  int ndx_back = 1;
  Cell cur = CELL_NONE;

  _grid[_cell] = Color::Empty;
  ++cd.size;

  while (!ndx_back == 0)
  {
    cur = queue[--ndx_back];

    // We remove the cells adjacent to `cur` with the target color
    // Look right
    if (cur % WIDTH < WIDTH - 1 && _grid[cur + 1] == color)
    {
      queue[ndx_back] = cur + 1;
      ++ndx_back;
      _grid[cur + 1] = Color::Empty;
      ++cd.size;
    }
    // Look down
    if (cur < (HEIGHT - 1) * WIDTH && _grid[cur + WIDTH] == color)
    {
      queue[ndx_back] = cur + WIDTH;
      ++ndx_back;
      _grid[cur + WIDTH] = Color::Empty;
      ++cd.size;
    }
    // Look left
    if (cur % WIDTH > 0 && _grid[cur - 1] == color)
    {
      queue[ndx_back] = cur-1;
      ++ndx_back;
      _grid[cur - 1] = Color::Empty;
      ++cd.size;
    }
    // Look up
    if (cur > WIDTH - 1 && _grid[cur - WIDTH] == color)
    {
      queue[ndx_back] = cur - WIDTH;
      ++ndx_back;
      _grid[cur - WIDTH] = Color::Empty;
      ++cd.size;
    }
  }
  // If only the rep was killed (cluster of size 1), restore it
  if (cd.size == 1)
    _grid[_cell] = color;

  return cd;
}

/// NOTE This is by far the hot spot in execution! (92% is spent here
/**
 * Kill a random cluster
 *
 * @Return The cluster that was killed, or some cluster of size 0 or 1.
 */
ClusterData kill_random_cluster(Grid& _grid, const Color target_color = Color::Empty)
{
  using CellnColor = std::pair<Cell, Color>;
  ClusterData ret{};

  // Random numbers from n_empty_rows to HEIGHT at the beginning of the array
  std::array<int, HEIGHT> rows = rand_util.gen_ordering<HEIGHT>(_grid.n_empty_rows, HEIGHT);

  // Array to hold the non-empty cells found.
  std::array<Cell, WIDTH> non_empty{};

  // Because n_empty_rows might get updated during the loop!
  const int len_rows = HEIGHT - _grid.n_empty_rows;
  Cell _cell{};
  Color _color{};
  int nonempty_ndx;
  int target_ndx;

  for (auto row_it = rows.begin(); row_it != rows.begin() + len_rows; ++row_it)
  {
    if (*row_it < _grid.n_empty_rows)
      continue;

    // Keep track of which cells are non-empty in that row.
    nonempty_ndx = -1;
    target_ndx = -1;
    for (Cell c = *row_it * WIDTH; c < (*row_it+1) * WIDTH -1; ++c)
    {
      if (_grid[c] != Color::Empty)
        non_empty[++nonempty_ndx] = c;
    }

    // If we just found a new empty row, update n_empty_rows.
    if (nonempty_ndx == -1)
    {
      _grid.n_empty_rows = *row_it;
      continue;
    }

    // Otherwise shuffle the non-empty cells and try to kill a cluster there
    // Aim for the target color first.
    rand_util.shuffle<WIDTH>(non_empty, nonempty_ndx);

    for (auto it = non_empty.begin(); it != non_empty.begin() + nonempty_ndx; ++it)
    {
      if (_grid[*it] == target_color)
        ret = kill_cluster(_grid, *it);
      if (ret.size > 1)
        return ret;
    }
    for (auto it = non_empty.begin(); it != non_empty.begin() + nonempty_ndx; ++it)
    {
      if (_grid[*it] != target_color)
        ret = kill_cluster(_grid, *it);
      if (ret.size > 1)
        return ret;
    }
  }

  return ret;
}

} // namespace

void input(std::istream& _in, Grid& _grid, ColorCounter& _cnt_colors)
{
  _grid.n_empty_rows = {0};
  int _in_color{0};
  Color _color{Color::Empty};
  bool row_empty{true};

  for (auto row = 0; row < HEIGHT; ++row)
  {
    row_empty = true;

    for (auto col = 0; col < WIDTH; ++col)
    {
      _in >> _in_color;
      _color = to_enum<Color>(_in_color + 1);
      _grid[col + row * WIDTH] = _color;

      // Generate the color data at the same time
      if (_color != Color::Empty)
      {
        row_empty = false;
        _cnt_colors[to_integral(_color)];
      }
    }
    // Count the number of empty rows (we're going from top to down)
    if (row_empty)
      ++_grid.n_empty_rows;
  }
}

std::vector<Cluster> get_valid_clusters(const Grid& _grid)
{
  std::vector<Cluster> ret;
  ret.reserve(MAX_CELLS);
  generate_clusters(_grid);
  for (auto it = grid_dsu.cbegin(); it != grid_dsu.cend(); ++it)
  {
    if (auto ndx = std::distance(grid_dsu.cbegin(), it);
        _grid[ndx] != Color::Empty && it->size() > 1)
      ret.emplace_back(*it);
  }
  return ret;
}

bool same_as_right_nbh(const Grid& _grid, const Cell _cell)
{
  const Color color = _grid[_cell];
  // check right if not already at the right edge of the _grid
  if (_cell % (WIDTH - 1) != 0 && _grid[_cell + 1] == color)
    return true;
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
  if (_cell % (WIDTH - 1) != 0 && _grid[_cell + 1] == color)
    return true;
  // check up if not on the first row
  if (_cell > CELL_UPPER_RIGHT && _grid[_cell - WIDTH] == color)
    return true;
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
  while (row > n_empty_rows)
  {
    // All the row except last cell
    for (auto cell = row * WIDTH; cell < (row + 1) * WIDTH - 1; ++cell)
    {
      if (_grid[cell] == Color::Empty)
        continue;
      row_empty = false;
      // compare up
      if (_grid[cell] == _grid[cell - WIDTH])
        return true;
      // compare right
      if (_grid[cell] == _grid[cell + 1])
        return true;
    }
    // If the last cell of the row is empty
    if (_grid[(row + 1) * WIDTH - 1] == Color::Empty)
    {
      if (row_empty)
        return false;
      continue;
    }
    // If not (compare up)
    if (_grid[(row + 1) * WIDTH - 1] == _grid[row * WIDTH])
      return true;
    --row;
    row_empty = true;
  }
  // The upmost non-empty row: only compare right
  for (auto cell = 0; cell < CELL_UPPER_RIGHT - 1; ++cell)
  {
    if (_grid[cell] != Color::Empty && _grid[cell + 1] == _grid[cell])
      return true;
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

  if (color == Color::Empty)
  {
    return Cluster();
    std::cerr << "WARNING! Called get_cluster with an empty color" << std::endl;
  }

  Cluster ret{
      _cell,
  };
  ret.members.reserve(MAX_CELLS);
  std::deque<Cell> queue{_cell};
  std::set<Cell> seen{_cell};

  Cell cur;

  while (!queue.empty())
  {
    cur = queue.back();
    queue.pop_back();
    // Look right
    if (cur % WIDTH < WIDTH - 1 && seen.insert(cur + 1).second)
    {
      if (_grid[cur + 1] == color)
      {
        ret.push_back(cur + 1);
        queue.push_back(cur + 1);
      }
    }
    // Look down
    if (cur < (HEIGHT - 1) * WIDTH && seen.insert(cur + WIDTH).second)
    {
      if (_grid[cur + WIDTH] == color)
      {
        ret.push_back(cur + WIDTH);
        queue.push_back(cur + WIDTH);
      }
    }
    // Look left
    if (cur % WIDTH > 0 && seen.insert(cur - 1).second)
    {
      if (_grid[cur - 1] == color)
      {
        ret.push_back(cur - 1);
        queue.push_back(cur - 1);
      }
    }
    // Look up
    if (cur > WIDTH - 1 && seen.insert(cur - WIDTH).second)
    {
      if (_grid[cur - WIDTH] == color)
      {
        ret.push_back(cur - WIDTH);
        queue.push_back(cur - WIDTH);
      }
    }
  }

  return ret;
}

ClusterData get_cluster_data(const Grid& _grid, const Cell _cell)
{
  const Cluster cluster = get_cluster(_grid, _cell);
  return ClusterData{
      .rep = cluster.rep, .color = _grid[_cell], .size = cluster.size()};
}

namespace {

/**
    * @Return The descriptor associated to the given cluster.
    */
ClusterData get_descriptor(const Grid& _grid, const Cluster& _cluster)
{
  ClusterData ret{.rep = _cluster.rep,
                  .color = _grid[_cluster.rep],
                  .size = _cluster.size()};
  return ret;
}
} // namespace

std::vector<ClusterData> get_valid_clusters_descriptors(const Grid& _grid)
{
  std::vector<ClusterData> ret{};

  std::vector<Cluster> tmp = get_valid_clusters(_grid);
  ret.reserve(tmp.size());

  std::transform(
      tmp.begin(),
      tmp.end(),
      back_inserter(ret),
      [&_grid](const auto& cluster) { return get_descriptor(_grid, cluster); });

  return ret;
}

ClusterData apply_action(Grid& _grid, const Cell _cell)
{
  ClusterData cd_ret = kill_cluster(_grid, _cell);
  if (cd_ret.size > 1)
  {
    pull_cells_down(_grid);
    pull_cells_left(_grid);
  }
  return cd_ret;
}

ClusterData apply_random_action(Grid& _grid, const Color target_color)
{
  ClusterData cd_ret = kill_random_cluster(_grid, target_color);
  if (cd_ret.size > 1)
  {
    pull_cells_down(_grid);
    pull_cells_left(_grid);
  }
  return cd_ret;
}

} // namespace sg::clusters
