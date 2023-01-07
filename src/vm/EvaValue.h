// Eva Value Types
#ifndef EvaValue_h
#define EvaValue_h

#include <functional>
#include <list>
#include "src/vm/Logger.h"

// Eva value type
enum class EvaValueType
{
    NUMBER,
    BOOLEAN,
    OBJECT,
};

// Object type
enum class ObjectType
{
    STRING,
    CODE,
    NATIVE,
    FUNCTION,
    CELL,
    CLASS,
    INSTANCE,
};

// Base traceable object
struct Traceable
{
    bool marked;
    size_t size;
    static size_t bytesAllocated;
    static std::list<Traceable *> objects;

    // Custom allocator for our traceability
    static void *operator new(size_t size)
    {
        void *object = ::operator new(size);
        ((Traceable *)object)->size = size;
        Traceable::objects.push_back((Traceable *)object);
        Traceable::bytesAllocated += size;
        return object;
    }

    // Custom deallocator for same
    static void operator delete(void *object, std::size_t sz)
    {
        Traceable::bytesAllocated -= ((Traceable *)object)->size;
        ::operator delete(object);
        // Note: remove from Traceable::objects during GC cycle.
    }

    // Clean up all objects
    static void cleanup()
    {
        for (auto &object : objects)
        {
            delete object;
        }
        objects.clear();
    }

    // Memory stats
    static void printStats()
    {
        std::cout << "------------------------------" << std::endl;
        std::cout << "Memory stats:" << std::endl
                  << std::endl;
        std::cout << "Objects allocated: " << std::dec << Traceable::objects.size() << std::endl;
        std::cout << "Bytes allocated: " << std::dec << Traceable::bytesAllocated << std::endl;
    }
};

size_t Traceable::bytesAllocated{0};
std::list<Traceable *> Traceable::objects{};

// Base Object
struct Object : public Traceable
{
    Object(ObjectType type) : type(type) {}
    ObjectType type;
};

// String object
struct StringObject : public Object
{
    StringObject(const std::string &str) : Object(ObjectType::STRING), string(str) {}
    std::string string;
};

// Native functions
using NativeFn = std::function<void()>;

struct NativeObject : public Object
{
    NativeObject(NativeFn function, const std::string &name, size_t arity) : Object(ObjectType::NATIVE),
                                                                             function(function),
                                                                             name(name),
                                                                             arity(arity) {}

    NativeFn function;
    std::string name;
    size_t arity;
};

// Eva value (tagged union)
struct EvaValue
{
    EvaValueType type;
    union
    {
        double number;
        bool boolean;
        Object *object;
    };
};

struct LocalVar
{
    std::string name;
    size_t scopeLevel;
};

// Code object
struct CodeObject : public Object
{
    CodeObject(const std::string &name, size_t arity) : Object(ObjectType::CODE), name(name), arity(arity) {}

    std::string name;
    size_t arity;
    std::vector<EvaValue> constants;
    std::vector<uint8_t> code;
    size_t scopeLevel = 0;
    std::vector<LocalVar> locals;
    std::vector<std::string> cellNames;
    size_t freeCount = 0;

    void insertAtOffset(int offset, uint8_t byte)
    {
        code.insert((offset < 0 ? code.end() : code.begin()) + offset, byte);
    }

    void addLocal(const std::string &name)
    {
        locals.push_back({name, scopeLevel});
    }

    void addConstant(const EvaValue &value)
    {
        constants.push_back(value);
    }

    int getLocalIndex(const std::string &name)
    {
        if (locals.size() > 0)
        {
            for (auto i = (int)locals.size() - 1; i >= 0; i--)
            {
                if (locals[i].name == name)
                {
                    return i;
                }
            }
        }
        return -1;
    }

    int getCellIndex(const std::string &name)
    {
        if (cellNames.size() > 0)
        {
            for (auto i = (int)cellNames.size() - 1; i >= 0; i--)
            {
                if (cellNames[i] == name)
                {
                    return i;
                }
            }
        }
        return -1;
    }
};

