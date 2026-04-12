#include <Tachyon/Debug.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Data.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

static ScratchStatus __hot Data_DeleteAllOfList(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);
    TachyonAssert(Field.Type == ScratchField::FieldType::ListField);

    Field_Variable FieldVar = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchList * List = Owner.GetListFromKey(FieldVar.VariableKey);
    TachyonAssert(List != nullptr);
    List->ClearElements();
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_AddToList(ScratchBlock & Block) {
    ScratchData Data = Block.GetInputData(0);
    ScratchField Field = Block.GetField(0);
    TachyonAssert(Field.Type == ScratchField::FieldType::ListField);

    Field_Variable FieldVar = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchList * List = Owner.GetListFromKey(FieldVar.VariableKey);
    TachyonAssert(List != nullptr);
    List->Append(std::move(Data));
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_ReplaceItem(ScratchBlock & Block) {
    ScratchData Index = Block.GetInputData(0);
    ScratchData Data = Block.GetInputData(1);
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::ListField);

    ScratchSprite & Owner = Block.GetOwnerSprite();
    Field_Variable FieldList = std::get<Field_Variable>(Field.Field);

    ScratchList * List = Owner.GetListFromKey(FieldList.VariableKey);
    TachyonAssert(List != nullptr);

    TachyonAssert(Index.Type != ScratchData::Type::Boolean);
    if (unlikely(Index.Type == ScratchData::Type::String)) {
        if (Index.String == "last") {
            List->Set(std::move(Data), List->TotalItems - 1);
        } else {
            DebugWarn("Invalid item replace index string.\n");
            return ScratchStatus::SCRATCH_NEXT;
        }
    }

    if (Index.AsDouble() < 0) {
        return ScratchStatus::SCRATCH_NEXT;
    }

    List->Set(std::move(Data), Index.AsDouble() - 1);

    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_SetVariable(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::VariableField);

    Field_Variable FieldVar = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchVariable * Variable = Owner.GetVariableFromKey(FieldVar.VariableKey);
    TachyonAssert(Variable != nullptr);
    ScratchData Data = Block.GetInputData(0);
    Variable->SetData(std::move(Data));
    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchStatus __hot Data_ChangeVariableBy(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::VariableField);

    Field_Variable FieldVar = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchVariable * Variable = Owner.GetVariableFromKey(FieldVar.VariableKey);
    TachyonAssert(Variable != nullptr);

    ScratchData Num = Block.GetInputData(0);
    ScratchData Data = Variable->GetData();

    Variable->SetData(Data.AsDouble() + Num.AsDouble());

    return ScratchStatus::SCRATCH_NEXT;
}

static ScratchData __hot Data_ItemOfList(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);
    ScratchData IndexInput = Block.GetInputData(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::ListField);

    Field_Variable FieldList = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchList * List = Owner.GetListFromKey(FieldList.VariableKey);
    TachyonAssert(List != nullptr);

    if (IndexInput.AsDouble() < 0) {
        return "";
    }

    return List->Get(IndexInput.AsDouble() - 1);
}

static ScratchData __hot Data_LengthOfList(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::ListField);

    Field_Variable FieldList = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchList * List = Owner.GetListFromKey(FieldList.VariableKey);
    TachyonAssert(List != nullptr);

    return double(List->TotalItems);
}

void Data::RegisterAll(void) {
    Tachyon::RegisterOpHandler("data_deletealloflist", Data_DeleteAllOfList);
    Tachyon::RegisterOpHandler("data_setvariableto", Data_SetVariable);
    Tachyon::RegisterOpHandler("data_addtolist", Data_AddToList);
    Tachyon::RegisterOpHandler("data_changevariableby", Data_ChangeVariableBy);
    Tachyon::RegisterOpHandler("data_replaceitemoflist", Data_ReplaceItem);
    Tachyon::RegisterEvaluationHandler("data_itemoflist", Data_ItemOfList);
    Tachyon::RegisterEvaluationHandler("data_lengthoflist", Data_LengthOfList);
}
