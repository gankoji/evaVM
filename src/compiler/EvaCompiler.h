/**
 * Eva Compiler
 */

#ifndef EvaCompiler_h
#define EvaCompiler_h

#include <map>
#include <string>

#include "src/bytecode/OpCode.h"
#include "src/disassembler/EvaDisassembler.h"
#include "src/parser/EvaParser.h"
#include "src/vm/EvaValue.h"
#include "src/vm/Logger.h"
#include "src/vm/Global.h"

// Allocates new constant in the constant pool
#define ALLOC_CONST(tester, converter, allocator, value) \
    do                                                   \
    {                                                    \
        for (auto i = 0; i < co->constants.size(); i++)  \
        {                                                \
            if (!tester(co->constants[i]))               \
            {                                            \
                continue;                                \
            }                                            \
            if (converter(co->constants[i]) == value)    \
            {                                            \
                return i;                                \
            }                                            \
        }                                                \
        co->constants.push_back(allocator(value));       \
    } while (false)

// Generate binary operator: (+ 1 2) OP_CONST, OP_CONST, OP_ADD
#define GEN_BINARY_OP(op) \
    do                    \
    {                     \
        gen(exp.list[1]); \
        gen(exp.list[2]); \
        emit(op);         \
    } while (false)
/**
 * Compiler class, emits bytecode, records constant pool, vars, etc.
 */
class EvaCompiler
{
public:
    EvaCompiler(std::shared_ptr<Global> global) : disassembler(std::make_unique<EvaDisassembler>(global)), global(global) {}

    /**
     * Main compile API
     */
    CodeObject *compile(const Exp &exp)
    {
        // Allocate new code object
        co = AS_CODE(ALLOC_CODE("main"));

        // Recursively generate from top-level
        gen(exp);

        // Explicitly stop execution
        emit(OP_HALT);
        return co;
    }

    void gen(const Exp &exp)
    {
        switch (exp.type)
        {
        case ExpType::NUMBER:
            emit(OP_CONST);
            emit(numericConstIdx(exp.number));
            break;
        case ExpType::STRING:
            emit(OP_CONST);
            emit(stringConstIdx(exp.string));
            break;
        case ExpType::SYMBOL:
            /**
             * Boolean
             */
            if (exp.string == "true" || exp.string == "false")
            {
                emit(OP_CONST);
                emit(booleanConstIdx(exp.string == "true" ? true : false));
            }
            else
            {
                // Variables
                // 1. Global vars
                if (!global->exists(exp.string))
                {
                    DIE << "[EvaCompiler]: Reference error: " << exp.string;
                }

                emit(OP_GET_GLOBAL);
                emit(global->getGlobalIndex(exp.string));
            }
            break;
        case ExpType::LIST:
            auto tag = exp.list[0];

            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                if (op == "+")
                {
                    GEN_BINARY_OP(OP_ADD);
                }
                else if (op == "-")
                {
                    GEN_BINARY_OP(OP_SUB);
                }
                else if (op == "*")
                {
                    GEN_BINARY_OP(OP_MUL);
                }
                else if (op == "/")
                {
                    GEN_BINARY_OP(OP_DIV);
                }
                else if (compareOps_.count(op) != 0)
                {
                    gen(exp.list[1]);
                    gen(exp.list[2]);

                    emit(OP_COMPARE);
                    emit(compareOps_[op]);
                }

                // Branch instruction
                // (if <test> <consequent> <alternate>)
                else if (op == "if")
                {
                    // Emit <test>
                    gen(exp.list[1]);
                    emit(OP_JMP_IF_FALSE);

                    // Else branch. Init with 0 address, will be
                    // patched.
                    emit(0);
                    emit(0);

                    auto elseJmpAddr = getOffset() - 2;

                    // Emit <consequent>
                    gen(exp.list[2]);
                    emit(OP_JMP);

                    // 2-byte address
                    emit(0);
                    emit(0);
                    auto endAddr = getOffset() - 2;

                    // Path the else branch address
                    auto elseBranchAddr = getOffset();
                    patchJumpAddress(elseJmpAddr, elseBranchAddr);

                    // Emit <alternate> if we have it
                    if (exp.list.size() == 4)
                    {
                        gen(exp.list[3]);
                    }

                    // Patch the end.
                    auto endBranchAddr = getOffset();
                    patchJumpAddress(endAddr, endBranchAddr);
                }

                // Variable declaration: (var x (+ y 10))
                else if (op == "var")
                {
                    // 1. Global vars
                    global->define(exp.list[1].string);

                    // Initializer
                    gen(exp.list[2]);
                    emit(OP_SET_GLOBAL);
                    emit(global->getGlobalIndex(exp.list[1].string));

                    // 2. Local vars
                }

                // Set variables: (set x 100)
                else if (op == "set")
                {
                    // 1. Global vars
                    auto varName = exp.list[1].string;

                    // Set Value on top of stack
                    gen(exp.list[2]);

                    if (!global->exists(varName))
                    {
                        DIE << "Reference error: " << varName << " is not defined." << std::endl;
                    }
                    auto globalIndex = global->getGlobalIndex(varName);
                    emit(OP_SET_GLOBAL);
                    emit(globalIndex);

                    // 2. Local vars (TODO)
                }
                else if (op == "begin")
                {
                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        bool isLast = i == exp.list.size() - 1;
                        gen(exp.list[i]);

                        if (!isLast)
                        {
                            emit(OP_POP);
                        }
                    }
                }
            }
            break; // TODO
        }
    }

    /**
     * Disassemble all compilation units
     */
    void disassembleBytecode()
    {
        disassembler->disassemble(co);
    }

