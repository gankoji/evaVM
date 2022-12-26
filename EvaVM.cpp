#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

/**
 * Eva VM main executable
 */
int main(int argc, char const *argv[])
{
    std::cout << "Hi, this is Eva." << std::endl;

    EvaVM vm;

    auto result = vm.exec(R"(
        (var count 0)
        (for (var i 0) (< i 10) (set i (+ i 1))
            (begin
                (set count (+  count 1))))
        count
    )");

    log(result);

    return 0;
}