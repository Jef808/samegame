#include <algorithm>
#include <fstream>
#include <vector>
#include <gtest/gtest.h>
#include "samegame.h"
#include "mcts.h"
#include "types.h"

using namespace mcts;
using namespace std;
using namespace sg;
using namespace literals::chrono_literals;

class AgentTest : public ::testing::Test {
    protected:

    const std::string filepath = "../data/input.txt";

    void SetUp() override {
        State::init();
        sd_root = StateData { };

        ifstream _if;
        _if.open(filepath, ios::in);
        if (!_if) {
            FAIL();//"Failed to open ../data/input.txt");
        }
        state = sg::State(_if, sd_root);
        _if.close();
    }

    StateData sd_root;
    sg::State state;
};

TEST_F(AgentTest, SetRootCopiesData) {
    Agent agent(state);

    states[0] = *(state.p_data);
    state.p_data = &(states[0]);

    EXPECT_EQ(states[0], *state.p_data);
    EXPECT_EQ(&(states[0]), state.p_data);
}
