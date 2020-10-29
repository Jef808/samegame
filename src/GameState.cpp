#include "GameState.h"


GameState::GameState ()
    : grid ()
{
  //ctor
}


GameState::GameState (std::istream &_in)
    : grid (_in)
{

}


GameState::GameState (const GameState &other)
    : grid (other.grid)
{
  //copy ctor
}


GameState::GameState (GameState &&other) noexcept
    : grid (std::move (other.grid)), legal_groups (std::move (other.legal_groups)), children (std::move (other.children))
{
  //move ctor
}


GameState::~GameState () = default;


void GameState::step (const std::vector<int> &group)
{
  grid.clear_group (group);
  grid.do_gravity ();
  grid.pull_columns ();
}


bool GameState::is_terminal () const
{
  return legal_groups.empty ();
}


bool GameState::is_cleared () const
{
  return grid.cells[WIDTH * (HEIGHT - 1)] == 0;
}


void GameState::render (bool labels) const
{
  grid.render (labels);
}
