/**
 * Logger and error reporter
 */

#ifndef Logger_h
#define Logger_h

#include <sstream>

class ErrorLogMessage : public std::basic_ostringstream<char>
{
public:
    ~ErrorLogMessage()
    {
        fprintf(stderr, "Fatal error: %s\n", str().c_str());
        exit(EXIT_FAILURE);
    }
};

#define DIE ErrorLogMessage()

#define log(value) std::cout << #value << " = " << (value) << "\n";
#define opcode_pretty(value) printf("Prettified opcode: %d 0x%.2X\n", opcode, opcode)

#endif /* __Logger_h */
