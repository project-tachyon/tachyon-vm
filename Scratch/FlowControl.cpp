#include "Compiler.hpp"
#include "Scratch/Data.hpp"
#include <Scratch/Common.hpp>
#include <Scratch/ControlFlow.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Debug.hpp>

using namespace Scratch;

static inline ScratchStatus __hot ControlFlow_If(ScratchBlock & Block) {
    ScratchInput Condition = Block.GetInput(0);
    ScratchInput Substack = Block.GetInput(1);
    
    if (unlikely(Condition.Type == ScratchInput::InputType::InvalidInput)) {
        /* no condition is always false. advance to the next block */
        return ScratchStatus::SCRATCH_NEXT;
    }

    if (unlikely(Substack.Type != ScratchInput::InputType::SubstackInput || Condition.Type != ScratchInput::InputType::ConditionInput)) {
        DebugError("Invalid if statement block.\n");
        DebugError("Substack type: %d\n", Substack.Type);
        DebugError("Condition type: %d\n", Condition.Type);
        return ScratchStatus::SCRATCH_END;
    }

    ScratchSprite & Owner = Block.GetOwnerSprite();
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);
    std::string SubstackFirstBlock = std::get<std::string>(Substack.Input);

    if (unlikely(SubstackFirstBlock.empty() == true)) {
        /* will always equate to false anyways */
        return ScratchStatus::SCRATCH_NEXT;
    }

    ScratchData Evaluation = Owner.GetBlockFromId(ConditionBlockId)->Evaluate();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();

    if (Evaluation.Boolean == true) {
        CurrentScript->ReturnStack.push_back({
            .ReturnId = Block.GetNextKey(),
            .RepeatId = std::string(),
            .RepeatCondition = double(-1),
            .InsideProcedure = bool(GetControlFlag(*CurrentScript, SCRIPT_INSIDE_PROCEDURE)) 
        });
        CurrentScript->CurrentBlockId = SubstackFirstBlock;
        SetControlFlag(*CurrentScript, SCRIPT_SHOULD_STAY);
    }
    return ScratchStatus::SCRATCH_NEXT;
}

static inline ScratchStatus __hot ControlFlow_IfElse(ScratchBlock & Block) {
    ScratchInput Condition = Block.GetInput(0);
    ScratchInput SubstackIf = Block.GetInput(1);
    ScratchInput SubstackElse = Block.GetInput(2);
    
    if (unlikely(Condition.Type == ScratchInput::InputType::InvalidInput)) {
        /* no condition is always false. advance to the next block */
        return ScratchStatus::SCRATCH_NEXT;
    }

    if (unlikely(SubstackIf.Type != ScratchInput::InputType::SubstackInput || SubstackElse.Type != ScratchInput::InputType::SubstackInput || Condition.Type != ScratchInput::InputType::ConditionInput)) {
        DebugError("Invalid if statement block.\n");
        DebugError("If substack type: %d\n", SubstackIf.Type);
        DebugError("Else substack type: %d\n", SubstackElse.Type);
        DebugError("Condition type: %d\n", Condition.Type);
        return ScratchStatus::SCRATCH_END;
    }

    ScratchSprite & Owner = Block.GetOwnerSprite();
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);
    std::string SubstackIfFirstBlock = std::get<std::string>(SubstackIf.Input);
    std::string SubstackElseFirstBlock = std::get<std::string>(SubstackElse.Input);

    ScratchData Evaluation = Owner.GetBlockFromId(ConditionBlockId)->Evaluate();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();

    /* we're going into another stack whether we like it or not */
    CurrentScript->ReturnStack.push_back({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = std::string(),
        .RepeatCondition = double(-1),
        .InsideProcedure = bool(GetControlFlag(*CurrentScript, SCRIPT_INSIDE_PROCEDURE)) 
    });

    CurrentScript->CurrentBlockId = Evaluation.Boolean ? SubstackIfFirstBlock : SubstackElseFirstBlock;
    SetControlFlag(*CurrentScript, SCRIPT_SHOULD_STAY);
    return ScratchStatus::SCRATCH_NEXT;
}

static inline ScratchStatus __hot ControlFlow_Stop(ScratchBlock & Block) {
    /* just retire the thread for now */
    ScratchMutation & Mutation = Block.GetMutation();
    ScratchField StopOption = Block.GetField(0);
    
    TachyonAssert(StopOption.Type == ScratchField::FieldType::StringField);

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
    ScratchInput Substack = Block.GetInput(0);
    ScratchData Times = Block.GetInputData(1);

    if (unlikely(Substack.Type != ScratchInput::InputType::SubstackInput)) {
        DebugError("Invalid repeat block.\n");
        DebugError("Substack type: %d\n", Substack.Type);
        return ScratchStatus::SCRATCH_END;
    }
    
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();
    std::string SubstackFirstBlock = std::get<std::string>(Substack.Input);

    if (unlikely(SubstackFirstBlock.empty() == true || Times.Type != ScratchData::Type::Number)) {
        return ScratchStatus::SCRATCH_NEXT;
    }

    CurrentScript->ReturnStack.push_back({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = SubstackFirstBlock,
        .RepeatCondition = Times.Number,
        .InsideProcedure = bool(GetControlFlag(*CurrentScript, SCRIPT_INSIDE_PROCEDURE)) 
    });
    CurrentScript->CurrentBlockId = SubstackFirstBlock;
    SetControlFlag(*CurrentScript, (SCRIPT_INVALIDATE_BLOCK));
    return ScratchStatus::SCRATCH_NEXT;
}

static inline ScratchStatus __hot ControlFlow_While(ScratchBlock & Block) {
    ScratchInput Condition = Block.GetInput(0);
    ScratchInput Substack = Block.GetInput(1);

    if (unlikely(Condition.Type == ScratchInput::InputType::InvalidInput)) {
        /* nothing to execute if the condition is false */
        return ScratchStatus::SCRATCH_NEXT;
    }

    TachyonAssert(Substack.Type == ScratchInput::InputType::SubstackInput && Condition.Type == ScratchInput::InputType::ConditionInput);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchScript * CurrentScript = Tachyon::GetCurrentScript();

    std::string SubstackFirstBlock = std::get<std::string>(Substack.Input);
    std::string ConditionBlockId = std::get<std::string>(Condition.Input);

    ScratchBlock * ConditionBlock = Owner.GetBlockFromId(ConditionBlockId);

    if (unlikely(SubstackFirstBlock.empty() == true)) {
        return ScratchStatus::SCRATCH_NEXT;
    }
    CurrentScript->ReturnStack.push_back({
        .ReturnId = Block.GetNextKey(),
        .RepeatId = SubstackFirstBlock,
        .RepeatCondition = ConditionBlock,
        .InsideProcedure = bool(GetControlFlag(*CurrentScript, SCRIPT_INSIDE_PROCEDURE)) 
    });
    CurrentScript->CurrentBlockId = SubstackFirstBlock;
    SetControlFlag(*CurrentScript, (SCRIPT_INVALIDATE_BLOCK));
    return ScratchStatus::SCRATCH_NEXT;
}

void ControlFlow::RegisterAll(void) {
    Tachyon::RegisterOpHandler("control_if", ControlFlow_If);
    Tachyon::RegisterOpHandler("control_if_else", ControlFlow_IfElse);
    Tachyon::RegisterOpHandler("control_stop", ControlFlow_Stop);
    Tachyon::RegisterOpHandler("control_repeat", ControlFlow_Repeat);
    Tachyon::RegisterOpHandler("control_while", ControlFlow_While);
}
