#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

TEST(Variables, GetPredefinedGlobal)
{
    EvaVM vm;

    // This test now fails because scope analysis
    // has no information about our predefined globals
    // (which are in the VM itself, while scope analysis
    // lives in the compiler).
    auto result = vm.exec(R"(
        (var x 10)
        x
    )");
    log(result);
    EXPECT_EQ(result.number, 10);
}

TEST(Variables, CreateBasic)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var c 100)
    )");
    log(result);
    EXPECT_EQ(result.number, 100);
}

TEST(Variables, CreateComplex)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 10)
        (var c (+ x 10))
    )");
    log(result);
    EXPECT_EQ(result.number, 20);
}

TEST(Variables, SetExistingNewValue)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 10)
        (set x 20)
    )");
    log(result);
    EXPECT_EQ(result.number, 20);
}

TEST(Variables, SetNewNewValue)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var z 10)
        (var a 30)
        (set z (+ a 10))
        z
    )");
    log(result);
    EXPECT_EQ(result.number, 40);
}