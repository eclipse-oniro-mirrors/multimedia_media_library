/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_TEMP_FILE_AGING_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_TEMP_FILE_AGING_TASK_H

#include <vector>
#include <string>
#include <filesystem>

#include "i_media_background_task.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media::Background {
static const std::string FILE_MANAGER_TEMP_FILE_AGING_EVENT =
    "/data/storage/el2/base/preferences/file_manager_temp_file_aging_events.xml";

struct AgingFilesInfo {
    std::vector<std::string> fileIds;
    std::vector<std::string> filePaths;
    std::vector<std::string> dateTakens;
};

class MediaFileManagerTempFileAgingTask : public IMediaBackGroundTask {
public:
    virtual ~MediaFileManagerTempFileAgingTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    void SetBatchStatus(int32_t startFileId);
    int32_t GetBatchStatus();
    AgingFilesInfo QueryAgingFiles(std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t startFileId,
        bool checkOmitted);
    void DeleteTempFiles(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        const AgingFilesInfo &agingFilesInfo);
    void HandleMediaFileManagerTempFileAging();
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_FILE_MANAGER_TEMP_FILE_AGING_TASK_H