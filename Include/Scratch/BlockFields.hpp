#pragma once

#include <Scratch/Data.hpp>
#include <string>
#include <cstdint>
#include <variant>

namespace Scratch {
    class ScratchBlock;

    enum class ScratchShadow : uint8_t {
        INPUT_SAME_BLOCK_SHADOW = 1,
        INPUT_BLOCK_NO_SHADOW,
        INPUT_DIFF_BLOCK_SHADOW
    };

    enum class ScratchPrimitive : uint8_t {
        INPUT_MATH_NUM = 4,
        INPUT_POSITIVE_NUM,
        INPUT_WHOLE_NUM,
        INPUT_INTEGER_NUM,
        INPUT_ANGLE_NUM,
        INPUT_COLOR_PICKER,
        INPUT_TEXT,
        INPUT_BROADCAST,
        INPUT_VAR,
        INPUT_LIST
    };

    /*
     * Field object
     */

    /* variable field */
    struct Field_Variable {
        std::string VariableName;
        std::string VariableKey;
        enum class VariableType : uint8_t { Regular, List } Type;
    };
    /**
     * Scratch field descriptor
     */
    struct ScratchField {
        std::variant<Field_Variable, std::string> Field;
        enum class FieldType : uint8_t { VariableField, ListField, BroadcastField, StopOption, ValueField, InvalidField } Type;
    };

    /*
     * Input object
     */

    /* value input */
    struct Input_Value {
        std::variant<ScratchData, Field_Variable, std::string> Value;
        ScratchPrimitive PrimitiveType;
    };

    /* operand input */
    struct Input_Operand {
        ScratchData OperandValue;
        ScratchPrimitive PrimitiveType;
    };

    struct ScratchInput {
        std::variant<Input_Value, std::string> Input;
        std::string ReporterKey;
        enum class InputType : uint8_t { ConditionInput, SubstackInput, ProcedureDefinition, ValueInput, BooleanInput, BroadcastInput, InvalidInput } Type;
        ScratchShadow ShadowType;
        bool HasReporter;
    };
};
