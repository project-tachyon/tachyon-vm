#include <Scratch/BlockFields.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>
#include <string_view>
#include <variant>
#include <vector>
#include <deque>

using namespace Scratch;

/**
 * A queue containing runnable threads/scripts.
 */
static std::deque<ScratchScript> SchedulerRunQueue;

/**
 * A queue containing blocked threads/scripts.
 */
static std::deque<ScratchScript> SchedulerYieldQueue;

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

ScratchSprite * __hot Tachyon::GetStage(void) {
    return Stage;
}

/* scheduler stuff */

ScratchScript * __hot Tachyon::GetCurrentScript(void) {
    return CurrentScript;
}

void Tachyon::InitializeScheduler(ScratchProject & Project) {
    Tachyon::GetVM()->Project = &Project;
    /* all scripts are BORN ready */
    for(auto & Sprite : Project.Sprites) {
        for(auto & Script : Sprite->Scripts) {
            SchedulerRunQueue.emplace_back(Script);
        }
        if (Sprite->IsStage() == true) {
            Stage = Sprite.get();
        }
    }
    SchedulerYieldQueue.resize(SchedulerRunQueue.size());
    if ((Tachyon::GetConfigVM() & TACHYON_CFG_PBLOCK) == false) {
        DebugWarn("Pseudo-blocks are disabled. This likely isn't a problem for projects that don't support Tachyon, but for those that do, you may notice a drop in performance and memory efficiency.\n");
    }
}

void __hot Tachyon::ScriptAddReadyQueue(ScratchScript Script) {
    SchedulerRunQueue.emplace_back(Script);
}

/**
 * This is where scripts are executed if you couldn't already tell.
 * @param The script to execute
 * @return Status of the script
 */
static ScratchStatus __hot ExecuteScript(ScratchScript & Script) {
    while(true) {
        ScratchBlock * Block = Script.Sprite->GetBlockFromId(Script.CurrentBlockId);
        TachyonAssert(Block != nullptr);
        /* execute block */
        //DebugInfo("opcode: %s\n", Block->GetOpcode().c_str());
        ScratchStatus Status = Block->Execute();
        if (Status != ScratchStatus::SCRATCH_NEXT) {
            return Status;
        }
        if (GetControlFlag(Script, SCRIPT_INVALIDATE_BLOCK) == true) {
            /* validate */
            UnsetControlFlag(Script, SCRIPT_INVALIDATE_BLOCK);
            if (unlikely(Script.CurrentBlockId.empty() == true)) {
                if (GetControlFlag(Script, SCRIPT_INSIDE_PROCEDURE) == true) {
                    /* get out */
                    ScriptReturn(Script);
                    UnbindParameters(Script);
                    continue;
                }
            }
            Block = Script.Sprite->GetBlockFromId(Script.CurrentBlockId);
        }
        if (GetControlFlag(Script, SCRIPT_SHOULD_STAY) == true) {
            UnsetControlFlag(Script, SCRIPT_SHOULD_STAY);
            continue;
        }
        std::string NextId = Block->GetNextKey();
        if (NextId.empty() == false) {
            Script.CurrentBlockId = NextId;
            continue;
        }
        /* we're either done executing, or repeating */
        if (Script.ReturnStack.empty() == true) {
            return ScratchStatus::SCRATCH_END;
        }
        Script_StackFrame & CurrentStackFrame = Script.ReturnStack.back();
        /* its possible we're just an ending procedure */
        if (ShouldRepeatConditionally(CurrentStackFrame) == false) {
            double & RepeatsLeft = std::get<double>(CurrentStackFrame.RepeatCondition);
            if (RepeatsLeft < 0) {
                if (GetControlFlag(Script, SCRIPT_INSIDE_PROCEDURE) == true) {
                    UnbindParameters(Script);
                }
                ScriptReturn(Script);
                return ScriptRecursiveReturn(Script);
            }
            /* repeats should only be below here */
            RepeatsLeft--;
            if (RepeatsLeft < 0) {
                ScriptReturn(Script);
                /* recursive check because the old logic was kinda shit */
                return ScriptRecursiveReturn(Script);
            }
            Script.CurrentBlockId = CurrentStackFrame.RepeatId;
            continue;
        } else {
            ScratchBlock * ConditionalBlock = std::get<ScratchBlock *>(CurrentStackFrame.RepeatCondition);
            TachyonAssert(ConditionalBlock != nullptr);
            ScratchData EvalResult = ConditionalBlock->Evaluate();
            TachyonAssert(EvalResult.Type == ScratchData::Type::Boolean);
            if (EvalResult.Boolean == true) {
                Script.CurrentBlockId = CurrentStackFrame.RepeatId;
                continue;
            }
            ScriptReturn(Script);
            /* recursive check because the old logic was kinda shit */
            return ScriptRecursiveReturn(Script);
        }
    }
    __unreachable;
}

