#include "agent_random.h"
#include "samegame.h"

#include <iostream>
#include <fstream>

int main()
{
    std::ifstream ifs("../data/input.txt");
    sg::State state(ifs);
    mcts::Agent_random<sg::State, sg::ClusterData> agent(state, 20000);

    agent.run();
    auto res = agent.get_best_variation();
    auto cnt = agent.get_simul_cnt();

    state.view_action_sequence(res);
    std::cout << "\nRan " << cnt << " simulations." << std::endl;
}
