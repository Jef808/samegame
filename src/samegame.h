/// samegame.h
///
#ifndef __SAMEGAME_H_
#define __SAMEGAME_H_

#include "types.h"

#include <algorithm>
#include <deque>
#include <iosfwd>
#include <memory>

namespace sg {

/**
 * @Class An interface to the game exposing it only as a general State/Action system.
 *
 * @Note `StateData` and `ClusterData` are compact descriptors for the Grid and the
 * Clusters respectively.
 */
class State
{
 public:
  using reward_type = double;
  using key_type = uint64_t;

  State();
  explicit State(std::istream&);
  State(Grid&&, ColorCounter&&);
  State(key_type, const Grid&, const ColorCounter&);

  ClusterDataVec valid_actions_data() const;
  bool apply_action(const ClusterData&);
  ClusterData apply_random_action(Color = Color::Empty);
  reward_type evaluate(const ClusterData&) const;
  reward_type evaluate_terminal() const;
  bool is_terminal() const;
  Key key();
  bool is_trivial(const ClusterData& cd) const { return cd.size < 2; }
  bool is_empty() const { return m_cells[CELL_BOTTOM_LEFT] == Color::Empty; }
  const Grid& grid() const { return m_cells; }
  const ColorCounter& color_counter() const { return m_cnt_colors; }
  friend std::ostream& operator<<(std::ostream&, const State&);

  ///TODO: Get rid of this! (Move to namespace scope)
  ClusterData get_cd(Cell rep) const;
  void display(Cell rep) const;
  void show_clusters() const;
  void view_action_sequence(const std::vector<ClusterData>&, int = 0) const;
  void log_action_sequence(std::ostream&,
                           const std::vector<ClusterData>&) const;

  bool operator==(const State& other) const { return m_cells == other.m_cells; }

 private:
  key_type m_key;
  Grid m_cells;
  ColorCounter m_cnt_colors;
};

/** Display a colored board with the chosen cluster highlighted. */
extern std::ostream& operator<<(std::ostream&,
                                const std::pair<const State&, Cell>&);
extern std::ostream& operator<<(std::ostream&, const std::pair<Grid&, int>&);
extern std::ostream& operator<<(std::ostream&, const State&);
extern std::ostream& operator<<(std::ostream&, const ClusterData&);

inline bool operator==(const ClusterData& a, const ClusterData& b)
{
 return a.rep == b.rep && a.color == b.color && a.size == b.size;
}
template<typename _Index_T, _Index_T IndexNone>
extern bool operator==(const ClusterT<_Index_T, IndexNone>& a,
                       const ClusterT<_Index_T, IndexNone>& b);
template<>
inline bool operator==(const ClusterT<Cell, MAX_CELLS>& a,
                       const ClusterT<Cell, MAX_CELLS>& b)
{
  return a == b;
}

} // namespace sg

#endif
