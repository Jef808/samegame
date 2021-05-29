// Basic tests only aimed at code robustness.
#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include "samegame.h"
#include "dsu.h"

using namespace sg;



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
