// samegame.cpp
#include "samegame.h"
//#include "types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "sghash.h"
#include "clusterhelper.h"
#include "display.h"

namespace sg {

State::State()
  : m_key(0), m_cells{}, m_cnt_colors{0}
{
}

State::State(Grid&& grid, ColorCounter&& ccolors)
  : m_key(0), m_cells(grid), m_cnt_colors(ccolors) { }

State::State(std::istream& _in) : m_key(), m_cells{}, m_cnt_colors{}
{
  clusters::input(_in, m_cells, m_cnt_colors);
}

State::State(key_type key, const Grid& cells, const ColorCounter& ccolors)
  : m_key(key), m_cells{cells}, m_cnt_colors{ccolors}
{
}

//****************************************** Actions methods ***************************************/

ClusterDataVec State::valid_actions_data() const
{
  return clusters::get_valid_clusters_descriptors(m_cells);
}

bool key_uninitialized(const Grid& grid, Key key)
{
  return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
}

Key State::key()
{
  if (key_uninitialized(m_cells, m_key))
    return m_key = zobrist::get_key(m_cells);
  return m_key;
}

/**
 * First check if the key contains the answer, check for clusters but return
 * false as soon as it finds one instead of computing all clusters.
 */
bool State::is_terminal() const
{
  // If the first bit is on, then it has been computed and stored in the second bit.
  if (m_key & 1)
  {
    return m_key & 2;
  }
  return !clusters::has_nontrivial_cluster(m_cells);
}

//******************************** Apply / Undo actions **************************************/

bool State::apply_action(const ClusterData& cd)
{
  ClusterData res = clusters::apply_action(m_cells, cd.rep);
  m_cnt_colors[to_integral(res.color)] -= (res.size > 1) * res.size;
  m_key = 0;
  return !is_trivial(res);
}

ClusterData State::apply_random_action()
{
  ClusterData cd = clusters::apply_random_action(m_cells);
  m_cnt_colors[to_integral(cd.color)] -= cd.size;
  return cd;
}

//*************************** Display *************************/

ClusterData State::get_cd(Cell rep) const
{
  return sg::clusters::get_cluster_data(m_cells, rep);
}

void State::display(Cell rep) const
{
  std::cout << display::to_string(m_cells, rep) << std::endl;
}

void State::show_clusters() const
{
  display::view_clusters(std::cout, m_cells);
}

void State::view_action_sequence(const std::vector<ClusterData>& actions,
                                 int delay_in_ms) const
{
  Grid grid_copy = m_cells;
  display::view_action_sequence(std::cout, grid_copy, actions, delay_in_ms);
}

void State::log_action_sequence(std::ostream& out,
                                const std::vector<ClusterData>& actions) const
{
  Grid grid_copy = m_cells;
  display::log_action_sequence(out, grid_copy, actions);
}

std::ostream& operator<<(std::ostream& _out, const std::pair<Grid&, Cell>& _ga)
{
  return _out << display::to_string(_ga.first, _ga.second);
}

std::ostream& operator<<(std::ostream& _out,
                         const std::pair<const State&, Cell>& _sc)
{
  return _out << display::to_string(_sc.first.grid(), _sc.second);
}

std::ostream& operator<<(std::ostream& _out, const State& _state)
{
  return _out << display::to_string(_state.m_cells, CELL_NONE);
}

std::ostream& operator<<(std::ostream& _out, const ClusterData& _cd)
{
  return _out << _cd.rep << ' ' << display::to_string(_cd.color) << ' '
              << std::to_string(_cd.size);
}

//*************************** Evaluation *************************/

/**
 * @Note clamp(v, l, h) assigns l to v if v < l or assigns h to v if h < v else returns v.
 * In this case, (2-size)^2 is always greater than (2-size) so we're just returning
 * [max(0, size-2)]^2 in a more efficient way.
 */
State::reward_type State::evaluate(const ClusterData& action) const
{
  double val = action.size - 2.0;
  val = (val + std::abs(val)) / 2.0;
  return std::pow(val, 2) * 0.0025;
}

State::reward_type State::evaluate_terminal() const
{
  return static_cast<reward_type>(is_empty()) * 1000.0 * 0.0025;
}


} //namespace sg
