cmake_minimum_required(VERSION 3.10)

project(SameGame VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_FLAGS "${CMAXE_CXX_FLAGS} -Wall -g")
set(CMAKE_VERBOSE_MAKEFILE on)

set(DATA_DIR ${PROJECT_SOURCE_DIR}/data)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INC_DIR ${PROJECT_SOURCE_DIR}/include)
set(TEST_DIR ${PROJECT_SOURCE_DIR}/tests)

#add_subdirectory(${TEST_DIR})

#include_directories(. ${SRC_DIR} ${INC_DIR})

set(samegame_src
   ${SRC_DIR}/samegame.cpp
   ${SRC_DIR}/samegame.h
   ${SRC_DIR}/types.h
   )

file(GLOB SOURCES
  ${SRC_DIR}/*.h
  ${SRC_DIR}/*.cpp
  )
# list(FILTER SOURCES EXCLUDE REGEX ".*main.cpp")

add_library(sglib ${samegame_src})
target_include_directories(sglib PUBLIC ${SRC_DIR} ${INC_DIR})

add_library(debuglib ${SRC_DIR}/debug.h ${SRC_DIR}/debug.cpp)
target_include_directories(sglib PUBLIC ${SRC_DIR} ${INC_DIR})

add_executable(main main.cpp ${SOURCES})
target_link_libraries(main sglib debuglib)
target_include_directories(main PUBLIC ${SRC_DIR} ${INC_DIR} ${DATA_DIR})
