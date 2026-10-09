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
 
#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CRITICAL_LABEL_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_CRITICAL_LABEL_TASK_H
 
#include <vector>
#include <thread>
#include <mutex>
#include "i_media_background_task.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_async_worker.h"

namespace OHOS::Media::Background {
 
struct PhotoInfo {
    int32_t fileId;
    std::string displayName;
    std::string filePath;
    int32_t mediaType;
    int64_t addedTime;
};
 
using PhotoBatchInfo = std::vector<PhotoInfo>;

class CriticalLabelAsyncTaskData : public AsyncTaskData {
public:
    CriticalLabelAsyncTaskData() = default;
    virtual ~CriticalLabelAsyncTaskData() override = default;
    PhotoBatchInfo batchInfo;
};

class MediaCriticalLabelTask : public IMediaBackGroundTask {
public:
    virtual ~MediaCriticalLabelTask() = default;
 
public:
    bool Accept() override;
    void Execute() override;
 
public:
    static void HandleCriticalLabelProcessing();

private:
    static std::mutex mtx;
private:
    static PhotoBatchInfo QueryPhotosBatch(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
        int32_t limit);
    static void SendToAnlyze(AsyncTaskData *data);
    static std::string ConstructPhotoUri(const std::string &fileAssetData, const std::string &displayName,
        int32_t fileId);
};
 
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_CRITICAL_LABEL_TASK_H