#include "Tachyon/Debug.hpp"
#include <Scratch/BlockFields.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Tachyon/Tachyon.hpp>
#include <string_view>
#include <vector>

using namespace Scratch;

/**
 * A queue.
 */
static std::vector<ScratchScript> SchedulerRunQueue;
static std::vector<ScratchScript> SchedulerYieldQueue;

static ScratchScript * CurrentScript;

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

void ScratchSprite::ResolveProcedureDefinitions(void) {
    std::cout << "resolving procedure definitions for " << this->Name << std::endl;
    for(auto & ProcDef : this->ProcedureDefinitions) {
        ScratchInput Input = ProcDef->GetInput(0);
        if (Input.Type == ScratchInput::InputType::InvalidInput || Input.Type != ScratchInput::InputType::ProcedureDefinition) {
            std::cerr << "Invalid procedure definition" << std::endl;
            continue;
        }
        struct ScratchProcedure Procedure;
        Procedure.DefinitionKey = ProcDef->GetKey();
        Procedure.PrototypeKey = std::get<std::string>(Input.Input);
        ScratchBlock * Prototype = this->GetBlockFromId(Procedure.PrototypeKey);
        /* we read from the prototype mutation from here */
        ScratchMutation Mutation = Prototype->GetMutation();
        Procedure.UseWarp = Mutation.UseWarp;
        Procedure.ProcCode = Mutation.ProcCode;
        Procedure.ParametersNames = Mutation.ParametersNames;
        Procedure.ParametersKeys = Mutation.ParametersKeys;
        this->Procedures.emplace_back(Procedure);
    }
}

static ScratchStatus __hot Procedures_Call(ScratchBlock & Block) {
    ScratchMutation & Mutation = Block.GetMutation();
    ScratchSprite & Owner = Block.GetOwnerSprite();
    for (auto & Procedure : Owner.Procedures) {
        if (Procedure.ProcCode == Mutation.ProcCode) {
            if (Tachyon::GetVM()->Configuration & TACHYON_CFG_PBLOCK) {
                if (Tachyon::Psuedo::IsPsuedo(Procedure.ProcCode) == true) {
                    /* act as if nothing ever happened */
                    CurrentScript->CurrentBlockId = Block.GetNextKey();
                    return Tachyon::Psuedo::Execute(Procedure.ProcCode, Block);
                }
            }
            ScratchBlock * ProcBlock = Owner.GetBlockFromId(Procedure.DefinitionKey);
            TachyonAssert(ProcBlock != nullptr);
            CurrentScript->ReturnStack.push({ .ReturnId = Block.GetNextKey(), .InsideProcedure = CurrentScript->InsideProcedure });
            CurrentScript->InsideProcedure = true;
            CurrentScript->CurrentBlockId = Procedure.DefinitionKey;
            return ScratchStatus::SCRATCH_NEXT;
        }
    }
    return ScratchStatus::SCRATCH_END;
}

void Procedures::RegisterAll(void) {
    Tachyon::RegisterOpHandler("procedures_call", Procedures_Call);
}

/* scheduler stuff */

void Tachyon::InitializeScheduler(ScratchProject & Project) {
    /* all scripts are BORN ready */
    for(auto & Sprite : Project.Sprites) {
        for(auto & Script : Sprite->Scripts) {
            SchedulerRunQueue.emplace_back(Script);
        }
    }
    SchedulerYieldQueue.resize(SchedulerRunQueue.size());
}

static inline ScratchStatus ExecuteScript(ScratchScript & Script) {
    while(Script.CurrentBlockId.empty() == false) {
        ScratchBlock * Block = Script.Sprite->GetBlockFromId(Script.CurrentBlockId);
        if (unlikely(Block == nullptr)) {
            DebugError("WARNING: Invalid block ID: ", Script.CurrentBlockId.c_str());
            return ScratchStatus::SCRATCH_END;
        }
        std::cout << "execute: " << Block->GetOpcode() << std::endl;
        /* execute block */
        ScratchStatus Status = Block->Execute();
        if (Status != ScratchStatus::SCRATCH_NEXT) {
            return Status;
        }
        if (Script.InsideProcedure == true) {
            if (unlikely(Block->GetNextKey().empty())) {
                Script_StackFrame CurrentStackFrame = Script.ReturnStack.top();
                Script.InsideProcedure = CurrentStackFrame.InsideProcedure;
                Script.CurrentBlockId = CurrentStackFrame.ReturnId;
                Script.ReturnStack.pop();
                return ScratchStatus::SCRATCH_NEXT;
            } else {
                Block = Script.Sprite->GetBlockFromId(Script.CurrentBlockId);
            }
        }
        Script.CurrentBlockId = Block->GetNextKey();
        if (unlikely(Script.CurrentBlockId.empty() == true)) {
            return ScratchStatus::SCRATCH_END;
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
