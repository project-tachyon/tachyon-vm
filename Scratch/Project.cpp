#include <Scratch.hpp>
#include <Json.hpp>
#include <cstdio>
#include <iostream>
#include <zip.h>

using json = nlohmann::ordered_json;
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
    /* i know it's very bad to allocate this all in one go but this works well for me as of right now */
    char * ProjectDataPointer = (char *)malloc(ProjectStat.size);
    if (ProjectDataPointer == nullptr) {
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
    this->ProjectJson = json::parse(ProjectDataPointer);
    free(ProjectDataPointer);
    zip_fclose(ProjectJsonFile);
    if (this->ProjectJson.empty() == true) {
        /* project.json should never be empty */
        Close();
        return -1;
    }
    if (this->ProjectJson["targets"].empty() == true) {
        /* project bug or unauthorized modification */
        Close();
        return -1;
    }
    /* load everything */
    size_t total_sprites = this->ProjectJson["targets"].size();
    for(size_t i = 0; i < total_sprites; i++) {
        ScratchSprite Sprite(this->ProjectJson["targets"][i]);
        this->Sprites.push_back(Sprite);
    }
    return 0;
}
