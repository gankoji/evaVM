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
        (var z 5)
        (set z (+ x 10))
        z
    )");

    log(result);

    return 0;
}