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

TEST(ComparisonOps, GreaterThan) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (> 2 1)
    )");
    log(result);
    EXPECT_TRUE(result.boolean);
    
    result = vm.exec(R"(
        (> 1 2)
    )");
    log(result);
    EXPECT_FALSE(result.boolean);
}

TEST(ComparisonOps, Equal) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (== 2 2)
    )");
    log(result);
    EXPECT_TRUE(result.boolean);
}

TEST(ComparisonOps, LessEq) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (<= 2 8)
    )");
    log(result);
    EXPECT_TRUE(result.boolean);
}

TEST(ComparisonOps, GreaterEq) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (>= 8 2)
    )");

    log(result);
    EXPECT_TRUE(result.boolean);
}

TEST(ComparisonOps, NotEq) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (!= 8 2)
    )");

    log(result);
    EXPECT_TRUE(result.boolean);
}