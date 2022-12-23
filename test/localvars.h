#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(LocalVariables, NestedScope)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 5)
        (set x (+ x 10))

        x

        (begin
            (var z 100)
            (set x 1000)
            (begin
                (var x 200)
                z)
            x)
        x
        
    )");
    EXPECT_EQ(result.number, 1000);
}

TEST(LocalVariables, FurtherNestedScope)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var z 5)
        (set z (+ z 10))

        z

        (begin
            (var z 100)
            (begin
                (var z 200)
                z)
            z)
        z
    )");
    EXPECT_EQ(result.number, 15);
}