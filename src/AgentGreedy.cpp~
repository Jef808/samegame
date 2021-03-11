//
// Created by jfa on 20-10-29.
//

#include <algorithm>
#include <AgentGreedy.h>

AgentGreedy::AgentGreedy () = default;


void AgentGreedy::sort_legal_groups (GameState &state)
{
  std::sort (state.legal_groups.begin (), state.legal_groups.end (), [&] (const auto &grp1, const auto &grp2)
  { return cmp_groups (state, *grp1, *grp2); });
}


bool AgentGreedy::cmp_groups (GameState &state, const std::vector<int> &grp1, const std::vector<int> &grp2)
{
  int clr_count1 = state.grid.clr_counter[state.grid.cells[grp1.back ()]];
  int clr_count2 = state.grid.clr_counter[state.grid.cells[grp2.back ()]];

  if (clr_count1 == clr_count2)
    {
      return grp1.size () < grp2.size ();
    }

  return clr_count1 < clr_count2;
}
