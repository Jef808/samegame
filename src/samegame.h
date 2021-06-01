/// samegame.h
///
#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include "types.h"
#include <deque>
#include <iosfwd>
#include <memory>

namespace sg {

class State {
public:
    static void init();
    explicit State(StateData&);
    explicit State(std::istream&, StateData&);
    State(const State&) = delete;
    State& operator=(const State&) = delete;

    // Actions
    ClusterDataVec valid_actions_data() const;
    ClusterData apply_action(const ClusterData&, StateData&) const;
    //ClusterData apply_action_blind(const Cell);
    //bool apply_action_blind(const ClusterData&);
    ClusterData apply_random_action();
    void undo_action(const ClusterData&);
    bool is_terminal() const;
    bool is_empty() const;
    // Accessors
    Key key() const;
    Color get_color(const Cell) const;
    void set_color(const Cell, const Color);
    int n_empty_rows() const;
    int set_n_empty_rows(int);
    //const Grid& cells() const { return p_data->cells; }
    Cluster get_cluster_blind(const Cell) const;
    void move_data(StateData* sd);

    // Game data
    StateData* p_data;
    ColorsCounter colors;

    std::string to_string(Cell rep = CELL_NONE, Output = Output::CONSOLE) const;
    void enumerate_clusters(std::ostream&) const;
    void view_clusters(std::ostream&) const;

    // Debugn
    friend std::ostream& operator<<(std::ostream&, const State&);
};

// For debugging
// TODO: Put all the 'display' stuff in its own compilation unit.
 template <typename _Cluster>
struct State_Action {
    const State& r_state;
    _Cluster m_cluster;
    State_Action(const State&, const _Cluster&);
    State_Action(const State&, const Cell);
    template <typename __Cluster>
    friend std::ostream& operator<<(std::ostream&, const State_Action<__Cluster>&);
};

// template < typename Index, Index DefaultValue >
// extern std::ostream& operator<<(std::ostream&, const ClusterT<Index, DefaultValue>&);
// template < >
// inline std::ostream& operator<<(std::ostream& _out, const ClusterT<int, MAX_CELLS>& cluster) {
//     return _out << cluster;
// }
// template <Cell DefaultValue>
// inline std::ostream& operator<<(std::ostream& _out, const ClusterT<Cell, DefaultValue>& cluster)
// {
//     _out << "Rep= " << cluster.rep << " { ";
//     for (auto it = cluster.members.cbegin();
//          it != cluster.members.cend();
//          ++it) {
//         _out << *it << ' ';
//     }
//     return _out << " }";
// }

// This is defined as inline in "dsu.h". This way, if we have a specified ClusterT in a source file that
// includes both "samegame.h" and "dsu.h", if _out << ClusterT is called for some ostream _out, the compiler
// will generate code (inline) following the instructions in "dsu.h"
//
// PROBLEM: There has to be an issue with the fact that it will use an inline function while this is declared
// to have external linkage.... It seems like two uncompatible ways to solve the same problem
template < typename Index, Index DefaultValue >
extern std::ostream& operator<<(std::ostream&, const ClusterT<Index, DefaultValue>&);

// template < >
// std::ostream& operator<< (std::ostream& _out, const ClusterT<Cell, CELL_NONE>& cluster) {
//   _out << "Rep= " << cluster.rep << " { ";
//     for (auto it = cluster.members.cbegin();
//          it != cluster.members.cend();
//          ++it)
//     {
//         _out << *it << ' ';
//     }
//     return _out << " }";
// }
// template <>
// inline std::ostream& operator<< (std::ostream& _out, const ClusterT<int, CELL_NONE>& cluster) {
//     return _out << cluster;
// }

extern std::ostream& operator<<(std::ostream&, const ClusterData&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern bool operator==(const StateData& a, const StateData& b);
// NOTE: This operator== is defined in samegame.cpp
// NOTE: Should maybe define this inline in dsu.h
template < typename _Index_T, _Index_T DefaultValue >
extern bool operator==(const ClusterT<_Index_T, DefaultValue>& a, const ClusterT<_Index_T, DefaultValue>& b);
// NOTE: The compiler doesn't recognize operator== as being defined if we don't do the following:
// (We can't define a template function specialisation as extern)
// Maybe declaring operator==(const sg::Cluster& a, const sg::Cluster& b) as extern here and defining in
// the cpp file would work, but really, since Cluster<Cell, MAX_CELLS> is a type only relevant if we're
// including "samegame.h", this makes sense
template < >
inline bool operator==(const ClusterT<int, MAX_CELLS>& a, const ClusterT<int, MAX_CELLS>& b) { return a == b; }
extern bool operator==(const ClusterData&, const ClusterData&);

} // namespace sg

#endif
