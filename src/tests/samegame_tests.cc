// Basic tests only aimed at code robustness.
#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include "samegame.h"

using namespace sg;


class SamegameTest : public ::testing::Test {
    protected:

    const std::string filepath = "../../data/input.txt";

    SamegameTest()
        : state(sd_root)
    {}

    void SetUp() override {

        sd_root = StateData { };

        std::ifstream _if;
        _if.open(filepath, std::ios::in);
        if (!_if) {
            FAIL();//"Failed to open ../../data/input.txt");
        }

        auto state = State(_if, sd_root);
        _if.close();
    }

    void TearDown() override {
        sd_root = { };//{ CELL_NONE, Color::Empty, 0 };
    }

    static inline StateData sd_root { };
    State state;
    std::ifstream _in;
};

TEST_F(SamegameTest, StateDataIsCopyAssignable) {

    StateData sd { };
    sd = *state.p_data;

    ASSERT_EQ(sd, *state.p_data);
}

TEST_F(SamegameTest, StateDataIsCopyInitializable) {

    StateData sd { *state.p_data };

    ASSERT_EQ(sd, *state.p_data);
}

TEST_F(SamegameTest, MoveDataMethodWorks) {

    StateData sd1 {  };
    state.move_data(&sd1);

    EXPECT_EQ(sd1, *state.p_data);
    EXPECT_EQ(&sd1, state.p_data);

    StateData* sd2 = new StateData();
    state.move_data(sd2);

    EXPECT_EQ(sd2, state.p_data);
    EXPECT_EQ(*sd2, *state.p_data);
}
