// samegame.h

#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <iosfwd>
#include <deque>
#include "types.h"

namespace sg {

// Before figuring something else, store the whole grid for undoing moves
struct StateData {
  Key        key           = 0;
  int        ply           = 1;
  Grid       cells         = {COLOR_NONE};

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
  ActionDVec valid_actions_data() const;
  ActionVec  valid_actions() const;
  VecClusterV valid_actions_clusters() const;
  void       apply_action(const ClusterData&, StateData&);
  ClusterData apply_random_action();
  void       undo_action(Action);
  void       undo_action(const ClusterData&);
  // Quick bitwise versions
  static bool is_terminal(Key);
  //static Key apply_action(Key, Key);

  // Game implementation
  void      pull_cells_down();
  void      pull_cells_left();
  void      generate_clusters() const;
  void      kill_cluster(Cluster*);
  Cluster * get_cluster(Cell) const;

  // Game data
  StateData*    p_data;
  ColorsCounter colors;
  Key           key() const;

  const Grid& cells() const { return p_data->cells; }

  // Debug
  friend std::ostream& operator<<(std::ostream&, const State&);
  friend struct State_Action;

  std::string display(const State&, Output = Output::CONSOLE) const;

private:
  ClusterList& r_clusters;                // A reference to the implementation's DSU
  void init_ccounter();                   // TODO: All methods that require generate_cluster() should be private
  bool check_terminal() const;            // Only used on initialization (if no key available)
  ClusterData kill_cluster_blind(Cell, Color);   // Used to apply a random action for simulations
};

// For debugging
struct State_Action {
    const State& state;
    Cell actionrep           = MAX_CELLS;
    const Cluster* p_cluster = nullptr;

    std::string display(Output = Output::CONSOLE) const;
  };

extern std::ostream& operator<<(std::ostream&, const State_Action&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);
extern std::ostream& operator<<(std::ostream&, const Cluster&);

inline bool operator==(const Cluster& a, const Cluster& b) {
  return a.rep == b.rep && a.members == b.members;
}
inline bool operator==(const ClusterData& a, const ClusterData& b) {
  return a.color == b.color && a.size == b.size && a.rep == b.rep;
}

inline bool State::is_terminal(Key key) {
    return key & 1;
}


} // namespace sg

#endif
