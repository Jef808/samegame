  cmake_minimum_required(VERSION 3.14)

project( SameGame VERSION 1.0 LANGUAGES CXX )

####################################################
# General Cmake presets                            #
####################################################
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
#set(CMAKE_BUILD_TYPE debug)
set(CMAKE_BUILD_TYPE Release)

set( SG_BUILD_TESTS off )

####################################################
# Third party libraries                            #
####################################################
include( FetchContent )

option( WITH_GTEST "Build with GoogleTest" ${SG_BUILD_TESTS} )
option( WITH_SPDLOG "Build with SpdLog" OFF )

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.10.x
  )
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.x
  )
FetchContent_MakeAvailable( googletest )
FetchContent_GetProperties( spdlog )

if ( NOT spdlog_POPULATED )
  FetchContent_Populate( spdlog )
  add_subdirectory( ${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} )
endif()

######################################################
# Main project directories and sources               #
######################################################
set(DATA_DIR ${PROJECT_SOURCE_DIR}/data)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

set( sg_SOURCES
  ${SRC_DIR}/samegame.cpp
  ${SRC_DIR}/clusterhelper.cpp
  ${SRC_DIR}/sghash.cpp
  ${SRC_DIR}/display.cpp
  )

######################################################
# Main project targets                               #
######################################################
# Stand-alone simulation of the samegame game
add_library( sg STATIC ${sg_SOURCES} )
target_include_directories( sg PUBLIC ${SRC_DIR} )

# The new main
add_executable( main main.cpp ${SRC_DIR}/mcts.hpp )
target_link_libraries( main sg )
target_include_directories( main PUBLIC ${DATA_DIR} ${SRC_DIR} )

# Benchmark with the codingGame dataset
add_executable( benchmark benchmark.cpp )
target_link_libraries( benchmark sg )
target_include_directories( benchmark PUBLIC ${DATA_DIR} ${SRC_DIR} )

# What was on codingame
add_executable( main_old old_version/old_main.cpp )
target_link_libraries( main_old sg )
target_include_directories( main_old PUBLIC ${DATA_DIR} ${SRC_DIR} )

# Same as main but bundled for codingame
add_executable( main_cg ${CMAKE_SOURCE_DIR}/cg/cg_bundle.cpp )

######################################################
# Misc test / debug / benchmark targets              #
######################################################
# To play on the console
add_executable(test_human_play tests/test_humanplay.cpp)
target_link_libraries(test_human_play sg spdlog::spdlog)
target_include_directories(test_human_play PRIVATE ${DATA_DIR})

# Test/vizualize the clusters on a grid
add_executable(display_clusters tests/display_clusters.cpp)
target_link_libraries(display_clusters sg spdlog::spdlog)
target_include_directories(display_clusters PRIVATE ${DATA_DIR})

# Admire fast random simulations in the console
add_executable( test_random_simulation_toconsole tests/test_random_simulation_toconsole.cpp )
target_link_libraries( test_random_simulation_toconsole sg )
target_include_directories( test_random_simulation_toconsole PRIVATE ${DATA_DIR} )

# Agent random
add_executable( agent_random ${SRC_DIR}/agent_random_test.cpp )
target_link_libraries( agent_random sg )
target_link_directories( agent_random PRIVATE ${DATA_DIR} )

#################################################################################
# Custom targets for project filesystem hygiene                                 #
#################################################################################
add_custom_target(log_clean rm ${PROJECT_SOURCE_DIR}/build/logs -r)

#################################################################################
# Automated tests                                                               #
#################################################################################
if ( ${SG_BUILD_TESTS} )
  set( TEST_DIR ${SRC_DIR}/tests )

  enable_testing()

  # The random utilities used throughout
  add_executable(
    randutil_tests
    ${TEST_DIR}/randutil_tests.cc
    )

  # The samegame implementation test files
  set( sg_tests_SOURCES
    ${TEST_DIR}/samegame_tests.cc
    ${TEST_DIR}/dsu_tests.cc
    ${TEST_DIR}/zobrist_tests.cc
    )

  add_executable(
    sg_tests
    ${sg_tests_SOURCES}
    )

  # Linking directives for the automatic tests.
  target_link_libraries( randutil_tests gtest_main spdlog::spdlog )
  target_include_directories( randutil_tests PRIVATE ${SRC_DIR} )
  target_link_libraries( sg_tests sg gtest_main spdlog::spdlog )
  #target_link_libraries( mcts_tests gtest_main spdlog::spdlog )

  include( GoogleTest )
  gtest_discover_tests( sg_tests )
endif()
