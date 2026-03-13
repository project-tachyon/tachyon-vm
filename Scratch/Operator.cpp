#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Operator.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

static inline __hot ScratchData Operator_Add(ScratchBlock & Block) {
    auto Operand1_Data = Block.GetInputData(0);
    auto Operand2_Data = Block.GetInputData(1);

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

void Operator::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("operator_add", Operator_Add);
    Tachyon::RegisterEvaluationHandler("operator_join", Operator_Join);
    Tachyon::RegisterEvaluationHandler("operator_equals", Operator_Equals);
}
