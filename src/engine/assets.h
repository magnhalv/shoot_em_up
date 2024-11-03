#pragma once

#include <platform/types.h>

struct FileAsset {
    char path[Max_Path_Length];
    TimeStamp last_modified;
};
