#include <iostream>
#include "src/vm/EvaVM.h"

/**
* Eva VM main executable
*/
int main(int argc, char const *argv[]) {
    EvaVM vm;

    vm.exec(R"(
        42
    )");

    std::cout << "All done!\n";

    return 0;
}