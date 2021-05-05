// Basic tests only aimed at code robustness.

#include <fstream>
#include <gtest/gtest.h>
#include "samegame.h"

using namespace sg;

TEST(ValidClusterTest, BasicAssertions)
{
    const Cluster cluster;
    EXPECT_LE(cluster.members.size(), 2);
}

// int main()
// {
//     State::init();

//     StateData sd_root {};
//     std::ifstream _if;
//     _if.open("../../data/input.txt", std::ios::in);
//     sg::State state (_if, sd_root);
//     _if.close();

//     if (!test_is_valid_cluster())
//     {
//         spdlog::error("Cluster is not default constructed with members.size() < 2.\nChecks for is_valid_action must be adjusted!");
//     }

//     return EXIT_SUCCESS;
// }
