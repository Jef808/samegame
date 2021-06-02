// samegame.cpp
#include "samegame.h"
#include "clusterhelper.h"
#include "display.h"
#include "sghash.h"
#include "types.h"
#include <array>
#include <iostream>
#include <string>
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
    clusters::input(_in, sd.cells, sd.cnt_colors);
}

void State::move_data(StateData* _sd)
{
    *_sd = *p_data;
    p_data = _sd;
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
    if (key_uninitialized(p_data->cells, p_data->key)) {
        p_data->key = zobrist::get_key(p_data->cells);
    }
    return p_data->key;
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

//******************************** Apply / Undo actions **************************************/

/**
 * Apply an action to a state in a persistent way: the new data is
 * recorded on the provided StateData object.
 */
ClusterData State::apply_action(const ClusterData& cd, StateData& sd) const
{
    sd.cells = p_data->cells;
    sd.previous = p_data;

    ClusterData cd_ret = clusters::apply_action(sd.cells, cd.rep);
    sd.cnt_colors[cd_ret.color] -= cd_ret.size;

    return cd_ret;
}

ClusterData State::apply_random_action()
{
    ClusterData cd = clusters::apply_random_action(p_data->cells);
    p_data->cnt_colors[cd.color] -= cd.size;
    return cd;
}

void State::undo_action(const ClusterData& cd)
{
    p_data = p_data->previous;
    p_data->cnt_colors[cd.color] += cd.size;
}
//*************************** Display *************************/

std::ostream& operator<<(std::ostream& _out, const std::pair<const Grid&, const Cell> _ga)
{
    return _out << display::to_string(_ga.first, _ga.second);
}

std::ostream& operator<<(std::ostream& _out, const State& state)
{
    return _out << display::to_string(state.p_data->cells, cell, Output::CONSOLE);
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

bool operator==(const ClusterData& a, const ClusterData& b)
{
    return a.rep == b.rep && a.color == b.color && a.size == b.size;
}

} //namespace sg
