cmake_minimum_required(VERSION 3.10)
project(SameGame VERSION 1.0)

set(DATA_DIR ${PROJECT_SOURCE_DIR}/data)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INC_DIR ${PROJECT_SOURCE_DIR}/include)
set(TEST_DIR ${PROJECT_SOURCE_DIR}/tests)

add_subdirectory(${TEST_DIR})

set(DATA
        ${DATA_DIR}/input.txt
        )

file(GLOB SOURCES
        ${SRC_DIR}/*.c
        ${SRC_DIR}/*.cpp
        )
list(FILTER SOURCES EXCLUDE REGEX ".*main.cpp")

add_library(sglib ${SOURCES})
target_include_directories(sglib PUBLIC ${SRC_DIR} ${INC_DIR})

add_executable(samegame main.cpp)

target_link_libraries(samegame sglib)