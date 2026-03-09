#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Lib/SIMDJson.h>
#include <Compiler.hpp>
#include <iostream>
#include <cstdint>

using namespace simdjson;
using namespace Scratch;

ScratchData __hot ScratchBlock::GetInputData(size_t InputNum) {
    if ((InputNum > this->Inputs.size()) || this->Inputs.empty() ) {
        return { .Type = ScratchData::Type::Number, .Number = 0 };
    }
    ScratchInput & Input = this->Inputs[InputNum];
    if (Input.HasReporter == true) {
        ScratchSprite & OwnerSprite = this->Sprite.get();
        ScratchBlock * ReporterBlock = OwnerSprite.GetBlockFromId(Input.ReporterKey);
        if (ReporterBlock == nullptr) {
            std::cerr << "Invalid reporter block" << std::endl;
            return { .Type = ScratchData::Type::Number, .Number = 0 };
        }
        ScratchData ReporterResult = ReporterBlock->Evaluate();
        return ReporterResult;
    }
    if (Input.Type == ScratchInput::InputType::ValueInput) {
        return std::get<struct Input_Value>(Input.Input).Value;
    }
    return { .Type = ScratchData::Type::Number, .Number = 0 };
}

ScratchInput __hot ScratchBlock::GetInput(size_t InputNum) {
    if ((InputNum > this->Inputs.size()) || this->Inputs.empty() ) {
        struct ScratchInput Input;
        Input.Type = ScratchInput::InputType::InvalidInput;
        return Input;
    }
    return this->Inputs[InputNum];
}

ScratchSprite & __hot ScratchBlock::GetOwnerSprite(void) {
    return this->Sprite.get();
}

static struct Input_Value SetupInputValue(ondemand::array & RawArray) {
    struct Input_Value Value;
    RawArray.reset();
    Value.PrimitiveType = uint8_t(RawArray.at(0).get_uint64() & 0xFF);
    RawArray.reset();
    Value.Value = SanitizeData(RawArray.at(1));
    RawArray.reset();
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
    Input.ShadowType = uint8_t(InputObject.at(0)->get_uint64() & 0xFF);
    InputObject.reset();
    Input.Type = ScratchInput::InputType::InvalidInput;
    if (Key == "VALUE" || Key == "INDEX" || Key == "TIMES" 
     || Key == "ITEM" || Key == "OPERAND" || Key == "OPERAND1"
     || Key == "OPERAND2" || Key == "X" || Key == "Y"
     || Key == "DX" || Key == "DY" || Key == "MESSAGE"
     || Key == "NUM1" || Key == "NUM2" || Key == "NUM") {
        Input.Type = ScratchInput::InputType::ValueInput;
        /* setup value input */
        struct Input_Value Value;
        switch(Input.ShadowType) {
            case INPUT_IS_SHADOW: {
                ondemand::array ValueArray = InputObject.at(1).get_array().value();
                InputObject.reset();
                /* these usually dont have reporters */
                Input.Input = SetupInputValue(ValueArray);
                InputObject.reset();
                Input.HasReporter = false;
                Input.ReporterKey = std::string();
                break;
            }
            case INPUT_NO_SHADOW: {
                return Input;
            }
            case INPUT_REPORTER_BLOCK: {
                std::string ReporterKeyString(InputObject.at(1)->get_string().value());
                InputObject.reset();

                ondemand::array ValueArray = InputObject.at(2).get_array().value();
                InputObject.reset();

                /* always has a reporter */
                Input.Input = SetupInputValue(ValueArray);
                InputObject.reset();
                Input.HasReporter = true;
                Input.ReporterKey = ReporterKeyString;
                break;
            }
            default: {
                std::cerr << "Invalid shadow type" << std::endl;
                return Input;
            }
        }
    } else if (Key == "CONDITION") {
        Input.Type = ScratchInput::InputType::ConditionInput;
        /* setup conditional input, ignore shadow type */
    } else if (Key == "SUBSTACK" || Key == "SUBSTACK2") {
        Input.Type = ScratchInput::InputType::SubstackInput;
    } else {
        if (this->IsProcedureDef() == true) {
            Input.Type = ScratchInput::InputType::ProcedureDefinition;
            Input.HasReporter = false;
            Input.Input = std::string(InputObject.at(1)->get_string().value());
        } else {
            std::cerr << "WARNING: Unknown/unhandled input key: " << Key << ". Input will be returned as empty" << std::endl;
        }
    }
    return Input;
}


ScratchField ScratchBlock::ParseField(std::string & Key, ondemand::array FieldObject) {
    ScratchField Field;
    return Field;
}
