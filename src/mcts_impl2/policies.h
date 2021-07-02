#ifndef __MCTS_POLICIES_H_
#define __MCTS_POLICIES_H_

#include <cmath>
#include <functional>

namespace policies {


/**
 * A UCB functor is used in the MCTS algorithm to select which edge to traverse
 * when exploring the state/action tree. Until we land on an unexplored node,
 * we choose the edge maximizing the functor's operator().
 */
struct Default_UCB_Func
{
    auto operator()(double expl_cst, unsigned int n_parent_visits)
    {
        return [expl_cst, n_parent_visits]<typename EdgeT>(const EdgeT& edge)
        {
            return edge.avg_val
               + expl_cst * sqrt(log(n_parent_visits) / (edge.n_visits + 1.0));
        };
    }
};


} // namespace policies

#endif
