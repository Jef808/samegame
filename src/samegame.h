/// samegame.h
///
#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_


#include <iosfwd>
#include <deque>
#include <memory>
#include "types.h"

template < typename IndexT >
struct ClusterT;
template < typename Cluster, size_t N >
class DSU;

namespace sg {

using Cluster = ClusterT<Cell>;
using DSU = DSU<Cluster, MAX_CELLS>;
using ClusterList = std::array<Cluster, MAX_CELLS>;
using ClusterVec = std::vector<Cluster*>;
using VecClusterV = std::vector<ClusterVec>;

// Before figuring something else, store the whole grid for undoing moves
struct StateData {
  Grid       cells         { Color::Empty };
  Key        key           { 0 };
  int        ply           { 0 };
  int        n_empty_rows  { 0 };
  StateData* previous      { nullptr };
};

// What will be copied to describe the possible actions
struct ClusterData {
  Cell rep                 { CELL_NONE };
  Color color              { Color::Empty };
  size_t size              { 0 };
};

using ClusterDataVec = std::vector<ClusterData>;

class State {
public:
  static void init();

  explicit State(std::istream&, StateData&);
  State(const State&) = delete;
  State& operator=(const State&) = delete;


  //Key        generate_key(const Cluster&) const;
  // Game interface
  bool           is_terminal() const;
  bool           is_empty() const;
  ClusterDataVec valid_actions_data() const;
  bool           apply_action(const ClusterData&, StateData&);
  ClusterData    apply_action_blind(const Cell);
  bool           apply_action_blind(const ClusterData&);
  ClusterData    apply_random_action();
  void           undo_action(const ClusterData&);

  // Game implementation
  void           pull_cells_down();
  void           pull_cells_left();

  Cluster        get_cluster_blind(const Cell) const;
  void           kill_cluster(const Cluster&);
  ClusterData    kill_cluster_blind(const Cell, const Color);
  bool           kill_cluster_blind(const ClusterData&);
  ClusterData    kill_random_valid_cluster();

  // Accessors
  Key            key() const;
  Color          get_color(const Cell) const;
  void           set_color(const Cell, const Color);
  int            n_empty_rows() const;
  const Grid&    cells() const { return p_data->cells; }

  const Cluster* dsu_cbegin() const;
  const Cluster* dsu_cend() const;

  // Game data
  StateData*     p_data;
  ColorsCounter  colors;

  // Debug
  friend std::ostream& operator<<(std::ostream&, const State&);

  std::string to_string(Cell rep = CELL_NONE, Output = Output::CONSOLE) const;
  void enumerate_clusters(std::ostream&) const;
  void view_clusters(std::ostream&) const;

private:
  void populate_color_counter();
  Key  compute_key() const;
  void generate_clusters() const;
  bool has_nontrivial_cluster() const;
};

// For debugging
template < typename _Cluster >
struct State_Action {
  const State& r_state;
  _Cluster m_cluster;
  State_Action(const State&, const Cluster&);
  State_Action(const State&, const Cell);
  template < typename __Cluster >
  friend std::ostream& operator<<(std::ostream&, const State_Action<__Cluster>&);
};

extern std::ostream& operator<<(std::ostream&, const ClusterData&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern bool operator==(const Cluster& a, const Cluster& b);
extern bool operator==(const ClusterData& a, const ClusterData& b);

} // namespace sg

#endif
