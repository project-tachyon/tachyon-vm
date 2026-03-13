#include <Scratch/BlockFields.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <string_view>
#include <vector>

using namespace Scratch;

/**
 * A queue.
 */
static std::vector<ScratchScript> SchedulerRunQueue;
static std::vector<ScratchScript> SchedulerYieldQueue;

static ScratchScript * CurrentScript = nullptr;
static ScratchSprite * Stage = nullptr;

/**
 * Map containing all opcode handlers.
 */
static std::unordered_map<std::string_view, OpcodeHandler> OpcodeHandlers;

/**
 * Map containing all reporter handlers.
 */
static std::unordered_map<std::string_view, EvaluationHandler> ReporterHandlers;

void ScratchBlock::LinkHandlers(void) {
    auto OpItem = OpcodeHandlers.find(this->Opcode);
    if (OpItem != OpcodeHandlers.end()) {
        this->Handler = OpItem->second;
    }
    auto EvalItem = ReporterHandlers.find(this->Opcode);
    if (EvalItem != ReporterHandlers.end()) {
        this->ReporterHandler = EvalItem->second;
    }
}

void Tachyon::RegisterOpHandler(std::string_view Opcode, OpcodeHandler Handler) {
    TachyonAssert(OpcodeHandlers.find(Opcode) == OpcodeHandlers.end());
    OpcodeHandlers.emplace(Opcode, Handler);
}

void Tachyon::RegisterEvaluationHandler(std::string_view Opcode, EvaluationHandler Handler) {
    TachyonAssert(ReporterHandlers.find(Opcode) == ReporterHandlers.end());
    ReporterHandlers.emplace(Opcode, Handler);
}

/* tachyon stuff */

ScratchSprite * Tachyon::GetStage(void) {
    return Stage;
}

/* scheduler stuff */

ScratchScript * Tachyon::GetCurrentScript(void) {
    return CurrentScript;
}

void Tachyon::InitializeScheduler(ScratchProject & Project) {
    /* all scripts are BORN ready */
    for(auto & Sprite : Project.Sprites) {
        for(auto & Script : Sprite->Scripts) {
            SchedulerRunQueue.emplace_back(Script);
        }
        if (unlikely(Sprite->IsStage() == true)) {
            Stage = Sprite.get();
        }
    }
    SchedulerYieldQueue.resize(SchedulerRunQueue.size());
    if ((Tachyon::GetVM()->Configuration & TACHYON_CFG_PBLOCK) == false) {
        DebugWarn("Psuedo-blocks are disabled. This likely isn't a problem for projects that don't support Tachyon, but for those that do, you may notice a drop in performance and memory efficiency.\n");
    }
}

static inline ScratchStatus ExecuteScript(ScratchScript & Script) {
    while(Script.CurrentBlockId.empty() == false) {
        ScratchBlock * Block = Script.Sprite->GetBlockFromId(Script.CurrentBlockId);
        if (unlikely(Block == nullptr)) {
            DebugError("WARNING: Invalid block ID: ", Script.CurrentBlockId.c_str());
            return ScratchStatus::SCRATCH_END;
        }
        /* execute block */
        ScratchStatus Status = Block->Execute();
        if (Status != ScratchStatus::SCRATCH_NEXT) {
            return Status;
        }
        if (Script.InsideProcedure == true) {
            if (unlikely(Block->GetNextKey().empty())) {
                Script_StackFrame CurrentStackFrame = Script.ReturnStack.back();
                Script.InsideProcedure = CurrentStackFrame.InsideProcedure;
                Script.CurrentBlockId = CurrentStackFrame.ReturnId;
                Script.ReturnStack.pop_back();
                continue;
            } else {
                Block = Script.Sprite->GetBlockFromId(Script.CurrentBlockId);
            }
        }
        if (Script.ShouldStay == false) {
            Script.CurrentBlockId = Block->GetNextKey();
            if (unlikely(Script.CurrentBlockId.empty() == true)) {
                Script_StackFrame & CurrentStackFrame = Script.ReturnStack.back();
                if (CurrentStackFrame.RepeatsLeft < 0) {
                    return ScratchStatus::SCRATCH_END;
                }
                CurrentStackFrame.RepeatsLeft--;
                if (likely(CurrentStackFrame.RepeatsLeft < 0)) {
                    Script.CurrentBlockId = CurrentStackFrame.ReturnId;
                    Script.InsideProcedure = CurrentStackFrame.InsideProcedure;
                    Script.ReturnStack.pop_back();
                    continue;
                }
                Script.CurrentBlockId = CurrentStackFrame.RepeatId;
            }
        } else {
            /* reset */
            Script.ShouldStay = false;
        }
    }
    return ScratchStatus::SCRATCH_END;
}

bool __hot Tachyon::Step(void) {
    if (unlikely(SchedulerRunQueue.empty() == true)) {
        /* exit */
        std::cout << "no more scripts to run. exiting..." << std::endl;
        return true;
    }
    /* NOTE: waking up scripts isnt implemeneted, so they'll be sleeping beauty for the time being */
    CurrentScript = &SchedulerRunQueue.at(0);
    ScratchStatus Status = ExecuteScript(*CurrentScript);
    if (Status == ScratchStatus::SCRATCH_YIELD_WAIT || Status == ScratchStatus::SCRATCH_YIELD_WAIT_UNTIL) {
        /* let others breathe */
        CurrentScript->CurrentStatus = Status;
        SchedulerYieldQueue.emplace_back(*CurrentScript);
        SchedulerRunQueue.erase(SchedulerRunQueue.begin());
    } else if (Status == ScratchStatus::SCRATCH_END) {
        SchedulerRunQueue.erase(SchedulerRunQueue.begin());
    }
    return false;
}
