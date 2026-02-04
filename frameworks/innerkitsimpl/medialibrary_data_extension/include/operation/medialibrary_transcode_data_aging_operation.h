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

#ifndef MEDIALIBRARY_TRANSCODE_DATA_AGING_OPERATION
#define MEDIALIBRARY_TRANSCODE_DATA_AGING_OPERATION

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "abs_predicates.h"
#include "abs_shared_result_set.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "file_asset.h"
#include "imedia_scanner_callback.h"
#include "media_column.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_command.h"
#include "photo_album.h"
#include "picture.h"
#include "value_object.h"
#include "values_bucket.h"
#include "medialibrary_rdb_transaction.h"
#include "asset_accurate_refresh.h"
#include "batch_download_resources_task_dao.h"
#include "medialibrary_asset_operations.h"

namespace OHOS {
namespace Media {

class MediaLibraryTranscodeDataAgingOperation {
public:
    EXPORT MediaLibraryTranscodeDataAgingOperation();
    EXPORT ~MediaLibraryTranscodeDataAgingOperation();
    EXPORT static MediaLibraryTranscodeDataAgingOperation* GetInstance();
    EXPORT static int32_t DeleteTranscodePhotos(const std::string &filePath);
    EXPORT static void DeleteTransCodeInfo(const std::string &filePath, const std::string &fileId,
        const std::string functionName);
    EXPORT static void ModifyTransCodeFileExif(const ExifType type, const std::string &path,
        const TransCodeExifInfo &exifInfo, const std::string &functionName);
    static int32_t SetTranscodeUriToFileAsset(std::shared_ptr<FileAsset> &fileAsset, const std::string &mode,
        const bool isHeif);
    static void DoTranscodeDfx(const int32_t &type);
    EXPORT void AgingTmpCompatibleDuplicates();
    EXPORT void InterruptAgingTmpCompatibleDuplicates();
    EXPORT int32_t AgingTmpCompatibleDuplicate(int32_t fileId, const std::string &filePath);
private:
    std::atomic_bool isAgingDup_ {false};
    static std::unique_ptr<MediaLibraryTranscodeDataAgingOperation> instance_;
    static std::mutex mutex_;
    void AgingTmpCompatibleDuplicatesThread();
};

} // namespace Media
} // namespace OHOS

#endif // MEDIALIBRARY_TRANSCODE_DATA_AGING_OPERATION