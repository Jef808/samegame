#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

#include <vector>
#include <cstdint>
#include <memory>
#include <istream>
#include "Grid.h"


class GameState {
 public:
  /**< Default constructor */
  GameState ();

  /**< Input constructor */
  explicit GameState (std::istream &);

  /**< Copy constructor */
  GameState (const GameState &);

  /**< Move constructor */
  GameState (GameState &&) noexcept;

  /**< Default destructor */
  ~GameState ();

  Grid grid;


  std::vector<std::unique_ptr<std::vector<int>>> legal_groups;
  std::vector<std::unique_ptr<GameState>> children;

  /**
   * @brief Apply the move corresonding to the given group.
  */
  void step (const std::vector<int> &);

  /**
  * @brief Checks if there are any legal groups remaining.
  */
  bool is_terminal () const;

  /**
  * @brief Checks if the board has any cell remaining.
  */
  bool is_cleared () const;

  /**
   * @brief Return a reference to the cell at given index in the state's grid.
   */
  int &operator[] (const int cell_ndx)
  {
    return grid.cells[cell_ndx];
  }

 private:
  /**
   * @brief Renders the grid of the state.
   */
  void render (bool labels = false) const;

};

#endif // GAMESTATE_HPP
