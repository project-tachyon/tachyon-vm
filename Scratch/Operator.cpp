#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Operator.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Compiler.hpp>

using namespace Scratch;

static inline __hot ScratchData Operator_Add(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    double Result = (Operand1_Data.Type == ScratchData::Type::Number ? Operand1_Data.Number : 0) + (Operand2_Data.Type == ScratchData::Type::Number ? Operand2_Data.Number : 0);

    return ScratchData(Result);
}

static inline __hot ScratchData Operator_Join(ScratchBlock & Block) {
    ScratchData Data1 = Block.GetInputData(0);
    ScratchData Data2 = Block.GetInputData(1);

    std::string String1 = Data2String(Data1);
    std::string String2 = Data2String(Data2);

    return ScratchData(String1 + String2);
}

static inline __hot ScratchData Operator_Equals(ScratchBlock & Block) {
    ScratchData Data1 = Block.GetInputData(0);
    ScratchData Data2 = Block.GetInputData(1);

    std::string String1 = Data2String(Data1);
    std::string String2 = Data2String(Data2);

    return bool(String1 == String2);
}

static inline __hot ScratchData Operator_Not(ScratchBlock & Block) {
    ScratchInput Condition = Block.GetInput(0);
    if (unlikely(Condition.Type != ScratchInput::InputType::BooleanInput)) {
        DebugError("Invalid input for not operator!\n");
        return false;
    }
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchBlock * Reporter = Owner.GetBlockFromId(ConditionBlockId);

    if (unlikely(Reporter == nullptr)) {
        DebugError("Invalid reporter block for not operator\n");
        return false;
    }

    ScratchData Evaluation = Reporter->Evaluate();
    return bool(Evaluation.Boolean == false);
}

static inline __hot ScratchData Operator_Length(ScratchBlock & Block) {
    ScratchData Data = Block.GetInputData(0);
    switch(Data.Type) {
        case ScratchData::Type::String: {
            return double(Data.String.length());
            break;
        }
        case ScratchData::Type::Boolean: {
            return double(Data.Boolean ? 4 : 5);                                     
        }
        case ScratchData::Type::Number: {
            return double(std::to_string(Data.Number).length());
        }
    }
    __unreachable;
}

static inline __hot ScratchData Operator_LetterOf(ScratchBlock & Block) {
    ScratchData Index = Block.GetInputData(0);
    ScratchData String = Block.GetInputData(1);

    if (Index.Type != ScratchData::Type::Number) {
        return "";
    }

    std::string RealString;

    switch(String.Type) {
        case ScratchData::Type::String: {
            RealString = String.String;
            break;
        }
        case ScratchData::Type::Boolean: {
            RealString = String.Boolean ? "true" : "false";                                     
            break;
        }
        case ScratchData::Type::Number: {
            RealString = std::to_string(String.Number);
            break;
        }
    }
    std::string Character(1, RealString[Index.Number]);
    return Character;
}

void Operator::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("operator_add", Operator_Add);
    Tachyon::RegisterEvaluationHandler("operator_join", Operator_Join);
    Tachyon::RegisterEvaluationHandler("operator_equals", Operator_Equals);
    Tachyon::RegisterEvaluationHandler("operator_not", Operator_Not);
    Tachyon::RegisterEvaluationHandler("operator_length", Operator_Length);
    Tachyon::RegisterEvaluationHandler("operator_letter_of", Operator_LetterOf);
}
