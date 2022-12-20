/**
 * Eva Compiler
*/
#ifndef __EvaCompiler_h
#define __EvaCompiler_h

#include <map>
#include <string>

#include "src/parser/EvaParser.h"
#include "src/vm/EvaValue.h"
#include "src/bytecode/OpCode.h"
#include "src/vm/Logger.h"

// Allocates new constant in the constant pool
#define ALLOC_CONST(tester, converter, allocator, value)    \
    do {                                                    \
        for (auto i=0; i < co->constants.size(); i++) {     \
            if (!tester(co->constants[i])) {                \
                continue;                                   \
            }                                               \
            if (converter(co->constants[i]) == value) {     \
                return i;                                   \
            }                                               \
        }                                                   \
        co->constants.push_back(allocator(value));          \
    } while (false)

// Generate binary operator: (+ 1 2) OP_CONST, OP_CONST, OP_ADD
#define GEN_BINARY_OP(op)   \
    do {                    \
        gen(exp.list[1]);   \
        gen(exp.list[2]);   \
        emit(op);           \
    } while (false)
/**
 * Compiler class, emits bytecode, records constant pool, vars, etc.
*/
class EvaCompiler {
    public:
        EvaCompiler() {}

        /**
         * Main compile API
        */
        CodeObject* compile(const Exp& exp) {
            // Allocate new code object
            co = AS_CODE(ALLOC_CODE("main"));
            
            // Recursively generate from top-level
            gen(exp);
            
            // Explicitly stop execution
            emit(OP_HALT);
            return co;
        }
        
        void gen(const Exp& exp) {
            switch (exp.type) {
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
                    if (exp.string == "true" || exp.string == "false") {
                        emit(OP_CONST);
                        emit(booleanConstIdx(exp.string == "true" ? true : false));
                    } else {
                        // It's a variable. TODO
                    }
                    break;
                case ExpType::LIST:
                    auto tag = exp.list[0];

                    if (tag.type == ExpType::SYMBOL) {
                        auto op = tag.string;

                        if (op == "+") {
                            GEN_BINARY_OP(OP_ADD);
                        } else if (op == "-") {
                            GEN_BINARY_OP(OP_SUB);
                        } else if (op == "*") {
                            GEN_BINARY_OP(OP_MUL);
                        } else if (op == "/") {
                            GEN_BINARY_OP(OP_DIV);
                        } else if (compareOps_.count(op) != 0) {
                            gen(exp.list[1]);
                            gen(exp.list[2]);
                            
                            emit(OP_COMPARE);
                            emit(compareOps_[op]);
                        }
                    }
                    break; //TODO
            }
        }
        
    private:
        /**
         * Emits bytecode
        */
        void emit(uint8_t code) { co->code.push_back(code); }
        
        /**
         * Allocates a numeric constant
        */
        size_t numericConstIdx(double value) {
            ALLOC_CONST(IS_NUMBER, AS_NUMBER, NUMBER, value);
            return co->constants.size()-1;
        }
        
        /**
         * Allocates a string constant
        */
        size_t stringConstIdx(const std::string& value) {
            ALLOC_CONST(IS_STRING, AS_CPPSTRING, ALLOC_STRING, value);
            return co->constants.size()-1;
        }

        /**
         * Allocates a boolean constant
        */
        size_t booleanConstIdx(const bool value) {
            ALLOC_CONST(IS_BOOLEAN, AS_BOOLEAN, BOOLEAN, value);
            return co->constants.size()-1;
        }

        /**
         * Compiled code object
        */
        CodeObject* co;
        
        /**
         * Comparison operators map
        */
        static std::map<std::string, uint8_t> compareOps_;
};

/**
 * Comparison operators map
*/
std::map<std::string, uint8_t> EvaCompiler::compareOps_ = {
    {"<", 0}, {">", 1}, {"==", 2}, {"<=", 3}, {">=", 4}, {"!=", 5},
};

#endif /* __EvaCompiler_h */
