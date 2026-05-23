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

#define MLOG_TAG "Media_Background"

#include "media_clone_pending_task.h"

#include "media_clone_pending_record_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media::Background {
using namespace std;
using namespace OHOS::NativeRdb;

bool MediaClonePendingTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaClonePendingTask::Execute()
{
    HandleClonePendingAssets();
}

void MediaClonePendingTask::HandleClonePendingAssets()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");

    size_t totalPendingCount = 0;
    size_t totalCleanedCount = 0;
    int32_t bucketCount = ClonePendingRecordUtils::GetPendingBucketCount();
    MEDIA_INFO_LOG("ClonePendingTask start, bucketCount=%{public}d", bucketCount);
    for (int32_t bucketIndex = 0; bucketIndex < bucketCount; ++bucketIndex) {
        CHECK_AND_RETURN_INFO_LOG(Accept(), "ClonePendingTask interrupted before bucket scan");
        std::vector<int32_t> pendingFileIds = ClonePendingRecordUtils::GetPendingFileIdsByBucket(bucketIndex);
        if (pendingFileIds.empty()) {
            continue;
        }
        MEDIA_DEBUG_LOG("ClonePendingTask scan bucket=%{public}d, pending=%{public}zu",
            bucketIndex, pendingFileIds.size());
        totalPendingCount += pendingFileIds.size();
        std::vector<int32_t> fileIdsToRemove;
        fileIdsToRemove.reserve(pendingFileIds.size());
        for (const auto fileId : pendingFileIds) {
            CHECK_AND_RETURN_INFO_LOG(Accept(), "ClonePendingTask interrupted by status change");
            if (!ClonePendingRecordUtils::IsPendingFileTouchExpired(fileId)) {
                MEDIA_DEBUG_LOG("Skip cleanup for active pending fileId=%{public}d", fileId);
                continue;
            }

            if (!ClonePendingRecordUtils::CleanupPendingAssetByFileId(rdbStore, fileId)) {
                continue;
            }
            fileIdsToRemove.push_back(fileId);
        }

        if (!fileIdsToRemove.empty()) {
            ClonePendingRecordUtils::RemovePendingFileIds(fileIdsToRemove);
            totalCleanedCount += fileIdsToRemove.size();
            MEDIA_INFO_LOG("ClonePendingTask cleaned bucket=%{public}d, cleaned=%{public}zu",
                bucketIndex, fileIdsToRemove.size());
        }
    }

    CHECK_AND_RETURN_INFO_LOG(totalPendingCount > 0, "No pending clone records");
    MEDIA_INFO_LOG("ClonePendingTask end, pending count: %{public}zu, cleaned count: %{public}zu",
        totalPendingCount, totalCleanedCount);
}
} // namespace OHOS::Media::Background