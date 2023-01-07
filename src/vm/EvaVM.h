// Eva Bytecode Interpreter

#ifndef EvaVM_h
#define EvaVM_h

#include <string>
#include <vector>
#include <array>
#include <memory>

#include "src/bytecode/OpCode.h"
#include "src/compiler/EvaCompiler.h"
#include "src/gc/EvaCollector.h"
#include "src/parser/EvaParser.h"
#include "src/vm/EvaValue.h"
#include "src/vm/Global.h"
#include "src/vm/Logger.h"

#define STACK_LIMIT 512
#define GC_THRESHOLD 1024

// Reads the current byte in the bytecode
// and advances the instruction pointer
#define READ_BYTE() *ip++

// Reads a short word (2 bytes) from bytecode
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

// Converts bytecode index to a pointer
#define TO_ADDRESS(index) &fn->co->code[index]

// Gets a constant at the index in pool
// defined by the next bytecode
#define GET_CONST() fn->co->constants[READ_BYTE()]

// Binary operation
#define BINARY_OP(op)                \
    do                               \
    {                                \
        auto op2 = AS_NUMBER(pop()); \
        auto op1 = AS_NUMBER(pop()); \
        auto result = op1 op op2;    \
        push(NUMBER(result));        \
    } while (false)

// Generic comparison operation
// Rather than testing the numbers here, we could use a map
// that is the inverse of compareOps_ in EvaCompiler.h
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

// Runtime allocation of memory, can call GC
#define MEM(allocator, ...) (maybeGC(), allocator(__VA_ARGS__))

// Stack frame for function calls.
struct Frame
{
    uint8_t *ra;        // Return address
    EvaValue *bp;       // Base pointer (stack frame)
    FunctionObject *fn; // Currently running function/code object/block
};

// Eva Virtual Machine.
class EvaVM
{
public:
    EvaVM()
        : global(std::make_shared<Global>()),
          parser(std::make_unique<syntax::EvaParser>()),
          compiler(std::make_unique<EvaCompiler>(global)),
          collector(std::make_unique<EvaCollector>())
    {
        setGlobalVariables();
    }

    ~EvaVM()
    {
        Traceable::cleanup();
    }

    // Pushes a value onto the stack
    void push(const EvaValue &value)
    {
        if ((size_t)(sp - stack.begin()) == STACK_LIMIT)
        {
            DIE << "push(): Stack overflow.\n";
        }
        *sp = value;
        sp++;
    }

    // Pops a value from the stack
    EvaValue pop()
    {
        if (sp == stack.begin() || stack.size() == 0)
        {
            DIE << "pop(): empty stack.\n";
        }
        if ((size_t)(sp - stack.begin()) > STACK_LIMIT)
        {
            DIE << "pop(): Stack pointer corrupted.\n";
        }
        --sp;
        auto result = *sp;
        return result;
    }

    // Pops N values from the stack
    void popN(size_t count)
    {
        if (stack.size() == 0)
        {
            DIE << "popN(): empty stack." << std::endl;
        }
        if (count > stack.size())
        {
            DIE << "popN(): count greater than stack size." << std::endl;
        }
        sp -= count;
    }

    // Get value from stack without popping
    EvaValue peek(const size_t offset = 0)
    {
        if (stack.size() == 0 || sp == stack.begin())
        {
            DIE << "peek(): empty stack." << std::endl;
        }
        return *(sp - 1 - offset);
    }

    // GC Operations
    void maybeGC()
    {
        if (Traceable::bytesAllocated < GC_THRESHOLD)
            return;

        auto roots = getGCRoots();
        if (roots.size() == 0)
            return;

        std::cout << "Before GC Stats" << std::endl;
        Traceable::printStats();
        collector->gc(roots);
        std::cout << "After GC Stats" << std::endl;
        Traceable::printStats();
    }

    // Roots are references that live the same length as the VM
    std::set<Traceable *> getGCRoots()
    {
        // Stack
        auto roots = getStackGCRoots();

        // Constant pool
        auto constantRoots = getConstantGCRoots();
        roots.insert(constantRoots.begin(), constantRoots.end());

        // Global poolt
        auto globalRoots = getGlobalGCRoots();
        roots.insert(globalRoots.begin(), globalRoots.end());

        return roots;
    }

