#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

TEST(MathOps, AddTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (+ 2 3)
    )");
    log(result);
    EXPECT_EQ(result.number, 5);
}

TEST(MathOps, AddThree) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (+ 2 (+ 3 1))
    )");
    log(result);
    EXPECT_EQ(result.number, 6);
}

TEST(MathOps, SubTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (- 2 1)
    )");
    log(result);
    EXPECT_EQ(result.number, 1);
}

TEST(MathOps, MulTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (* 2 8)
    )");
    log(result);
    EXPECT_EQ(result.number, 16);
}

TEST(MathOps, DivTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (/ 8 2)
    )");

    log(result);
    EXPECT_EQ(result.number, 4);
}