// Basic tests only aimed at code robustness.
#include "samegame.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <algorithm>
#include <fstream>


namespace sg {

namespace {


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

    StateData sd_root {};
    State state;
};

TEST_F(SamegameTest, StateDataIsZeroInitializable)
{
    StateData sd {};
    StateData sd_empty = [](){
        StateData sd_ret {};
        std::fill(sd_ret.cells.begin(), sd_ret.cells.end(), Color::Empty);
        return sd_ret;
    }();

    EXPECT_THAT(sd.cells, ::testing::ContainerEq(sd_empty.cells));
}

TEST_F(SamegameTest, CanCopyStateData)
{
    StateData sd_copy {};
    state.copy_data_to(sd_copy);
    EXPECT_THAT(sd_copy.cells, ::testing::ContainerEq(sd_root.cells));

    sd_copy.cells[0] = Color::Empty;
    EXPECT_THAT(sd_root.cells[0], ::testing::Ne(Color::Empty));

    sd_root.cells[0] = Color(1);
    EXPECT_THAT(sd_copy.cells[0], ::testing::Eq(Color::Empty));

}

TEST_F(SamegameTest, CanCloneStateData)
{
    StateData sd_clone = state.clone_data();
    EXPECT_THAT(sd_clone.cells, ::testing::ContainerEq(sd_root.cells));

    sd_clone.cells[0] = Color::Empty;
    EXPECT_THAT(sd_root.cells[0], ::testing::Ne(Color::Empty));

    sd_root.cells[0] = Color(1);
    EXPECT_THAT(sd_clone.cells[0], ::testing::Eq(Color::Empty));
}

TEST_F(SamegameTest, CanMoveStateData)
{
    StateData sd_moved;
    state.move_data_to(sd_moved);
    StateData sd_cloned = state.clone_data();
    EXPECT_THAT(sd_moved.cells, ::testing::ContainerEq(sd_cloned.cells));

    sd_moved.cells[0] = Color::Empty;
    state.copy_data_to(sd_cloned);
    EXPECT_THAT(sd_cloned.cells[0], ::testing::Eq(Color::Empty));
}

// TEST_F(SamegameTest, MoveDataMethodWorks)
// {

//     StateData sd1 {};
//     state.move_data(&sd1);

//     EXPECT_EQ(sd1, *state.p_data);
//     EXPECT_EQ(&sd1, state.p_data);

//     StateData* sd2 = new StateData();
//     state.move_data(sd2);

//     EXPECT_EQ(sd2, state.p_data);
//     EXPECT_EQ(*sd2, *state.p_data);
// }


} // namespace
} // namespace sg
