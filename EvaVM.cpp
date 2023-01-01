#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

// Eva VM main executable
int main(int argc, char const *argv[])
{
    EvaVM vm;

    auto result = vm.exec(R"(
        (var x 10)
        (def foo () x) // x is a free variable, but is not closed since it's global

        (begin
            (var y 100) // a cell variable
            (var q 300) // local
            (begin
                (var z 200) // cell
                (def bar () (+ y z)) // y is free, and should be closed over, its not global
                (bar)))


    )");

    log(result);

    return 0;
}