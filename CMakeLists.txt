cmake_minimum_required(VERSION 3.14)

project( SameGame VERSION 1.0 LANGUAGES CXX )

include( FetchContent )

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)

option( WITH_GTEST "Build with GoogleTest" ON )
option( WITH_SPDLOG "Build with SpdLog" ON )

# Third party dependencies
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
  )
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.x
  )
FetchContent_MakeAvailable( googletest )
#FetchContent_MakeAvailable( spdlog )
FetchContent_GetProperties( spdlog )
if ( NOT spdlog_POPULATED )
  FetchContent_Populate( spdlog )
  add_subdirectory( ${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} )
endif()

# Project directories
set(DATA_DIR ${PROJECT_SOURCE_DIR}/data)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INC_DIR ${PROJECT_SOURCE_DIR}/include)
set(TEST_DIR ${PROJECT_SOURCE_DIR}/tests)
set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)

#if (CMAKE_BUILD_TYPE STREQUAL DEBUG)
#  set($ENV{SPDLOG_LEVEL} debug,m_logger=trace)
#endif()

#include_directories(. ${SRC_DIR} ${INC_DIR})

set( sg_SOURCES
   ${SRC_DIR}/samegame.cpp
   ${SRC_DIR}/clusterhelper.cpp
   ${SRC_DIR}/sghash.cpp
   ${SRC_DIR}/display.cpp
   )

 set( mcts_SOURCES
   ${SRC_DIR}/mcts.cpp
   )

 set( sg_tests_SOURCES
   ${SRC_DIR}/tests/samegame_tests.cc
   ${SRC_DIR}/tests/dsu_tests.cc
   )

set( mcts_tests_SOURCES
  ${SRC_DIR}/tests/mcts_tests.cc
  )

set( SOURCES ${samegame_src} ${mcts_src} )

# Samegame as a stand-alone library
add_library( sg STATIC ${sg_SOURCES} )
target_include_directories( sg PUBLIC ${SRC_DIR} )

# target for the samegame tests
add_executable( sg_tests ${sg_tests_SOURCES} )
target_link_libraries( sg_tests sg gtest_main spdlog::spdlog )
target_include_directories( sg_tests PUBLIC ${SRC_DIR} )

# target for the samegame tests
add_executable( mcts_tests ${mcts_tests_SOURCES} ${mcts_SOURCES} )
target_link_libraries( mcts_tests sg gtest_main spdlog::spdlog )
target_include_directories( mcts_tests PUBLIC ${SRC_DIR} )

# MCTS/Samegame as a stand-alone library
add_library( sg_mcts STATIC ${mcts_SOURCES} )
target_link_libraries( sg_mcts sg spdlog::spdlog )
target_include_directories( sg_mcts PUBLIC ${SRC_DIR} )

# target for the source tests using GTest
# add_executable( test_sources ${SRC_TESTS_SOURCES} )
# target_link_libraries( test_sources sg_mcts gtest_main spdlog::spdlog )
# target_include_directories( test_sources PUBLIC ${SRC_DIR} )


# add_executable(test_avg_val tests/test_avg_val_time.cpp ${SOURCES})
# target_link_libraries(test_avg_val PRIVATE spdlog::spdlog)
# target_include_directories(test_avg_val PUBLIC ${SRC_DIR} ${INC_DIR} ${DATA_DIR})
# add_custom_target(debug_avg_val SPDLOG_LEVEL=debug,m_logger=trace, && ./test_avg_val
#   DEPENDS test_avg_val
#   BYPRODUCTS log/avg_val.txt)

# To play without an agent
add_executable(test_human_play tests/test_humanplay.cpp)
target_link_libraries(test_human_play sg spdlog::spdlog)
target_include_directories(test_human_play PRIVATE ${DATA_DIR})

# Test/vizualize the clusters
add_executable(display_clusters tests/display_clusters.cpp)
target_link_libraries(display_clusters sg)# spdlog::spdlog)
target_include_directories(display_clusters PRIVATE ${DATA_DIR})

add_executable(test_random_simulation tests/test_random_simulation.cpp)
target_link_libraries(test_random_simulation sg_mcts spdlog::spdlog)
target_include_directories(test_random_simulation PRIVATE ${DATA_DIR})

# Hacky test executables
add_executable(test_pv tests/test_pv.cpp)
target_link_libraries(test_pv PRIVATE sg_mcts spdlog::spdlog)
target_include_directories(test_pv PUBLIC ${DATA_DIR})

# Basic tree generation
# add_executable(test_init_children tests/test_init_children.cpp)
# target_link_libraries(test_init_children PRIVATE sg_mcts spdlog::spdlog)
# target_include_directories(test_init_children PUBLIC ${SRC_DIR} ${DATA_DIR})



# add_custom_target(run_test_simul SPDLOG_LEVEL=debug,m_logger=trace, && ./test_random_sim
#   DEPENDS test_random_simulation
#   BYPRODUCTS log/test_random_simul.txt)

add_custom_target(log_clean rm ${PROJECT_SOURCE_DIR}/build/logs -r)

# Enable googletest features
include( GoogleTest )
gtest_discover_tests( sg_tests )#mcts_tests )
