#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Looks.hpp>

using namespace Scratch;

static inline ScratchStatus Looks_Say(ScratchBlock & Block) {
    ScratchData Data = Block.GetInputData(0);
    std::cout << "looks_say: " << Data << std::endl;
    return ScratchStatus::SCRATCH_NEXT;
}

void Looks::RegisterAll(void) {
    Tachyon::RegisterOpHandler("looks_say", Looks_Say);
}
