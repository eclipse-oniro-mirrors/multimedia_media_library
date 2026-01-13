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
#include "media_delete_file_processor.h"

#include "dfx_utils.h"
#include "media_lake_monitor_rdb_utils.h"
#include "media_log.h"
#include "medialibrary_notify.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

inline void NotifyAssetChange(int fileId)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId),
        NotifyType::NOTIFY_REMOVE);
}

bool MediaDeleteFileProcessor::ProcessInner(const std::string& path)
{
    // 1. 构造查询条件并查询 albumId 与 file_id; 根据 file_id 查询对应的 AnalysisAlbum id
    LakeMonitorQueryResultData data;
    CHECK_AND_RETURN_RET_LOG(MediaLakeMonitorRdbUtils::QueryDataByDeletedStoragePath(rdbStore_, path, data),
        false, "Failed to get valid albumId, path: %{public}s", DfxUtils::GetSafePath(path).c_str());
    std::set<std::string> analysisAlbumIds;
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets({to_string(data.fileId)}, analysisAlbumIds);

    // 2. 删除该路径对应的资产
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(MediaLakeMonitorRdbUtils::DeleteAssetByStoragePath(assetRefresh, path), false,
        "Failed to DeleteAssetByStoragePath, path: %{public}s", DfxUtils::GetSafePath(path).c_str());

    // 3. 删除该图片对应湖外资源
    MediaLakeMonitorRdbUtils::DeleteRelatedResource(data.photoPath,
        std::to_string(data.fileId), std::to_string(data.dateTaken));

    // 4. 更新相册信息并发送相册更新通知
    assetRefresh->RefreshAlbum();
    std::vector<std::string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());
    if (!albumIds.empty() && rdbStore_ != nullptr) {
        MediaLakeMonitorRdbUtils::NotifyAnalysisAlbum(albumIds);
    }

    // 5. 发送资产更新通知
    assetRefresh->Notify();
    NotifyAssetChange(data.fileId);

    return true;
}
} // namespace OHOS::Media