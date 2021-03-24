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

  void       init_ccounter();
  Key        generate_key() const;
  Key        generate_key(const Cluster&) const;
  // Game interface
  bool       is_terminal() const;
  bool       is_empty() const;
  ActionDVec valid_actions_data() const;
  ActionVec  valid_actions() const;
  VecClusterV valid_actions_clusters() const;
  Action     random_valid_action() const;
  void       apply_action_cluster(const Cluster*, StateData&);    // TODO have the state manage the various apply_actions
  void       apply_action(const ClusterData&, StateData&);
  void       apply_action_blind(Action, StateData&);
  void       undo_action(Action);
  void       undo_action(const ClusterData&);

  // Game implementation
  void      pull_cells_down();
  void      pull_cells_left();
  void      generate_clusters() const;
  Cluster*  get_cluster(Cell) const;
  void      kill_cluster(Cluster*);    // TODO Just pick one of those, also make it nonmember?
  void      kill_cluster(const Cluster*);

  // Quick bitwise versions
  static bool is_terminal(Key);
  static Key apply_action(Key, Key);

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
  ClusterList& r_clusters;    // A reference to the implementation's DSU

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

inline bool operator==(const ClusterData& a, const ClusterData& b) {
  return a.color == b.color && a.size == b.size && a.rep == b.rep;
}




} // namespace sg

#endif
