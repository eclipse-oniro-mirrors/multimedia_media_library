/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_SERVICE_ADAPTER_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_SERVICE_ADAPTER_H

#include <string>
#include <map>
#include <memory>

#include "dynamic_loader.h"
#include "enhancement_thread_manager.h"
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
#include "media_enhance_constants_c_api.h"
#include "media_enhance_handles.h"
#include "media_enhance_client_c_api.h"
#include "media_enhance_bundle_c_api.h"
#endif
// LCOV_EXCL_START
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class EnhancementServiceAdapter {
public:
    EXPORT EnhancementServiceAdapter();
    EXPORT virtual ~EnhancementServiceAdapter();

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    void InitEnhancementClient(MediaEnhance::MediaEnhance_TASK_TYPE taskType);
    void DestroyEnhancementClient();
    int32_t SetResultCallback();
    int32_t LoadSA();
    bool IsConnected(MediaEnhance::MediaEnhanceClientHandle* clientWrapper);
    bool IsConnected();
    EXPORT MediaEnhance::MediaEnhanceBundleHandle* CreateBundle();
    void DestroyBundle(MediaEnhance::MediaEnhanceBundleHandle* bundle);
    int32_t GetInt(MediaEnhance::MediaEnhanceBundleHandle* bundle, const char* key);
    int32_t FillTaskWithResultBuffer(MediaEnhance::MediaEnhanceBundleHandle* bundle,
        CloudEnhancementThreadTask& task);
    EXPORT void PutInt(MediaEnhance::MediaEnhanceBundleHandle* bundle, const char* key,
        int32_t value);
    void PutString(MediaEnhance::MediaEnhanceBundleHandle* bundle, const char* key,
        const char* value);
    void DeleteRawData(MediaEnhance::Raw_Data* rawData, uint32_t size);
    void DeletePendingTasks(MediaEnhance::Pending_Task* taskIdList, uint32_t size);
    EXPORT int32_t AddTask(const std::string& taskId, MediaEnhance::MediaEnhanceBundleHandle* bundle);
    EXPORT int32_t RemoveTask(const std::string &taskId);
    EXPORT int32_t CancelTask(const std::string &taskId);
    EXPORT int32_t CancelAllTasks();
    EXPORT int32_t LoadEnhancementService();
    EXPORT int32_t GetPendingTasks(std::vector<std::string> &taskIdList);
    EXPORT int32_t PauseAllTasks(MediaEnhance::MediaEnhanceBundleHandle* bundle);
    EXPORT int32_t ResumeAllTasks(MediaEnhance::MediaEnhanceBundleHandle* bundle);
#endif
    
private:
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    void ClientFuncInit();
    void TaskFuncInit();
    void BundleFuncInit();
#endif

    static std::mutex mtx_;
    static std::shared_ptr<DynamicLoader> dynamicLoader_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_SERVICE_ADAPTER_H
// LCOV_EXCL_STOP