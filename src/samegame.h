#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <array>
#include <vector>
#include <cstdint>
#include <deque>
#include <istream>
#include <memory>
#include "types.h"


namespace sg {

typedef std::list<Cell> Cluster;     // Which data structure is best for this?
typedef std::vector<Cluster*> ClusterVec;

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
  State(std::istream& _in);

  // Game logic
  void pull_down();
  void pull_left();
  bool is_terminal() const;

  // Making moves
  ClusterVec& cluster_list();
  Cluster* find_cluster(Cell cell);
  std::pair<int, Color> kill_cluster(Cluster* cluster);
  void apply_action(Action action, StateData& sd);

  // Debug
  void display(std::ostream& _out, Cell ndx, bool labels = false) const;

  Grid          m_cells;
  ColorsCounter m_colors;
  ClusterVec    m_clusters;
  int           m_ply = 1;
  StateData*    data;

private:

};

} // namespace sg

#endif
