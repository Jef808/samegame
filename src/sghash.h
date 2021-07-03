#ifndef __SGHASH_H_
#define __SGHASH_H_

#include "types.h"

namespace zobrist {
template<typename HashFunctor, typename Key, std::size_t NKeys>
class KeyTable;
}

namespace sg::zobrist {

/** The key associated to an individual cell. */
Key get_key(const Cell, const Color);
/**
 * Generate the grid's key using a Zobrist hashing scheme.
 */
Key get_key(const Grid&);

/**
 * A functor that computes an index from the building blocks of the states (cell, color)
 */
struct ZobristIndex
{
  //  NOTE: The upper left cell in the grid corresponds to 0,
  // so wee need to increment the cells when computing the key!
  auto operator()(const Cell cell, const Color color)
  {
    return (cell + 1) * to_integral(color);
  }
};

typedef ::zobrist::KeyTable<ZobristIndex, sg::Key, N_ZOBRIST_KEYS> ZTable;
extern ZTable Table;

} // namespace sg::zobrist

#endif
