// samegame.cpp
#include "samegame.h"
#include "types.h"

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

State::State(std::istream& _in) :
    m_key(), m_cells{}, m_cnt_colors{}
{
    clusters::input(_in, m_cells, m_cnt_colors);
}

State::State(key_type key, const Grid& cells, const ColorCounter& ccolors) :
    m_key(key), m_cells{cells}, m_cnt_colors{ccolors}
{
}

// State::State(StateData& sd)
//     : p_data(&sd)
// {
// }

// State::State(std::istream& _in, StateData& sd)
//     : p_data(&sd)
// {
//     clusters::input(_in, sd);
//     sd.key = key();
// }

// State::State(const State& other)
// {
//     StateData* new_data = new StateData;
//     *p_data = *other.p_data;
// }

// State::~State()
// {
//     delete p_data;
//     p_data = nullptr;
// }

// State& State::operator=(const State& other)
// {
//     if (this == &other)
//         return *this;
//     delete p_data;
//     *p_data = *other.p_data;
//     return *this;
// }

// State State::clone() const
// {
//     State new_state(*this);
//     return new_state;
// }

// State& State::reset(const State& other)
// {
//     return *this = other;
// }

// StateData State::clone_data() const
// {
//     StateData sd { *p_data };
//     return sd;
// }

// StateData& State::copy_data_to(StateData& _sd) const
// {
//     return _sd = *p_data;
// }

// StateData& State::redirect_data_to(StateData& _sd)
// {
//     if (p_data == &_sd)
//         return _sd;
//     delete p_data;
//     p_data = &_sd;
//     return _sd;
// }

// StateData& State::move_data_to(StateData& _sd)
// {
//     _sd = std::move(*p_data);
//     p_data = &_sd;
//     return _sd;
// }

// void State::copy_data_from(const StateData& _sd)
// {
//     *p_data = _sd;
// }

// const StateData& State::data() const
// {
//     return *p_data;
// }
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
    if (m_key & 1) {
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

ClusterData State::get_cd(Cell rep) const {
    return sg::clusters::get_cluster_data(m_cells, rep);
}

void State::display(Cell rep) const {
    std::cout << display::to_string(m_cells, rep) << std::endl;
}

void State::show_clusters() const {
    display::view_clusters(std::cout, m_cells);
}

void State::view_action_sequence(const std::vector<ClusterData>& actions, int delay_in_ms) const {
    Grid grid_copy = m_cells;
    display::view_action_sequence(std::cout, grid_copy, actions, delay_in_ms);
}

void State::log_action_sequence(std::ostream& out, const std::vector<ClusterData>& actions) const {
    Grid grid_copy = m_cells;
    display::log_action_sequence(out, grid_copy, actions);
}

std::ostream& operator<<(std::ostream& _out, const std::pair<Grid&, Cell>& _ga)
{
    return _out << display::to_string(_ga.first, _ga.second);
}

std::ostream& operator<<(std::ostream& _out, const std::pair<const State&, Cell>& _sc)
{
    return _out << display::to_string(_sc.first.grid(), _sc.second);
}

std::ostream& operator<<(std::ostream& _out, const State& _state)
{
    return _out << display::to_string(_state.m_cells, CELL_NONE);
}

std::ostream& operator<<(std::ostream& _out, const StateData& _sd)
{
    return _out << display::to_string(_sd.cells, CELL_NONE) << "\n    Key = " << _sd.key;
}

std::ostream& operator<<(std::ostream& _out, const ClusterData& _cd)
{
    return _out << _cd.rep << ' ' << display::to_string(_cd.color) << ' ' << std::to_string(_cd.size);
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
    
    //return std::max(0.0, (action.size - 2.0)) * std::max(0.0, (action.size - 2.0)) / 3e02 ;
}
State::reward_type State::evaluate_terminal() const
{
    return static_cast<reward_type>(is_empty()) * 1000.0 * 0.0025;
}

//**************************** Comparison ***********************/

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

} //namespace sg
