/**
 * Scope Analysis
 * Parser 2nd Pass
 */

#ifndef Scope_h
#define Scope_h

#include <map>
#include <set>
#include "src/bytecode/OpCode.h"

// Scope type.
enum class ScopeType
{
    GLOBAL,
    FUNCTION,
    BLOCK,
};

// Allocation type
enum class AllocType
{
    GLOBAL,
    LOCAL,
    CELL,
};

// Scope analysis structure
struct Scope
{
    Scope(ScopeType type, std::shared_ptr<Scope> parent)
        : type(type), parent(parent) {}

    // Register a local variable
    void addLocal(const std::string &name)
    {
        allocInfo[name] = type == ScopeType::GLOBAL ? AllocType::GLOBAL : AllocType::LOCAL;
    }

    // Registers/promotes to a cell
    void addCell(const std::string &name)
    {
        cell.insert(name);
        allocInfo[name] = AllocType::CELL;
    }

    // Registers a free variable
    void addFree(const std::string &name)
    {
        free.insert(name);
        allocInfo[name] = AllocType::CELL;
    }

    // Promote a free var to a cell
    void maybePromote(const std::string &name)
    {
        auto initAllocType = type == ScopeType::GLOBAL ? AllocType::GLOBAL : AllocType::LOCAL;

        if (allocInfo.count(name) != 0)
        {
            initAllocType = allocInfo[name];
        }

        auto [ownerScope, allocType] = resolve(name, initAllocType);

        // Update the alloc type based on resolution
        allocInfo[name] = allocType;

        // If we resolve it as a cell, promote to the heap:
        if (allocType == AllocType::CELL)
        {
            promote(name, ownerScope);
        }
    }

    // Resoves a variable in the scope chain
    std::pair<Scope *, AllocType> resolve(const std::string &name, AllocType allocType)
    {
        // Initially a variable is treated as a local. However, if during the
        // resolution we pass the own-function boundary, it has to be free, and
        // hence should be promoted to a cell (unless its actually global).

        // Found in current scope:
        if (allocInfo.count(name) != 0)
        {
            return std::make_pair(this, allocType);
        }

        // We crossed the boundary of the function and still didn't resolve the
        // variable. It has to be free.
        if (type == ScopeType::FUNCTION)
        {
            allocType = AllocType::CELL;
        }

        // We crossed the boundary but there are no more scopes to check.
        if (parent == nullptr)
        {
            DIE << "[Scope] Reference error: " << name << " is not defined.";
        }

        // If parent's scope is global, that's what the resolution is too.
        if (parent->type == ScopeType::GLOBAL)
        {
            allocType = AllocType::GLOBAL;
        }

        // Recursively resolve in the parent scope.
        return parent->resolve(name, allocType);
    }

    // Determine which get opcode should be emitted
    int getNameGetter(const std::string &name)
    {
        switch (allocInfo[name])
        {
        case AllocType::GLOBAL:
            return OP_GET_GLOBAL;
        case AllocType::LOCAL:
            return OP_GET_LOCAL;
        case AllocType::CELL:
            return OP_GET_CELL;
        default:
            DIE << "[Scope] Invalid allocType for var " << name << ". Cannot proceed." << std::endl;
        }
    }

    // Determine which set opcode should be emitted
    int getNameSetter(const std::string &name)
    {
        switch (allocInfo[name])
        {
        case AllocType::GLOBAL:
            return OP_SET_GLOBAL;
        case AllocType::LOCAL:
            return OP_SET_LOCAL;
        case AllocType::CELL:
            return OP_SET_CELL;
        default:
            DIE << "[Scope] Invalid allocType for var " << name << ". Cannot proceed with emitting a set opcode." << std::endl;
        }
    }

    // Promote a variable from local (stack) to cell (heap).
    void promote(const std::string &name, Scope *ownerScope)
    {
        ownerScope->addCell(name);

        // Thread the variable as free in all parent scopes so it's propagated
        // down to the scope where it's needed.
        auto scope = this;
        while (scope != ownerScope)
        {
            scope->addFree(name);
            scope = scope->parent.get();
        }
    }

    ScopeType type;
    std::shared_ptr<Scope> parent;
    std::map<std::string, AllocType> allocInfo;
    std::set<std::string> free;
    std::set<std::string> cell;
};

#endif // Scope_h