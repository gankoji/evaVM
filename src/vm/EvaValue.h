/**
 * Eva Value Types
 */

#ifndef EvaValue_h
#define EvaValue_h

#include "src/vm/Logger.h"

/**
 * Eva value type
 */
enum class EvaValueType
{
    NUMBER,
    BOOLEAN,
    OBJECT,
};

/**
 * Object type
 */
enum class ObjectType
{
    STRING,
    CODE,
};

/**
 * Base Object
 */
struct Object
{
    Object(ObjectType type) : type(type) {}
    ObjectType type;
};

/**
 * String object
 */
struct StringObject : public Object
{
    StringObject(const std::string &str) : Object(ObjectType::STRING), string(str) {}
    std::string string;
};

/**
 * Eva value (tagged union)
 */
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

/**
 * Code object
 */
struct CodeObject : public Object
{
    CodeObject(const std::string &name) : Object(ObjectType::CODE), name(name) {}

    std::string name;
    std::vector<EvaValue> constants;
    std::vector<uint8_t> code;
    size_t scopeLevel = 0;
    std::vector<LocalVar> locals;

    void addLocal(const std::string &name)
    {
        locals.push_back({name, scopeLevel});
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
};

/**
 * Constructors
 */
#define NUMBER(value) ((EvaValue){.type = EvaValueType::NUMBER, .number = value})
#define BOOLEAN(value) ((EvaValue){.type = EvaValueType::BOOLEAN, .boolean = value})
#define ALLOC_STRING(value) \
    ((EvaValue){.type = EvaValueType::OBJECT, .object = (Object *)new StringObject(value)})
#define ALLOC_CODE(name) \
    ((EvaValue){.type = EvaValueType::OBJECT, .object = (Object *)new CodeObject(name)})

/**
 * Accessors
 */
#define AS_NUMBER(evaValue) ((double)(evaValue).number)
#define AS_BOOLEAN(evaValue) ((bool)(evaValue).boolean)
#define AS_OBJECT(evaValue) ((Object *)(evaValue).object)
#define AS_STRING(evaValue) ((StringObject *)(evaValue).object)
#define AS_CPPSTRING(evaValue) (AS_STRING(evaValue)->string)
#define AS_CODE(evaValue) ((CodeObject *)(evaValue).object)

/**
 * Predicates
 */
#define IS_NUMBER(evaValue) ((evaValue).type == EvaValueType::NUMBER)
#define IS_BOOLEAN(evaValue) ((evaValue).type == EvaValueType::BOOLEAN)
#define IS_OBJECT(evaValue) ((evaValue).type == EvaValueType::OBJECT)
#define IS_OBJECT_TYPE(evaValue, objectType) \
    (IS_OBJECT(evaValue) && AS_OBJECT(evaValue)->type == objectType)
#define IS_STRING(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::STRING)
#define IS_CODE(evaValue) IS_OBJECT_TYPE(evaValue, ObjectType::CODE)

/**
 * Output stream
 */
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
        ss << "code: " << code << ": " << code->name;
    }
    else
    {
        DIE << "evaValueToTypeString: unknown type " << (int)evaValue.type;
    }
    return ss.str();
}

std::ostream &operator<<(std::ostream &os, const EvaValue &evaValue)
{
    return os << "EvaValue (" << evaValueToTypeString(evaValue)
              << "): " << evaValueToConstantString(evaValue);
}
#endif /* __EvaValue_h */
