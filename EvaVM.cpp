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
        (var z 10)
        (set z 100)
        (begin
            (var a 200)
        )
        z
    )");

    printf("Yeah we done.\n");
    log(result);

    return 0;
}