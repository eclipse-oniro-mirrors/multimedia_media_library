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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_DAO_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_DAO_H

#include <string>

#include "camera_mapper.h"
#include "rdb_store.h"
#include "file_asset.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MultiStagesCaptureDao {
public:
    int32_t UpdatePhotoDirtyNew(const int32_t fileId);
    EXPORT std::shared_ptr<FileAsset> QueryDataByPhotoId(const std::string &videoId,
        const std::vector<std::string> &columns);
    // 一阶段: openFile时, 更新time_pending
    EXPORT static int32_t UpdateTimePendingForOpenFile(const int32_t &fileId, const int64_t &pendingTime);
    // 二阶段落盘: 图片查询
    EXPORT static std::shared_ptr<FileAsset> QueryForOnProcess(const int32_t &fileId,
        const std::string &photoId, MediaDpsMetadata &metadata);
    // 二阶段落盘: 图片更新数据(可能会被一阶段调用)
    EXPORT static int32_t UpdateHighQualityInfo(const int32_t &fileId, const MediaDpsMetadata &metadata,
        bool isOnProcess);
    // 二阶段: 仅更新photo_quality = 高
    EXPORT static int32_t UpdatePhotoQuality(const int32_t &fileId);
    // 二阶段: 周同步任务查询
    EXPORT static std::vector<std::shared_ptr<FileAsset>> QueryForSessionSyncImage();
    EXPORT static std::shared_ptr<FileAsset> QueryForDeferredPictureInfo(int32_t fileId);
    // 异常流程中, 需要恢复pipeline
    EXPORT static std::shared_ptr<FileAsset> RecoverPipelineByFileId(int32_t fileId);
    EXPORT static std::shared_ptr<FileAsset> RecoverPipelineByPhotoId(const std::string& photoId);
};
}  // namespace OHOS::Media
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTI_STAGES_CAPTURE_DAO_H