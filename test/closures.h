#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(Closures, CellVars)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 10)
        (def foo () x) // x is a free variable, but is not closed since it's global

        (begin
            (var y 100) // a cell variable (alloc)
            (set y 1000) // y: cell (update)
            (var q 300) // local
            q
            (+ y x)
            (begin
                (var z 200) // cell
                z
                (def bar () (+ y z)) // y is free, and should be closed over, its not global
                (bar)))
    )");
    log(result);
    EXPECT_EQ(result.number, 1200);
}

TEST(Closures, MoreClosures)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (def createCounter ()
            (begin
                (var value 0)
                (def inc () (set value (+ value 1)))
                inc))
        (var fn1 (createCounter))
        (fn1)
        (fn1)
    )");
    log(result);
    EXPECT_EQ(result.number, 2);
}