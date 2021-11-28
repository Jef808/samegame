#include "agent_random.h"
#include "samegame.h"

#include <iostream>
#include <fstream>


void display_stats(mcts::Agent_random<sg::State, sg::ClusterData>& agent)
{
    std::cout << "Number of simulations: " << agent.get_simul_cnt() << '\n';
    std::cout << "Average depth: " << agent.get_avg_depth() << '\n';
    std::cout << "Average branching number: \n";

    auto branching_data = agent.get_avg_n_children();

    for (int i=0; i<branching_data.size(); ++i)
    {
        std::cout << "Depth " << i << " : " << branching_data[i].first << std::endl;
    }
}

void tree_expansion_rate(mcts::Agent_random<sg::State, sg::ClusterData>& agent)
{
    std::cout << "Computing tree expansion rate..." << std::endl;
    agent.set_time_limit(2000);

    auto cnt = 0;
    auto cnt_leaf = 0;

    for (int i=0; i<10; ++i)
    {
        auto res = agent.view_expansion_rate();
        cnt += res.first;
        cnt_leaf += res.second;
        std::cout << i * 2 << " seconds --> "
                  << cnt << " states viewed, "
                  << cnt_leaf << " playouts completed... "
                  << " average depth : " << cnt / cnt_leaf << std::endl;
    }
}

void tree_generation_rate(mcts::Agent_random<sg::State, sg::ClusterData>& agent)
{
    std::cout << "Generating tree..." << std::endl;

    agent.set_time_limit(2000);

    auto cnt = 0;
    auto children_cnt = 0;
    auto leaf_cnt = 0;

    for (int i=0; i<10; ++i)
    {
        auto [_cnt, _children_cnt, _leaf_cnt] = agent.generating_valid_actions_rate();
        cnt += _cnt;
        children_cnt += _children_cnt;
        leaf_cnt += _leaf_cnt;

        std::cout << i * 2 << " seconds --> "
                  << cnt << " states viewed, "
                  << leaf_cnt << " playouts completed... "
                  << " average depth : " << cnt / leaf_cnt
                  << " average branching number: " << children_cnt / cnt
                  << std::endl;
    }
}

int main()
{
    std::ifstream ifs("../data/input.txt");
    sg::State state(ifs);
    mcts::Agent_random<sg::State, sg::ClusterData> agent(state);

    agent.set_time_limit(0);
    agent.set_n_iterations(0);
    agent.complete_search_near_leaf(20);

    // auto res = agent.get_best_variation();
    // auto cnt = agent.get_simul_cnt();


}
