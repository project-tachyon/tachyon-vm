#pragma once

#include <Scratch/Data.hpp>
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
    };
    /* procedure parameter */
    struct Field_ProcParam {
        std::string ParameterName;
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
        std::variant<struct Field_Variable, struct Field_ProcParam, struct Field_Broadcast> Field;
        enum class FieldType : uint8_t { ProcedureDefinition, VariableField, ListField, ProcedureParam, BroadcastField } Type;
    };

    /*
     * Input object
     */

    /* value input */
    struct Input_Value {
        ScratchData Value;
        uint8_t PrimitiveType;
    };

    /* operand input */
    struct Input_Operand {
        ScratchData OperandValue;
        uint8_t PrimitiveType;
    };

    struct ScratchInput {
        std::variant<ScratchBlock *, struct Input_Value, struct Input_Operand, std::string> Input;
        std::string ReporterKey;
        enum class InputType : uint8_t { ValueInput, ConditionInput, SubstackInput, ProcedureDefinition, InvalidInput } Type;
        bool HasReporter;
        uint8_t ShadowType;
    };
};
