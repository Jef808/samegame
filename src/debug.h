#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <ostream>
#include "types.h"

namespace sg {

class State;

} // namespace sg


namespace Debug {

void display(std::ostream& _out, const sg::State& state, sg::Cell ndx = sg::MAX_CELLS, bool labels = false);

} // namespace DEBUG


#endif
