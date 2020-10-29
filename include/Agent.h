#ifndef AGENT_HPP
#define AGENT_HPP

#include <vector>
#include <memory>
#include "constants.h"
#include "dsu.h"
#include "GameState.h"


class Agent {
 public:
  /** Default constructor */
  Agent ();

  DSU dsu;
  int score;

 private:
  /** @brief Populate the GameState's `legal_groups` vector with all the groups of size >= 2.
   *
   */
  void set_legal_groups (GameState &);

  /** @brief Rule is (n - 2)^2 where n is the size of the group.
   *
   */
  static int score_move (int);

  /** @brief Appends a state to the GameState's `children` according to a given legal group.
   *
   */
  std::unique_ptr<GameState> make_child (const GameState &, std::unique_ptr<std::vector<int>> &);

  /** @brief Populates the Agent's DSU (disjoint set union) structure with all groups of cells in given GameState.
  *
  */
  void find_groups (GameState &);

  /** @brief Reinitialize the agent's DSU structure.
   *
   */
  void dsu_reset ();

};

#endif // AGENT_HPP
