#ifndef MCTSTREE_H_
#define MCTSTREE_H_

#include <array>
#include <iosfwd>
#include <unordered_map>
#include <vector>

namespace mctsimpl {

template < typename KeyT, typename EdgeT >
struct NodeT;
template < typename KeyT, typename ActionDescriptorT, typename RewardT >
struct EdgeT;
template < typename KeyT, typename EdgeT >
std::ostream& operator<< (std::ostream&, const NodeT<KeyT, EdgeT>&);
template < typename KeyT, typename ActionDescriptorT, typename RewardT >
std::ostream& operator<< (std::ostream&, const EdgeT<KeyT, ActionDescriptorT, RewardT>&);

template < typename KeyT, typename EdgeT >
struct NodeT {
    KeyT key;
    int n_visits;
    std::vector<EdgeT> children;
    friend std::ostream& operator<< <> (std::ostream&, const NodeT&);
};

template < typename KeyT, typename ActionDescriptorT, typename RewardT >
struct EdgeT {
    using ActionDescriptor = ActionDescriptorT;
    using Node = NodeT<KeyT, EdgeT>;

    ActionDescriptor action_descriptor;
    RewardT avg_val;
    RewardT best_val;
    int n_visits;
    Node* parent;
    friend std::ostream& operator<< <> (std::ostream&, const EdgeT&);
};


template < typename KeyT, typename NodeT, typename EdgeT, typename RewardT=double >
class MctsTree {
    static constexpr auto MAX_DEPTH = 128;
    static constexpr auto DEFAULT_EXPLORATION_CST = 0.7;
public:
    constexpr MctsTree() : m_table() {}
    std::pair<NodeT*, bool> get_node(const KeyT key) {
        auto [ret_it, success] =  m_table.emplace(std::make_pair(key, NodeT{.key=key, .n_visits=0, .children{}}));
        return std::make_pair(&(ret_it->second), success);
    }
    NodeT* push_node(NodeT* const p_node) { return node_stack[++m_current_depth] = p_node; }
    EdgeT* push_edge(EdgeT* const p_edge) { return edge_stack[m_current_depth] = p_edge; }
    void backpropagate(NodeT* node, RewardT reward) {

    }
private:
    using MCTSLookupTable = std::unordered_map<KeyT, NodeT>;
    MCTSLookupTable m_table;
    std::array<NodeT*, MAX_DEPTH> node_stack;
    std::array<EdgeT*, MAX_DEPTH> edge_stack;

    size_t m_current_depth;
};



// inline bool operator==(const EdgeT& a, const EdgeT& b) {
//   return a.cd == b.cd;
// }
// inline bool operator==(const NodeT& a, const NodeT& b) {
//   return a.key == b.key;
// }

// extern double ucb(const NodeT&, const EdgeT&, const double exploration_cst);
// extern std::ostream& operator<<(std::ostream&, const EdgeT&);
// extern std::ostream& operator<<(std::ostream&, const NodeT&);

} // namespaceimpl
#endif // MCTSTREE_H_
