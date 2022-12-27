#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(Functions, NativeFunctions)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (square 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 4);
}