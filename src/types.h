// types.h
#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <bits/stdint-uintn.h>
#include <list>
#include <vector>


namespace sg {

  struct ClusterData;

  const uint8_t WIDTH      = 15;
  const uint8_t HEIGHT     = 15;
  const uint8_t MAX_COLORS = 5;
  const uint8_t MAX_CELLS  = HEIGHT * WIDTH;    // Must be less than 256

  enum Color : uint8_t {
      COLOR_NONE = 0,
      COLOR_NB   = MAX_COLORS + 2
  };

  using Cell = uint8_t;
  const Cell CELL_NONE = MAX_CELLS;

  struct Cluster {
      Cell rep{ CELL_NONE };
      std::list<Cell> members;
  };

  struct ClusterV {
      Cell rep{ CELL_NONE };
      std::vector<Cell> members;
  };

  using Grid = std::array<Color, MAX_CELLS>;
  using ColorsCounter = std::array<uint8_t, COLOR_NB>;
  using ClusterList = std::array<Cluster, MAX_CELLS>;
  using ClusterVec = std::vector<Cluster*>;

  using Key = uint64_t;

  using Action = Cell;
  using ActionVec = std::vector<Action>;
  using ActionDVec = std::vector<ClusterData>;
  using VecClusterV = std::vector<ClusterV>;
} // namespace sg


namespace mcts {

  // Global aliases and constants
  const int MAX_PLY      = 128;
  const int MAX_CHILDREN = 256;
  const int MAX_ITER     = 10000;
  const int MAX_TIME     = 5000;    // In milliseconds.
  const int MAX_NODES    = 10000;

  typedef double Reward;
  using sg::Action;
  using sg::ActionVec;
  using sg::Key;

  const Action ACTION_NONE = sg::MAX_CELLS;

  const bool propagate_minimax = false;
  const bool use_time          = false;
  const double exploration_cst = 0.7;

  enum GameNbPlayers {
      GAME_NONE,
      GAME_1P,
      GAME_2P
  };

} // namespace mcts



#endif
