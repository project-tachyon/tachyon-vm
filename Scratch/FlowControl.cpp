#include <Scratch/Common.hpp>
#include <Scratch/ControlFlow.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>

using namespace Scratch;

static inline ScratchStatus __hot ControlFlow_If(ScratchBlock & Block) {
    ScratchInput Substack = Block.GetInput(0);
    ScratchInput Condition = Block.GetInput(1);
    
    if (unlikely(Substack.Type != ScratchInput::InputType::SubstackInput || Condition.Type != ScratchInput::InputType::ConditionInput)) {
        return ScratchStatus::SCRATCH_END;
    }

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchData Evaluation = Owner.GetBlockFromId(std::get<std::string>(Condition.Input))->Evaluate();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
    if (Evaluation.Boolean == true) {
        CurrentScript->ReturnStack.push_back({ .ReturnId = Block.GetNextKey(), .RepeatId = std::string(), .RepeatsLeft = -1, .InsideProcedure = CurrentScript->InsideProcedure });
        CurrentScript->CurrentBlockId = std::get<std::string>(Substack.Input);
        CurrentScript->ShouldStay = true;
    }
    return ScratchStatus::SCRATCH_NEXT;
}

static inline ScratchStatus __hot ControlFlow_Stop(ScratchBlock & Block) {
    /* just retire the thread for now */
    ScratchMutation & Mutation = Block.GetMutation();
    ScratchField StopOption = Block.GetField(0);
    if (unlikely(StopOption.Type != ScratchField::FieldType::StopOption)) {
        DebugError("Invalid stop option field\n");
        return ScratchStatus::SCRATCH_END;
    }
    std::string Option(std::get<std::string>(StopOption.Field));
    if (Option == "this script") {
        return ScratchStatus::SCRATCH_END;
    } else if (Option == "other scripts in sprite") {
        DebugInfo("TODO: Retire all but the current script\n");
        return ScratchStatus::SCRATCH_NEXT;
    } else {
        DebugInfo("TODO: Retire every script\n");
        return ScratchStatus::SCRATCH_END;
    }
    return ScratchStatus::SCRATCH_END;
}

static inline ScratchStatus __hot ControlFlow_Repeat(ScratchBlock & Block) {
    ScratchData Times = Block.GetInputData(0);
    ScratchInput Substack = Block.GetInput(1);

    if (unlikely(Substack.Type != ScratchInput::InputType::SubstackInput || Times.Type != ScratchData::Type::Number)) {
        DebugError("Invalid repeat!");
        return ScratchStatus::SCRATCH_END;
    }

    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
    CurrentScript->ReturnStack.push_back({ .ReturnId = Block.GetNextKey(), .RepeatId = std::get<std::string>(Substack.Input), .RepeatsLeft = Times.Number - 1, .InsideProcedure = CurrentScript->InsideProcedure });
    CurrentScript->CurrentBlockId = std::get<std::string>(Substack.Input);
    CurrentScript->ShouldStay = true;
    return ScratchStatus::SCRATCH_NEXT;
}

void ControlFlow::RegisterAll(void) {
    Tachyon::RegisterOpHandler("control_if", ControlFlow_If);
    Tachyon::RegisterOpHandler("control_stop", ControlFlow_Stop);
    Tachyon::RegisterOpHandler("control_repeat", ControlFlow_Repeat);
}
