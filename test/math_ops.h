#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(MathOps, AddTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (+ 2 3)
    )");

    EXPECT_EQ(result.number, 5);
}

TEST(MathOps, AddThree) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (+ 2 (+ 3 1))
    )");

    EXPECT_EQ(result.number, 6);
}
