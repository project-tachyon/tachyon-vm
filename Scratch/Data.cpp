#include <Tachyon/Debug.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Data.hpp>
#include <Tachyon/Tachyon.hpp>

using namespace Scratch;

static inline ScratchStatus __hot Data_DeleteAllOfList(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);
    TachyonAssert(Field.Type != ScratchField::FieldType::InvalidField || Field.Type == ScratchField::FieldType::ListField);

    Field_Variable FieldVar = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchList * List = Owner.GetList(FieldVar.VariableKey);
    TachyonAssert(List != nullptr);
    //List->DiscardData();
    return ScratchStatus::SCRATCH_NEXT;
}

static inline ScratchStatus __hot Data_SetVariable(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type != ScratchField::FieldType::InvalidField || Field.Type == ScratchField::FieldType::VariableField);

    Field_Variable FieldVar = std::get<Field_Variable>(Field.Field);
    ScratchSprite & Owner = Block.GetOwnerSprite();
    ScratchVariable * Variable = Owner.GetVariable(FieldVar.VariableKey);
    TachyonAssert(Variable != nullptr);
    ScratchData Data = Block.GetInputData(0);
    Variable->SetData(Data);
    return ScratchStatus::SCRATCH_NEXT;
}

void Data::RegisterAll(void) {
    Tachyon::RegisterOpHandler("data_deletealloflist", Data_DeleteAllOfList);
    Tachyon::RegisterOpHandler("data_setvariableto", Data_SetVariable);
}
