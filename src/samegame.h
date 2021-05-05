// samegame.h

#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <iosfwd>
#include <deque>
#include <memory>
#include "types.h"

template < typename Index >
struct Cluster_T;
template < typename Index, std::size_t N >
class DSU;

//extern DSU<sg::Cell, sg::MAX_CELLS> dsu;

namespace sg {

using DSU = DSU<Cell, MAX_CELLS>;
using Cluster = Cluster_T<Cell>;
using ClusterList = std::array<Cluster, MAX_CELLS>;
using ClusterVec = std::vector<Cluster*>;
using VecClusterV = std::vector<ClusterVec>;

// Before figuring something else, store the whole grid for undoing moves
struct StateData {
  Key        key           = 0;
  int        ply           = 1;
  Grid       cells         = {COLOR_NONE};
  int n_empty_rows         = 0;

  StateData* previous      = nullptr;
};

// What will be copied to describe the possible actions
struct ClusterData {
  Cell rep                 = CELL_NONE;
  Color color              = COLOR_NONE;
  size_t size              = 0;
};

// class State;
// struct State_Action;

class State {
public:
  static void init();

  explicit State(std::istream&, StateData&);
  State(const State&) = delete;
  State& operator=(const State&) = delete;

  Key        generate_key() const;
  Key        generate_key(const Cluster&) const;
  // Game interface
  bool       is_terminal() const;
  bool       is_empty() const;
  bool       is_valid(const Cluster&) const;
  ActionDVec valid_actions_data() const;
  ActionVec  valid_actions() const;
  VecClusterV valid_actions_clusters() const;
  void       apply_action(const ClusterData&, StateData&);
  void       apply_action_oneway(const ClusterData&);
  ClusterData apply_random_action();
  void       undo_action(Action);
  void       undo_action(const ClusterData&);
  // Quick bitwise versions
  static bool is_terminal(const State&);
  //static Key apply_action(Key, Key);

  // Game implementation
  void        pull_cells_down();
  void        pull_cells_left();
  void        generate_clusters() const;
  void        generate_clusters_old() const;
  void        kill_cluster(const Cluster&);
  Cluster     get_cluster(const Cell) const;
  const Cluster& see_cluster(const Cell) const;
  ClusterData kill_cluster_blind(const Cell, const Color);   // Used to apply a random action for simulations
  ClusterData kill_random_valid_cluster();
  ClusterData kill_random_valid_cluster_new();
  ClusterData kill_random_valid_cluster_generating();

  // Game data
  StateData*    p_data;
  ColorsCounter colors;
  Key           key() const;

  const Grid& cells() const { return p_data->cells; }

  // Debug
  friend std::ostream& operator<<(std::ostream&, const State&);

  std::string to_string(Cell rep = CELL_NONE, Output = Output::CONSOLE) const;
  void view_clusters(std::ostream&) const;

private:
  DSU* p_dsu;
  void init_ccounter();                   // TODO: All methods that require generate_cluster() should be private
  bool check_terminal() const;            // Only used on initialization (if no key available)

};

// For debugging
struct State_Action {
  using localCluster = std::vector<Cell>;
  const State& r_state;
  Cell m_action;
  localCluster m_cluster {};

  State_Action(const State& _state, Cell _action)
    : r_state(_state)
    , m_action(_action)
  {
    load_cluster(m_action);
  }

  void load_cluster(Cell rep);
  std::string to_string(Output = Output::CONSOLE) const;
  friend std::ostream& operator<<(std::ostream&, const State_Action&);
};

extern std::ostream& operator<<(std::ostream&, const State_Action&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);
template < typename _Index >
extern std::ostream& operator<<(std::ostream&, const Cluster_T<_Index>&);

extern bool operator==(const Cluster& a, const Cluster& b);
extern bool operator==(const ClusterData& a, const ClusterData& b);

  inline bool State::is_terminal(const State& state) {
    return state.key() & 1;
}


} // namespace sg

#endif
