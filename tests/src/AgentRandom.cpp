//
// Created by jfa on 20-10-29.
//

#include <iostream>
#include "AgentRandom.h"
#include <unistd.h>


AgentRandom::AgentRandom (std::istream &_in)
    : AgentTest (_in)
{

}


int AgentRandom::choose_action ()
{
  int r = state.legal_groups.size ();

  return rand () % r;
}


void AgentRandom::run_rand ()
{
  while (!state.is_terminal ())
    {
      std::vector<int> &group = *state.legal_groups[choose_action ()];

      render (default_clr, group);
      sleep (1);
      int points = score_move (group.size ());
      std::cout << "+ " << points << " points!" << std::endl;

      state.step (group);
      set_legal_groups (state);
      render ();
      score += points;
      std::cout << "Score is now " << score << std::endl;
      sleep (1);
    }

  if (state.is_cleared ())
    {
      score += 1000;
    }

  std::cout << "Game over! Final score: " << score << std::endl;
}