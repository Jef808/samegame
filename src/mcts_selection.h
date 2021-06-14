#ifndef MCTS_SELECTION_H_
#define MCTS_SELECTION_H_

#include <math.h>

namespace mctsimpl {


auto static constexpr DEFAULT_EXPLORATION_CST = 0.7;

enum class SelectionCriterion {
    ucb=0, n_visits, avg_val, best_val=4
};

template < typename NodeT, typename EdgeT, typename RewardT=double >
EdgeT* best_child(NodeT* const node, const SelectionCriterion crit) {
    EdgeT* ret = nullptr;
    RewardT best_val = std::numeric_limits<RewardT>::min();

    for (auto child_it = node->children.begin(); child_it != node->children.end(); ++child_it) {
        if (RewardT r = get_value(*child_it, crit); r > best_val) {
            best_val = r;
            ret = &(*child_it);
        }
    }
    return ret;
}

template < typename NodeT, typename EdgeT, typename RewardT=double >
double ucb(const NodeT& node, const EdgeT& edge, const double exploration_cst = DEFAULT_EXPLORATION_CST) {
    auto result = edge.n_visits > 0 ? edge.avg_val : edge.best_val;
    result += exploration_cst * sqrt(log(node.n_visits) / static_cast<double>(edge.n_visits + 1));
    result /= 5000.0;  // hacky normalization
    return result;
}

template < typename NodeT, typename EdgeT, typename RewardT=double >
RewardT get_value(const EdgeT& edge, const SelectionCriterion crit) {
    RewardT ret = 0;
    switch(crit) {
        case SelectionCriterion::ucb:
            ret = static_cast<RewardT>(ucb(*edge.parent, edge)); break;
        case SelectionCriterion::n_visits:
            ret = edge.n_visits; break;
        case SelectionCriterion::avg_val:
            ret = edge.avg_val; break;
        case SelectionCriterion::best_val:
            ret = edge.best_val; break;
    }
    return ret;
}

} // namespace mctsimpl

#endif // MCTS_SELECTION_H_