    // Stack roots
    std::set<Traceable *> getStackGCRoots()
    {
        std::set<Traceable *> roots;
        auto stackEntry = sp;
        while (stackEntry-- != stack.begin())
        {
            if (IS_OBJECT(*stackEntry))
            {
                roots.insert((Traceable *)stackEntry->object);
            }
        }
        return roots;
    }

    // Get constants
    std::set<Traceable *> getConstantGCRoots()
    {
        return compiler->getConstantObjects();
    }

    // Global roots
    std::set<Traceable *> getGlobalGCRoots()
    {
        std::set<Traceable *> roots;
        for (const auto &global : global->globals)
        {
            if (IS_OBJECT(global.value))
            {
                roots.insert((Traceable *)global.value.object);
            }
        }

        return roots;
    }

    // Executes a program
    EvaValue exec(const std::string &program)
    {
        // 1. Parse the program
        auto ast = parser->parse("(begin " + program + ")");

        // 2. Compile program to Eva bytecode
        compiler->compile(ast);
        fn = compiler->getMainFunction();

        // Set instruction pointer to the beginning, sp to top of stack
        ip = &fn->co->code[0];
        sp = &stack[0];
        bp = sp;

        // Emit the disassembly
        compiler->disassembleBytecode();

        return eval();
    }

    // Main eval loop
    EvaValue eval()
    {
        for (;;)
        {
            auto opcode = READ_BYTE();
            // opcode_pretty(opcode);
            // dumpStack();
            switch (opcode)
            {
            case OP_HALT:
            {
                auto result = pop();
                return result;
            }
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
                    push(MEM(ALLOC_STRING, s1 + s2));
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
            case OP_POP:
            {
                pop();
                break;
            }
            case OP_GET_LOCAL:
            {
                auto localIndex = READ_BYTE();
                if (localIndex < 0 || localIndex >= stack.size())
                {
                    DIE << "OP_GET_LOCAL: invalid variable index: " << (int)localIndex;
                }
                push(bp[localIndex]);
                break;
            }
            case OP_SET_LOCAL:
            {
                auto localIndex = READ_BYTE();
                auto value = peek(0);
                if (localIndex < 0 || localIndex >= stack.size())
                {
                    DIE << "OP_SET_LOCAL: invalid variable index: " << (int)localIndex;
                }
                bp[localIndex] = value;
                break;
            }
            case OP_SCOPE_EXIT:
            {
                auto count = READ_BYTE();

                // Simple operation: value on top of stack is the result of the
                // block. We need to put it back after popping all local vars
                auto result = pop();
                popN(count);
                push(result);
                break;
            }
            case OP_CALL:
            {
                auto argsCount = READ_BYTE();
                auto fnValue = peek(argsCount);

                // Native function
                if (IS_NATIVE(fnValue))
                {
                    AS_NATIVE(fnValue)->function();
                    auto result = pop();

                    // Pop args and function:
                    popN(argsCount + 1);

                    // Put result back on top of the stack
                    push(result);

                    break;
                }

                // User defined function
                auto callee = AS_FUNCTION(fnValue);

                // Need to save state of machine to restore after call
                callStack.push(Frame{ip, bp, fn});

                // Now set the machine state to the new function
                fn = callee;                         // Access local values for the function
                fn->cells.resize(fn->co->freeCount); // Shrink cells vector to the size of *only* free vars
                bp = sp - argsCount - 1;             // Base (frame) pointer for the call
                ip = &callee->co->code[0];           // Jumps to the function code

                break;
            }
            case OP_RETURN:
            {
                // Get the machine state we're restoring
                auto callerFrame = callStack.top();

                ip = callerFrame.ra; // Jump back to the caller's code
                bp = callerFrame.bp; // Restore the operand stack
                fn = callerFrame.fn; // And restore local variables

                callStack.pop();
                break;
            }
            case OP_GET_CELL:
            {
                auto cellIndex = READ_BYTE();
                push(fn->cells[cellIndex]->value);
                break;
            }
            case OP_SET_CELL:
            {
                auto cellIndex = READ_BYTE();
                auto value = peek(0);

                if (fn->cells.size() <= cellIndex)
                {
                    // Allocate the cell if it doesn't yet exist
                    fn->cells.push_back(AS_CELL(MEM(ALLOC_CELL, value)));
                }
                else
                {
                    // Update the cell
                    fn->cells[cellIndex]->value = value;
                }
                break;
            }
            case OP_LOAD_CELL:
            {
                auto cellIndex = READ_BYTE();
                push(CELL(fn->cells[cellIndex]));
                break;
            }
            case OP_MAKE_FUNCTION:
            {
                auto co = AS_CODE(pop());
                auto cellsCount = READ_BYTE();

                auto fnValue = MEM(ALLOC_FUNCTION, co);
                auto fn = AS_FUNCTION(fnValue);

                for (auto i = 0; i < cellsCount; i++)
                {
                    fn->cells.push_back(AS_CELL(pop()));
                }

                push(fnValue);
                break;
            }
            case OP_NEW:
            {
                auto classObject = AS_CLASS(pop());
                auto instance = MEM(ALLOC_INSTANCE, classObject);

                // Push the constructor
                auto ctorValue = classObject->getProp("constructor");
                push(ctorValue);

                // And the instance we've created
                push(instance);

                // NOTE: the code for constructor parameters is
                // generated at compile time, followed by OP_CALL

                break;
            }
            case OP_GET_PROP:
            {
                auto prop = AS_CPPSTRING(GET_CONST());
                auto object = pop();
                if (IS_INSTANCE(object))
                    push(AS_INSTANCE(object)->getProp(prop));
                else if (IS_CLASS(object))
                    push(AS_CLASS(object)->getProp(prop));
                else
                    DIE << "[EvaVM]: Unknown object for OP_GET_PROP " << prop;

                break;
            }
            case OP_SET_PROP:
            {
                auto prop = AS_CPPSTRING(GET_CONST());
                auto instance = AS_INSTANCE(pop()); // TODO: add classes
                auto value = pop();
                push(instance->properties[prop] = value);
                break;
            }
            default:
                opcode_pretty(opcode);
                DIE << "Unknown opcode: " << std::hex << opcode << std::dec << opcode;
            }
        }
    }

