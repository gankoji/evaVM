#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

TEST(ComparisonOps, LessThan) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (< 2 3)
    )");
    log(result);
    EXPECT_TRUE(result.boolean);
}

TEST(ComparisonOps, AddThree) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (+ 2 (+ 3 1))
    )");
    log(result);
    EXPECT_EQ(result.number, 6);
}

TEST(ComparisonOps, SubTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (- 2 1)
    )");
    log(result);
    EXPECT_EQ(result.number, 1);
}

TEST(ComparisonOps, MulTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (* 2 8)
    )");
    log(result);
    EXPECT_EQ(result.number, 16);
}

TEST(ComparisonOps, DivTwo) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (/ 8 2)
    )");

    log(result);
    EXPECT_EQ(result.number, 4);
}