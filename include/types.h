#ifndef __TYPES_H_
#define __TYPES_H_

#include <array>
#include <list>
#include <vector>
#include <bits/stdint-uintn.h>

auto constexpr HEIGHT    = 15;
auto constexpr WIDTH     = 15;
auto constexpr MAX_CELLS = HEIGHT * WIDTH;
auto constexpr MAX_COLORS = 5;


namespace sg {

typedef uint8_t Cell;

enum Color : uint8_t {
    COLOR_NONE = 0,
    COLOR_NULL  = MAX_COLORS + 2
};

typedef std::array<Color, MAX_CELLS>        Grid;
typedef std::array<uint8_t, MAX_COLORS + 1> ColorsCounter;    // Counter remembering the colors distribution.

typedef uint8_t Action;
typedef uint8_t Key;

}
#endif
