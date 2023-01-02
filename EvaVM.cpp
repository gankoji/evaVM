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
                (def inc () (set value (+ value 1)))
                inc))
        (var fn1 (createCounter))
        (fn1)
        (fn1)
    )");

    log(result);

    return 0;
}