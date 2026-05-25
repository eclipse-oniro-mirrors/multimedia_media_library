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

#ifndef OHOS_MEDIA_CLONE_TO_ALBUM_SERVICE_H
#define OHOS_MEDIA_CLONE_TO_ALBUM_SERVICE_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "clone_to_album_vo.h"
#include "iclone_to_album_callback.h"
#include "result_set.h"

namespace OHOS {
namespace Media {

struct CloneAssetInfo {
    int64_t fileId {-1};
    std::string filePath = "";
    std::string displayName = "";
    int32_t mediaType {-1};
    int64_t size {0};
    int32_t hidden {0};
    int64_t dateTrashed {-1};
    int32_t position {-1};
    std::string storagePath = "";
    std::string sourcePath = "";
    std::string burstKey = "";
    int32_t mode {0};
    std::string albumLpath = "";
    int32_t albumId {-1};
    int32_t albumSubType {-1};
    int32_t albumType {-1};
    int32_t requestId {0};
    std::string targetFilePath = "";
    std::string targetFileTitle = "";
    std::string targetDisplayName = "";
    int32_t photoSubType {0};
    int32_t movingPhotoEffectMode {0};
    int32_t fileSourceType {-1};
    std::vector<CloneAssetInfo> burstCloneAssetList;
};

enum CloneCallbackType {
    URI = 0,
    FILEPATH,
    PHOTOASSET,
};

struct CloneTaskInfo {
    std::vector<CloneAssetInfo> cloneAssetInfo;
    std::atomic<uint64_t> processedSize{0};
    std::atomic<uint32_t> processedCount{0};
    sptr<IRemoteObject> progressCallback;
    int32_t albumType {-1};
    int32_t albumSubType {-1};
    int32_t requestId {0};
    std::string targetDir = "";
    CloneCallbackType cloneCallbackType {CloneCallbackType::URI};

    CloneTaskInfo() = default;

    CloneTaskInfo(const CloneTaskInfo& tmp)
    {
        cloneAssetInfo = tmp.cloneAssetInfo;
        processedSize.store(tmp.processedSize.load());
        processedCount.store(tmp.processedCount.load());
        progressCallback = tmp.progressCallback;
        albumType = tmp.albumType;
        albumSubType = tmp.albumSubType;
        targetDir = tmp.targetDir;
        cloneCallbackType = tmp.cloneCallbackType;
    }
};

class CloneToAlbumService {
public:
    CloneToAlbumService() = default;
    ~CloneToAlbumService() = default;

    int32_t CloneToAlbum(CloneToAlbumReqBody &reqBody);
    int32_t CloneToDir(CloneToAlbumReqBody &reqBody);
    int32_t CloneAssetByPath(CloneToAlbumReqBody &reqBody);
    int32_t CloneToAlbumCancel(const CloneToAlbumReqBody &reqBody);

private:
    int32_t StartCopy(uint64_t totalSize, uint32_t totalCount, CloneTaskInfo &cloneTaskInfo);
    int32_t ValidateRequest(CloneToAlbumReqBody &reqBody);
    int32_t QueryAllAssetsInfo(const CloneToAlbumReqBody &reqBody,
        CloneTaskInfo &assets, uint64_t &displayTotalSize, uint64_t &actualTotalSize);
    int32_t QueryBurstAssetInfo(CloneAssetInfo &cloneAssetInfo, uint64_t &displayTotalSize,
        uint64_t &actualTotalSize);
    int32_t HandleAssetClone(const CloneAssetInfo &asset, std::string &newFileId,
        std::atomic<uint64_t> &processedSize, std::atomic<uint32_t> &processedCount,
        CloneCallbackType &cloneCallbackType);
    int32_t QueryAssetInfo(const std::string &fileId, CloneAssetInfo &info);
    int32_t DoBurstAssetsClone(const CloneAssetInfo &cloneAssetInfo, std::function<void(uint64_t)> progressCallback);
    int32_t GetUriFromResult(std::shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet,
        std::vector<std::string> &resultUris, CloneCallbackType cloneCallbackType);
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_CLONE_TO_ALBUM_SERVICE_H
