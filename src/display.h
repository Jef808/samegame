#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <iostream>
#include <map>
#include <string>
#include "types.h"

namespace sg::display {

template<typename... Args>
inline void PRINT(Args... args) {
    (std::cout << ... << args) << std::endl;
}

/**
 * The string containing the color representation of the grid with the given cell's cluster
 * highlighted.
 */
extern const std::string to_string(const Grid& grid, const Cell cell = CELL_NONE, sg::Output output_mode = Output::CONSOLE);

inline const std::string to_string(const Color& color) {
    return std::to_string(to_integral(color));
}

void enumerate_clusters(std::ostream&, const Grid&);
void view_clusters(std::ostream&, const Grid&);

void view_action_sequence(std::ostream&, Grid&, const std::vector<ClusterData>&, int delay_in_ms=0);

} // namespace sg::display


// ********************** SCRAPS ************************************* //
//
// template < typename _ClusterT > struct State_Action;
// template < typename __ClusterT > std::ostream& operator<<(std::ostream&, const State_Action<__ClusterT>&);
// template < typename _ClusterT >
// struct State_Action {
//     const State& r_state;
//     _ClusterT m_cluster;
//     State_Action(const State&, const _ClusterT&);
//     State_Action(const State&, const Cell);
//     friend std::ostream& operator<< <>(std::ostream&, const State_Action<_ClusterT>&);
// };

// template < typename _Cluster >
// std::ostream& operator<<(std::ostream& _out, const State_Action<_Cluster>& sa);
// template < typename _ClusterT >
// State_Action<_ClusterT>::State_Action(const State& _state, const _ClusterT& _cluster)
//     : r_state(_state)
//     , m_cluster(_cluster)
// {
// }
// template <>
// sg::State_Action<Cluster>::State_Action(const State& _state, const Cell _cell)
//     : r_state(_state)
//     , m_cluster(CLUSTER_NONE)
// {
//     if (_cell != CELL_NONE)
//         m_cluster = _state.get_cluster_blind(_cell);
// }
// This is defined as inline in "dsu.h".
//
// PROBLEM: There has to be an issue with the fact that it will use an inline function while this is declared
// to have external linkage.... It seems like two uncompatible ways to solve the same problem



#endif
