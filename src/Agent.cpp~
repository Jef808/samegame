#include <Agent.h>


Agent::Agent ()
    : dsu (), score (0) {}


int Agent::score_move (int legal_group_size)
{
  return (legal_group_size - 2) * (legal_group_size - 2);
}


std::unique_ptr<GameState>
Agent::make_child (const GameState &state, std::unique_ptr<std::vector<int>> &legal_group)
{
  std::unique_ptr<GameState> child = std::make_unique<GameState> (state);
  child->step (*legal_group);

  return child;
}


void Agent::set_legal_groups (GameState &state)
{
  dsu.reset ();
  state.legal_groups.clear ();

  find_groups (state);

  for (std::vector<int> &group: dsu.groups)
    {
      if (group.size () < 2)
        { continue; }

      state.legal_groups.emplace_back (std::make_unique<std::vector<int>> (group));
    }
}


void Agent::find_groups (GameState &state)
{
  /**< Traverse first row */
  for (int i = 0; i < WIDTH - 1; ++i)
    {
      if (state[i + 1] != 0 && state[i + 1] == state[i])
        dsu.unite (i + 1, i);
    }

  /**< Traverse other rows */
  for (int i = WIDTH; i < NB_CELLS; ++i)
    {
      /**< Look at cell above */
      if (state[i - WIDTH] != 0 && state[i - WIDTH] == state[i])
        dsu.unite (i - WIDTH, i);

      /**< Look at cell to the right */
      if ((i + 1) % WIDTH != 0 && state[i + 1] != 0 && state[i + 1] == state[i])
        dsu.unite (i + 1, i);
    }
}


void Agent::dsu_reset ()
{
  for (int v = 0; v < NB_CELLS; ++v)
    {
      dsu.parent[v] = v;
      dsu.groups[v] = std::vector<int> (1, v);
    }
}