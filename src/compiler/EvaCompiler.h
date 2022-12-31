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
        co->addConstant(allocator(value));               \
    } while (false)

// Generate binary operator: (+ 1 2) OP_CONST, OP_CONST, OP_ADD
#define GEN_BINARY_OP(op) \
    do                    \
    {                     \
        gen(exp.list[1]); \
        gen(exp.list[2]); \
        emit(op);         \
    } while (false)

// Call a function
// Push function onto the stack:
#define FUNCTION_CALL(exp)                         \
    do                                             \
    {                                              \
        gen(exp.list[0]);                          \
        for (auto i = 1; i < exp.list.size(); i++) \
        {                                          \
            gen(exp.list[i]);                      \
        }                                          \
        emit(OP_CALL);                             \
        emit(exp.list.size() - 1);                 \
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
    void compile(const Exp &exp)
    {
        // Allocate new code object
        co = AS_CODE(createCodeObjectValue("main"));
        main = AS_FUNCTION(ALLOC_FUNCTION(co));

        // Recursively generate from top-level
        gen(exp);

        // Explicitly stop execution
        emit(OP_HALT);
    }

    // Generate bytecode for an expression
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
            // Booleans
            if (exp.string == "true" || exp.string == "false")
            {
                emit(OP_CONST);
                emit(booleanConstIdx(exp.string == "true" ? true : false));
            }
            // Variables
            else
            {
                auto varName = exp.string;
                auto localIndex = co->getLocalIndex(varName);

                if (localIndex != -1)
                {
                    // 1. Local vars
                    emit(OP_GET_LOCAL);
                    emit(localIndex);
                }

                // 2. Global vars
                else
                {
                    if (!global->exists(varName))
                    {
                        DIE << "[EvaCompiler]: Reference error: " << varName << " does not exist, could not get its value." << std::endl;
                    }

                    emit(OP_GET_GLOBAL);
                    emit(global->getGlobalIndex(varName));
                }
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
                // Logical comparison (< a b)
                else if (compareOps_.count(op) != 0)
                {
                    gen(exp.list[1]);
                    gen(exp.list[2]);

                    emit(OP_COMPARE);
                    emit(compareOps_[op]);
                }
                // Branch: (if <test> <consequent> <alternate>)
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
                // While loop: (while <test> <body>)
                else if (op == "while")
                {
                    auto loopStartAddr = getOffset();

                    // Emit <test>
                    gen(exp.list[1]);

                    // Jump to loop end. init with 0 address, will be patched.
                    emit(OP_JMP_IF_FALSE);

                    // Loop start: 2-byte dummy address
                    emit(0);
                    emit(0);

                    // The point in the bytecode where we'll patch the jump
                    // address for loop end
                    auto loopEndJmpAddr = getOffset() - 2;

                    // Emit <body>
                    gen(exp.list[2]);

                    // Jump to start of loop
                    emit(OP_JMP);

                    // Loop end: dummy address
                    emit(0);
                    emit(0);

                    // Finally, patch the addresses. Start address:
                    patchJumpAddress(getOffset() - 2, loopStartAddr);

                    // and end address
                    auto loopEndAddr = getOffset() + 1;
                    patchJumpAddress(loopEndJmpAddr, loopEndAddr);
                }
                // For loop: (for <vardec> <test> <varchange> <body>)
                else if (op == "for")
                {
                    // Declare variable
                    gen(exp.list[1]);

                    // Loop start
                    auto loopStartAddr = getOffset();

                    // Emit <test>
                    gen(exp.list[2]);

                    // Jump to loop end. init with 0 address, will be patched.
                    emit(OP_JMP_IF_FALSE);

                    // Loop start: 2-byte dummy address
                    emit(0);
                    emit(0);

                    // The point in the bytecode where we'll patch the jump
                    // address for loop end
                    auto loopEndJmpAddr = getOffset() - 2;

                    // Emit <varchange>
                    gen(exp.list[3]);

                    // Emit <body>
                    gen(exp.list[4]);

                    // Jump to start of loop
                    emit(OP_JMP);

                    // Loop end: dummy address
                    emit(0);
                    emit(0);

                    // Finally, patch the addresses. Start address:
                    patchJumpAddress(getOffset() - 2, loopStartAddr);

                    // and end address
                    auto loopEndAddr = getOffset() + 1;
                    patchJumpAddress(loopEndJmpAddr, loopEndAddr);
                }
                // Variable declaration: (var x (+ y 10))
                else if (op == "var")
                {
                    auto varName = exp.list[1].string;

                    // Special treatment of (var foo (lambda ...))
                    // To capture function name from variable
                    if (isLambda(exp.list[2]))
                    {
                        compileFunction(
                            /* exp */ exp.list[2],
                            /* name */ exp.list[1].string,
                            /* params */ exp.list[2].list[1],
                            /* body */ exp.list[2].list[2]);
                    }
                    else
                    {
                        // Initializer
                        gen(exp.list[2]);
                    }

                    // 1. Global vars
                    if (isGlobalScope())
                    {
                        global->define(varName);
                        emit(OP_SET_GLOBAL);
                        emit(global->getGlobalIndex(varName));
                    }
                    // 2. Local vars
                    else
                    {
                        co->addLocal(varName);
                        emit(OP_SET_LOCAL);
                        emit(co->getLocalIndex(varName));
                    }

                    break;
                }
                // Set variables: (set x 100)
                else if (op == "set")
                {
                    auto varName = exp.list[1].string;

                    // Set Value on top of stack
                    gen(exp.list[2]);

                    // Check if var is local
                    auto localIndex = co->getLocalIndex(varName);

                    // 1. Local vars
                    if (localIndex != -1)
                    {
                        emit(OP_SET_LOCAL);
                        emit(localIndex);
                    }

                    // 2. Global vars
                    else
                    {
                        auto globalIndex = global->getGlobalIndex(varName);
                        if (globalIndex == -1)
                        {
                            DIE << "[EvaCompiler] Reference error: " << varName << " does not exist, cannot set it." << std::endl;
                        }
                        emit(OP_SET_GLOBAL);
                        emit(globalIndex);
                    }
                }
                // Blocks/environments/scopes/closures (begin <body>)
                else if (op == "begin")
                {
                    scopeEnter();

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        // The value of the last expression is the only value
                        // that should be kept on the stack.
                        bool isLast = i == exp.list.size() - 1;

                        // Local variables are the exception to the above rule
                        bool isLocalDeclaration = isDeclaration(exp.list[i]); //&&!isGlobalScope();

                        // Generate the code for this expression
                        gen(exp.list[i]);

                        if (!isLast && !isLocalDeclaration)
                        {
                            emit(OP_POP);
                        }
                    }

                    scopeExit();
                }
                // User defined functions (def <name> <params> <body>)
                else if (op == "def")
                {
                    // Actually syntactic sugar for
                    // (var <name> (lambda <params> <body>))
                    auto fnName = exp.list[1].string;

                    compileFunction(
                        /* exp */ exp,
                        /* name */ fnName,
                        /* params */ exp.list[2],
                        /* body */ exp.list[3]);

                    // Install the function as a variable
                    if (isGlobalScope())
                    {
                        global->define(fnName);
                        emit(OP_SET_GLOBAL);
                        emit(global->getGlobalIndex(fnName));
                    }
                    else
                    {
                        co->addLocal(fnName);
                        emit(OP_SET_LOCAL);
                        emit(co->getLocalIndex(fnName));
                    }
                }
                // Lambda expressions (lambda <params> <body)
                else if (op == "lambda")
                {
                    compileFunction(
                        /* exp */ exp,
                        /* name */ "lambda",
                        /* params */ exp.list[1],
                        /* body */ exp.list[2]);
                }
                // Named function calls
                else
                {
                    FUNCTION_CALL(exp);
                }
            }
            // Expression is a list but tag is not a symbol
            // Lambda function calls
            else
            {
                FUNCTION_CALL(exp);
            }
            break; // TODO
        }
    }

    /**
     * Disassemble all compilation units
     */
    void disassembleBytecode()
    {
        for (auto &co : codeObjects_)
        {
            disassembler->disassemble(co);
        }
    }

    // Get the compiled (main) function object
    FunctionObject *getMainFunction() { return main; }

