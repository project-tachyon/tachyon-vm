#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Operator.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Compiler.hpp>
#include <cmath>
#include <limits>

using namespace Scratch;

static inline __hot ScratchData Operator_Add(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    double Result = (Operand1_Data.Type == ScratchData::Type::Number ? Operand1_Data.Number : 0) + (Operand2_Data.Type == ScratchData::Type::Number ? Operand2_Data.Number : 0);

    return Result;
}

static inline __hot ScratchData Operator_Modulo(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    double Result = std::fmod(
        (Operand1_Data.Type == ScratchData::Type::Number ? Operand1_Data.Number : 0),
        (Operand2_Data.Type == ScratchData::Type::Number ? Operand2_Data.Number : 0)
    );

    return Result;
}

static inline Operator::MathOperation __hot GetMathOperation(ScratchField & Field) {
    std::string Operation = std::get<std::string>(Field.Field);

    if (Operation == "floor") return Operator::MathOperation::MathFloor;

    return Operator::MathOperation::MathInvalid;
} 

static inline __hot ScratchData Operator_MathOp(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::StringField);

    Operator::MathOperation Type = GetMathOperation(Field);

    switch(Type) {
        case Operator::MathOperation::MathFloor: {
            ScratchData Data = Block.GetInputData(0);
            if (Data.Type != ScratchData::Type::Number) {
                return double(0);
            }
            return std::floor(Data.Number);
        }
        case Operator::MathOperation::MathInvalid: {
            TachyonUnimplemented("Unknown math operation: %s\n", std::get<std::string>(Field.Field).c_str());
        }
    }
    __unreachable;
}

static inline __hot ScratchData Operator_Divide(ScratchBlock & Block) {
    ScratchData Num1 = Block.GetInputData(0);
    ScratchData Num2 = Block.GetInputData(1);

    if (Num2.Type != ScratchData::Type::Number) {
        return std::numeric_limits<double>::infinity();
    }
    double Result = (Num1.Type == ScratchData::Type::Number ? Num1.Number : 0) / Num2.Number;
    return Result;
}

static inline __hot ScratchData Operator_Join(ScratchBlock & Block) {
    ScratchData Data1 = Block.GetInputData(0);
    ScratchData Data2 = Block.GetInputData(1);

    std::string String1 = Data2String(Data1);
    std::string String2 = Data2String(Data2);

    return String1 + String2;
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
    if (unlikely(Condition.Type != ScratchInput::InputType::ValueInput)) {
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
    Tachyon::RegisterEvaluationHandler("operator_mod", Operator_Modulo);
    Tachyon::RegisterEvaluationHandler("operator_mathop", Operator_MathOp);
    Tachyon::RegisterEvaluationHandler("operator_divide", Operator_Divide);
}
