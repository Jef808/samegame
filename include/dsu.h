#ifndef DSU_HPP
#define DSU_HPP

#include <array>
#include <vector>
#include "constants.h"


class DSU {
 public:
  /**< Default constructor */
  DSU ();

  std::array<int, NB_CELLS> parent;
  std::array<std::vector<int>, 225> groups;

  /** @brief Initialize cell as a singleton. */

  /** @brief is_cleared */
  int find_rep (int);

  /** @brief Merge two disjoint sets */
  void unite (int, int);

  /** @brief resets the data */
  void reset ();

};

#endif // DSU_HPP
