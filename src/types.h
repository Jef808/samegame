// types.h
#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <cstdint>
#include <list>
#include <string>
#include <vector>


/**
 * Converts any enum class to its underlying integral type.
 */
template <typename E>
auto inline constexpr to_integral(E e) {
    return static_cast<std::underlying_type_t<E>>(e);
}

template <typename E, typename I>
auto inline constexpr to_enum(I i) {
    return static_cast<E>(i);
}

namespace sg {

  auto constexpr const WIDTH      = 15;
  auto constexpr const HEIGHT     = 15;
  auto constexpr const MAX_COLORS = 5;
  auto constexpr const MAX_CELLS  = HEIGHT * WIDTH;

  enum class Color {
    Empty = 0,
    Nb    = MAX_COLORS + 1
  };

  using Cell = int;
  auto constexpr const CELL_BOTTOM_LEFT = (HEIGHT - 1) * WIDTH;
  auto constexpr const CELL_NONE = MAX_CELLS;

  inline const std::string to_string(const Color& color)
  {
      return std::to_string(to_integral(color));
  }
  
  // struct Cluster {
  //   Cell rep{ CELL_NONE };
  //   std::vector<Cell> members {};
  // };

  // struct ClusterV {
  //     Cell rep{ CELL_NONE };
  //     std::vector<Cell> members;
  // };


  struct Grid {
      std::array<Color, MAX_CELLS> m_grid;

      Color& operator[](std::size_t i) {
        return m_grid[i];
      }
      Color operator[](std::size_t i) const {
        return m_grid[i];
      }
      // Color& operator[](Cell cell) {
      //   return m_grid[cell];
      // }
      // Color operator[](Cell cell) const {
      //   return m_grid[cell];
      // }

      auto begin()        {  return std::begin(m_grid);  }
      auto end()          {  return std::end(m_grid);    }
      auto cbegin() const {  return std::cbegin(m_grid); }
      auto cend()   const {  return std::cend(m_grid);   }
  };

  struct ColorsCounter {
    std::array<int, to_integral(Color::Nb)> m_colors;

      auto& operator[](Color color) {
        return m_colors[to_integral(color)];
      }
      const auto& operator[](Color color) const {
        return m_colors[to_integral(color)];
      }
      auto& operator[](std::size_t i) {
        return m_colors[i];
      }
      const auto& operator[](std::size_t i) const {
        return m_colors[i];
      }

      auto begin()        {  return std::begin(m_colors);  }
      auto end()          {  return std::end(m_colors);    }
      auto cbegin() const {  return std::cbegin(m_colors); }
      auto cend()   const {  return std::cend(m_colors);   }
  };

  using Key = uint64_t;
  using Action = Cell;

  using ActionVec = std::vector<Action>;
  //using ClusterDataVec = std::vector<ClusterData>;
  //using VecClusterV = std::vector<ClusterV>;

  enum class Output {
    CONSOLE,
    FILE
  };
} // namespace sg


namespace mcts {

  typedef double Reward;
  using sg::Action;
  using sg::ActionVec;
  using sg::Key;

  const bool propagate_minimax = false;

  enum GameNbPlayers {
      GAME_NONE,
      GAME_1P,
      GAME_2P
  };



} // namespace mcts


#endif
