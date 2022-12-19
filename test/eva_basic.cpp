#include <gtest/gtest.h>

#include "src/vm/EvaVM.h"

TEST(EvaBasic, SimpleExpression) {
    EvaVM vm;

    auto result = vm.exec(R"(
        (+ 2 3)
    )");

    EXPECT_EQ(result.number, 5);
}

TEST(EvaBasic, SanityCheck) {
    EXPECT_EQ("hi", "hi");
}