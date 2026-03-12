#include "Scratch/Data.hpp"
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>

using namespace Tachyon;
using namespace Scratch;

/**
 * Map containing all psuedo-block handlers.
 */
static std::unordered_map<std::string_view, OpcodeHandler> PsuedoHandlers;

bool Psuedo::IsPsuedo(std::string ProcCode) {
    return (PsuedoHandlers.find(ProcCode) != PsuedoHandlers.end());
}


static inline ScratchStatus __hot Tachyon_LoadU8Buffer(ScratchBlock & Block) {
    /* unimplemented for now */
    return ScratchStatus::SCRATCH_NEXT;
}

/* TODO: add ^ operator overload */
// static inline ScratchData __hot Tachyon_XOR(ScratchBlock & Block) {
//     ScratchData Operand1 = Block.GetInputData(0);
//     ScratchData Operand2 = Block.GetInputData(1);
//     return (Operand1 ^ Operand2);
// }

ScratchStatus __hot Psuedo::Execute(std::string ProcCode, Scratch::ScratchBlock & Block) {
    TachyonAssert(Psuedo::IsPsuedo(ProcCode) == true);
    auto Item = PsuedoHandlers.find(ProcCode);
    return Item->second(Block);
}

static inline void RegisterPsuedoHandler(std::string_view ProcCode, OpcodeHandler Handler) {
    TachyonAssert(PsuedoHandlers.find(ProcCode) == PsuedoHandlers.end());
    PsuedoHandlers.emplace(ProcCode, Handler);
}

void Psuedo::RegisterAll(void) {
    RegisterPsuedoHandler("Tachyon: Load large UINT8 buffer %s", Tachyon_LoadU8Buffer);
}
