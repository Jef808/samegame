#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>

auto constexpr HEIGHT    = 15;
auto constexpr WIDTH     = 15;
auto constexpr MAX_CELLS = HEIGHT * WIDTH;
auto constexpr MAX_COLORS = 5;


enum Color {
    COLOR_NONE,
    COLOR_END=MAX_COLORS + 2
};
typedef std::array<Color, MAX_CELLS>   Grid;
typedef std::array<int, MAX_COLORS + 1> ColorsCounter;    // Counter remembering the colors distribution.


#endif
