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
#ifndef OHOS_CLONE_GROUP_PHOTO_ALBUM_H
#define OHOS_CLONE_GROUP_PHOTO_ALBUM_H

#include "rdb_store.h"
#include "backup_const.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace OHOS {
namespace Media {
constexpr int32_t INVALID_EXIT_CODE = -1;
constexpr int32_t INVALID_SCENE_CODE = -1;

class CloneGroupPhotoAlbum {
public:
struct GroupAlbumInfo {
    std::vector<std::string> groupTagVec;
    std::unordered_map<std::string, std::vector<std::string>> groupTagMap;
    std::vector<std::string> fileIdVec;
    int32_t userOperation {0};
    int32_t renameOperation {0};
    std::string tagId;
    std::string groupTag;
    std::string tagName;
    int32_t userDisplayLevel {1};
    int32_t fileIdCount{0};
};

public:
    CloneGroupPhotoAlbum(int32_t sceneCode, const std::string& taskId, std::shared_ptr<NativeRdb::RdbStore>
        mediaLibraryRdb, std::shared_ptr<NativeRdb::RdbStore> galleryRdb);
    ~CloneGroupPhotoAlbum() = default;
    void RestoreGroupPhotoAlbum(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);

private:
    bool GetFileIdsByGroupTag(CloneGroupPhotoAlbum::GroupAlbumInfo &info);
    std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> GetGroupPhotoAlbumInfo(int32_t offset);
    int64_t GetShouldEndTime(const std::unordered_map<int32_t, PhotoInfo> &photoInfoMap);
    void ModifyGroupVersion(const std::map<int32_t, std::vector<string>> &groupPhotoMap);
    void QueryGroupPhotoAlbum(const std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> &groupPhotoAlbumInfos,
        std::map<int32_t, std::vector<string>> &groupPhotoMap);
    void InsertAnalysisPhotoMap(const std::map<int32_t, std::vector<string>> &groupPhotoMap);
    void QueryTagIdFromMergeTag(CloneGroupPhotoAlbum::GroupAlbumInfo &info);
    int32_t BatchInsertWithRetry(const std::string &tableName,
        const std::vector<NativeRdb::ValuesBucket> &values, int64_t &rowNum);
    void InsertAnalysisAlbumTable(const std::vector<CloneGroupPhotoAlbum::GroupAlbumInfo> &result,
        const std::vector<NativeRdb::ValuesBucket> &values);

    std::atomic<int32_t> exitCode_ {INVALID_EXIT_CODE};
    int32_t sceneCode_ {INVALID_SCENE_CODE};
    std::string taskId_;
    std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb_;
    std::shared_ptr<NativeRdb::RdbStore> galleryRdb_;
};
}
}
#endif // OHOS_CLONE_GROUP_PHOTO_ALBUM_H
