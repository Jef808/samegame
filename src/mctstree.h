#ifndef MCTSTREE_H_
#define MCTSTREE_H_

#include "types.h"
#include <array>

namespace mctsimpl {

using mcts::MAX_DEPTH;
using mcts::MAX_NODES;
using mcts::MAX_CHILDREN;
using mcts::DEFAULT_EXPLORATION_CST;
using mcts::Key;
using mcts::Reward;

struct Node;
struct Edge;
enum class EdgeField {
  ucb=0, n_visits, avg_val, best_val=4
};

class MctsTree {
    void init();

private:
    std::array<Node*, MAX_DEPTH> node_stack;
    std::array<Edge*, MAX_DEPTH> edge_stack;

    double exploration_cst = 0.4;

    Edge* best_child(Node* const, EdgeField);

};

struct Node {
    Key key;
    int n_visits;
    std::vector<Edge> children;
    friend std::ostream& operator<<(std::ostream&, const Node&);
};

struct Edge {
    sg::ClusterData cd;
    Reward avg_val;
    Reward best_val;
    int n_visits;
    Node* parent;
    friend std::ostream& operator<<(std::ostream&, const Edge&);
};

inline bool operator==(const Edge& a, const Edge& b) {
  return a.cd == b.cd;
}
inline bool operator==(const Node& a, const Node& b) {
  return a.key == b.key;
}

extern double ucb(const Node&, const Edge&, const double exploration_cst);
extern std::ostream& operator<<(std::ostream&, const Edge&);
extern std::ostream& operator<<(std::ostream&, const Node&);

} // namespaceimpl
#endif // MCTSTREE_H_
