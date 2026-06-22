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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CLONE_PENDING_RECORD_UTILS_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_CLONE_PENDING_RECORD_UTILS_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace OHOS::Media {
class MediaLibraryRdbStore;
}

namespace OHOS::Media::Background {
struct ClonePendingInfo {
    int32_t fileId = -1;
    std::string filePath;
    int64_t dateTaken = 0;
    int32_t subType = 0;
    int64_t timePending = 0;
};

enum class QueryClonePendingStatus {
    FOUND = 0,
    NOT_FOUND,
    QUERY_ERROR,
};

class ClonePendingRecordUtils {
public:
    static bool AddPendingFileId(int32_t fileId);
    static bool UpdatePendingFileTouch(int32_t fileId, bool forcePersist = false);
    static bool IsPendingFileTouchExpired(int32_t fileId, int64_t timeoutMs = 120000);
    static int32_t GetPendingBucketCount();
    static std::vector<int32_t> GetPendingFileIdsByBucket(int32_t bucketIndex);
    static bool RemovePendingFileId(int32_t fileId);
    static bool RemovePendingFileIds(const std::vector<int32_t> &fileIds);
    static bool CleanupPendingAssetByFileId(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        int32_t fileId);

    static QueryClonePendingStatus QueryClonePendingInfo(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        int32_t fileId, ClonePendingInfo &info);
    static bool CleanupPendingAsset(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        const ClonePendingInfo &info);
};
} // namespace OHOS::Media::Background

#endif // OHOS_MEDIA_BACKGROUND_MEDIA_CLONE_PENDING_RECORD_UTILS_H