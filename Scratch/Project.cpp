#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Lib/SIMDJson.h>
#include <stack>
#include <zip.h>

using namespace simdjson;
using namespace Scratch;

void ScratchSprite::CreateScripts(void) {
    for(auto & Block : this->GreenFlags) {
        ScratchScript Script {
            .FirstBlockId = Block->GetNextKey(),
            .CurrentBlockId = Block->GetNextKey(),
            .ReturnStack = std::stack<Script_StackFrame>(),
            .Sprite = this,
            .CurrentStatus = ScratchStatus::SCRATCH_END,
            .InsideProcedure = false,
        };
        this->Scripts.push_back(Script);
    }
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
