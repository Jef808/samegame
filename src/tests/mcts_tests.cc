#include <algorithm>
#include <chrono>
#include <fstream>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "spdlog/spdlog.h"
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
        : state(sd_root)
        , agent(state)
    {
        sd_root = StateData {};

        ifstream _if;
        _if.open(filepath, ios::in);
        if (!_if) {
            spdlog::error("Failed to open ../data/input.txt\nState is initialized with empty cells");
        } else {
            auto tmp_state = sg::State(_if, sd_root);
        }
        _if.close();
    }

    StateData sd_root;
    State state;
    Agent agent;
};

TEST_F(AgentTest, InitSearchStoresStateDescriptor)
{
    agent.init_search();

    EXPECT_THAT(agent.states[0].cells, ::testing::ContainerEq(agent.state.data().cells));
}

TEST_F(AgentTest, InitSearchCreatesRootNodeInHashTable)
{
    agent.init_search();

    EXPECT_EQ(agent.nodes[1]->key, state.key());
    EXPECT_EQ(MCTS.size(), 1);
}

TEST_F(AgentTest, InitSearchTwiceCreatesOnlyOneNode)
{
    agent.init_search();

    Node* first = agent.current_node();

    agent.init_search();
    Node* second = agent.current_node();

    EXPECT_EQ(MCTS.size(), 1);
    EXPECT_THAT(static_cast<void*>(first), testing::Eq(static_cast<void*>(second)));
}
