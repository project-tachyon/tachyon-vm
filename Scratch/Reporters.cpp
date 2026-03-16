#include <Scratch/Reporters.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <string>

using namespace Scratch;

static inline __hot ScratchData Reporter_Boolean(ScratchBlock & Block) {
    const ScratchField Field = Block.GetField(0);
    std::string Name = std::get<std::string>(Field.Field);
    if (Name == "Is Tachyon?") {
        return true;
    }
    return false;
}

void Reporters::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("argument_reporter_boolean", Reporter_Boolean);
}
