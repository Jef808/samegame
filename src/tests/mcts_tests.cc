#include <algorithm>
#include <chrono>
#include <fstream>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "spdlog/spdlog.h"
#include <vector>
#include "mcts.h"
#include "samegame.h"
#include "display.h"
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

        spdlog::default_logger()->set_level(spdlog::level::debug);
    }

    StateData sd_root;
    State state;
    Agent agent;
};

class MCTSHashTest : public AgentTest {
protected:
    MCTSHashTest() = default;
    void SetUp() override {

    }

    Node root {state.key(), 0, 0, std::vector<Edge>{}};  // {key, visit, n_children, children}
};

TEST_F(AgentTest, AgentCtorSavesStateRef)
{
    EXPECT_THAT(agent.state.key(), ::testing::Eq(sd_root.key));
}

TEST_F(AgentTest, InitSearchStoresStateDescriptor)
{
    agent.init_search();
    EXPECT_EQ(agent.states[0].key, agent.state.key());
}

TEST_F(AgentTest, InitSearchDoesntLoseState)
{
    agent.init_search();
    EXPECT_THAT(agent.states[0].key, ::testing::Eq(sd_root.key));
}


TEST_F(AgentTest, ApplyValidActionIncreasesPly)
{
    agent.init_search();
    EXPECT_THAT(agent.get_ply(), ::testing::Eq(1));

    agent.apply_action({2, Color(1), 5});
    EXPECT_THAT(agent.get_ply(), ::testing::Eq(2));
}

TEST_F(AgentTest, ApplyActionIsIdempotent)
{
    agent.apply_action({2, Color(1), 5});
    agent.apply_action({2, Color(1), 5});
    EXPECT_THAT(agent.get_ply(), ::testing::Eq(2));
}

TEST_F(AgentTest, ApplyInvalidActionDoesntIncreasePly)
{
    agent.init_search();
    agent.apply_action({CELL_NONE, Color(1), 2});
    EXPECT_THAT(agent.get_ply(), ::testing::Eq(1));
    agent.apply_action({CELL_NONE, Color(1), 1});
    EXPECT_THAT(agent.get_ply(), ::testing::Eq(1));

    agent.apply_action({2, Color(1), 5});
    agent.apply_action({2, Color::Empty, 5});
    EXPECT_THAT(agent.get_ply(), ::testing::Eq(2));
}

TEST_F(MCTSHashTest, InitSearchInsertsOneInHashTable)
{
    agent.init_search();
    EXPECT_EQ(agent.nodes[1]->key, state.key());
    EXPECT_EQ(MCTS.size(), 1);
}

TEST_F(MCTSHashTest, InitSearchTwiceInsertsOneInHashTable)
{
    agent.init_search();
    Node* first = agent.current_node();
    agent.init_search();
    Node* second = agent.current_node();

    EXPECT_EQ(MCTS.size(), 1);
    EXPECT_THAT(static_cast<void*>(first), testing::Eq(static_cast<void*>(second)));
}

TEST_F(MCTSHashTest, TreePolicyOnceAddsOneNode)
{
    agent.init_search();
    agent.tree_policy();

    EXPECT_THAT(MCTS.size(), ::testing::Eq(2));
}

TEST_F(MCTSHashTest, TreePolicyFiveTimesAddsFiveNode)
{
    agent.init_search();
    for (int i=0; i<5; ++i) {
        agent.tree_policy();
    }

    EXPECT_THAT(MCTS.size(), ::testing::Eq(6));
}
