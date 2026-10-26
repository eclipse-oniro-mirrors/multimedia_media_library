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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_LAKE_SIZE_REPAIR_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_LAKE_SIZE_REPAIR_TASK_H

#include <string>

#include "i_media_background_task.h"
#include "media_lake_size_repair_dao.h"

namespace OHOS::Media::Background {

class MediaLakeSizeRepairTask : public IMediaBackGroundTask {
public:
    virtual ~MediaLakeSizeRepairTask() = default;

    bool Accept() override;
    void Execute() override;

private:
    void PrepareProgress();
    void RepairCorruptedPhotos();
    void LogBatchResult(const char *stage, int32_t startCursor, int32_t endCursor,
        size_t scannedCount, int64_t processedCount) const;

    int32_t GetSavedTaskVersion();
    void SaveTaskVersion(int32_t version);
    int32_t LoadCursor(const std::string &key);
    void SaveCursor(const std::string &key, int32_t value);

    MediaLakeSizeRepairDao repairDao_;
};

}  // namespace OHOS::Media::Background

#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_LAKE_SIZE_REPAIR_TASK_H
