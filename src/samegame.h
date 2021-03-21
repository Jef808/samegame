// samegame.h

#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <iosfwd>
#include <deque>
#include "types.h"

namespace sg {

// Before figuring something else, store the whole grid for undoing moves
struct StateData {
  Key        key;
  int        ply;
  Grid       cells;

  StateData* previous;
};

// What will be copied to describe the possible actions
struct ClusterData {
  Cell rep;
  Color color              = COLOR_NONE;
  uint8_t size             = 0;
};

// class State;
// struct State_Action;

class State {
public:
  static void init();

  explicit State(std::istream&, StateData*);
  State(const State&) = delete;
  State& operator=(const State&) = delete;

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
  void       undo_action(const ClusterData& cd);

  // Quick versions (no generate_clusters)
  bool is_terminal_quick() const;

  // Game implementation
  void      pull_cells_down();
  void      pull_cells_left();
  void      generate_clusters() const;
  Cluster*  get_cluster(Cell) const;
  void      kill_cluster(Cluster*);    // TODO Just pick one of those, also make it nonmember?
  void      kill_cluster(const Cluster*);

  // Game data
  void          init_ccounter();
  Key           generate_key() const;
  Key           key() const;

  StateData*    p_data;
  ColorsCounter colors;

  Grid& cells() const { return p_data->cells; }

  // Debug
  friend std::ostream& operator<<(std::ostream&, const State&);
  friend struct State_Action;

  std::string display(const State&, Output = Output::CONSOLE) const;

private:
  ClusterList& r_clusters;    // A reference to the implementation's DSU
  mutable bool need_refresh_clusters;

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

inline bool operator==(const ClusterData& a, const ClusterData& b) {
  return a.color == b.color && a.size == b.size && a.rep == b.rep;
}


} // namespace sg

#endif
