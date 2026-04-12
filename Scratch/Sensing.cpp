#include "Compiler.hpp"
#include <Tachyon/Debug.hpp>
#include <Scratch/Data.hpp>
#include <Scratch/BlockFields.hpp>
#include <Scratch/Blocks.hpp>
#include <Scratch/Sensing.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Tachyon/Time.hpp>

using namespace Scratch;

static __hot ScratchData Sensing_Current(ScratchBlock & Block) {
    ScratchField Field = Block.GetField(0);

    TachyonAssert(Field.Type == ScratchField::FieldType::StringField);

    std::string What = std::get<std::string>(Field.Field);

    if (What == "YEAR") { return Tachyon::Time::GetYear(); }
    if (What == "MONTH") { return Tachyon::Time::GetMonth(); }
    if (What == "DATE") { return Tachyon::Time::GetDayOfMonth(); }
    if (What == "DAYOFWEEK") { return Tachyon::Time::GetDayOfWeek(); }
    if (What == "HOUR") { return Tachyon::Time::GetHour(); }
    if (What == "MINUTE") { return Tachyon::Time::GetMinute(); }
    if (What == "SECOND") { return Tachyon::Time::GetSeconds(); }

    TachyonUnimplemented("What kind of option is this?? %s\n", What.c_str());

    __unreachable;
}

static __hot ScratchData Sensing_DaysSinceY2K( [[maybe_unused]] ScratchBlock & Block) {
    return Tachyon::Time::GetDaysSince2000();
}

void Sensing::RegisterAll(void) {
    //Tachyon::RegisterEvaluationHandler("sensing_timer", Sensing_Timer);
    Tachyon::RegisterEvaluationHandler("sensing_dayssince2000", Sensing_DaysSinceY2K);
    Tachyon::RegisterEvaluationHandler("sensing_current", Sensing_Current);
}
