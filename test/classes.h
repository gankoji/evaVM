#include <gtest/gtest.h>
#include "src/vm/EvaVM.h"

TEST(Classes, BasicClass)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (class Point null
            (def constructor (self x y)
                (begin
                    (set (prop self x) x)
                    (set (prop self y) y)
                    self))
            (def calc (self)
                (+ (prop self x) (prop self y))))
                
        (var p (new Point 10 20))
        ((prop p calc) p)
    )");
    EXPECT_EQ(result.number, 30);
}

TEST(Classes, NoSelfInConstructor)
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (class Point null
            (def constructor (self x y)
                (begin
                    (set (prop self x) x)
                    (set (prop self y) y)
                    ))
            (def calc (self)
                (+ (prop self x) (prop self y))))
                
        (var p (new Point 10 20))
        ((prop p calc) p) // 3
    )");
    EXPECT_EQ(result.number, 30);
}