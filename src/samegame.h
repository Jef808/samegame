/// samegame.h
///
#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include "types.h"
#include <algorithm>
#include <deque>
#include <iosfwd>
#include <memory>


namespace sg {

/**
 * @Class An interface to the game exposing it only as a general State/Action system.
 *
 * @Note `StateData` and `ClusterData` are compact descriptors for the Grid and the
 * Clusters respectively.
 */
class State {
public:
    using reward_type = double;
    using key_type = uint64_t;
    using pointer_to_data = std::shared_ptr<StateData>;

    // explicit State(StateData&);
    // explicit State(std::istream&, StateData&);
    explicit State(std::istream&);
    State(key_type, const Grid&, const ColorCounter&);
    ///TODO Get rid of this!
    // State(const State&);
    // State& operator=(const State&);
    //~State();
    // State clone() const;
    //State& reset(const State&);
    // StateData clone_data() const;
    // StateData& copy_data_to(StateData&) const;
    // StateData& move_data_to(StateData&);
    // StateData& redirect_data_to(StateData&);
    // void copy_data_from(const StateData&);
    // StateData* pget_data() const;
    // const StateData& data() const;

    ClusterDataVec valid_actions_data() const;
    //ClusterData apply_action(const ClusterData&, StateData&);
    bool apply_action(const ClusterData&);
    ClusterData apply_random_action();
    //void undo_action(const ClusterData&);
    reward_type evaluate(const ClusterData&) const;
    reward_type evaluate_terminal() const;
    bool is_terminal() const;
    Key key();
    bool is_trivial(const ClusterData& cd) const
        { return cd.size < 2; }
    /// TODO inline at namespace scope
    bool is_empty() const
        { return m_cells[CELL_BOTTOM_LEFT] == Color::Empty; }

    const Grid& grid() const
        { return m_cells; }

    const ColorCounter& color_counter() const
        { return m_cnt_colors; }

    friend std::ostream& operator<<(std::ostream&, const State&);

    ///TODO: Get rid of this! (Move to namespace scope)
    ClusterData get_cd(Cell rep) const;
    void display(Cell rep) const;
    void show_clusters() const;
    void view_action_sequence(const std::vector<ClusterData>&, int=0) const;
    void log_action_sequence(std::ostream&, const std::vector<ClusterData>&) const;

    bool operator==(const State& other) const
        { return m_cells == other.m_cells; }

    ///TODO: Get rid of this!
    //StateData* p_data;

    key_type m_key;
    Grid m_cells;
    ColorCounter m_cnt_colors;
};

/** Display a colored board with the chosen cluster highlighted. */
extern std::ostream& operator<<(std::ostream&, const std::pair<const State&, Cell>&);
extern std::ostream& operator<<(std::ostream&, const std::pair<Grid&, int>&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const StateData&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);

// inline bool operator==(const State& a, const State& b)
//     { return a.operator==(b); }
//extern bool operator==(const StateData& a, const StateData& b);
template < typename _Index_T, _Index_T IndexNone >
extern bool operator==(const ClusterT<_Index_T, IndexNone>& a, const ClusterT<_Index_T, IndexNone>& b);
template < >
inline bool operator==(const ClusterT<Cell, MAX_CELLS>& a, const ClusterT<Cell, MAX_CELLS>& b) { return a == b; }

} // namespace sg

#endif