private:
    /**
     * Disassembler
     */
    std::unique_ptr<EvaDisassembler> disassembler;

    /**
     * Emits bytecode
     */
    void emit(uint8_t code) { co->code.push_back(code); }

    /**
     * Allocates a numeric constant
     */
    size_t numericConstIdx(double value)
    {
        ALLOC_CONST(IS_NUMBER, AS_NUMBER, NUMBER, value);
        return co->constants.size() - 1;
    }

    /**
     * Allocates a string constant
     */
    size_t stringConstIdx(const std::string &value)
    {
        ALLOC_CONST(IS_STRING, AS_CPPSTRING, ALLOC_STRING, value);
        return co->constants.size() - 1;
    }

    /**
     * Allocates a boolean constant
     */
    size_t booleanConstIdx(const bool value)
    {
        ALLOC_CONST(IS_BOOLEAN, AS_BOOLEAN, BOOLEAN, value);
        return co->constants.size() - 1;
    }

    /**
     * Writes byte at offset in code object
     */
    void writeByteAtOffset(size_t offset, uint8_t value)
    {
        co->code[offset] = value;
    }

    /**
     * Patches jump addresses for branching. Implicitly assumes that
     * addresses are two bytes long.
     */
    void patchJumpAddress(size_t offset, uint16_t value)
    {
        writeByteAtOffset(offset, (value >> 8) & 0xFF);
        writeByteAtOffset(offset + 1, value & 0xFF);
    }

    /**
     * Global vars object
     */
    std::shared_ptr<Global> global;

    /**
     * Compiled code object
     */
    CodeObject *co;

    /**
     * Comparison operators map
     */
    static std::map<std::string, uint8_t> compareOps_;

    /**
     * Returns current bytecode offset.
     */
    uint16_t getOffset() { return (uint16_t)co->code.size(); }
};

/**
 * Comparison operators map
 */
std::map<std::string, uint8_t> EvaCompiler::compareOps_ = {
    {"<", 0},
    {">", 1},
    {"==", 2},
    {"<=", 3},
    {">=", 4},
    {"!=", 5},
};

#endif /* __EvaCompiler_h */
