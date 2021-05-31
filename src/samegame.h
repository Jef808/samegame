/// samegame.h
///
#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_


#include <iosfwd>
#include <deque>
#include <memory>
#include "types.h"

namespace sg {

template < typename IndexT, IndexT Default>
struct ClusterT;
// template < typename IndexT, IndexT Default >
// extern std::ostream& operator<<(std::ostream&, const ClusterT<IndexT, Default>);

//using DSU = DSU<Cluster, MAX_CELLS>;
//using ClusterList = std::array<Cluster, MAX_CELLS>;

// Smaller objects the minimal amount of data for our algorithms.
struct StateData {
  Grid       cells         { Color::Empty };
  Key        key           { 0 };
  int        ply           { 0 };
  int        n_empty_rows  { 0 };
  StateData* previous      { nullptr };
};
struct ClusterData;

class State {
public:
  using Cluster = ClusterT<Cell, CELL_NONE>;
  using ClusterDataVec = std::vector<ClusterData>;

  static void init();
  explicit State(StateData&);
  explicit State(std::istream&, StateData&);
  State(const State&) = delete;
  State& operator=(const State&) = delete;

  // Actions
  ClusterDataVec valid_actions_data() const;
  bool           apply_action(const ClusterData&, StateData&);
  ClusterData    apply_action_blind(const Cell);
  bool           apply_action_blind(const ClusterData&);
  ClusterData    apply_random_action();
  void           undo_action(const ClusterData&);
  bool           is_terminal() const;
  bool           is_empty() const;
  // Accessors
  Key            key() const;
  Color          get_color(const Cell) const;
  void           set_color(const Cell, const Color);
  int            n_empty_rows() const;
  int            set_n_empty_rows(int);
  const Grid&    cells() const { return p_data->cells; }

  void           move_data(StateData* sd);

  // Game data
  StateData*     p_data;
  ColorsCounter  colors;

  // Debugn
  friend std::ostream& operator<<(std::ostream&, const State&);

  std::string to_string(Cell rep = CELL_NONE, Output = Output::CONSOLE) const;
  void enumerate_clusters(std::ostream&) const;
  void view_clusters(std::ostream&) const;

  // Should be private
  Cluster        get_cluster_blind(const Cell) const;
  void           kill_cluster(const Cluster&);
  ClusterData    kill_cluster_blind(const Cell, const Color);
  bool           kill_cluster_blind(const ClusterData&);
  ClusterData    kill_random_valid_cluster();

private:
  //void           populate_color_counter();
  //Key            compute_key() const;
  //void           generate_clusters() const;
  //bool           has_nontrivial_cluster() const;
  void           pull_cells_down();
  void           pull_cells_left();

};




struct ClusterData {
  Cell rep                 { CELL_NONE };
  Color color              { Color::Empty };
  size_t size              { 0 };
};


// For debugging
// TODO: Put all the 'display' stuff in its own compilation unit.
template < typename _Cluster >
struct State_Action {
  const State& r_state;
  _Cluster m_cluster;
  State_Action(const State&, const _Cluster&);
  State_Action(const State&, const Cell);
  template < typename __Cluster >
  friend std::ostream& operator<<(std::ostream&, const State_Action<__Cluster>&);
};

// template < typename Index, Index DefaultValue >
// extern std::ostream& operator<<(std::ostream&, const ClusterT<Index, DefaultValue>&);
template < Cell DefaultValue >
inline std::ostream& operator<<(std::ostream& _out, const ClusterT<Cell, DefaultValue>& cluster) {
  _out << "Rep= " << cluster.rep << " { ";
    for (auto it = cluster.members.cbegin();
         it != cluster.members.cend();
         ++it)
    {
        _out << *it << ' ';
    }
    return _out << " }";
}
// template < >
// inline std::ostream& operator<<(std::ostream& _out, const ClusterT<Cell, CELL_NONE>& cluster) {
//   _out << "Rep= " << cluster.rep << " { ";
//     for (auto it = cluster.members.cbegin();
//          it != cluster.members.cend();
//          ++it)
//     {
//         _out << *it << ' ';
//     }
//     return _out << " }";
// }
extern std::ostream& operator<<(std::ostream&, const ClusterData&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern bool operator==(const StateData& a, const StateData& b);
inline bool operator==(const State::Cluster&, const State::Cluster&);
extern bool operator==(const ClusterData&, const ClusterData&);

} // namespace sg

#endif
