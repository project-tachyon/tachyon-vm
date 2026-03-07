#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Operator.hpp>
#include <Tachyon.hpp>

using namespace Scratch;

static ScratchData Operator_Add(ScratchBlock & Block) {
    ScratchData Operand1_Data = Block.GetInputData(0);
    ScratchData Operand2_Data = Block.GetInputData(1);

    double Result = (Operand1_Data.Type == ScratchData::Type::Number ? Operand1_Data.Number : 0) + (Operand2_Data.Type == ScratchData::Type::Number ? Operand2_Data.Number : 0);

    return { .Type = ScratchData::Type::Number, .Number = Result };
}

void Operator::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("operator_add", Operator_Add);
}