    // Sets up global variables and functions
    void setGlobalVariables()
    {
        // Native square function
        global->addNativeFunction(
            "square",
            [&]()
            {
                auto x = AS_NUMBER(peek(0));
                push(NUMBER(x * x));
            },
            1);

        // Native sum function
        global->addNativeFunction(
            "sum",
            [&]()
            {
                auto v2 = AS_NUMBER(peek(0));
                auto v1 = AS_NUMBER(peek(1));
                push(NUMBER(v1 + v2));
            },
            2);
        global->addConst("x", 10);
        global->addConst("y", 20);
    }

    // Global vars object
    std::shared_ptr<Global> global;

    // Parser
    std::unique_ptr<syntax::EvaParser> parser;

    // Compiler
    std::unique_ptr<EvaCompiler> compiler;

    // Garbage collector
    std::unique_ptr<EvaCollector> collector;

    // Instruction pointer (aka Program counter)
    uint8_t *ip;

    // Stack pointer
    EvaValue *sp;

    // Base (stack frame) pointer
    EvaValue *bp;

    // Operands stack.
    std::array<EvaValue, STACK_LIMIT> stack;

    // Call stack
    std::stack<Frame> callStack;

    // Code object
    FunctionObject *fn;

    //--------------------------------------
    // Debug functions

    // Dumps the current stack
    void dumpStack()
    {
        std::cout << "\n------- Stack --------\n";
        if (sp == stack.begin())
        {
            std::cout << "(empty)";
        }
        auto csp = sp - 1;
        while (csp >= stack.begin())
        {
            std::cout << *csp-- << std::endl;
        }
        std::cout << std::endl;
    }
};

#endif