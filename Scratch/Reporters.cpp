#include <Scratch/Common.hpp>
#include <Scratch/Reporters.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <string>
#include <unordered_map>

using namespace Scratch;

static inline ScratchData __hot GetBindedParameter(std::string ParamName, ScratchScript & CurrentScript) {
    TachyonAssert(CurrentScript.ParamBindings.empty() == false);

    std::unordered_map<std::string, ScratchData> Map = CurrentScript.ParamBindings.back();

    auto Item = Map.find(ParamName);

    if (unlikely(Item == Map.end())) {
        return double(0);
    }

    return Item->second;
}

static inline __hot ScratchData Reporter_Boolean(ScratchBlock & Block) {
    const ScratchField Field = Block.GetField(0);
    std::string ParamName = std::get<std::string>(Field.Field);
    if (ParamName == "Is Tachyon?") {
        return true;
    }
    ScratchData Return = GetBindedParameter(ParamName, *Tachyon::GetCurrentScript());
    return Return;
}

static inline __hot ScratchData Reporter_StringNum(ScratchBlock & Block) {
    const ScratchField Field = Block.GetField(0);
    std::string ParamName = std::get<std::string>(Field.Field);
    ScratchData Return = GetBindedParameter(ParamName, *Tachyon::GetCurrentScript());
    return Return;
}

void Reporters::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("argument_reporter_boolean", Reporter_Boolean);
    Tachyon::RegisterEvaluationHandler("argument_reporter_string_number", Reporter_StringNum);
}
