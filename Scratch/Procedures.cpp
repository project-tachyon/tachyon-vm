#include "Scratch/BlockFields.hpp"
#include <Tachyon/Debug.hpp>
#include <Scratch/Procedures.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Common.hpp>
#include <Scratch/Data.hpp>
#include <Tachyon/Tachyon.hpp>
#include <unordered_map>

using namespace Scratch;

void ScratchSprite::ResolveProcedureDefinitions(void) {
    std::cout << "resolving procedure definitions for " << this->Name << std::endl;
    for(auto & Item : this->ProcedureDefinitions) {
        auto & ProcDef = Item.second;
        ScratchInput Input = ProcDef->GetInput(0);

        TachyonAssert(Input.Type == ScratchInput::InputType::ProcedureDefinition);

        struct ScratchProcedure Procedure;
        Procedure.DefinitionKey = ProcDef->GetKey();
        Procedure.PrototypeKey = std::get<std::string>(Input.Input);

        ScratchBlock * Prototype = this->GetBlockFromId(Procedure.PrototypeKey);
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
            if (Tachyon::GetConfigVM() & TACHYON_CFG_PBLOCK) {
                if (Tachyon::Pseudo::IsPseudo(Procedure.ProcCode) == true) {
                    /* act as if nothing ever happened */
                    CurrentScript->CurrentBlockId = Block.GetNextKey();
                    return Tachyon::Pseudo::Execute(Procedure.ProcCode, Block);
                }
            }
            ScratchBlock * ProcBlock = Owner.GetBlockFromId(Procedure.DefinitionKey);
            TachyonAssert(ProcBlock != nullptr);

            /* bind parameters */
            size_t TotalParams = Procedure.ParametersNames.size();
            std::unordered_map<std::string, ScratchData> ParamBindings;
            ParamBindings.reserve(TotalParams);

            for(size_t i = 0; i < TotalParams; i++) {
                std::string & ParamName = Procedure.ParametersNames[i];
                ScratchData Data = Block.GetInputData(i);
                /* off you go little one */
                ParamBindings[ParamName] = std::move(Data);
            }

            CurrentScript->ReturnStack.push_back({
                .ReturnId = Block.GetNextKey(),
                .RepeatId = std::string(),
                .RepeatCondition = double(-1),
                .InsideProcedure = GetControlFlag(*CurrentScript, SCRIPT_INSIDE_PROCEDURE) 
            });

            CurrentScript->ParamBindings.push_back(ParamBindings);

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
