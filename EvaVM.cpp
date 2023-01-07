#include <fstream>
#include <iostream>
#include <string>

#include "src/vm/EvaVM.h"
#include "src/vm/Logger.h"

void printHelp()
{
    std::cout << "\nUsage: eva-em [options]\n\n"
              << "Options:\n"
              << "    -e, Expression to parse\n"
              << "    -f, File to parse\n\n";
}

// Eva VM main executable
int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        printHelp();
        return 0;
    }

    // Evaluation mode
    std::string mode = argv[1];

    // Program to execute
    std::string program;

    // If its a simple expression on the command line
    if (mode == "-e")
        program = argv[2];
    else if (mode == "-f")
    {
        std::ifstream programFile(argv[2]);
        std::stringstream buffer;
        buffer << programFile.rdbuf() << std::endl;

        program = buffer.str();
    }
    EvaVM vm;
    auto result = vm.exec(program);

    log(result);

    return 0;
}