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

TEST(Functions, NativeSum)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (sum 1 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 3);
}

TEST(Functions, NativeSumWithVars)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 3)
        (sum 2 x)
    )");
    log(result);
    EXPECT_EQ(result.number, 5);
}

TEST(Functions, UserDefFunc1)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (def mysquare (x) (* x x))

        (mysquare 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 4);
}

// TEST(Functions, UserDefFunc2)
// {
//     EvaVM vm;
//
//     auto result = vm.exec(R"(
//         (def factorial (x)
//             (if (== x 1)
//                 1
//                 (* x (factorial (- x 1)))))
//
//         (factorial 5)
//     )");
//     log(result);
//     EXPECT_EQ(result.number, 120);
// }