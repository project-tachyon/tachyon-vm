#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Sensing.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

static inline __hot ScratchData Sensing_Timer(ScratchBlock & Block) {
    return "Cool";
}

void Sensing::RegisterAll(void) {
    Tachyon::RegisterEvaluationHandler("sensing_timer", Sensing_Timer);
}
