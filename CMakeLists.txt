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

include_directories(. ${SRC_DIR} ${INC_DIR})

#set(DATA
#        ${DATA_DIR}/input.txt
#        )

set(samegame_src
  ${INC_DIR}/types.h
  ${SRC_DIR}/samegame.cpp
  ${INC_DIR}/samegame.h
  )

#file(GLOB SOURCES
#        ${SRC_DIR}/*.c
#        ${SRC_DIR}/*.cpp
#        )
#list(FILTER SOURCES EXCLUDE REGEX ".*main.cpp")

add_library(sglib ${samegame_src})
#target_include_directories(sglib PUBLIC ${SRC_DIR} ${INC_DIR})

set(main_src
  ${PROJECT_SOURCE_DIR}/main.cpp ${samegame_src})

add_executable(main ${main_src})
target_link_libraries(main sglib)
target_include_directories(main PUBLIC ${DATA_DIR})
