#include <Tachyon/Debug.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Data.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

void ScratchSprite::ResolveProcedureDefinitions(void) {
    std::cout << "resolving procedure definitions for " << this->Name << std::endl;
    for(auto & Item : this->ProcedureDefinitions) {
        auto & ProcDef = Item.second;
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

static inline ScratchStatus __hot Procedures_Call(ScratchBlock & Block) {
    ScratchMutation & Mutation = Block.GetMutation();
    ScratchSprite & Owner = Block.GetOwnerSprite();
    for (auto & Procedure : Owner.Procedures) {
        if (Procedure.ProcCode == Mutation.ProcCode) {
            ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
            if (unlikely(CurrentScript == nullptr)) {
                return ScratchStatus::SCRATCH_END;
            }
            if (Tachyon::GetVM()->Configuration & TACHYON_CFG_PBLOCK) {
                if (Tachyon::Psuedo::IsPsuedo(Procedure.ProcCode) == true) {
                    /* act as if nothing ever happened */
                    CurrentScript->CurrentBlockId = Block.GetNextKey();
                    return Tachyon::Psuedo::Execute(Procedure.ProcCode, Block);
                }
            }
            ScratchBlock * ProcBlock = Owner.GetBlockFromId(Procedure.DefinitionKey);
            TachyonAssert(ProcBlock != nullptr);
            CurrentScript->ReturnStack.push_back({
                .ReturnId = Block.GetNextKey(),
                .RepeatId = std::string(),
                .RepeatsLeft = -1,
                .InsideProcedure = GetControlFlag(*CurrentScript, SCRIPT_INSIDE_PROCEDURE) 
            });
            SetControlFlag(*CurrentScript, (SCRIPT_SHOULD_STAY | SCRIPT_INSIDE_PROCEDURE | SCRIPT_INVALIDATE_BLOCK));
            CurrentScript->CurrentBlockId = ProcBlock->GetNextKey();
            return ScratchStatus::SCRATCH_NEXT;
        }
    }
    return ScratchStatus::SCRATCH_END;
}

void Procedures::RegisterAll(void) {
    Tachyon::RegisterOpHandler("procedures_call", Procedures_Call);
}
