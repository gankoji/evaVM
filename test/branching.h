#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(Branching, BasicIf)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (if (< 2 3) 1 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 1);
}

TEST(Branching, BasicIf2)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (if (> 2 3) 1 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 2);
}