// Class object
struct ClassObject : public Object
{
    ClassObject(const std::string &name, ClassObject *superClass)
        : Object(ObjectType::CLASS),
          name(name),
          properties{},
          superClass(superClass) {}

    std::string name;
    std::map<std::string, EvaValue> properties;
    ClassObject *superClass;

    EvaValue getProp(const std::string &prop)
    {
        if (properties.count(prop) != 0)
            return properties[prop];

        if (superClass == nullptr)
            DIE << "Unresolved property " << prop << " in class " << name;

        return superClass->getProp(prop);
    }

    void setProp(const std::string &prop, const EvaValue &value)
    {
        properties[prop] = value;
    }
};

// Instances of classes
struct InstanceObject : public Object
{
    InstanceObject(ClassObject *cls)
        : Object(ObjectType::INSTANCE), cls(cls), properties{} {}

    ClassObject *cls;
    std::map<std::string, EvaValue> properties;

    EvaValue getProp(const std::string &prop)
    {
        if (properties.count(prop) != 0)
            return properties[prop];

        // If not in own properties, check the class object
        return cls->getProp(prop);
    }
};

// Heap-allocated cell.
// Used to capture closure variables
struct CellObject : public Object
{
    CellObject(EvaValue value) : Object(ObjectType::CELL), value(value) {}
    EvaValue value;
};

// Function object
struct FunctionObject : public Object
{
    FunctionObject(CodeObject *co) : Object(ObjectType::FUNCTION), co(co) {}

    // Reference to the code object:
    // contains function code, locals, etc.
    CodeObject *co;

    // Captured cells (for closures).
    std::vector<CellObject *> cells;
};

// Constructors
#define NUMBER(value) ((EvaValue){.type = EvaValueType::NUMBER, .number = value})
#define BOOLEAN(value) ((EvaValue){.type = EvaValueType::BOOLEAN, .boolean = value})
#define OBJECT(value) ((EvaValue){.type = EvaValueType::OBJECT, .object = value})
#define CELL(cellObject) OBJECT((Object *)cellObject)
#define CLASS(classObject) OBJECT((Object *)classObject)

#define ALLOC_STRING(value) \
    ((EvaValue){.type = EvaValueType::OBJECT, .object = (Object *)new StringObject(value)})
#define ALLOC_CODE(name, arity) \
    ((EvaValue){.type = EvaValueType::OBJECT, .object = (Object *)new CodeObject(name, arity)})
#define ALLOC_NATIVE(fn, name, arity)         \
    ((EvaValue){.type = EvaValueType::OBJECT, \
                .object = (Object *)new NativeObject(fn, name, arity)})
#define ALLOC_FUNCTION(co)                    \
    ((EvaValue){.type = EvaValueType::OBJECT, \
                .object = (Object *)new FunctionObject(co)})
#define ALLOC_CELL(evaValue)                  \
    ((EvaValue){.type = EvaValueType::OBJECT, \
                .object = (Object *)new CellObject(evaValue)})
#define ALLOC_CLASS(name, superClass)         \
    ((EvaValue){.type = EvaValueType::OBJECT, \
                .object = (Object *)new ClassObject(name, superClass)})
#define ALLOC_INSTANCE(cls)                   \
    ((EvaValue){.type = EvaValueType::OBJECT, \
                .object = (Object *)new InstanceObject(cls)})

// Accessors
#define AS_NUMBER(evaValue) ((double)(evaValue).number)
#define AS_BOOLEAN(evaValue) ((bool)(evaValue).boolean)
#define AS_OBJECT(evaValue) ((Object *)(evaValue).object)
#define AS_STRING(evaValue) ((StringObject *)(evaValue).object)
#define AS_CPPSTRING(evaValue) (AS_STRING(evaValue)->string)
#define AS_CODE(evaValue) ((CodeObject *)(evaValue).object)
#define AS_NATIVE(evaValue) ((NativeObject *)(evaValue).object)
#define AS_FUNCTION(evaValue) ((FunctionObject *)(evaValue).object)
#define AS_CELL(evaValue) ((CellObject *)(evaValue).object)
#define AS_CLASS(evaValue) ((ClassObject *)(evaValue).object)
#define AS_INSTANCE(evaValue) ((InstanceObject *)(evaValue).object)

