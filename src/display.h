#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <iostream>
#include <map>
#include <string>
#include "types.h"

namespace sg::display {

template<typename... Args>
inline void PRINT(Args... args) {
    (std::cout << ... << args) << std::endl;
}

/**
 * The string containing the color representation of the grid with the given cell's cluster
 * highlighted.
 */
extern const std::string to_string(const Grid& grid, const Cell cell = CELL_NONE, sg::Output output_mode = Output::CONSOLE);

inline const std::string to_string(const Color& color) {
    return std::to_string(to_integral(color));
}

void enumerate_clusters(std::ostream&, const Grid&);
void view_clusters(std::ostream&, const Grid&);

void view_action_sequence(std::ostream&, Grid&, const std::vector<ClusterData>&, int delay_in_ms=0);
void log_action_sequence(std::ostream&, Grid&, const std::vector<ClusterData>&);

} // namespace sg::display



#endif
