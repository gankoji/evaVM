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

TEST(Classes, Inheritance)
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
                
        (class Point3D Point
            (def constructor (self x y z)
                (begin
                    ((prop (super Point3D) constructor) self x y)
                    (set (prop self z) z)))
                    
            (def calc (self)
                (+ ((prop (super Point3D) calc) self) (prop self z))))

        (var p (new Point3D 10 20 30))
        ((prop p calc) p) // 60
    )");
    EXPECT_EQ(result.number, 60);
}