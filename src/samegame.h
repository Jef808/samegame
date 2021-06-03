/// samegame.h
///
#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include "types.h"
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
    explicit State(StateData&);
    explicit State(std::istream&, StateData&);
    State(const State&) = delete;
    State& operator=(const State&) = delete;

    StateData clone_data() const;
    StateData& copy_data_to(StateData&) const;
    StateData& move_data_to(StateData&);
    StateData& redirect_data_to(StateData&);
    void copy_data_from(const StateData&);
    const StateData& data() const;

    ClusterDataVec valid_actions_data() const;
    ClusterData apply_action(const ClusterData&, StateData&);
    void apply_action(const ClusterData&);
    ClusterData apply_random_action();
    void undo_action(const ClusterData&);
    bool is_terminal() const;
    bool is_empty() const;

    Key key() const;

    friend std::ostream& operator<<(std::ostream&, const State&);
private:
    StateData* p_data;
};

/** Display a colored board with the chosen cluster highlighted. */
extern std::ostream& operator<<(std::ostream&, const std::pair<const State&, Cell>&);
extern std::ostream& operator<<(std::ostream&, const std::pair<Grid&, int>&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);

extern bool operator==(const StateData& a, const StateData& b);
template < typename _Index_T, _Index_T IndexNone >
extern bool operator==(const ClusterT<_Index_T, IndexNone>& a, const ClusterT<_Index_T, IndexNone>& b);
template < >
inline bool operator==(const ClusterT<Cell, MAX_CELLS>& a, const ClusterT<Cell, MAX_CELLS>& b) { return a == b; }
extern bool operator==(const ClusterData&, const ClusterData&);

} // namespace sg

#endif
