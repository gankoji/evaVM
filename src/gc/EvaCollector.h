// EvaCollector - Garbage Collector for the EvaVM
#ifndef EvaCollector_h
#define EvaCollector_h

#include <set>
// Mark-Sweep garbage collector
struct EvaCollector
{
    void gc(const std::set<Traceable *> &roots)
    {
        mark(roots);
        sweep();
    }

    void mark(const std::set<Traceable *> &roots)
    {
        std::vector<Traceable *> worklist(roots.begin(), roots.end());

        while (!worklist.empty())
        {
            auto object = worklist.back();
            worklist.pop_back();

            if (!object->marked)
            {
                object->marked = true;
                for (auto &p : getPointers(object))
                    worklist.push_back(p);
            }
        }
    }

    std::set<Traceable *> getPointers(const Traceable *object)
    {
        std::set<Traceable *> pointers;

        auto evaValue = OBJECT((Object *)object);

        // Function cells are traced:
        if (IS_FUNCTION(evaValue))
        {
            auto fn = AS_FUNCTION(evaValue);
            for (auto &cell : fn->cells)
                pointers.insert((Traceable *)cell);
        }

        // Class instances.
        if (IS_INSTANCE(evaValue))
        {
            auto instance = AS_INSTANCE(evaValue);
            for (auto &prop : instance->properties)
            {
                if (IS_OBJECT(prop.second))
                    pointers.insert((Traceable *)AS_OBJECT(prop.second));
            }
        }

        return pointers;
    }

    void sweep()
    {
        auto it = Traceable::objects.begin();
        while (it != Traceable::objects.end())
        {
            auto object = (Traceable *)*it;
            if (object->marked)
            {
                object->marked = false;
                it++;
            }
            else
            {
                it = Traceable::objects.erase(it);
                delete object;
            }
        }
    }
};

#endif // EvaCollector_h