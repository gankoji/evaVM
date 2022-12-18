/**
* Logger and error reporter
*/
#ifndef __Logger_h
#define __Logger_h

#include <sstream>

class ErrorLogMessage : public std::basic_ostringstream<char> {
    public:
        ~ErrorLogMessage() {
            fprintf(stderr, "Fatal error: %s\n", str().c_str());
            exit(EXIT_FAILURE);
        }
};

#define DIE ErrorLogMessage()

#define log(value) std::cout << #value << " = " << (value) << "\n";
#define opcode_log(value) std::cout << #value << " = " << std::hex << static_cast<int16_t>(value) << std::endl;
#define opcode_pretty(value) printf("Opcode: 0x%.2X\n", value);

#endif /* __Logger_h */
