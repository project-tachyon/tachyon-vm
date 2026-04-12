#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Operator.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Compiler.hpp>
#include <random>
#include <cmath>
#include <limits>

using namespace Scratch;

static ScratchData __hot Operator_Add(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    return Operand1_Data.AsDouble() + Operand2_Data.AsDouble();
}

static ScratchData __hot Operator_Subtract(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    return Operand1_Data.AsDouble() - Operand2_Data.AsDouble();
}

static ScratchData __hot Operator_Modulo(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    return std::fmod(
        (Operand1_Data.AsDouble()),
        (Operand2_Data.AsDouble())
    );
}

static ScratchData __hot Operator_MathOp(ScratchBlock & Block) {
    const ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::StringField);

    const std::string Operation = std::get<std::string>(Field.Field);

    if (Operation == "floor") {
        ScratchData Data = Block.GetInputData(0);
        return std::floor(Data.AsDouble());
    }

    DebugError("Invalid math operation: %s\n", Operation.c_str());

    return ScratchData(double(0));
}

static ScratchData __hot Operator_Divide(ScratchBlock & Block) {
    ScratchData Left = Block.GetInputData(0);
    ScratchData Right = Block.GetInputData(1);

    const double Operand1 = Left.AsDouble();
    const double Operand2 = Right.AsDouble();

    if (Operand2 == 0) {
        if (Operand1 == 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return (Operand1 < 0) ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    return double(Operand1 / Operand2);
}

static ScratchData __hot Operator_Multiply(ScratchBlock & Block) {
    ScratchData Left = Block.GetInputData(0);
    ScratchData Right = Block.GetInputData(1);

    const double Operand1 = Left.AsDouble();
    const double Operand2 = Right.AsDouble();

    return double(Operand1 * Operand2);
}

static ScratchData __hot Operator_Random(ScratchBlock & Block) {
    ScratchData Left = Block.GetInputData(0);
    ScratchData Right = Block.GetInputData(1);

    const double LeftNum = Left.AsDouble();
    const double RightNum = Right.AsDouble();

    if (LeftNum == RightNum) { return double(LeftNum); }

    const double Low = std::min(LeftNum, RightNum);
    const double Max = std::max(LeftNum, RightNum);

    static std::mt19937 Generator(std::random_device{ /* empty */ }());
    std::uniform_real_distribution<double> Dist(Low, Max);

    return double(Dist(Generator));
}

static ScratchData __hot Operator_Join(ScratchBlock & Block) {
    ScratchData Data1 = Block.GetInputData(0);
    ScratchData Data2 = Block.GetInputData(1);

    const std::string String1 = Data1.AsString();
    const std::string String2 = Data2.AsString();

    return String1 + String2;
}

static ScratchData __hot Operator_Equals(ScratchBlock & Block) {
    ScratchData Data1 = Block.GetInputData(0);
    ScratchData Data2 = Block.GetInputData(1);

    const std::string String1 = Data1.AsString();
    const std::string String2 = Data2.AsString();

    return bool(String1 == String2);
}

static ScratchData __hot Operator_Not(ScratchBlock & Block) {
    const ScratchInput Condition = Block.GetInput(0);
    TachyonAssert(Condition.Type == ScratchInput::InputType::ValueInput);

    std::string ConditionBlockId = std::get<std::string>(Condition.Input);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchBlock * Reporter = Owner.GetBlockFromId(ConditionBlockId);

    TachyonAssert(Reporter != nullptr);

    const ScratchData Evaluation = Reporter->Evaluate();
    return bool(Evaluation.Boolean == false);
}

static ScratchData __hot Operator_Or(ScratchBlock & Block) {
    const ScratchInput ConditionA = Block.GetInput(0);
    const ScratchInput ConditionB = Block.GetInput(1);
    
    TachyonAssert(ConditionA.Type == ScratchInput::InputType::ValueInput);
    TachyonAssert(ConditionB.Type == ScratchInput::InputType::ValueInput);

    const std::string ConditionA_BlockId = std::get<std::string>(ConditionA.Input);
    const std::string ConditionB_BlockId = std::get<std::string>(ConditionB.Input);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchBlock * ReporterA = Owner.GetBlockFromId(ConditionA_BlockId);
    ScratchBlock * ReporterB = Owner.GetBlockFromId(ConditionB_BlockId);

    TachyonAssert(ReporterA != nullptr);
    TachyonAssert(ReporterB != nullptr);

    const ScratchData EvaluationA = ReporterA->Evaluate();
    const ScratchData EvaluationB = ReporterB->Evaluate();

    return bool(EvaluationA.Boolean == true || EvaluationB.Boolean == true);
}

static ScratchData __hot Operator_Length(ScratchBlock & Block) {
    ScratchData Data = Block.GetInputData(0);
    return double(Data.AsString().length());
}

static ScratchData __hot Operator_LetterOf(ScratchBlock & Block) {
    ScratchData IndexInput = Block.GetInputData(0);
    ScratchData Input = Block.GetInputData(1);

    if (unlikely(IndexInput.Type != ScratchData::Type::Number)) {
        return "";
    }

    const double Index = IndexInput.AsDouble();
    const std::string RealString = Input.AsString();

    if (unlikely(Index < 0 || RealString.length() < Index)) {
        return "";
    }

    std::string Character(1, RealString[Index - 1]);
    return Character;
}

void Operator::RegisterAll(void) {
    /* arithmetic related */
    Tachyon::RegisterEvaluationHandler("operator_add", Operator_Add);
    Tachyon::RegisterEvaluationHandler("operator_subtract", Operator_Subtract);
    Tachyon::RegisterEvaluationHandler("operator_equals", Operator_Equals);
    Tachyon::RegisterEvaluationHandler("operator_or", Operator_Or);
    Tachyon::RegisterEvaluationHandler("operator_not", Operator_Not);
    Tachyon::RegisterEvaluationHandler("operator_mod", Operator_Modulo);
    Tachyon::RegisterEvaluationHandler("operator_mathop", Operator_MathOp);
    Tachyon::RegisterEvaluationHandler("operator_divide", Operator_Divide);
    Tachyon::RegisterEvaluationHandler("operator_multiply", Operator_Multiply);
    Tachyon::RegisterEvaluationHandler("operator_random", Operator_Random);
    /* string manipulation */
    Tachyon::RegisterEvaluationHandler("operator_length", Operator_Length);
    Tachyon::RegisterEvaluationHandler("operator_letter_of", Operator_LetterOf);
    Tachyon::RegisterEvaluationHandler("operator_join", Operator_Join);
}
