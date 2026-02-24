#include <Scratch/Common.hpp>
#include <Scratch/Blocks.hpp>
#include <Lib/SIMDJson.h>
#include <zip.h>

using namespace simdjson;
using namespace std;

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
    this->ProjectJson = parser.iterate(data);
    free(ProjectDataPointer);
    zip_fclose(ProjectJsonFile);
    /* load everything */
    for (ondemand::object SpriteObject: this->ProjectJson["targets"]) {
        ScratchSprite Sprite(SpriteObject);
        this->Sprites.push_back(Sprite);
    }
    return 0;
}
