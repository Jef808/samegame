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

class State {
public:
  static void init();
  State(std::istream& _in);

  // Game logic
  void pull_down();
  void pull_left();
  bool is_terminal() const;

  // Making moves
  void kill_cluster(const Cluster&);
  ClusterVec& cluster_list();

  // Debug
  void display(std::ostream& _out, bool labels = false) const;

private:
  Grid          m_cells;
  ColorsCounter m_colors;
  ClusterVec    m_clusters;
  int           m_ply = 1;
};

} // namespace sg

#endif
