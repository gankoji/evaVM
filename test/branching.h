#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(Branching, BasicIf)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (if (< 2 3) 1 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 1);
}

TEST(Branching, BasicIf2)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (if (> 2 3) 1 2)
    )");
    log(result);
    EXPECT_EQ(result.number, 2);
}

TEST(Branching, WhileLoop)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var i 10)
        (var count 0)

        (while (> i 0)
            (begin
                (set i (- i 1)) // TODO (++ i)
                (set count (+ count 1))))
        count

    )");
    log(result);
    EXPECT_EQ(result.number, 10);
}

TEST(Branching, ForLoop)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var count 0)
        (for (var i 0) (< i 10) (set i (+ i 1))
            (begin
                (set count (+  count 1))))
        count
    )");
    log(result);
    EXPECT_EQ(result.number, 10);
}
