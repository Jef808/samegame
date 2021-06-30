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
    using reward_type = double;
    using key_type = uint64_t;

    explicit State(StateData&);
    explicit State(std::istream&, StateData&);
    State(State&& other)
        { p_data = other.p_data; }
    ~State() = default;

    State clone() const;
    void reset(const State&);
    StateData clone_data() const;
    StateData& copy_data_to(StateData&) const;
    StateData& move_data_to(StateData&);
    StateData& redirect_data_to(StateData&);
    void copy_data_from(const StateData&);
    StateData* pget_data() const;
    const StateData& data() const;

    ClusterDataVec valid_actions_data() const;
    ClusterData apply_action(const ClusterData&, StateData&);
    bool apply_action(const ClusterData&);
    ClusterData apply_random_action();
    void undo_action(const ClusterData&);
    bool is_terminal() const;
    bool is_empty() const;
    bool is_trivial(const ClusterData&) const;

    ClusterData get_cd(Cell rep) const;
    void display(Cell rep) const;
    void show_clusters() const;
    template < typename ActionT >
    reward_type evaluate(const ActionT& action) const
        { return std::max(0LU, (action.size - 2) * (action.size - 2)); }
    reward_type evaluate_terminal() const
        { return is_empty() * 1000; }

    Key key() const;

    friend std::ostream& operator<<(std::ostream&, const State&);
private:
    State(const State&);
    State& operator=(const State&);

    StateData* p_data;
};

/** Display a colored board with the chosen cluster highlighted. */
extern std::ostream& operator<<(std::ostream&, const std::pair<const State&, Cell>&);
extern std::ostream& operator<<(std::ostream&, const std::pair<Grid&, int>&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const StateData&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);

extern bool operator==(const StateData& a, const StateData& b);
template < typename _Index_T, _Index_T IndexNone >
extern bool operator==(const ClusterT<_Index_T, IndexNone>& a, const ClusterT<_Index_T, IndexNone>& b);
template < >
inline bool operator==(const ClusterT<Cell, MAX_CELLS>& a, const ClusterT<Cell, MAX_CELLS>& b) { return a == b; }

} // namespace sg

#endif
