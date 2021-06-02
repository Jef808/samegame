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

set( samegame_src
   ${SRC_DIR}/samegame.cpp
   ${SRC_DIR}/clusterhelper.cpp
   ${SRC_DIR}/sghash.cpp
   )

 set( mcts_src
   ${SRC_DIR}/mcts.cpp
   )

file( GLOB SRC_TESTS_SOURCES
  ${SRC_DIR}/tests/*_tests.cc
  )

set( SOURCES ${samegame_src} ${mcts_src} )

# The main program
add_library( sg_mcts STATIC ${SOURCES} )
target_link_libraries( sg_mcts spdlog::spdlog )
target_include_directories( sg_mcts PUBLIC ${SRC_DIR} )

# target for the source tests using GTest
add_executable( test_sources ${SRC_TESTS_SOURCES} )
target_link_libraries( test_sources sg_mcts gtest_main spdlog::spdlog )
target_include_directories( test_sources PUBLIC ${SRC_DIR} )

# list(FILTER SOURCES EXCLUDE REGEX ".*main.cpp")
#add_library(sglib ${samegame_src})
#target_link_libraries(sglib PRIVATE spdlog::spdlog)
#target_include_directories(sglib PUBLIC ${SRC_DIR} ${INC_DIR})

#add_library(debuglib ${SRC_DIR}/debug.cpp)
#target_link_libraries(debuglib PRIVATE spdlog::spdlog)
#target_include_directories(debuglib PUBLIC ${SRC_DIR} ${INC_DIR})

# add_executable(main main.cpp ${SOURCES})
# target_link_libraries(main PRIVATE spdlog::spdlog)
# target_include_directories(main PUBLIC ${SRC_DIR} ${DATA_DIR})



# add_custom_target(log_debug SPDLOG_LEVEL=debug,m_logger=trace, && ./main
#   DEPENDS main
#   BYPRODUCTS log/logfile.txt)

# add_executable(test_avg_val tests/test_avg_val_time.cpp ${SOURCES})
# target_link_libraries(test_avg_val PRIVATE spdlog::spdlog)
# target_include_directories(test_avg_val PUBLIC ${SRC_DIR} ${INC_DIR} ${DATA_DIR})
# add_custom_target(debug_avg_val SPDLOG_LEVEL=debug,m_logger=trace, && ./test_avg_val
#   DEPENDS test_avg_val
#   BYPRODUCTS log/avg_val.txt)

# Test/vizualize the clusters
add_executable(display_clusters tests/display_clusters.cpp)
target_link_libraries(display_clusters sg_mcts spdlog::spdlog)
target_include_directories(display_clusters PUBLIC ${SRC_DIR} ${DATA_DIR})

# To play without an agent
add_executable(test_human_play tests/test_humanplay.cpp)
target_link_libraries(test_human_play sg_mcts spdlog::spdlog)
target_include_directories(test_human_play PUBLIC ${SRC_DIR} ${DATA_DIR})

# Basic tree generation
add_executable(test_init_children tests/test_init_children.cpp)
target_link_libraries(test_init_children PRIVATE sg_mcts spdlog::spdlog)
target_include_directories(test_init_children PUBLIC ${SRC_DIR} ${DATA_DIR})

# Hacky test executables
add_executable(test_pv tests/test_pv.cpp)
target_link_libraries(test_pv PRIVATE sg_mcts spdlog::spdlog)
target_include_directories(test_pv PUBLIC ${SRC_DIR} ${DATA_DIR})

add_executable(test_clusters tests/display_clusters.cpp)
target_link_libraries(test_clusters PRIVATE sg_mcts spdlog::spdlog)
target_include_directories(test_clusters PUBLIC ${SRC_DIR} ${DATA_DIR})

add_executable(test_random_simulation tests/test_random_simulation.cpp)
target_link_libraries(test_random_simulation PRIVATE sg_mcts spdlog::spdlog)
target_include_directories(test_random_simulation PUBLIC ${SRC_DIR} ${DATA_DIR})
# add_custom_target(run_test_simul SPDLOG_LEVEL=debug,m_logger=trace, && ./test_random_sim
#   DEPENDS test_random_simulation
#   BYPRODUCTS log/test_random_simul.txt)

add_custom_target(log_clean rm ${PROJECT_SOURCE_DIR}/build/logs -r)

# Enable googletest features
include( GoogleTest )
gtest_discover_tests( test_sources )
