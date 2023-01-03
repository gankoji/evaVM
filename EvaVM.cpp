#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

// Eva VM main executable
int main(int argc, char const *argv[])
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (def createCounter ()
            (begin
                (var value 0)
                (def inc () (begin (set value (+ value 1))))
                inc))
        (var fn1 (createCounter))
        // (fn1)
        // (fn1)
        // 
        // (var fn2 (createCounter))
        // (fn2)
        // 
        // (+ (fn1) (fn2))
    )");

    log(result);

    return 0;
}