#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <array>
#include <vector>
#include <cstdint>
#include <istream>
#include "types.h"

struct Cluster {
  std::array<int, MAX_CELLS> ndx;    // The indices of the cells in Samegame::Grid
  int size;
  bool empty();
  Color color();
  int& operator[](int i) { return ndx[i]; }
};

struct ClusterList {
  Cluster& operator[](int i) { return cl[i]; }
  std::array<Cluster, MAX_CELLS> cl;
};

class State {
public:
  //static void init();

  // Game logic
  void pull_down();
  void pull_left();
  bool is_terminal() const;

  // Making moves
  void kill_cluster(const Cluster&);
  ClusterList& cluster_list();

  // Debug
  void display(std::ostream& _out, bool labels = false) const;

private:
  Grid          cells;
  ColorsCounter colors;
  ClusterList   clusters;
  int           ply = 1;
};


#endif
