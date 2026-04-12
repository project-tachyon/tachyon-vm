#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Motion.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

static ScratchStatus Motion_GoToXY(ScratchBlock & Block) {
    ScratchData X_Data = Block.GetInputData(0);
    ScratchData Y_Data = Block.GetInputData(1);

    double DestX = X_Data.Type == ScratchData::Type::Number ? X_Data.Number : 0;
    double DestY = Y_Data.Type == ScratchData::Type::Number ? Y_Data.Number : 0;

    ScratchSprite & Owner = Block.GetOwnerSprite();
    Owner.Position.first = (DestX > 255) ? 255 : DestX;
    Owner.Position.second = (DestY > 255) ? 255 : DestY;

    return ScratchStatus::SCRATCH_NEXT;
}

void Motion::RegisterAll(void) {
    Tachyon::RegisterOpHandler("motion_gotoxy", Motion_GoToXY);
}
