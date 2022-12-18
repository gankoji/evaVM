#ifndef __EvaValue_h
#define __EvaValue_h

/**
* Eva value type
*/
enum class EvaValueType {
    Number,
};

/**
* Eva value (tagged union)
*/
struct EvaValue {
    EvaValueType type;
    union {
        double number;
    };
};

// ------------------------
// Constructors:
#define NUMBER(value) ((EvaValue){.type = EvaValueType::Number, .number = value})
#define AS_NUMBER(evaValue) ((double)(evaValue).number)

#endif /* __EvaValue_h */
