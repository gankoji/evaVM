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
        (def square (x) (* x x))
        (square 2) // 4

//        (def factorial (x)
//            (if (== x 1)
//            1
//            (* x (factorial (- x 1)))))
//        (factorial 5)
    )");

    log(result);

    return 0;
}