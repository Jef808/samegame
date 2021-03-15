#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <iostream>
#include <ostream>
#include "types.h"

namespace sg {

  class State;

} // namespace sg

namespace Debug {

namespace mcts {

  // MCTS Settings
  extern bool debug_random_sim;
  extern bool debug_counters;
  extern bool debug_main_methods;
  extern bool debug_tree;
  extern bool debug_best_visits;
  extern bool debug_init_children;
  extern bool debug_final_scores;
  extern bool debug_ucb;

  inline void set_debug_counters(bool c)      { debug_counters      = c; }
  inline void set_debug_main_methods(bool c)  { debug_main_methods  = c; }
  inline void set_debug_tree(bool c)          { debug_tree          = c; }
  inline void set_debug_best_visits(bool c)   { debug_best_visits   = c; }
  inline void set_debug_init_children(bool c) { debug_init_children = c; }
  inline void set_debug_random_sim(bool c)    { debug_random_sim    = c; }
  inline void set_debug_final_scores(bool c)  { debug_final_scores  = c; }
  inline void set_debug_ucb(bool c)           { debug_ucb           = c; }

  extern std::vector<::mcts::Reward> rewards;

} // namespace mcts

namespace sg {
  extern bool debug_key;

  inline void set_debug_key(bool c)           { debug_key           = c; }

} // namespace sg


inline void press_any_key() {
     std::cerr << "Hit any key to continue..." << std::endl << std::flush;
       system("read");   // on Windows, should be system("pause");
}

// TODO allow to specify multiples cells to be highlighted (to highlight a cluster)
void display(std::ostream&, const ::sg::State&, ::sg::Cell = ::sg::MAX_CELLS, const ::sg::Cluster* = nullptr);

} // namespace Debug

#endif
