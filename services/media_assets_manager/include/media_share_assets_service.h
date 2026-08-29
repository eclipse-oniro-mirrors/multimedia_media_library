/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_SHARE_ASSETS_SERVICE_H
#define OHOS_MEDIA_SHARE_ASSETS_SERVICE_H

#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

#include "cloud_share_sync_foundation_service.h"
#include "dao/media_share_assets_dao.h"
#include "media_log.h"
#include "medialibrary_errno.h"

// LCOV_EXCL_START
namespace OHOS::Media {

class MediaShareAssetsService {
public:
    MediaShareAssetsService() {}
    ~MediaShareAssetsService() {}

    int32_t RemoveShareAlbumAndAsset();

private:
    MediaShareAssetsService(const MediaShareAssetsService &) = delete;
    const MediaShareAssetsService &operator=(const MediaShareAssetsService &) = delete;

    // 标记待删除的共享资产
    int32_t MarkShareAssetsToRemove();
    // 异步删除已标记的共享资产
    void StartRemoveShareAssetsTask();
    // 删除已标记共享资产的循环操作
    void RemoveShareAssetsTask();
    // 分批处理待删除的共享资产: 清理文件、删库、删缩略图
    bool RemoveShareAssets(const std::vector<PhotosPo> &queryResult);
    void TryToStartSync();
    // 清理共享资产的下载任务表
    void CleanShareAssetsDownloadTasksTable();
    int32_t RemoveShareAssetsInner();
    void BeforeRemoveShareAlbumAndAsset();
    void AfterRemoveShareAlbumAndAsset();

private:
    MediaShareAssetsDao shareAssetsDao_;
    CloudShareSyncFoundationService cloudShareSyncFoundationService_;
    std::mutex updateMutex_;
};
} // namespace OHOS::Media
// LCOV_EXCL_STOP
#endif // OHOS_MEDIA_SHARE_ASSETS_SERVICE_H
