// types.h
#ifndef __TYPES_H_
#define __TYPES_H_

#include "grid.h"
#include <array>
#include <cstdint>
#include <iterator>
#include <vector>

template<typename E>
auto inline constexpr to_integral(E e)
{
  return static_cast<std::underlying_type_t<E>>(e);
}

template<typename E, typename I>
auto inline constexpr to_enum(I i)
{
  return static_cast<E>(i);
}

// Forward declare the template defined in "dsu.h"
template<typename Index, Index IndexNone>
struct ClusterT;

namespace sg {

using Key = uint64_t;
auto inline constexpr N_ZOBRIST_KEYS = (MAX_CELLS + 1) * MAX_COLORS;

// State descriptor
typedef std::array<int, MAX_COLORS + 1> ColorCounter;
using Cluster = ClusterT<Cell, CELL_NONE>;

// Cluster or Action descriptor
struct ClusterData
{
  Cell rep{CELL_NONE};
  Color color{Color::Empty};
  size_t size{0};
};
using ClusterDataVec = std::vector<ClusterData>;
enum class Output
{
  CONSOLE,
  FILE
};

using Action = ClusterData;
using ActionVec = ClusterDataVec;

} // namespace sg

#endif
