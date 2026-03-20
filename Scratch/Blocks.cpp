#include <Tachyon/Debug.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Lib/SIMDJson.h>
#include <Compiler.hpp>
#include <iostream>
#include <variant>

using namespace simdjson;
using namespace Scratch;

ScratchData __hot ScratchBlock::GetInputData(size_t InputNum) {
    /* bad InputNum */
    if (unlikely((InputNum > this->Inputs.size() - 1) || this->Inputs.empty())) {
        return ScratchData(double(0));
    }
    ScratchInput & Input = this->Inputs[InputNum];
    if (Input.HasReporter == true) {
        /* get it from the connected block instead */
        ScratchSprite & OwnerSprite = this->Sprite.get();
        ScratchBlock * ReporterBlock = OwnerSprite.GetBlockFromId(Input.ReporterKey);
        TachyonAssert(ReporterBlock != nullptr);
        ScratchData ReporterResult = ReporterBlock->Evaluate();
        return ReporterResult;
    }
    /* a normal input value */
    if (Input.Type == ScratchInput::InputType::ValueInput) {
        Input_Value InputValue = std::get<Input_Value>(Input.Input);
        ScratchSprite & OwnerSprite = this->GetOwnerSprite();
        ScratchData Data;
        if (std::holds_alternative<Field_Variable>(InputValue.Value)) {
            Field_Variable VariableField = std::get<Field_Variable>(InputValue.Value);
            if (VariableField.Type == Field_Variable::VariableType::Regular) {
                ScratchVariable * Variable = OwnerSprite.GetVariable(VariableField.VariableKey);

                TachyonAssert(Variable != nullptr);

                Data = Variable->GetData();
            } else {
                ScratchList * List = OwnerSprite.GetList(VariableField.VariableKey);

                TachyonAssert(List != nullptr);

                /* UNIMPLEMENTED */
                Data = ScratchData(double(0));
                DebugError("Unimplemented #1\n");
            }

        } else if (std::holds_alternative<ScratchData>(InputValue.Value)) {
            Data = std::get<ScratchData>(InputValue.Value);
        }
        return Data;
    }
    return ScratchData(double(0));
}

ScratchInput __hot ScratchBlock::GetInput(size_t InputNum) {
    if (unlikely((InputNum > this->Inputs.size() - 1) || this->Inputs.empty())) {
        struct ScratchInput Input;
        Input.Type = ScratchInput::InputType::InvalidInput;
        return Input;
    }
    return this->Inputs[InputNum];
}

ScratchField __hot ScratchBlock::GetField(size_t FieldNum) {
    if (unlikely((FieldNum > this->Fields.size() - 1) || this->Fields.empty())) {
        struct ScratchField Field;
        Field.Type = ScratchField::FieldType::InvalidField;
        return Field;
    }
    return this->Fields[FieldNum];
}

ScratchSprite & __hot ScratchBlock::GetOwnerSprite(void) {
    return this->Sprite.get();
}

static struct Input_Value SetupInputValue(ondemand::array & RawArray) {
    struct Input_Value Value;
    RawArray.reset();
    Value.PrimitiveType = ScratchPrimitive(RawArray.at(0).get_uint64() & 0xFF);
    RawArray.reset();
    /* yes i know the code looks like shit, but it works */
    switch(Value.PrimitiveType) {
        case ScratchPrimitive::INPUT_VAR: {
            Field_Variable Variable;
            Variable.VariableName = std::string(RawArray.at(1)->get_string().value());
            RawArray.reset();
            Variable.VariableKey = std::string(RawArray.at(2)->get_string().value());
            RawArray.reset();
            Variable.Type = Field_Variable::VariableType::Regular;
            Value.Value = Variable;
            break;
        }
        case ScratchPrimitive::INPUT_LIST: {
            Field_Variable Variable;
            Variable.VariableName = std::string(RawArray.at(1)->get_string().value());
            RawArray.reset();
            Variable.VariableKey = std::string(RawArray.at(2)->get_string().value());
            RawArray.reset();
            Variable.Type = Field_Variable::VariableType::List;
            Value.Value = Variable;
            break;
        }
        default: {
            Value.Value = SanitizeData(RawArray.at(1));
            RawArray.reset();
            break;
        }
    }
    return Value;
}

