#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

/**
 * Eva VM main executable
 */
int main(int argc, char const *argv[])
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 1)
        (var y (+ x 1))
        
        (begin
            (var a 10)
            (var b 20)
            (set a 100)
            (+ a b))
    )");

    log(result);

    return 0;
}