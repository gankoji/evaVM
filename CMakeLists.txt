# Project level config
cmake_minimum_required(VERSION 3.10)
project(eva-vm)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

enable_testing()
# Google test
set(CMAKE_CXX_STANDARD 17)
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
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
add_compile_options(-Wall -ggdb3 -fsized-deallocation)

include_directories(./)
add_executable(eva-vm EvaVM.cpp)