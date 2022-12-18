/**
 * Eva Compiler
*/
#ifndef __EvaCompiler_h
#define __EvaCompiler_h

#include "src/parser/EvaParser.h"
#include "src/vm/EvaValue.h"
#include "src/bytecode/OpCode.h"

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
                case ExpType::LIST:
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
            for (auto i=0; i < co->constants.size(); i++) {
                if (!IS_NUMBER(co->constants[i])) {
                    continue;
                }
                if (AS_NUMBER(co->constants[i]) == value) {
                    return i;
                }
            }
            co->constants.push_back(NUMBER(value));
            return co->constants.size()-1;
        }
        
        /**
         * Allocates a string constant
        */
        size_t stringConstIdx(const std::string& value) {
            for (auto i=0; i < co->constants.size(); i++) {
                if (!IS_STRING(co->constants[i])) {
                    continue;
                }
                if (AS_CPPSTRING(co->constants[i]) == value) {
                    return i;
                }
            }
            co->constants.push_back(ALLOC_STRING(value));
            return co->constants.size()-1;
        }

        /**
         * Compiled code object
        */
        CodeObject* co;
};

#endif /* __EvaCompiler_h */