ScratchMutation ScratchBlock::ParseMutation(ondemand::object MutationObject) {
    ScratchMutation BlockMutation;
    if (this->Opcode == "procedures_prototype" || this->Opcode == "procedures_call") {
        BlockMutation.ProcCode = std::string(MutationObject["proccode"]->get_string().value());
        BlockMutation.UseWarp = (MutationObject["warp"]->get_string().value() == "true");
        padded_string ParamKeysJSON(MutationObject["argumentids"]->get_string().value());
        ondemand::parser KeyParser;
        ondemand::document ParamKeys = KeyParser.iterate(ParamKeysJSON);
        for(ondemand::value KeyValue : ParamKeys.get_array()) {
            std::string Key(KeyValue.get_string().value());
            BlockMutation.ParametersKeys.push_back(Key);
        }
        if (this->Opcode == "procedures_prototype") {
            /* has extras */
            ondemand::parser NameParser;
            ondemand::parser DefaultsParser;
            padded_string ParamNamesJSON(MutationObject["argumentnames"]->get_string().value());
            padded_string ParamDefaultsJSON(MutationObject["argumentdefaults"]->get_string().value());
            ondemand::document ParamNames = NameParser.iterate(ParamNamesJSON);
            ondemand::document ParamDefaults = DefaultsParser.iterate(ParamDefaultsJSON);
            for(ondemand::value NameValue : ParamNames.get_array()) {
                std::string Name(NameValue.get_string().value());
                BlockMutation.ParametersNames.push_back(Name);
            }
            for(ondemand::value DefaultValue : ParamDefaults.get_array()) {
                BlockMutation.ParameterDefaults.push_back(SanitizeData(DefaultValue));
            }
        }
    } else {
        BlockMutation.HasNext = (MutationObject["hasnext"]->get_string().value() == "true");
    }
    return BlockMutation;
}

