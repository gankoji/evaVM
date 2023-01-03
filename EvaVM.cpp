#include <iostream>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

// Eva VM main executable
int main(int argc, char const *argv[])
{
    {
        EvaVM vm;

        Traceable::printStats();

        auto result = vm.exec(R"(
        // --- Garbage Collection (GC) ----
        // 1. Tracing heap
        // 2. GC roots (globals, constants, variables on the stack)
        // 3. Mark-Sweep GC algorithm
        (+ "hello" ", world") // leaking "hello, world" string
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        (+ "hello" ", world")
        )");

        log(result);

        Traceable::printStats();
    }

    Traceable::printStats();
    return 0;
}