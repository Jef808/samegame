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
    //void move_data(StateData* sd);

    ClusterDataVec valid_actions_data() const;
    ClusterData apply_action(const ClusterData&, StateData&) const;
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
extern std::ostream& operator<<(std::ostream&, const std::pair<Grid&, const Cell>);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);

extern bool operator==(const StateData& a, const StateData& b);
template < typename _Index_T, _Index_T DefaultValue >
extern bool operator==(const ClusterT<_Index_T, DefaultValue>& a, const ClusterT<_Index_T, DefaultValue>& b);

template < >
inline bool operator==(const ClusterT<Cell, MAX_CELLS>& a, const ClusterT<Cell, MAX_CELLS>& b) { return a == b; }
extern bool operator==(const ClusterData&, const ClusterData&);

// NOTE: The compiler doesn't recognize operator== as being defined if we don't do the following:
// (We can't define a template function specialisation as extern)
// Maybe declaring operator==(const sg::Cluster& a, const sg::Cluster& b) as extern here and defining in
// the cpp file would work, but really, since Cluster<Cell, MAX_CELLS> is a type only relevant if we're
// including "samegame.h", this makes sense

} // namespace sg

#endif
