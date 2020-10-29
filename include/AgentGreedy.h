//
// Created by jfa on 20-10-29.
//

#ifndef _AGENTGREEDY_H_
#define _AGENTGREEDY_H_

#include "Agent.h"

class AgentGreedy: public Agent {
 public:
  AgentGreedy();

 private:
  /** @brief Sort the legal groups according to `cmp_groups` for the greedy approach to exploring the game tree.
   *
   */
  void sort_legal_groups (GameState &);

  /** @brief Util function to order legal groups.
  *
  */
  static bool cmp_groups (GameState &state, const std::vector<int> &, const std::vector<int> &);
};

#endif //_AGENTGREEDY_H_
