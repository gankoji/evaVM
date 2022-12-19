#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(EvaSymbols, BooleanTrue) {
    EvaVM vm;

    auto result = vm.exec(R"(
        true
    )");
    EXPECT_TRUE(result.boolean);
}

TEST(EvaSymbols, BooleanFalse) {
    EvaVM vm;
    auto result = vm.exec(R"(
        false
    )");
    EXPECT_FALSE(result.boolean);
}
