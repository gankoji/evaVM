#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

// Eva VM main executable
int main(int argc, char const *argv[])
{
    {
        EvaVM vm;

        Traceable::printStats();
        auto result = vm.exec(R"(
            (class Point null
                (def constructor (self x y)
                    (begin
                        // (set (prop self x) x)
                        // (set (prop self y) y)
                        self))
                (def calc (self)
                    1))
                    // (+ (prop self x) (prop self y))))
            Point
        )");
        log(result);

        Traceable::printStats();
    }

    Traceable::printStats();
    return 0;
}