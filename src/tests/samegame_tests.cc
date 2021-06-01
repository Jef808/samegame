// Basic tests only aimed at code robustness.
#include "samegame.h"
#include <algorithm>
#include <fstream>
#include <spdlog/spdlog.h>
#include <gtest/gtest.h>

using namespace sg;

class SamegameTest : public ::testing::Test {
protected:
    const std::string filepath = "../data/input.txt";

    SamegameTest()
        : state(sd_root)
    {
        std::ifstream _if;
        _if.open("../data/input.txt", std::ios::in);
        if (_if)
        {
            auto tmp_state = State(_if, sd_root);
        }
        else
        {
            spdlog::error("Failed to open .../../data/input.txt\nState is initialized with empty cells");
        }
        _if.close();
    }

    void SetUp() override
    {
        State::init();
    }

    StateData sd_root {};
    State state;
};

TEST_F(SamegameTest, StateDataIsCopyAssignable)
{
    StateData sd {};
    sd = *state.p_data;

    ASSERT_EQ(sd, *state.p_data);
}

TEST_F(SamegameTest, StateDataIsCopyInitializable)
{

    StateData sd { *state.p_data };

    ASSERT_EQ(sd, *state.p_data);
}

TEST_F(SamegameTest, MoveDataMethodWorks)
{

    StateData sd1 {};
    state.move_data(&sd1);

    EXPECT_EQ(sd1, *state.p_data);
    EXPECT_EQ(&sd1, state.p_data);

    StateData* sd2 = new StateData();
    state.move_data(sd2);

    EXPECT_EQ(sd2, state.p_data);
    EXPECT_EQ(*sd2, *state.p_data);
}
