#ifndef __SGHASH_H_
#define __SGHASH_H_

#include "types.h"

namespace sg::zobrist {

/**
 * Generate the grid's key using a Zobrist hashing scheme.
 */
Key get_key(const Grid&);


} // namespace sg


#endif
