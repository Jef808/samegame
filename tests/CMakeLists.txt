set(TEST_DATA_DIR ${TEST_DIR}/data)
set(TEST_SRC_DIR ${TEST_DIR}/src)
set(TEST_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(TEST_DATA
        ${TEST_DATA_DIR}/input.txt
        )

file(GLOB TEST_SOURCES
        ${TEST_SRC_DIR}/*.c
        ${TEST_SRC_DIR}/*.cpp
        )
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*test_+cpp$")

add_library(testlib ${TEST_SOURCES})
target_link_libraries(testlib sglib)
target_include_directories(testlib PUBLIC ${TEST_SRC_DIR} ${TEST_INC_DIR} $<TARGET_PROPERTY:sglib,INTERFACE_INCLUDE_DIRECTORIES>)

add_executable(test_agent test_agent.cpp)
target_link_libraries(test_agent testlib)

add_executable(test_random test_random.cpp)
target_link_libraries(test_random testlib)

file(COPY ${TEST_DATA_DIR}/input.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
