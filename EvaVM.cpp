#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

/**
* Eva VM main executable
*/
int main(int argc, char const *argv[]) {
    EvaVM vm;

    auto result = vm.exec(R"(
        42
    )");

    log(result.number);

    std::cout << "All done!\n";

    return 0;
}