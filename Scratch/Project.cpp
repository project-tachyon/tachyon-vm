#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Tachyon/Tachyon.hpp>
#include <Lib/SIMDJson.h>
#include <zip.h>

using namespace simdjson;
using namespace Scratch;

void __hot ScratchSprite::CreateScript(ScratchBlock & Block) {
    ScratchScript Script {
        .FirstBlockId = Block.GetNextKey(),
        .CurrentBlockId = Block.GetNextKey(),
        .ReturnStack = std::vector<Script_StackFrame>(),
        .Sprite = this,
        .CurrentStatus = ScratchStatus::SCRATCH_END,
        .ControlFlags = 0,
    };
    this->Scripts.push_back(Script);
    Script.ReturnStack.reserve(32);
    Tachyon::ScriptAddReadyQueue(Script);
}

void ScratchSprite::CreateScripts(void) {
    for(auto & Item : this->GreenFlags) {
        ScratchBlock & Block = *Item.second;
        ScratchScript Script {
            .FirstBlockId = Block.GetNextKey(),
            .CurrentBlockId = Block.GetNextKey(),
            .ReturnStack = std::vector<Script_StackFrame>(),
            .Sprite = this,
            .CurrentStatus = ScratchStatus::SCRATCH_END,
            .ControlFlags = 0,
        };
        this->Scripts.push_back(Script);
        Script.ReturnStack.reserve(32);
    }
}

ScratchList * __hot ScratchSprite::GetList(std::string ListKey) {           
    /* check if it's local */
    auto LocalItem = this->Lists.find(ListKey);
    if (LocalItem == this->Lists.end()) {
        ScratchSprite * Stage = Tachyon::GetStage();
        if (unlikely(Stage == nullptr)) {
            return nullptr;
        }
        auto GlobalItem = Stage->Lists.find(ListKey);
        if (unlikely(GlobalItem == Stage->Lists.end())) {
            return nullptr;
        }
        return &GlobalItem->second;
    }
    return &LocalItem->second;
}

ScratchVariable * __hot ScratchSprite::GetVariable(std::string VarKey) {           
    /* check if it's local */
    auto LocalItem = this->Variables.find(VarKey);
    if (LocalItem == this->Variables.end()) {
        ScratchSprite * Stage = Tachyon::GetStage();
        if (unlikely(Stage == nullptr)) {
            return nullptr;
        }
        auto GlobalItem = Stage->Variables.find(VarKey);
        if (unlikely(GlobalItem == Stage->Variables.end())) {
            return nullptr;
        }
        return &GlobalItem->second;
    }
    return &LocalItem->second;
}

int ScratchProject::ParseContents(void) {
    /* get file size */
    struct zip_stat ProjectStat;
    zip_stat(this->ProjectZip, "project.json", ZIP_STAT_SIZE, &ProjectStat);
    zip_file_t * ProjectJsonFile = zip_fopen(this->ProjectZip, "project.json", 0);
    if (ProjectJsonFile == nullptr) {
        Close();
        return -1;
    }
    char * ProjectDataPointer = (char *)malloc(ProjectStat.size);
    if (unlikely(ProjectDataPointer == nullptr)) {
        Close();
        return -1;
    }
    if (zip_fread(ProjectJsonFile, ProjectDataPointer, ProjectStat.size) < 0) {
        free(ProjectDataPointer);
        zip_fclose(ProjectJsonFile);
        Close();
        return -1;
    }
    /* convert to json */
    ondemand::parser parser;
    padded_string data(ProjectDataPointer, ProjectStat.size);
    ondemand::document ProjectJson = parser.iterate(data);
    /* load everything */
    for (ondemand::object SpriteObject: ProjectJson["targets"]) {
        this->Sprites.push_back(std::make_unique<ScratchSprite>(SpriteObject));
    }
    /* we are done */
    free(ProjectDataPointer);
    zip_fclose(ProjectJsonFile);
    return 0;
}
