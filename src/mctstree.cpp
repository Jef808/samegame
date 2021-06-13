#include "mctstree.h"
#include <math.h>
#include <utility>

namespace mctsimpl {

// Sentinel node
Node NODE_NONE { .key=0, .n_visits=0, .children={} };
// Sentinel edge
Edge EDGE_NONE { .cd=sg::ClusterData{}, .avg_val=0, .best_val=0, .n_visits=0, .parent=&NODE_NONE };

using MCTSLookupTable = std::unordered_map<Key, Node>;
MCTSLookupTable MCTS {};

Node* get_node(Key key) {
    // auto node_it = MCTS.find(key);
    //
    // Node* p_ret = [&](){
    //     if (node_it != MCTS.end()) {
    //         return &(node_it->second);
    //     }
    //     Node new_node = { .key = key, .n_visits = 0, .children {}};
    //     auto [ret_it, success] =  MCTS.insert(std::make_pair(key, new_node));
    //     return &(ret_it->second);
    // }();

    auto [ret_it, success] =  MCTS.emplace(std::make_pair(key, Node{.key=key, .n_visits=0, .children{}}));
    return &(ret_it->second);
}

double ucb(const Node& node, const Edge& edge, const double exploration_cst = DEFAULT_EXPLORATION_CST)
{
    Reward result = edge.n_visits ? edge.avg_val : edge.best_val;
    result += exploration_cst * sqrt(static_cast<double>(log(node.n_visits)) / static_cast<double>(edge.n_visits + 1));

    // Some random normlization to have the rewards between 0 and 1.
    result = result / 5000.0;

    return result;
}

Reward get_value(const Edge& edge, const EdgeField field)
{
    switch(field)
    {
    case EdgeField::ucb:
        return ucb(*edge.parent, edge);
    case EdgeField::n_visits:
        return edge.n_visits;
    case EdgeField::avg_val:
        return edge.avg_val;
    case EdgeField::best_val:
        return edge.best_val;
    }
}
    
Edge* MctsTree::best_child(Node* const node, const EdgeField field)
{
    Edge* ret = &EDGE_NONE;
    Reward best_val = std::numeric_limits<Reward>::min();

    for (auto child_it = node->children.begin(); child_it != node->children.end(); ++child_it) {
        if (Reward r = get_value(*child_it, field); r > best_val) {
            best_val = r;
            ret = &(*child_it);
        }
    }
    return ret;
}

} // namespace mctsimpl
