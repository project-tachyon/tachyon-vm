#include <Scratch/Data.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>

using namespace Tachyon;
using namespace Scratch;

/**
 * Map containing all pseudo-block handlers.
 */
static std::unordered_map<std::string_view, OpcodeHandler> PseudoHandlers;

bool Pseudo::IsPseudo(std::string ProcCode) {
    return (PseudoHandlers.find(ProcCode) != PseudoHandlers.end());
}

static inline ScratchStatus __hot Tachyon_LoadU8Buffer(ScratchBlock & Block) {
    /* unimplemented for now */
    DebugInfo("Byte buffer loading is not yet implemented. Let's continue...\n");
    return ScratchStatus::SCRATCH_NEXT;
}

static inline ScratchStatus __hot Tachyon_Log(ScratchBlock & Block) {
    ScratchData String = Block.GetInputData(0);
    std::cout << String << std::endl;
    return ScratchStatus::SCRATCH_NEXT;
}

ScratchStatus __hot Pseudo::Execute(std::string ProcCode, Scratch::ScratchBlock & Block) {
    TachyonAssert(Pseudo::IsPseudo(ProcCode) == true);
    auto Item = PseudoHandlers.find(ProcCode);
    return Item->second(Block);
}

static inline void RegisterPseudoHandler(std::string_view ProcCode, OpcodeHandler Handler) {
    TachyonAssert(PseudoHandlers.find(ProcCode) == PseudoHandlers.end());
    PseudoHandlers.emplace(ProcCode, Handler);
}

void Pseudo::RegisterAll(void) {
    RegisterPseudoHandler("Tachyon: Load large UINT8 buffer %s", Tachyon_LoadU8Buffer);
    RegisterPseudoHandler("Tachyon: Log %s", Tachyon_Log);
}
