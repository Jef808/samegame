#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <iosfwd>
#include "types.h"

namespace sg {


struct StateData {
  // std::vector<Cell>    cluster_reprs;     // A representative for each cluster
  // std::vector<Color>   cluster_colors;    // The color of the corresponding cluster
  // std::vector<uint8_t> cluster_sizes;     // The size of the corresponding cluster
  Key                  key;
  int                  ply;
  StateData*           previous;
};


class State {
public:
  static void init();

  explicit State(std::istream& _in);
  State(const State&) = delete;
  State& operator=(const State&) = delete;

  // Game logic
  void pull_down();
  void pull_left();
  bool is_terminal() const;

  // Making moves
  ClusterVec& cluster_list();
  ActionVec  valid_actions() const;
  Cluster* find_cluster(Cell cell) const;
  using sizeNcolor = std::pair<int, Color>;
  sizeNcolor kill_cluster(Cluster* cluster);
  void apply_action(Action action, StateData& sd);
  void undo_action(Action action);

  Key key() const;

  Grid          m_cells;
  ColorsCounter m_colors;
  ClusterVec    m_clusters;
  int           m_ply = 1;
  StateData*    data;

private:

};


} // namespace sg

#endif
