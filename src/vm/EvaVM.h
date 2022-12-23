/**
 * Eva Bytecode Interpreter
 */

#ifndef EvaVM_h
#define EvaVM_h

#include <string>
#include <vector>
#include <array>
#include <memory>

#include "src/bytecode/OpCode.h"
#include "src/vm/Logger.h"
#include "src/vm/EvaValue.h"
#include "src/vm/Global.h"
#include "src/parser/EvaParser.h"
#include "src/compiler/EvaCompiler.h"

#define STACK_LIMIT 512

/**
 * Reads the current byte in the bytecode
 * and advances the instruction pointer
 */
#define READ_BYTE() *ip++

/**
 * Reads a short word (2 bytes) from bytecode
 */
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

/**
 * Converts bytecode index to a pointer
 */
#define TO_ADDRESS(index) &co->code[index]

/**
 * Gets a constant at the index in pool
 * defined by the next bytecode
 */
#define GET_CONST() co->constants[READ_BYTE()]

/**
 * Binary operation
 */
#define BINARY_OP(op)                \
    do                               \
    {                                \
        auto op2 = AS_NUMBER(pop()); \
        auto op1 = AS_NUMBER(pop()); \
        auto result = op1 op op2;    \
        push(NUMBER(result));        \
    } while (false)

/**
 * Generic comparison operation
 * Rather than testing the numbers here, we could use a map
 * that is the inverse of compareOps_ in EvaCompiler.h
 */
#define COMPARE_VALUES(op, v1, v2)                                     \
    do                                                                 \
    {                                                                  \
        bool res;                                                      \
        switch (op)                                                    \
        {                                                              \
        case 0:                                                        \
            res = v1 < v2;                                             \
            break;                                                     \
        case 1:                                                        \
            res = v1 > v2;                                             \
            break;                                                     \
        case 2:                                                        \
            res = v1 == v2;                                            \
            break;                                                     \
        case 3:                                                        \
            res = v1 <= v2;                                            \
            break;                                                     \
        case 4:                                                        \
            res = v1 >= v2;                                            \
            break;                                                     \
        case 5:                                                        \
            res = v1 != v2;                                            \
            break;                                                     \
        default:                                                       \
            res = false;                                               \
            DIE << "Unknown comparison operator: " << op << std::endl; \
        }                                                              \
        push(BOOLEAN(res));                                            \
    } while (false)

/**
 * Eva Virtual Machine.
 */
class EvaVM
{
public:
    EvaVM()
        : global(std::make_shared<Global>()),
          parser(std::make_unique<syntax::EvaParser>()),
          compiler(std::make_unique<EvaCompiler>(global))
    {
        setGlobalVariables();
    }

    /**
     * Pushes a value onto the stack
     */
    void push(const EvaValue &value)
    {
        if ((size_t)(sp - stack.begin()) == STACK_LIMIT)
        {
            DIE << "push(): Stack overflow.\n";
        }
        *sp = value;
        sp++;
    }

    /**
     * Pops a value from the stack
     */
    EvaValue pop()
    {
        if (sp == stack.begin())
        {
            DIE << "pop(): empty stack.\n";
        }
        --sp;
        return *sp;
    }

    /**
     * Get value from stack without popping
     */
    EvaValue peek(const size_t offset = 0)
    {
        if (stack.size() == 0)
        {
            DIE << "peek(): empty stack." << std::endl;
        }
        return *(sp - 1 - offset);
    }

    /**
     * Executes a program
     */
    EvaValue exec(const std::string &program)
    {
        // 1. Parse the program
        auto ast = parser->parse(program);

        // 2. Compile program to Eva bytecode
        co = compiler->compile(ast);

        // Set instruction pointer to the beginning, sp to top of stack
        ip = &co->code[0];
        sp = stack.begin();

        // Emit the disassembly
        compiler->disassembleBytecode();

        return eval();
    }

    /**
     * Main eval loop
     */
    EvaValue eval()
    {
        for (;;)
        {
            auto opcode = READ_BYTE();
            switch (opcode)
            {
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
                if (IS_NUMBER(op1) && IS_NUMBER(op2))
                {
                    auto v1 = AS_NUMBER(op1);
                    auto v2 = AS_NUMBER(op2);
                    push(NUMBER(v1 + v2));
                }
                else if (IS_STRING(op1) && IS_STRING(op2))
                {
                    auto s1 = AS_CPPSTRING(op1);
                    auto s2 = AS_CPPSTRING(op2);
                    push(ALLOC_STRING(s1 + s2));
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
            case OP_COMPARE:
            {
                auto op = READ_BYTE();

                auto op2 = pop();
                auto op1 = pop();

                if (IS_NUMBER(op1) && IS_NUMBER(op2))
                {
                    auto v1 = AS_NUMBER(op1);
                    auto v2 = AS_NUMBER(op2);
                    COMPARE_VALUES(op, v1, v2);
                }
                else if (IS_STRING(op1) && IS_STRING(op2))
                {
                    auto s1 = AS_CPPSTRING(op1);
                    auto s2 = AS_CPPSTRING(op2);
                    COMPARE_VALUES(op, s1, s2);
                }

                break;
            }
            case OP_JMP_IF_FALSE:
            {
                auto cond = AS_BOOLEAN(pop());
                auto address = READ_SHORT();

                if (!cond)
                {
                    ip = TO_ADDRESS(address);
                }

                break;
            }
            case OP_JMP:
            {
                ip = TO_ADDRESS(READ_SHORT());
                break;
            }
            case OP_GET_GLOBAL:
            {
                auto globalIndex = READ_BYTE();
                push(global->get(globalIndex).value);
                break;
            }
            case OP_SET_GLOBAL:
            {
                auto globalIndex = READ_BYTE();
                auto value = peek(0);
                global->set(globalIndex, value);
                break;
            }
            default:
                printf("Better logging? opcode at fault: %d 0x%.2X\n", opcode, opcode);
                DIE << "Unknown opcode: " << std::hex << opcode << std::dec << opcode;
            }
        }
    }

    /**
     * Sets up global variables and functions
     */
    void setGlobalVariables()
    {
        global->addConst("x", 10);
        global->addConst("y", 20);
    }

    /**
     * Global vars object
     */
    std::shared_ptr<Global> global;

    /**
     * Parser
     */
    std::unique_ptr<syntax::EvaParser> parser;

    /**
     * Compiler
     */
    std::unique_ptr<EvaCompiler> compiler;

    /**
     * Instruction pointer (aka Program counter)
     */
    uint8_t *ip;

    /**
     * Stack pointer
     */
    EvaValue *sp;

    /**
     * Operands stack.
     */
    std::array<EvaValue, STACK_LIMIT> stack;

    /**
     * Code object
     */
    CodeObject *co;
};

#endif