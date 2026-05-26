/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef FILE_MANAGEMENT_UTILS_H
#define FILE_MANAGEMENT_UTILS_H

#include <string>
#include <map>

#include "medialibrary_errno.h"
#include "asset_accurate_refresh.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace AccurateRefresh;
const int32_t TARGET_DIR_NOT_SUPPORT = 1;
const int32_t TARGET_FILE_NOT_EXIST = 2;
const int32_t RENAME_NOT_SUPPORT = 3;
const int32_t CANCEL_TASK_ERROR = 4;
const std::string FILE_MANAGEMENT_PREFIX = "/storage/media/local/files/Docs/";
const std::vector<std::string> FORBIDDEN_PREFIXES = {
    "/storage/media/local/files/Docs/HO_DATA_EXT_MISC",
    "/storage/media/local/files/Docs/.thumbs",
    "/storage/media/local/files/Docs/.Recent",
    "/storage/media/local/files/Docs/.backup",
    "/storage/media/local/files/Docs/.Trash",
    "/storage/media/local/files/Docs/.VMDocs",
    "/storage/media/local/files/Docs/.ohpm",
    "/storage/media/local/files/Docs/PCEngine",
    "/storage/media/local/files/Docs/appdata",
};
struct FileAssetsInfo {
    int32_t fileId;
    int32_t mediaType;
    int32_t photoSubtype;
    int32_t position;
    int32_t fileSourceType;
    std::string displayName;
    std::string data;
    std::string storagePath;
    int32_t targetAlbumId;
    std::string burstKey;
    int64_t size;
    int32_t ownerAlbumId;
    std::string title;
};

struct FileAlbumInfo {
    int32_t albumId;
    std::string albumName;
    std::string lpath;
    std::string targetDir;
};

enum ResultInfoType {
    FILE_MANAGEMENT_URI,
    FILE_MANAGEMENT_PATH
};

class FileManagementUtils {
public:
    EXPORT static int32_t GetRelativeDir(std::string& target, std::string& relativePath);
    EXPORT static std::string GetLocalPath(const std::string &path);
    EXPORT static std::string ReplaceLastSegment(const std::string& data, const std::string& displayname);
    EXPORT static std::string GetLastDirName(const std::string& path);
    EXPORT static int64_t CalculateTotalSizeByPath(const std::vector<std::string> &assetpaths);
    EXPORT static int64_t CalculateTotalSize(const std::vector<std::string> &assets);
    EXPORT static int32_t QueryMoveAssetInfos(const NativeRdb::RdbPredicates& predicate,
        std::map<int32_t, FileAssetsInfo> &moveAssetMap);
    EXPORT static int32_t QueryTargetAlbumInfo(const std::string relativePath, int32_t &albumId);
    EXPORT static int64_t InsertFileAlbum(const FileAlbumInfo &fileAlbumInfo);
    EXPORT static int32_t UpdateBurstNumber(std::shared_ptr<AssetAccurateRefresh> &refresh, const FileAssetsInfo &info);
    EXPORT static int32_t UpdateMoveAsset(std::shared_ptr<AssetAccurateRefresh> refresh, const FileAssetsInfo &info);
    EXPORT static int32_t GetFileMtime(const string &filePath, time_t &mtime);
};
} //OHOS::Media
#endif