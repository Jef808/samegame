// samegame.cpp
#include "samegame.h"
#include "clusterhelper.h"
#include "display.h"
#include "sghash.h"
#include "types.h"
#include <array>
#include <iostream>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <utility>
#include <vector>


namespace sg {

State::State(StateData& sd)
    : p_data(&sd)
{
}

State::State(std::istream& _in, StateData& sd)
    : p_data(&sd)
{
    clusters::input(_in, sd);
    sd.key = key();
}

State::State(const State& other)
{
    p_data = new StateData(*other.p_data);
}

State& State::operator=(const State& other)
{
    *p_data = *other.p_data;
    return *this;
}

State State::clone() const
{
    State new_state(*this);
    return new_state;
}

void State::reset(const State& other)
{
    *p_data = *other.p_data;
}

StateData State::clone_data() const
{
    StateData sd { *p_data };
    return sd;
}

StateData& State::copy_data_to(StateData& _sd) const
{
    return _sd = *p_data;
}

StateData& State::redirect_data_to(StateData& _sd)
{
    p_data = &_sd;
    return _sd;
}

StateData& State::move_data_to(StateData& _sd)
{
    _sd = std::move(*p_data);
    p_data = &_sd;
    return _sd;
}

void State::copy_data_from(const StateData& _sd)
{
    *p_data = _sd;
}

const StateData& State::data() const
{
    return *p_data;
}
//****************************************** Actions methods ***************************************/

ClusterDataVec State::valid_actions_data() const
{
    return clusters::get_valid_clusters_descriptors(p_data->cells);
}

bool key_uninitialized(const Grid& grid, Key key)
{
   return key == 0 && grid[CELL_BOTTOM_LEFT] != Color::Empty;
}

Key State::key() const
{
    if (p_data->key != 0) {
        return p_data->key;
    }
    return p_data->key = zobrist::get_key(p_data->cells);
}

/**
 * First check if the key contains the answer, check for clusters but return
 * false as soon as it finds one instead of computing all clusters.
 */
bool State::is_terminal() const
{
    Key key = p_data->key;
    // If the first bit is on, then it has been computed and stored in the second bit.
    if (key & 1) {
        return key & 2;
    }
    return !clusters::has_nontrivial_cluster(p_data->cells);
}

bool State::is_empty() const
{
    return p_data->cells[((HEIGHT - 1) * WIDTH)] == Color::Empty;
}

bool State::is_trivial(const ClusterData& cd) const {
    return cd.rep == CELL_NONE || cd.color == Color::Empty || cd.size < 2;
}

//******************************** Apply / Undo actions **************************************/

/**
 * Apply an action to a state in a persistent way: the new data is
 * recorded on the provided StateData object.
 */
ClusterData State::apply_action(const ClusterData& cd, StateData& sd)
{
    if (p_data->cells[cd.rep] != cd.color) {
        spdlog::warn("State::apply_action called with ClusterData of wrong color");
        return ClusterData{ .rep = CELL_NONE, .color = Color::Empty, .size = 0 };
    }
    copy_data_to(sd);
    sd.previous = p_data;
    p_data = &sd;

    ClusterData cd_ret = clusters::apply_action(sd.cells, cd.rep);
    sd.cnt_colors[to_integral(cd_ret.color)] -= (cd_ret.size > 1) * cd_ret.size;

    return cd_ret;
}

bool State::apply_action(const ClusterData& cd)
{
    ClusterData res = clusters::apply_action(p_data->cells, cd.rep);
    p_data->cnt_colors[to_integral(res.color)] -= (res.size > 1) * res.size;
    p_data->key = 0;
    return !is_trivial(res);
}

ClusterData State::apply_random_action()
{
    ClusterData cd = clusters::apply_random_action(p_data->cells);
    p_data->cnt_colors[to_integral(cd.color)] -= cd.size;
    return cd;
}

void State::undo_action(const ClusterData& cd)
{
    p_data = p_data->previous;
    p_data->cnt_colors[to_integral(cd.color)] += cd.size;
}
//*************************** Display *************************/

ClusterData State::get_cd(Cell rep) const {
    return sg::clusters::get_cluster_data(p_data->cells, rep);
}

void State::display(Cell rep) const {
    std::cout << display::to_string(p_data->cells, rep) << std::endl;
}

void State::show_clusters() const {
    display::view_clusters(std::cout, p_data->cells);
}

std::ostream& operator<<(std::ostream& _out, const std::pair<Grid&, Cell>& _ga)
{
    return _out << display::to_string(_ga.first, _ga.second);
}

std::ostream& operator<<(std::ostream& _out, const std::pair<const State&, Cell>& _sc)
{
    return _out << display::to_string(_sc.first.data().cells, _sc.second);
}

std::ostream& operator<<(std::ostream& _out, const State& _state)
{
    return _out << display::to_string(_state.p_data->cells, CELL_NONE);
}

std::ostream& operator<<(std::ostream& _out, const StateData& _sd)
{
    return _out << display::to_string(_sd.cells, CELL_NONE) << "\n    Key = " << _sd.key;
}

std::ostream& operator<<(std::ostream& _out, const ClusterData& _cd)
{
    return _out << _cd.rep << ' ' << display::to_string(_cd.color) << ' ' << std::to_string(_cd.size);
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
