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

#define MLOG_TAG "DfxPhotoError"

#include "dfx_photo_error_processor.h"

#include "dfx_photo_error_helper.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "moving_photo_file_utils.h"

namespace OHOS {
namespace Media {

std::vector<PhotoErrorCount> DfxPhotoErrorProcessor::GetPhotoErrorBatches(int32_t scanBatchSize, int32_t packBatchSize)
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::map<int32_t, int32_t> typeToCount;
    int32_t lastFileId = 0;
    bool completed = false;
    while (MedialibrarySubscriber::IsCurrentStatusOn()) {
        auto rows = dao_.QueryBatch(lastFileId, scanBatchSize);
        if (rows.empty()) {
            completed = true;
            break;
        }
        for (const auto& row : rows) {
            lastFileId = row.fileId;
            int32_t type = ProcessRow(row);
            CHECK_AND_CONTINUE(type != static_cast<int32_t>(PhotoErrorType::CONSISTENT));
            typeToCount[type]++;
        }
    }
    CHECK_AND_RETURN_RET_LOG(completed, {},
        "Scan interrupted, discard partial stat. lastFileId=%{public}d", lastFileId);
    std::vector<PhotoErrorCount> batches = DfxPhotoErrorHelper::PackPhotoErrors(typeToCount, packBatchSize);
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("lastFileId: %{public}d, typeToCount: %{public}zu, batches: %{public}zu, timeCost: %{public}" PRId64,
        lastFileId, typeToCount.size(), batches.size(), endTime - startTime);
    return batches;
}

int32_t DfxPhotoErrorProcessor::ProcessRow(const PhotoErrorRow& row)
{
    std::string path = DfxPhotoErrorHelper::ResolveRealPath(row.fileSourceType, row.data, row.storagePath);
    CHECK_AND_RETURN_RET_LOG(!path.empty(), static_cast<int32_t>(PhotoErrorType::CONSISTENT),
        "ResolveRealPath empty, skip. fileId=%{public}d fst=%{public}d", row.fileId, row.fileSourceType);

    size_t primarySize = 0;
    bool exists = MediaFileUtils::GetFileSize(path, primarySize);
    int64_t diskSize = 0;
    if (exists) {
        if (MovingPhotoFileUtils::IsMovingPhoto(row.subtype, row.movingPhotoEffectMode, row.originalSubtype)) {
            diskSize = static_cast<int64_t>(MovingPhotoFileUtils::GetMovingPhotoSize(path));
        } else {
            diskSize = static_cast<int64_t>(primarySize);
        }
    }
    PhotoErrorType err = DfxPhotoErrorHelper::ClassifyPhotoError(exists, diskSize, row.size);
    CHECK_AND_RETURN_RET(err != PhotoErrorType::CONSISTENT, static_cast<int32_t>(PhotoErrorType::CONSISTENT));

    PhotoErrorDimension dim{row.fileSourceType, row.southDeviceType, row.position, row.mediaType, err};
    return DfxPhotoErrorHelper::EncodePhotoErrorType(dim);
}
} // namespace Media
} // namespace OHOS
