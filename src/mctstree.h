#ifndef MCTSTREE_H_
#define MCTSTREE_H_

#include <array>

struct Node;
struct Edge;
enum class EdgeField {
  ucb, visits, avg_val, best_val=4
};

template < std::size_t Depth >
class MctsTree {
    void init();

private:
    std::array<Node*, Depth> node_stack;
    std::array<Edge*, Depth> edge_stack;

    Edge* best_child(EdgeField);
};

#endif // MCTSTREE_H_
