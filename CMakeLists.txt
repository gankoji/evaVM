# Project level config
cmake_minimum_required(VERSION 3.10)
project(eva-vm)

# Google test
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

# My tests
#enable_testing()
#
#add_executable(
#  hello_test
#  hello_test.cpp
#)
#target_link_libraries(
#  hello_test
#  GTest::gtest_main
#)
#
#include(GoogleTest)
#gtest_discover_tests(hello_test)
add_subdirectory(test)

# Is this a comment? Looks like it
# Build our actual program
include_directories(./)
add_executable(eva-vm EvaVM.cpp)