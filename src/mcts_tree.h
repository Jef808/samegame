#ifndef __MCTSTREE_H_
#define __MCTSTREE_H_

#include <unordered_map>
#include <vector>

namespace mcts {

template<typename StateT, typename ActionT, size_t MAX_DEPTH>
class MctsTree
{
 public:
  struct Node;
  struct Edge;
  using node_pointer = Node*;
  using edge_pointer = Edge*;
  using ChildrenContainer = std::vector<Edge>;
  using key_type = typename StateT::key_type;
  using reward_type = typename StateT::reward_type;
  struct Node
  {
    int n_visits;
    ChildrenContainer children;
  };
  struct Edge
  {
    ActionT action;
    reward_type avg_val;
    reward_type best_val;
    int n_visits;
    bool subtree_completed;
  };

  MctsTree(key_type key)
    : m_table(), m_edge_stack{}, m_depth{0}, p_root(get_node(key))
  {
  }

  void set_root(const key_type key)
  {
    p_root = get_node(key);
    m_depth = 0;
  }
  node_pointer get_root()
  {
    m_depth = 0;
    return p_root;
  }
  node_pointer get_node(const key_type key)
  {
    auto [node_it, inserted] = m_table.insert(std::pair{key, Node{}});
    return &(node_it->second);
  }
  void traversal_push(edge_pointer edge)
  {
    m_edge_stack[m_depth] = edge;
    ++m_depth;
  }
  void backpropagate(reward_type reward)
  {
    std::for_each(m_edge_stack.begin(),
                  m_edge_stack.begin() + m_depth,
                  update_stats(reward));
  }
  size_t size()
  {
    return m_table.size();
  }

 private:
  using LookupTable = typename std::unordered_map<key_type, Node>;
  using TraversalStack = std::array<edge_pointer, MAX_DEPTH>;

  LookupTable m_table;
  TraversalStack m_edge_stack;
  size_t m_depth;
  Node* p_root;

  struct update_stats
  {
    reward_type val;
    update_stats(reward_type _val) : val(_val) {}
    void operator()(edge_pointer edge)
    {
      ++edge->n_visits;
      edge->avg_val += (val - edge->avg_val) / (edge->n_visits);
      edge->best_val = std::max(
          edge->best_val, val); // > edge->best_val ? val : edge->best_val;
    }
  };
};

} // namespace mcts

#endif
