#pragma once

#include <Scratch/Data.hpp>
#include <string>
#include <cstdint>
#include <variant>

namespace Scratch {
    class ScratchBlock;

    /*
     * Field object
     */

    /* variable field */
    struct Field_Variable {
        std::string VariableName;
        std::string VariableKey;
        enum class VariableType : uint8_t { Regular, List } Type;
    };
    /* broadcast field */
    struct Field_Broadcast {
        std::string BroadcastName;
        std::string BroadcastKey;
    };

    /**
     * Scratch field descriptor
     */
    struct ScratchField {
        std::variant<Field_Variable, Field_Broadcast, std::string> Field;
        enum class FieldType : uint8_t { InvalidField, VariableField, ListField, BroadcastField, StopOption } Type;
    };

    /*
     * Input object
     */

    /* value input */
    struct Input_Value {
        std::variant<ScratchData, Field_Broadcast, Field_Variable> Value;
        uint8_t PrimitiveType;
    };

    /* operand input */
    struct Input_Operand {
        ScratchData OperandValue;
        uint8_t PrimitiveType;
    };

    struct ScratchInput {
        std::variant<Input_Value, std::string> Input;
        std::string ReporterKey;
        enum class InputType : uint8_t { ValueInput, ConditionInput, SubstackInput, ProcedureDefinition, InvalidInput } Type;
        uint8_t ShadowType;
        bool HasReporter;
    };
};