// Predicates
#define IS_NUMBER(evaValue) ((evaValue).type == EvaValueType::NUMBER)
#define IS_BOOLEAN(evaValue) ((evaValue).type == EvaValueType::BOOLEAN)
#define IS_OBJECT(evaValue) ((evaValue).type == EvaValueType::OBJECT)
#define IS_OBJECT_TYPE(evaValue, objectType) \
    (IS_OBJECT(evaValue) && AS_OBJECT(evaValue)->type == objectType)
#define IS_STRING(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::STRING)
#define IS_CODE(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::CODE)
#define IS_NATIVE(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::NATIVE)
#define IS_FUNCTION(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::FUNCTION)
#define IS_CELL(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::CELL)
#define IS_CLASS(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::CLASS)
#define IS_INSTANCE(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::INSTANCE)

// Output stream
std::string evaValueToTypeString(const EvaValue &evaValue)
{
    if (IS_NUMBER(evaValue))
    {
        return "NUMBER";
    }
    else if (IS_BOOLEAN(evaValue))
    {
        return "BOOLEAN";
    }
    else if (IS_STRING(evaValue))
    {
        return "STRING";
    }
    else if (IS_CODE(evaValue))
    {
        return "CODE";
    }
    else if (IS_NATIVE(evaValue))
    {
        return "NATIVE";
    }
    else if (IS_FUNCTION(evaValue))
    {
        return "FUNCTION";
    }
    else if (IS_CELL(evaValue))
    {
        return "CELL";
    }
    else if (IS_CLASS(evaValue))
    {
        return "CLASS";
    }
    else if (IS_INSTANCE(evaValue))
    {
        return "INSTANCE";
    }
    else
    {
        DIE << "evaValueToTypeString: unknown type " << (int)evaValue.type;
        return "";
    }
}

std::string evaValueToConstantString(const EvaValue &evaValue)
{
    std::stringstream ss;
    if (IS_NUMBER(evaValue))
    {
        ss << evaValue.number;
    }
    else if (IS_BOOLEAN(evaValue))
    {
        ss << (evaValue.boolean ? "true" : "false");
    }
    else if (IS_STRING(evaValue))
    {
        ss << '"' << AS_CPPSTRING(evaValue) << '"';
    }
    else if (IS_CODE(evaValue))
    {
        auto code = AS_CODE(evaValue);
        ss << "code: " << code << ": " << code->name << "/" << code->arity;
    }
    else if (IS_FUNCTION(evaValue))
    {
        auto fn = AS_FUNCTION(evaValue);
        ss << fn->co->name << "/" << fn->co->arity;
    }
    else if (IS_NATIVE(evaValue))
    {
        auto fn = AS_NATIVE(evaValue);
        ss << fn->name << "/" << fn->arity;
    }
    else if (IS_CELL(evaValue))
    {
        auto cell = AS_CELL(evaValue);
        ss << "cell: " << evaValueToConstantString(cell->value);
    }
    else if (IS_CLASS(evaValue))
    {
        auto cls = AS_CLASS(evaValue);
        ss << "class: " << cls->name;
    }
    else if (IS_INSTANCE(evaValue))
    {
        auto inst = AS_INSTANCE(evaValue);
        ss << "instance: " << inst->cls->name;
    }
    else
    {
        DIE << "evaValueToConstantString: unknown type " << (int)evaValue.type;
    }
    return ss.str();
}

std::ostream &operator<<(std::ostream &os, const EvaValue &evaValue)
{
    return os << "EvaValue (" << evaValueToTypeString(evaValue)
              << "): " << evaValueToConstantString(evaValue);
}
#endif /* __EvaValue_h */
