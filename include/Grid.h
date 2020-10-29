#ifndef GRID_HPP
#define GRID_HPP

#include <array>
#include <vector>
#include <cstdint>
#include <istream>
#include "constants.h"


class Grid {
 public:
  /** Default constructor */
  Grid ();

  /**< Input constructor */
  explicit Grid (std::istream &);

  /**< Copy constructor */
  Grid (const Grid &);

  /**< Move constructor */
  Grid (Grid &&) noexcept;

  /** Default destructor */
  ~Grid () noexcept;

  std::array<int, NB_CELLS> cells{};
  std::array<int, NB_COLORS + 1> clr_counter{};

  /**
  * @brief Replace the cells in the given group by empty cells.
  */
  void clear_group (const std::vector<int> &);

  /**
  * @brief Pull down floating cells.
  */
  void do_gravity ();

  /**
  * @brief Pull colums to the left so that empty columns are the rightmost columns.
  */
  void pull_columns ();

  /**
  * @brief Display the grid to stdout.
  */
  void render (bool labels = false) const;
};

#endif // GRID_HPP
