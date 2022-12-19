
add_executable(
    eva_basic
    eva_basic.cpp
)
target_link_libraries(
    eva_basic
    GTest::gtest_main
)

include_directories(../)
include(GoogleTest)
gtest_discover_tests(eva_basic)