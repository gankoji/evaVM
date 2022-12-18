#ifndef __EvaVM_h
#define __EvaVM_h

#include <string>
#include <vector>
#include <array>
#include <memory>

#include "src/bytecode/OpCode.h"
#include "src/vm/Logger.h"
#include "src/vm/EvaValue.h"
#include "src/parser/EvaParser.h"

#define STACK_LIMIT 512

/**
 * Reads the current byte in the bytecode
 * and advances the instruction pointer
*/
#define READ_BYTE() *ip++

/**
 * Gets a constant at the index in pool
 * defined by the next bytecode
*/
#define GET_CONST() constants[READ_BYTE()]

/**
 * Binary operation
*/
#define BINARY_OP(op) \
    do {\
        auto op1 = AS_NUMBER(pop()); \
        auto op2 = AS_NUMBER(pop()); \
        auto result = op1 op op2; \
        push(NUMBER(result)); \
    } while (false)


/**
 * Eva Virtual Machine.
*/
class EvaVM {
    public:
        EvaVM() : parser (std::make_unique<syntax::EvaParser>()) {}

    /**
     * Pushes a value onto the stack
    */
    void push(const EvaValue& value) {
        if ((size_t)(sp - stack.begin()) == STACK_LIMIT) {
            DIE << "push(): Stack overflow.\n";
        }
        *sp = value;
        sp++;
    }

    /**
     * Pops a value from the stack
    */
    EvaValue pop() {
        if (sp == stack.begin()) {
            DIE <<"pop(): empty stack.\n";
        }
        --sp;
        return *sp;
    }

    /**
     * Executes a program
    */
    EvaValue exec(const std::string &program) {
        // 1. Parse the program
        auto ast = parser->parse(program);
        log(program)
        // log(ast)
        ast.print();
        // 2. Compile program to Eva bytecode
        // code = compiler->compile(ast)

        // constants.push_back(NUMBER(100));
        // constants.push_back(NUMBER(42));
        // code = {OP_CONST, 0, OP_CONST, 1, OP_MUL, OP_HALT};

        constants.push_back(ALLOC_STRING("Henlo, "));
        constants.push_back(ALLOC_STRING("world!"));
        code = {OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT};
        // Set instruction pointer to the beginning, sp to top of stack
        ip = &code[0];
        sp = stack.begin();

        return eval();
    }
    
    /**
     * Main eval loop
    */
    EvaValue eval() {
        for (;;) {
            auto opcode = READ_BYTE();
            opcode_pretty(opcode);
            switch (opcode) {
                case OP_HALT:
                    return pop();
                case OP_CONST:
                    {
                        EvaValue constant = GET_CONST();
                        push(constant);
                        break;
                    }
                case OP_ADD:
                    {
                        auto op2 = pop();
                        auto op1 = pop();

                        // Numeric addition:
                        if (IS_NUMBER(op1) && IS_NUMBER(op2)) {
                            auto v1 = AS_NUMBER(op1);
                            auto v2 = AS_NUMBER(op2);
                            push(NUMBER(v1+v2));
                        } else if (IS_STRING(op1) && IS_STRING(op2)) {
                            auto s1 = AS_CPPSTRING(op1);
                            auto s2 = AS_CPPSTRING(op2);
                            push(ALLOC_STRING(s1+s2));
                        }

                        break;
                    }
                case OP_SUB:
                    {
                        BINARY_OP(-);
                        break;
                    }
                case OP_MUL:
                    {
                        BINARY_OP(*);
                        break;
                    }
                case OP_DIV:
                    {
                        BINARY_OP(/);
                        break;
                    }
                default:
                    DIE << "Unknown opcode: " << std::hex << opcode;
            }
        }
    }

    /**
     * Parser
    */
    std::unique_ptr<syntax::EvaParser> parser;

    /**
     * Instruction pointer (aka Program counter)
    */
    uint8_t* ip;
    
    /**
     * Stack pointer
    */
    EvaValue* sp;

    /**
     * Operands stack.
    */
    std::array<EvaValue, STACK_LIMIT> stack;

    /**
     * Bytecode
    */
    std::vector<uint8_t> code;
    
    /**
     * Constants pool
    */
    std::vector<EvaValue> constants;
};

#endif