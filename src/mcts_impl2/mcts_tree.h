#ifndef __MCTSTREE_H_
#define __MCTSTREE_H_

#include <execution>
#include <unordered_map>
#include <vector>

namespace mcts_impl2 {


template < typename StateT,
           typename ActionT,
           size_t MAX_DEPTH >
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
    struct Node {
        key_type key;
        int n_visits;
        ChildrenContainer children;
    };
    struct Edge {
        ActionT     action;
        reward_type avg_val;
        reward_type best_val;
        int         n_visits;
    };

    MctsTree(key_type key) :
        m_table(),
        m_edge_stack{},
        m_depth{0},
        p_root(get_node(key))
    { }

    void set_root(const key_type key) {
        p_root = &get_node(key);
    }
    Node& get_root() {
        return *p_root;
    }
    void traversal_push(edge_pointer edge) {
        edge_stack[m_depth] = &edge; ++m_depth;
    }
    void backpropagate(reward_type reward) {
        std::for_each(std::execution::par_unseq,
                      edge_stack.begin(),
                      edge_stack.begin() + current_depth,
                      update_stats(reward));
    }

private:
    using LookupTable = typename std::unordered_map<key_type, Node>;
    using TraversalStack = std::array<edge_pointer, MAX_DEPTH>;

    LookupTable    m_table;
    TraversalStack m_edge_stack;
    size_t         m_depth;
    Node*          p_root;

    std::pair<Node*, bool> get_node(const key_type key) {
        auto [node_it, inserted] = m_table.insert(std::pair{ key, Node{ .key=key, } });
        return std::pair{&*node_it, inserted};
    }

    struct update_stats {
        reward_type val;
        update_stats(reward_type _val) :
            val(_val) { }
        void operator()(edge_pointer edge) {
            ++edge->n_visits;
            edge->avg_val += (val - edge->avg_val) / (edge->n_visits);
            edge->best_val = std::max(edge->best_val, val);// > edge->best_val ? val : edge->best_val;
        }
    };
};


} // namespace mcts_impl2


#endif
