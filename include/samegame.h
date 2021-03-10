#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include <array>
#include <vector>
#include <cstdint>
#include <istream>
#include "types.h"

class samegame {
public:
  static void init();

  // Game logic
  void pull_down();
  void pull_left();
  bool is_terminal();

  // Making moves
  void kill_cluster(Cluster*);

  // Debug
  void render (bool labels = false) const;

private:
  Grid cells;
  ColorsCounter cc;
};

#endif
