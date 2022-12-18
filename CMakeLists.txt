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
add_subdirectory(test)

# Build our actual program
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
add_compile_options(-Wall -Wpedantic -ggdb3)
endif()

include_directories(./)
add_executable(eva-vm EvaVM.cpp)