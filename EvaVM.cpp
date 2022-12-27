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
        (square 2) // 4
    )");

    log(result);

    return 0;
}