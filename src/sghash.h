#ifndef __SGHASH_H_
#define __SGHASH_H_

#include "types.h"

namespace sg::zobrist {

Key get_key(const Grid&, const Cell);
Key get_key(const Grid&);


} // namespace sg


#endif