private:
    /**
     * Disassembler
     */
    std::unique_ptr<EvaDisassembler> disassembler;

    /**
     * Global vars object
     */
    std::shared_ptr<Global> global;

    /**
     * Compiled code object
     */
    CodeObject *co;

    /**
     * Main entry point (function)
     */
    FunctionObject *main;

    /**
     * All code objects
     */
    std::vector<CodeObject *> codeObjects_;

    /**
     * Comparison operators map
     */
    static std::map<std::string, uint8_t> compareOps_;

    /**
     * Emits bytecode
     */
    void emit(uint8_t code) { co->code.push_back(code); }

    /**
     * Compile a function
     */
    void compileFunction(const Exp &exp, const std::string fnName, const Exp &params, const Exp &body)
    {
        auto arity = params.list.size();

        // Save previous code object:
        auto prevCo = co;

        // Function code object:
        auto coValue = createCodeObjectValue(fnName, arity);
        co = AS_CODE(coValue);

        // Store new co as a constant
        prevCo->addConstant(coValue);

        // Function name is registered as a local constant,
        // so the function can call itself recursively
        co->addLocal(fnName);

        // parameters are added as variables
        for (auto i = 0; i < arity; i++)
        {
            auto argName = params.list[i].string;
            co->addLocal(argName);
        }

        // Compile body in the new code object
        gen(body);

        if (!isBlock(body))
        {
            emit(OP_SCOPE_EXIT);
            emit(arity + 1);
        }

        // Explicit return to restore caller address
        emit(OP_RETURN);

        // Create the function:
        auto fn = ALLOC_FUNCTION(co);

        // Restore the previous code object
        co = prevCo;

        // Add function as a constant to our co
        co->addConstant(fn);

        // And emit code for this new constant:
        emit(OP_CONST);
        emit(co->constants.size() - 1);
    }

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
     * Creates a new code object.
     */
    EvaValue createCodeObjectValue(const std::string &name, size_t arity = 0)
    {
        auto coValue = ALLOC_CODE(name, arity);
        auto co = AS_CODE(coValue);
        codeObjects_.push_back(co);
        return coValue;
    }

    /**
     * Enter a new scope. Increase the scope level
     */
    void scopeEnter() { co->scopeLevel++; }

    /**
     * Exit a scope. Decrease the scope level
     */
    void scopeExit()
    {
        // Pop vars from the stack if they were declared
        // within this specific scope
        auto varsCount = getVarsCountOnScopeExit();

        if (varsCount > 0 || co->arity > 0)
        {
            emit(OP_SCOPE_EXIT);

            // For functions, do caller cleanup: pop all arguments
            // plus the function name from the stack
            if (isFunctionBody())
            {
                varsCount += co->arity + 1;
            }
            emit(varsCount);
        }

        co->scopeLevel--;
    }

    /**
     * Check whether we're at global scope
     */
    bool isGlobalScope() { return co->name == "main" && co->scopeLevel == 1; }

    /**
     * Check if we're in a function body
     */
    bool isFunctionBody() { return co->name != "main" && co->scopeLevel == 1; }

    /**
     * Check if expression is a declaration
     */
    bool isDeclaration(const Exp &exp) { return isVarDeclaration(exp); }

    /**
     * (var <name> <value>)
     */
    bool isVarDeclaration(const Exp &exp) { return isTaggedList(exp, "var"); }

    // Check if Exp is a lambda (lambda ...)
    bool isLambda(const Exp &exp) { return isTaggedList(exp, "lambda"); }

    /**
     * Blocks
     */
    bool isBlock(const Exp &exp)
    {
        return isTaggedList(exp, "begin");
    }

    /**
     * Tagged lists
     */
    bool isTaggedList(const Exp &exp, const std::string &tag)
    {
        return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL &&
               exp.list[0].string == tag;
    }

    /**
     * Number of local variables in this scope
     */
    size_t getVarsCountOnScopeExit()
    {
        auto varsCount = 0;

        if (co->locals.size() > 0)
        {
            while (co->locals.back().scopeLevel == co->scopeLevel)
            {
                co->locals.pop_back();
                varsCount++;
            }
        }

        return varsCount;
    }

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
