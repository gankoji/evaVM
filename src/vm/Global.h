/**
 * Global object
 */

#ifndef Global_h
#define Global_h

#include "src/vm/EvaValue.h"

/**
 * Global variables
 */
struct GlobalVar
{
    std::string name;
    EvaValue value;
};

struct Global
{
    /**
     * Returns a global value
     */
    GlobalVar &get(size_t index) { return globals[index]; }

    /**
     * Set a variable by its index
     */
    void set(size_t index, const EvaValue &value)
    {
        if (index >= globals.size())
        {
            DIE << "Global " << index << " doesn't exist.";
        }
        globals[index].value = value;
    }

    /**
     * Get GlobalVar's index by name
     */
    int getGlobalIndex(const std::string &name)
    {
        if (globals.size() > 0)
        {
            for (auto i = (int)globals.size() - 1; i >= 0; i--)
            {
                if (globals[i].name == name)
                {
                    return i;
                }
            }
        }
        return -1;
    }

    /**
     * Registers a global var
     */
    void define(const std::string &name)
    {
        auto index = getGlobalIndex(name);

        if (index != -1)
        {
            // It's already defined
            return;
        }

        // Default value is the number 0
        globals.push_back({name, NUMBER(0)});
    }

    /**
     * Adds a native function
     */
    void addNativeFunction(const std::string &name, std::function<void()> fn, size_t arity)
    {
        if (exists(name))
        {
            return;
        }

        globals.push_back({name, ALLOC_NATIVE(fn, name, arity)});
    }
    /**
     * Adds a global constant
     */
    void addConst(const std::string &name, double value)
    {
        if (exists(name))
        {
            return;
        }
        globals.push_back({name, NUMBER(value)});
    }

    /**
     * Check whether a global var exists
     */
    bool exists(const std::string &name) { return getGlobalIndex(name) != -1; }

    /**
     * Global variables and functions
     */
    std::vector<GlobalVar> globals;
};

#endif // Global_h