/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef MEDIALIBRARY_LAKE_FILE_OPERATIONS_H
#define MEDIALIBRARY_LAKE_FILE_OPERATIONS_H
#include <string>
 
#include "file_const.h"
#include "asset_accurate_refresh.h"
 
namespace OHOS::Media {

enum class HiddenState : int32_t {
    NOT_UPDATE = -1,
    NOT_HIDDEN = 0,
    HIDDEN = 1,
};

struct HiddenStateInfo {
    HiddenState hiddenState = HiddenState::NOT_UPDATE;
    bool NeedUpdate() const { return hiddenState != HiddenState::NOT_UPDATE; }
};

struct MoveAssetsToLakeUpdateData {
    int32_t mediaId;
    std::string title;
    std::string displayName;
    std::string storagePath;
};
 
class LakeFileOperations {
public:
    // 湖外到湖内，用于隐藏、删除恢复场景
    static int32_t MoveAssetsToLake(
        AccurateRefresh::AccurateRefreshBase &refresh, const std::vector<std::string> &ids,
        const HiddenStateInfo &hiddenInfo = {});
    // 湖内到湖外，用于隐藏、删除场景
    static int32_t MoveAssetsFromLake(AccurateRefresh::AccurateRefreshBase &refresh,
        const std::vector<std::string> &ids, const HiddenStateInfo &hiddenInfo = {});
    // 用于更新编辑操作
    static int32_t UpdateMediaAssetEditData(std::string& fileUri);
    static int32_t RenamePhoto(AccurateRefresh::AccurateRefreshBase &refresh, const int32_t &fileId,
        const std::string &displayName, const std::string &storagePath, const std::string &data);
    // 湖内文件移动到新相册
    static int32_t MoveInnerLakeAssetsToNewAlbum(AccurateRefresh::AccurateRefreshBase &refresh,
        const std::vector<std::string> &ids, int32_t targetAlbumId);
    // 筛选湖内数据
    static std::vector<MoveAssetsToLakeUpdateData> GetInnerLakeAssets(const std::vector<std::string> &ids);
    static int32_t MoveLakeFile(const std::string &srcPath, const std::string &destPath);

private:
    static int32_t UpdateTimePendingAsHideInProgress(const NativeRdb::AbsRdbPredicates &predicates,
        const HiddenStateInfo &hiddenInfo);
};
} // namespace OHOS::Media
#endif // MEDIALIBRARY_LAKE_FILE_OPERATIONS_H