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
                    
            (var p (new Point 10 20))
            ((prop p calc) p) // 30
        )");
        log(result);
    }

    return 0;
}