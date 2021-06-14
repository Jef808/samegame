#include "mctstree.h"
#include <math.h>
#include <utility>
#include <vector>

namespace mctsimpl {

// Sentinel node
Node NODE_NONE { .key=0, .n_visits=0, .children={} };
// Sentinel edge
Edge EDGE_NONE { .cd=sg::ClusterData{}, .avg_val=0, .best_val=0, .n_visits=0, .parent=&NODE_NONE };

using MCTSLookupTable = std::unordered_map<Key, Node>;
MCTSLookupTable MCTS {};

MctsTree::MctsTree()
{
    // Allocate the desired amount of memory for the lookup table.
}

Node* MctsTree::find_or_make_node(Key key) {

}

// WARNING: Make sure this doesn't backfire, it seems like the pointer
// will simply be const_casted but who knows
Node* MctsTree::push_node(Node* const node)
{

}

Edge* MctsTree::push_edge(Edge* const edge)


// Node* MctsTree::set_root(const Key key)
// {
//     m_current_depth = 1;
//     return node_stack[m_current_depth] = p_root = find_or_create_node(key);
// }

// void MctsTree::set_children(Node* const node, const std::vector<Edge>& _children)
// {
//     node->children = _children;
//     auto target_size = _children.size() > max_children ? max_children : _children.size();
//     node->children.resize(target_size);
// }

std::size_t MctsTree::size() const
{
    return MCTS.size();
}





bool is_leaf(Node* node)
{
    return node->n_visits > 0 && node->children.size() == 0;
}

Node* MctsTree::current_node()
{
    return node_stack[m_current_depth];
}

// Node* MctsTree::best_leaf(const EdgeField field)
// {
//     m_current_depth = 1;

//     while (!is_leaf(current_node())) {
//         edge_stack[m_current_depth] = best_child(node, field);
//         ++m_current_depth;
//     }
//     return node;
// }


} // namespace mctsimpl
