#include <Scratch/BlockFields.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Events.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Debug.hpp>

using namespace Scratch;

static ScratchStatus EventBroadcast(ScratchBlock & Block) {
    ScratchSprite & Owner = Block.GetOwnerSprite();
    /* TODO: do the same thing as EventBroadcastAsync but instead of creating a seperate script, push it in the call stack */
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus EventBroadcastAsync(ScratchBlock & Block) {
    ScratchProject & Project = *Tachyon::GetLoadedProject();
    ScratchInput Input = Block.GetInput(0);
    if (Input.Type != ScratchInput::InputType::BroadcastInput) {
        return ScratchStatus::SCRATCH_END;
    }
    const std::string BroadcastInputKey = std::get<std::string>(Input.Input);
    /* search in the sprite */
    for(auto & Sprite : Project.Sprites) {
        for(const auto & Broadcast : Sprite->BroadcastReceivers) {
            ScratchBlock BroadcastBlock = *Broadcast.second;
            ScratchField Field = BroadcastBlock.GetField(0);
            if (Field.Type != ScratchField::FieldType::BroadcastOption) {
                continue;
            }
            const std::string BroadcastFieldKey = std::get<std::string>(Field.Field);
            if (BroadcastInputKey == BroadcastFieldKey) {
                std::string NextBroadcastKey = BroadcastBlock.GetKey();
                if (NextBroadcastKey.empty() == true) {
                    /* nothing to do for the broadcast */
                    continue;
                }
                ScratchBlock * NextBroadcastBlock = Sprite->GetBlockFromId(NextBroadcastKey);
                if (NextBroadcastBlock == nullptr) {
                    /* bad broadcast */
                    continue;
                }
                Sprite->CreateScript(*NextBroadcastBlock);
            }
        }
    }
    return ScratchStatus::SCRATCH_NEXT;
}

void Events::RegisterAll(void) {
    Tachyon::RegisterOpHandler("event_broadcast", EventBroadcastAsync);
    Tachyon::RegisterOpHandler("event_broadcastandwait", EventBroadcast);
}
