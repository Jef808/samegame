#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <bits/stdint-uintn.h>
#include <list>
#include <vector>


// Global aliases and constants
const int MAX_PLY              = 128;
const int MAX_CHILDREN         = 256;
const int MAX_TIME             = 5000;    // In milliseconds.

typedef double Reward;
typedef uint64_t Key;

// Samegame specific aliases and constants
namespace sg {

const uint8_t HEIGHT           = 15;
const uint8_t WIDTH            = 15;
const uint8_t MAX_CELLS        = HEIGHT * WIDTH;
const uint8_t MAX_COLORS       = 5;

typedef uint8_t Cell;

enum Action : uint8_t {
    ACTION_NONE                = 0,
    ACTION_NB                  = 225
};

enum Color : uint8_t {
    COLOR_NONE                 = 0,
    COLOR_NB                   = MAX_COLORS + 2
};

typedef std::array<Color, MAX_CELLS> Grid;
typedef std::array<uint8_t, MAX_COLORS + 1> ColorsCounter;

typedef std::list<Cell> Cluster; // Should keep the local State Cluster objects as arrays.
typedef std::vector<Cluster*> ClusterVec;
typedef std::vector<Action>   ActionVec;      // Only store the representative


} // namespace sg

// Monte Carlo specific aliases and constants
namespace mcts {

const int MAX_ITER             = 10000;
const double exploration_cst   = 0.7;
const bool use_time            = false;
const bool propagate_minimax   = false;

} // namespace mcts



#endif
