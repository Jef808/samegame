cmake_minimum_required(VERSION 3.14)

project( SameGame VERSION 1.0 LANGUAGES CXX )

include( FetchContent )

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_VERBOSE_MAKEFILE on)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

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
  GIT_TAG        v1.8.5
  )
FetchContent_MakeAvailable( googletest )
FetchContent_MakeAvailable( spdlog )

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
   #${SRC_DIR}/samegame.h
   #${SRC_DIR}/types.h
   )

 set( mcts_src
   ${SRC_DIR}/mcts.cpp
   #${SRC_DIR}/mcts.h
   )

file( GLOB SRC_TESTS_SOURCES
  ${SRC_DIR}/tests/*_tests.cc
  )

# target for the source tests using GTest
add_executable( test_sources ${SRC_TESTS_SOURCES} )
target_link_libraries( test_sources  gtest_main )
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

set(SOURCES ${samegame_src} ${mcts_src} )

# Hacky test executables
add_executable(test_pv tests/test_pv.cpp ${SOURCES})
target_link_libraries(test_pv PRIVATE spdlog::spdlog)
target_include_directories(test_pv PUBLIC ${SRC_DIR} ${DATA_DIR})
# add_custom_target(play_game SPDLOG_LEVEL=debug,m_logger=trace, && ./test_pv
#   DEPENDS test_pv
#   BYPRODUCTS log/game.txt)

add_executable(test_clusters tests/display_clusters.cpp ${SOURCES})
target_link_libraries(test_clusters PRIVATE spdlog::spdlog)
target_include_directories(test_clusters PUBLIC ${SRC_DIR} ${DATA_DIR})
# add_custom_target(run_test_clusters SPDLOG_LEVEL=debug,m_logger=trace, && ./test_clusters
#   DEPENDS test_clusters
#   BYPRODUCTS log/game.txt)

add_executable(test_random_simulation tests/test_random_simulation.cpp ${SOURCES})
target_link_libraries(test_random_simulation PRIVATE spdlog::spdlog)
target_include_directories(test_random_simulation PUBLIC ${SRC_DIR} ${DATA_DIR})
# add_custom_target(run_test_simul SPDLOG_LEVEL=debug,m_logger=trace, && ./test_random_sim
#   DEPENDS test_random_simulation
#   BYPRODUCTS log/test_random_simul.txt)

add_custom_target(log_clean rm ${PROJECT_SOURCE_DIR}/build/logs -r)

# Enable googletest features
include( GoogleTest )
gtest_discover_tests( test_sources )