ScratchInput ScratchBlock::ParseInput(std::string & Key, ondemand::array InputObject) {
    ScratchInput Input;
    Input.ShadowType = ScratchShadow(InputObject.at(0)->get_uint64() & 0xFF);
    InputObject.reset();
    Input.Type = ScratchInput::InputType::InvalidInput;
    if (Key == "VALUE" || Key == "MESSAGE" || Key == "STRING1"
        || Key == "STRING1" || Key == "STRING2" || Key == "OPERAND1"
        || Key == "OPERAND2" || Key == "ITEM" || Key == "INDEX"
        || Key == "TIMES" || Key == "NUM1" || Key == "NUM2"
        || Key == "STRING" || Key == "NUM" || Key == "LETTER") {
        Input.Type = ScratchInput::InputType::ValueInput;
        /* setup value input */
        struct Input_Value Value;
        switch(Input.ShadowType) {
            case ScratchShadow::INPUT_SAME_BLOCK_SHADOW: {
                ondemand::array ValueArray = InputObject.at(1).get_array().value();
                InputObject.reset();
                /* these usually dont have reporters */
                Input.Input = SetupInputValue(ValueArray);
                InputObject.reset();
                Input.HasReporter = false;
                break;
            }
            case ScratchShadow::INPUT_DIFF_BLOCK_SHADOW: {
                ondemand::array ValueArray;
                if (InputObject.at(1).is_string() == true) {
                    /* the typical block chain */
                    InputObject.reset();
                    std::string ReporterKeyString(InputObject.at(1)->get_string().value());
                    InputObject.reset();
                    ValueArray = InputObject.at(2).get_array().value();
                    InputObject.reset();
                    Input.ReporterKey = ReporterKeyString;
                    /* always has a reporter */
                    Input.Input = SetupInputValue(ValueArray);
                    InputObject.reset();
                    Input.HasReporter = true;
                } else {
                    /* variable, list, or broadcast */
                    InputObject.reset();
                    ValueArray = InputObject.at(1)->get_array().value();
                    InputObject.reset();
                    Input.Input = SetupInputValue(ValueArray);
                    InputObject.reset();
                    Input.HasReporter = false;
                }
                break;
            }
            default: {
                std::cerr << "Invalid shadow type" << std::endl;
                return Input;
            }
        }
    } else if (Key == "CONDITION") {
        Input.Type = ScratchInput::InputType::ConditionInput;
        if (InputObject.at(1)->is_null() == true) {
            InputObject.reset();
            Input.Input = std::string();
        } else {
            InputObject.reset();
            Input.Input = std::string(InputObject.at(1)->get_string().value());
        }
    } else if (Key == "SUBSTACK" || Key == "SUBSTACK2") {
        Input.Type = ScratchInput::InputType::SubstackInput;
        if (InputObject.at(1)->is_null() == true) {
            InputObject.reset();
            Input.Input = std::string();
        } else {
            InputObject.reset();
            Input.Input = std::string(InputObject.at(1)->get_string().value());
        }
    } else if (Key == "OPERAND") {
        Input.Type = ScratchInput::InputType::BooleanInput;
        if (InputObject.at(1)->is_null() == true) {
            Input.Input = std::string();
        } else {
            InputObject.reset();
            Input.Input = std::string(InputObject.at(1)->get_string().value());
        }
        return Input;
    } else if (Key == "BROADCAST_INPUT") {
        Input.Type = ScratchInput::InputType::BroadcastInput;
        ondemand::array ValueArray = InputObject.at(1)->get_array().value();
        Input.Input = std::string(ValueArray.at(2)->get_string().value());
        return Input;
    } else {
        if (this->IsProcedureDef() == true) {
            Input.Type = ScratchInput::InputType::ProcedureDefinition;
            Input.HasReporter = false;
            Input.Input = std::string(InputObject.at(1)->get_string().value());
        } else {
            DebugWarn("Unknown/unhandled input key \"%s\". Input will be returned as empty.\n", Key.c_str());
        }
    }
    return Input;
}


ScratchField ScratchBlock::ParseField(std::string & Key, ondemand::array FieldObject) {
    ScratchField Field;
    Field.Type = ScratchField::FieldType::InvalidField;
    if (Key == "LIST" || Key == "VARIABLE") {
        std::string VariableName(FieldObject.at(0)->get_string().value());
        FieldObject.reset();
        std::string VariableKey(FieldObject.at(1).get_string().value());
        FieldObject.reset();

        ScratchSprite & Owner = this->GetOwnerSprite();
        Field_Variable VariableField;

        Field.Type = (Key == "LIST") ? ScratchField::FieldType::ListField : ScratchField::FieldType::VariableField;
        VariableField.VariableKey = VariableKey;
        VariableField.VariableName = VariableName;
        Field.Field = VariableField;
    } else if (Key == "VALUE") {
        std::string Value(FieldObject.at(0)->get_string().value());
        FieldObject.reset();

        Field.Type = ScratchField::FieldType::ValueField;
        Field.Field = Value;
    } else if (Key == "STOP_OPTION") {
        std::string StopOption(FieldObject.at(0)->get_string().value());
        FieldObject.reset();

        Field.Type = ScratchField::FieldType::StopOption;
        Field.Field = StopOption;
    } else if (Key == "BROADCAST_OPTION") {
        std::string BroadcastOption(FieldObject.at(1).get_string().value());
        FieldObject.reset();

        Field.Type = ScratchField::FieldType::BroadcastField;
        Field.Field = BroadcastOption;
    }
    return Field;
}
