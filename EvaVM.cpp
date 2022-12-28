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
        // (def mysquare (x) (* x x))
        // (mysquare 2) // 4
        (def sum (a b)
            (begin
                (var x 10)
                (+ x (+ a b))))

//        (def factorial (x)
//            (if (== x 1)
//            1
//            (* x (factorial (- x 1)))))
//        (factorial 5)
    )");

    log(result);

    return 0;
}