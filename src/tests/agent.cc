#include <algorithm>
#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <spdlog/fwd.h>
#include <vector>
#include "mcts.h"
#include "samegame.h"
#include "types.h"

using namespace mcts;
using namespace std;
using namespace sg;
using namespace literals::chrono_literals;

class AgentTest : public ::testing::Test {
protected:
    const std::string filepath = "../data/input.txt";

    AgentTest()
        : sd_root()
        , state(sd_root)
    {}

    void SetUp() override
    {
        State::init();
        sd_root = StateData {};

        ifstream _if;
        _if.open(filepath, ios::in);
        if (!_if) {
            FAIL(); //"Failed to open ../data/input.txt");
        }
        auto state = sg::State(_if, sd_root);
        _if.close();
    }

    StateData sd_root;
    State state;
};

TEST_F(AgentTest, InitSearchStoresStateDescriptor)
{
    Agent agent(state);
    agent.init_search();

    EXPECT_EQ(state.p_data, &agent.states[0]);
}

TEST_F(AgentTest, InitSearchCreatesRootNode)
{
    Agent agent(state);
    agent.init_search();

    EXPECT_EQ(agent.nodes[1]->key, state.key());
}

// TEST_F(AgentTest, RootNodeIsStoredInHashTable)
// {
//     Agent agent(state);
//     agent.init_search();

//     EXPECT_TRUE()
// }
