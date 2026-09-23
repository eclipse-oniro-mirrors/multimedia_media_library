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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ENHANCE_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ENHANCE_SERVICE_H

#include <string>
#include <vector>
#include <thread_pool.h>

#include "cloud_media_enhance_dao.h"
#include "cloud_media_define.h"
#include "cloud_sync_prepare_data_type.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaEnhanceService {
public:
    int32_t GetCloudSyncUnPreparedData(CloudSyncPrepareDataType type, int32_t &result);
    int32_t SubmitCloudSyncPreparedDataTask(CloudSyncPrepareDataType type);
private:
    void SubmitNextCloudSyncPreparedDataTask();
    void SubmitTaskTimeoutCheck();
    void StopSubmit();

private:
    CloudMediaEnhanceDao enhanceDao_;
    std::unique_ptr<OHOS::ThreadPool> executor_{nullptr};
    int32_t submitCount_{0};
    std::string submitPhotoId_;
    std::atomic<bool> submitRunning_{false};
    std::atomic<bool> callbackDone_{false};
    std::condition_variable cv_;
    std::mutex mtx_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ENHANCE_SERVICE_H