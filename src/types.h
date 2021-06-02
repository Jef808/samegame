// types.h
#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <cstdint>
#include <vector>


template <typename E>
auto inline constexpr to_integral(E e) {
    return static_cast<std::underlying_type_t<E>>(e);
}

template <typename E, typename I>
auto inline constexpr to_enum(I i) {
    return static_cast<E>(i);
}

// Forward declare the template defined in "dsu.h"
template < typename Index, Index DefaultValue >
struct ClusterT;

namespace sg {

// Game constants
auto constexpr const WIDTH = 15;
auto constexpr const HEIGHT = 15;
auto constexpr const MAX_COLORS = 5;
auto constexpr const MAX_CELLS = HEIGHT * WIDTH;

using Cell = int;
auto constexpr const CELL_UPPER_LEFT = 0;
auto constexpr const CELL_BOTTOM_LEFT = (HEIGHT - 1) * WIDTH;
auto constexpr const CELL_UPPER_RIGHT = WIDTH - 1;
auto constexpr const CELL_NONE = MAX_CELLS;

enum class Color {
    Empty = 0,
    Nb = MAX_COLORS + 1
};

using Key = uint64_t;
auto constexpr const N_ZOBRIST_KEYS = MAX_CELLS * MAX_COLORS;

class Grid {
public:
    Grid() = default;
    Color& operator[](Cell c) {
        return m_grid[c];
    }
    Color operator[](Cell c) const {
        return m_grid[c];
    }
    auto begin() { return std::begin(m_grid); }
    auto end() { return std::end(m_grid); }
    auto cbegin() const { return std::cbegin(m_grid); }
    auto cend() const { return std::cend(m_grid); }

    mutable int n_empty_rows { 0 };

private:
    std::array<Color, MAX_CELLS> m_grid { Color::Empty };
};

class ColorsCounter {
public:
    ColorsCounter() = default;
    auto& operator[](Color color) {
        return m_colors[to_integral(color)];
    }
    const auto& operator[](Color color) const {
        return m_colors[to_integral(color)];
    }
    auto begin() { return std::begin(m_colors); }
    auto end() { return std::end(m_colors); }
    auto cbegin() const { return std::cbegin(m_colors); }
    auto cend() const { return std::cend(m_colors); }

private:
    std::array<int, to_integral(Color::Nb)> m_colors { 0 };
};

// State descriptor
struct StateData {
    Grid          cells        { };
    ColorsCounter cnt_colors   { };
    Key           key          { 0 };
    int           ply          { 0 };
    int           n_empty_rows { 0 };
    StateData*    previous     { nullptr };
};

using Cluster = ClusterT<Cell, CELL_NONE>;

// Cluster or Action descriptor
struct ClusterData {
    Cell   rep   { CELL_NONE };
    Color  color { Color::Empty };
    size_t size  { 0 };
};
using ClusterDataVec = std::vector<ClusterData>;

enum class Output {
    CONSOLE,
    FILE
};

using Action = ClusterData;
using ActionVec = ClusterDataVec;

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
