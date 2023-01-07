#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

// Eva VM main executable
int main(int argc, char const *argv[])
{
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
        log(result);
    }

    return 0;
}