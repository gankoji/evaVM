/**
 * Eva Disassembler.
 * Reconstruct textual representation of Eva bytecode
 */

#ifndef EvaDisassembler_h
#define EvaDisassembler_h

#include <iostream>
#include <iomanip>

#include "src/bytecode/OpCode.h"
#include "src/vm/EvaValue.h"
#include "src/vm/Global.h"
#include "src/vm/Logger.h"

class EvaDisassembler
{
public:
    EvaDisassembler(std::shared_ptr<Global> global) : global(global) {}

    /**
     * Disassembles a code unit
     */
    void disassemble(CodeObject *co)
    {
        std::cout << std::endl
                  << "---------- Disassembly: " << co->name
                  << " -----------" << std::endl
                  << std::endl;

        size_t offset = 0;
        while (offset < co->code.size())
        {
            offset = disassembleInstruction(co, offset);
            std::cout << std::endl;
        }

        std::cout << "--------- Disassembly complete ---------" << std::endl;
    }

private:
    /**
     * Global var object
     */
    std::shared_ptr<Global> global;

    /**
     * Disassembles a single instruction/bytecode.
     */
    size_t disassembleInstruction(CodeObject *co, size_t offset)
    {
        std::ios_base::fmtflags f(std::cout.flags());

        // Print bytecode offset:
        std::cout << std::uppercase << std::hex << std::setfill('0') << std::setw(4)
                  << offset << "    ";

        auto opcode = co->code[offset];

        switch (opcode)
        {
        case OP_HALT:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_POP:
            return disassembleSimple(co, opcode, offset);
        case OP_SCOPE_EXIT:
            return disassembleWord(co, opcode, offset);
        case OP_CONST:
            return disassembleConst(co, opcode, offset);
        case OP_COMPARE:
            return disassembleCompare(co, opcode, offset);
        case OP_JMP_IF_FALSE:
        case OP_JMP:
            return disassembleJump(co, opcode, offset);
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
            return disassembleGlobal(co, opcode, offset);
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
            return disassembleLocal(co, opcode, offset);
        default:
            DIE << "disassembleInstruction: no disassembly for "
                << opcodeToString(opcode)
                << std::endl;
        }

        std::cout.flags(f);

        return 0;
    }

    /**
     * Disassembles a simple instruction
     */
    size_t disassembleSimple(CodeObject *co, uint8_t opcode, size_t offset)
    {
        dumpBytes(co, offset, 1);
        printOpCode(opcode);
        return offset + 1;
    }

    /**
     * Disassembles a const instruction
     */
    size_t disassembleWord(CodeObject *co, uint8_t opcode, size_t offset)
    {
        dumpBytes(co, offset, 2);
        printOpCode(opcode);
        std::cout << (int)co->code[offset + 1];
        return offset + 2;
    }

    /**
     * Disassembles a const instruction
     */
    size_t disassembleConst(CodeObject *co, uint8_t opcode, size_t offset)
    {
        dumpBytes(co, offset, 2);
        printOpCode(opcode);
        auto constIndex = co->code[offset + 1];
        std::cout << (int)constIndex << " ("
                  << evaValueToConstantString(co->constants[constIndex]) << ")";
        return offset + 2;
    }

    /**
     * Disassemble instructions to handle global vals
     */
    size_t disassembleGlobal(CodeObject *co, uint8_t opcode, size_t offset)
    {
        dumpBytes(co, offset, 2);
        printOpCode(opcode);
        auto globalIndex = co->code[offset + 1];
        std::cout << (int)globalIndex << " (" << global->get(globalIndex).name
                  << ")";
        return offset + 2;
    }

    /**
     * Disassemble instructions to handle local vals
     */
    size_t disassembleLocal(CodeObject *co, uint8_t opcode, size_t offset)
    {
        dumpBytes(co, offset, 2);
        printOpCode(opcode);
        auto localIndex = co->code[offset + 1];
        std::cout << (int)localIndex << " (" << co->locals[localIndex].name << ")";
        return offset + 2;
    }

    /**
     * Disassembles the compare instruction
     */
    size_t disassembleCompare(CodeObject *co, uint8_t opcode, size_t offset)
    {
        dumpBytes(co, offset, 2);
        printOpCode(OP_COMPARE);
        auto compareOp = co->code[offset + 1];
        std::cout << (int)compareOp << " (";
        std::cout << inverseCompareOps_[compareOp] << ")";
        return offset + 2;
    }

    /**
     * Disassembles jumps
     */
    size_t disassembleJump(CodeObject *co, uint8_t opcode, size_t offset)
    {
        std::ios_base::fmtflags f(std::cout.flags());

        dumpBytes(co, offset, 3);
        printOpCode(opcode);
        uint16_t address = readWordAtOffset(co, offset + 1);

        std::cout << std::uppercase << std::hex << std::setfill('0') << std::setw(4)
                  << (int)address << " ";

        std::cout.flags(f);

        return offset + 3;
    }

    /**
     * Dumps raw memory from the bytecode
     */
    void dumpBytes(CodeObject *co, size_t offset, size_t count)
    {
        std::ios_base::fmtflags f(std::cout.flags());
        std::stringstream ss;
        for (auto i = 0; i < count; i++)
        {
            ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
               << (((int)co->code[offset + i]) & 0xFF) << " ";
        }

        std::cout << std::left << std::setfill(' ') << std::setw(12) << ss.str();
        std::cout.flags(f);
    }

    /**
     * Prints a single opcode
     */
    void printOpCode(uint8_t opcode)
    {
        std::ios_base::fmtflags f(std::cout.flags());
        std::cout << std::left << std::setfill(' ') << std::setw(20)
                  << opcodeToString(opcode) << " ";
        std::cout.flags(f);
    }

    /**
     * Reads a (two byte) word at given offset
     */
    uint16_t readWordAtOffset(CodeObject *co, size_t offset)
    {
        return (uint16_t)((co->code[offset] << 8) | co->code[offset + 1]);
    }
    static std::array<std::string, 6> inverseCompareOps_;
};

std::array<std::string, 6> EvaDisassembler::inverseCompareOps_ = {
    "<",
    ">",
    "==",
    "<=",
    ">=",
    "!=",
};

#endif // EvaDisassembler_h