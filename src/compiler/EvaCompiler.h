// Eva Compiler

#ifndef EvaCompiler_h
#define EvaCompiler_h

#include <map>
#include <unordered_map>
#include <string>

#include "src/bytecode/OpCode.h"
#include "src/compiler/Scope.h"
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

// Compiler class, emits bytecode, records constant pool, vars, etc.
class EvaCompiler
{
public:
    EvaCompiler(std::shared_ptr<Global> global) : disassembler(std::make_unique<EvaDisassembler>(global)), global(global) {}

    // Main compile API
    void compile(const Exp &exp)
    {
        // Allocate new code object
        co = AS_CODE(createCodeObjectValue("main"));

        main = AS_FUNCTION(ALLOC_FUNCTION(co));
        constantObjects_.insert((Traceable *)main);

        // Scope analysis.
        analyze(exp, nullptr);

        // Recursively generate from top-level
        gen(exp);

        // Explicitly stop execution
        emit(OP_HALT);
    }

    // Scope analysis
    void analyze(const Exp &exp, std::shared_ptr<Scope> scope)
    {
        if (exp.type == ExpType::SYMBOL)
        {
            if (exp.string == "true" || exp.string == "false")
            {
                // Do nothing
            }
            else
            {
                scope->maybePromote(exp.string);
            }
        }
        else if (exp.type == ExpType::LIST)
        {
            auto tag = exp.list[0];
            // Special cases
            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                // Block scope:
                if (op == "begin")
                {
                    auto newScope = std::make_shared<Scope>(
                        scope == nullptr ? ScopeType::GLOBAL : ScopeType::BLOCK, scope);

                    scopeInfo_[&exp] = newScope;

                    for (auto i = 1; i < exp.list.size(); ++i)
                    {
                        analyze(exp.list[i], newScope);
                    }
                }
                // Variable declaration
                else if (op == "var")
                {
                    scope->addLocal(exp.list[1].string);
                    analyze(exp.list[2], scope);
                }
                // Function declaration
                else if (op == "def")
                {
                    auto fnName = exp.list[1].string;
                    auto arity = exp.list[2].list.size();

                    scope->addLocal(fnName);

                    auto newScope = std::make_shared<Scope>(ScopeType::FUNCTION, scope);
                    scopeInfo_[&exp] = newScope;

                    newScope->addLocal(fnName);

                    // Params
                    for (auto i = 0; i < arity; i++)
                    {
                        newScope->addLocal(exp.list[2].list[i].string);
                    }

                    // Body
                    analyze(exp.list[3], newScope);
                }
                // Lambda functions
                else if (op == "lambda")
                {
                    auto arity = exp.list[1].list.size();

                    auto newScope = std::make_shared<Scope>(ScopeType::FUNCTION, scope);
                    scopeInfo_[&exp] = newScope;

                    // Params
                    for (auto i = 0; i < arity; i++)
                    {
                        newScope->addLocal(exp.list[1].list[i].string);
                    }

                    // Body
                    analyze(exp.list[2], newScope);
                }
                else
                {
                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        analyze(exp.list[i], scope);
                    }
                }
            }
            else
            {
                for (auto i = 1; i < exp.list.size(); i++)
                {
                    analyze(exp.list[i], scope);
                }
            }
        }
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

                // Get the appropriate opcode for this variable based on its scope
                auto opCodeGetter = scopeStack_.top()->getNameGetter(varName);
                emit(opCodeGetter);

                // Check if its local
                if (opCodeGetter == OP_GET_LOCAL)
                {
                    emit(co->getLocalIndex(varName));
                }
                // or if its a cell
                else if (opCodeGetter == OP_GET_CELL)
                {
                    emit(co->getCellIndex(varName));
                }
                // if not, it must be global
                else
                {
                    if (!global->exists(varName))
                    {
                        DIE << "[EvaCompiler]: Reference error: " << varName << " does not exist, could not get its value." << std::endl;
                    }
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
                    auto opCodeSetter = scopeStack_.top()->getNameSetter(varName);

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
                    if (opCodeSetter == OP_SET_GLOBAL)
                    {
                        global->define(varName);
                        emit(OP_SET_GLOBAL);
                        emit(global->getGlobalIndex(varName));
                    }
                    // 2. Cells
                    else if (opCodeSetter == OP_SET_CELL)
                    {
                        co->cellNames.push_back(varName);
                        emit(OP_SET_CELL);
                        emit(co->cellNames.size() - 1);
                        // Explicitly pop the value from the stack,
                        // since it's promoted to the heap:
                        emit(OP_POP);
                    }
                    // 3. Local vars
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
                    auto opCodeSetter = scopeStack_.top()->getNameSetter(varName);

                    // Set Value on top of stack
                    gen(exp.list[2]);

                    // 1. Local vars
                    if (opCodeSetter == OP_SET_LOCAL)
                    {
                        emit(OP_SET_LOCAL);
                        emit(co->getLocalIndex(varName));
                    }
                    // 2. Cell vars
                    else if (opCodeSetter == OP_SET_CELL)
                    {
                        emit(OP_SET_CELL);
                        emit(co->getCellIndex(varName));
                    }
                    // 3. Global vars
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
                    scopeStack_.push(scopeInfo_.at(&exp));
                    blockEnter();

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        // The value of the last expression is the only value
                        // that should be kept on the stack.
                        bool isLast = i == exp.list.size() - 1;

                        // Local variables are the exception to the above rule
                        bool isDecl = isDeclaration(exp.list[i]);

                        // Generate the code for this expression
                        gen(exp.list[i]);

                        if (!isLast && !isDecl && !isGlobalSet(exp.list[i]))
                        {
                            emit(OP_POP);
                        }
                    }

                    blockExit();
                    scopeStack_.pop();
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

    // Disassemble all compilation units
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
    // Disassembler
    std::unique_ptr<EvaDisassembler> disassembler;

    // Global vars object
    std::shared_ptr<Global> global;

    // Scope info
    std::map<const Exp *, std::shared_ptr<Scope>> scopeInfo_;

    // Scopes stack
    std::stack<std::shared_ptr<Scope>> scopeStack_;

    // Compiled code object
    CodeObject *co;

    // Main entry point (function)
    FunctionObject *main;

    // All code objects
    std::vector<CodeObject *> codeObjects_;

    // GC Roots (things that should live as long as the VM)
    std::set<Traceable *> constantObjects_;

    // Comparison operators map
    static std::map<std::string, uint8_t> compareOps_;

    // Emits bytecode
    void emit(uint8_t code) { co->code.push_back(code); }

    // Compile a function
    void compileFunction(const Exp &exp, const std::string fnName, const Exp &params, const Exp &body)
    {
        auto scopeInfo = scopeInfo_.at(&exp);
        scopeStack_.push(scopeInfo);

        auto arity = params.list.size();

        // Save previous code object:
        auto prevCo = co;

        // Function code object:
        auto coValue = createCodeObjectValue(fnName, arity);
        co = AS_CODE(coValue);

        // Put 'free' and 'cells' from the scope into the
        // cellNames of the code object.
        co->freeCount = scopeInfo->free.size();
        co->cellNames.reserve(scopeInfo->free.size() + scopeInfo->cell.size());
        co->cellNames.insert(co->cellNames.end(), scopeInfo->free.begin(),
                             scopeInfo->free.end());
        co->cellNames.insert(co->cellNames.end(), scopeInfo->cell.begin(),
                             scopeInfo->cell.end());

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

            // NOTE: if the param is captured by a cell, emit the code
            // for it. We also don't pop the param value in this case,
            // since OP_SCOPE_EXIT would pop it.
            auto cellIndex = co->getCellIndex(argName);
            if (cellIndex != -1)
            {
                emit(OP_SET_CELL);
                emit(cellIndex);
            }
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

        // 1. Simple functions (allocated at compile time):
        // If it's not a closure (i.e. this function doesn't
        // have free variables), allocate it at compile time
        // and store as a constant. Closures are allocated at
        // runtime, but reuse the same code object.
        if (scopeInfo->free.size() == 0)
        {
            // Create the function:
            auto fn = ALLOC_FUNCTION(co);
            constantObjects_.insert((Traceable *)AS_OBJECT(fn));

            // Restore the previous code object
            co = prevCo;

            // Add function as a constant to our co
            co->addConstant(fn);

            // And emit code for this new constant:
            emit(OP_CONST);
            emit(co->constants.size() - 1);
        }
        // 2. Closures
        // 2.1 Load all free vars to capture (indices are taken
        // from the cells of the parent co)
        // 2.2 Load code object for the current function
        // 2.3 Make function
        else
        {
            // Restore the code object
            co = prevCo;

            for (const auto &freeVar : scopeInfo->free)
            {
                emit(OP_LOAD_CELL);
                emit(prevCo->getCellIndex(freeVar));
            }

            // Load code object
            emit(OP_CONST);
            emit(co->constants.size() - 1);

            // Create the function
            emit(OP_MAKE_FUNCTION);

            // How many cells to capture:
            emit(scopeInfo->free.size());
        }
        scopeStack_.pop();
    }

    // Allocates a numeric constant
    size_t numericConstIdx(double value)
    {
        ALLOC_CONST(IS_NUMBER, AS_NUMBER, NUMBER, value);
        return co->constants.size() - 1;
    }

    // Allocates a string constant
    size_t stringConstIdx(const std::string &value)
    {
        ALLOC_CONST(IS_STRING, AS_CPPSTRING, ALLOC_STRING, value);
        return co->constants.size() - 1;
    }

    // Allocates a boolean constant
    size_t booleanConstIdx(const bool value)
    {
        ALLOC_CONST(IS_BOOLEAN, AS_BOOLEAN, BOOLEAN, value);
        return co->constants.size() - 1;
    }

    // Writes byte at offset in code object
    void writeByteAtOffset(size_t offset, uint8_t value)
    {
        co->code[offset] = value;
    }

    // Patches jump addresses for branching. Implicitly assumes that
    // addresses are two bytes long.
    void patchJumpAddress(size_t offset, uint16_t value)
    {
        writeByteAtOffset(offset, (value >> 8) & 0xFF);
        writeByteAtOffset(offset + 1, value & 0xFF);
    }

    // Creates a new code object.
    EvaValue createCodeObjectValue(const std::string &name, size_t arity = 0)
    {
        auto coValue = ALLOC_CODE(name, arity);
        auto co = AS_CODE(coValue);
        codeObjects_.push_back(co);
        constantObjects_.insert((Traceable *)co);
        return coValue;
    }

    // Get all GC Roots
    std::set<Traceable *> &getConstantObjects() { return constantObjects_; }

    // Enter a new block. Increase the scope level
    void blockEnter() { co->scopeLevel++; }

    // Exit a block. Decrease the scope level
    void blockExit()
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

    // Check whether we're at global scope
    bool isGlobalScope() { return co->name == "main" && co->scopeLevel == 1; }

    // Check if we're in a function body
    bool isFunctionBody() { return co->name != "main" && co->scopeLevel == 1; }

    // Check if expression is a declaration
    bool isDeclaration(const Exp &exp) { return isVarDeclaration(exp); }

    // (var <name> <value>)
    bool isVarDeclaration(const Exp &exp) { return isTaggedList(exp, "var"); }

    // Check if Exp is a lambda (lambda ...)
    bool isLambda(const Exp &exp) { return isTaggedList(exp, "lambda"); }

    bool isGlobalSet(const Exp &exp) { return isTaggedList(exp, "set") && co->scopeLevel == 1; }
    // Blocks
    bool isBlock(const Exp &exp)
    {
        return isTaggedList(exp, "begin");
    }

    // Tagged lists
    bool isTaggedList(const Exp &exp, const std::string &tag)
    {
        return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL &&
               exp.list[0].string == tag;
    }

    // Number of local variables in this scope
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

    // Returns current bytecode offset.
    uint16_t getOffset() { return (uint16_t)co->code.size(); }
};

// Comparison operators map
std::map<std::string, uint8_t> EvaCompiler::compareOps_ = {
    {"<", 0},
    {">", 1},
    {"==", 2},
    {"<=", 3},
    {">=", 4},
    {"!=", 5},
};

#endif /* __EvaCompiler_h */
