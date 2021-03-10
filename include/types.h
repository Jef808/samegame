#ifndef __TYPES_H_
#define __TYPES_H_

auto constexpr NB_COLORS = 5;
auto constexpr HEIGHT    = 15;
auto constexpr WIDTH     = 15;
auto constexpr MAX_CELLS = HEIGHT * WIDTH;

enum Colors {
    COLOR_NONE,
    COLOR_END=NB_COLORS + 2
};

typedef std::array<Colors, N_CELLS> Grid;
typedef std::array<int, N_COLORS + 1> ColorsCounter;    // Counter remembering the colors distribution.
typedef std::array<int, MAX_CELLS> Cluster;
typedef std::array<Cluster, MAX_CELLS> ClusterList;




#endif