static inline void DumpScriptInformation(ScratchScript & Script) {
    DebugInfo("---- SCRIPT INFORMATION DUMP ----\n");
    DebugInfo("Owner sprite name: %s\n", Script.Sprite->GetName().data());
    DebugInfo("First block ID: %s\n", Script.FirstBlockId.c_str());
    DebugInfo("Return stack total pending calls: %d\n", Script.ReturnStack.size());
    DebugInfo("Script CFLAGS: 0x%02x\n", Script.ControlFlags);
    DebugInfo("Script stack backtrace (top = most recent frame):\n");
    for(int i = Script.ReturnStack.size() - 1; i >= 0; i--) {
        const Script_StackFrame & StackFrame = Script.ReturnStack.at(i);
        DebugInfo("Frame #%lu:\n", i);
        DebugInfo("\tReturn block ID: %s\n", StackFrame.ReturnId.c_str());
        DebugInfo("\tWas in procedure? %s\n", StackFrame.InsideProcedure ? "true" : "false");
        if (StackFrame.RepeatId.empty() == false) {
            DebugInfo("\tRepeat start block ID: %s\n", StackFrame.RepeatId.c_str());
            if (std::holds_alternative<double>(StackFrame.RepeatCondition) == true) {
                /* repeat times */
                const double RepeatsLeft = std::get<double>(StackFrame.RepeatCondition);
                DebugInfo("\tTotal repeats left: %.0f\n", RepeatsLeft + 1);
            } else {
                /* repeat condition */
                ScratchBlock * ConditionBlock = std::get<ScratchBlock *>(StackFrame.RepeatCondition);
                TachyonAssert(ConditionBlock != nullptr);
                DebugInfo("\tRepeat condition block ID: %s\n", ConditionBlock->GetKey().c_str());
            }
        }
    }
}

static inline ScratchScript * __hot GetNextScript(void) {
    if (likely(SchedulerRunQueue.empty() == false)) {
        return &SchedulerRunQueue.front();
    }
    return nullptr;
} 

bool __hot Tachyon::Step(void) {
    /* NOTE: waking up scripts isnt implemeneted, so they'll be sleeping beauty for the time being */
    CurrentScript = GetNextScript();
    if (unlikely(CurrentScript == nullptr)) {
        return true;
    }
    ScratchStatus Status = ExecuteScript(*CurrentScript);
    if (Status == ScratchStatus::SCRATCH_WAIT || Status == ScratchStatus::SCRATCH_WAIT_UNTIL || Status == ScratchStatus::SCRATCH_PAUSE) {
        /* let others breathe */
        CurrentScript->CurrentStatus = Status;
        SchedulerYieldQueue.emplace_back(*CurrentScript);
        SchedulerRunQueue.erase(SchedulerRunQueue.begin());
    } else if (Status == ScratchStatus::SCRATCH_END) {
        DumpScriptInformation(*CurrentScript);
        SchedulerRunQueue.erase(SchedulerRunQueue.begin());
    }
    return false;
}
