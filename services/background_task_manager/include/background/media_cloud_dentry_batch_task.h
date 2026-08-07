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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CLOUD_DENTRY_BATCH_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_CLOUD_DENTRY_BATCH_TASK_H

#include <string>
#include <vector>

#include "cloud_sync_helper.h"
#include "photos_po.h"

namespace OHOS::Media::Background {
using namespace OHOS::Media::ORM;
class MediaCloudDentryBatchTask {
public:
    ~MediaCloudDentryBatchTask() = default;

    static void CheckDentryCreation();

private:
    static std::mutex batchDentryMutex_;

    static std::vector<FileManagement::CloudSync::DentryFileInfo> dentryOrigin_;
    static std::vector<FileManagement::CloudSync::DentryFileInfo> dentryLcd_;
    static std::vector<FileManagement::CloudSync::DentryFileInfo> dentryThm_;

    static void HandleBatchDentryCreation(const int32_t lastFileId);

    static bool NeedCreateDentryForPhoto(const PhotosPo &photosPo);

    static void BatchInsertDentry(const std::vector<FileManagement::CloudSync::DentryFileInfo> &dentryList,
                                  const std::string &type);
                                  
    static void DoDentryCreate(int32_t &currentLastFileId, bool &terminate, std::vector<PhotosPo> &photosPoVec);
};

}  // namespace OHOS::Media::Background

#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_CLOUD_DENTRY_BATCH_TASK_H