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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CONTEXT_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CONTEXT_H

#include <atomic>
#include <string>
#include <unordered_map>

#include "ipc_context.h"
#include "medialibrary_data_manager_utils.h"
#include "media_log.h"
#include "media_column.h"
#include "cloud_share_album_define.h"

namespace OHOS::Media::CloudSync {
class CloudMediaContext {
public:
    static CloudMediaContext& GetInstance()
    {
        static CloudMediaContext instance;
        return instance;
    }

    void SetCloudType(int32_t cloudType)
    {
        cloudType_.store(cloudType);
    }

    int32_t GetCloudType() const
    {
        return cloudType_.load();
    }

    int32_t GetSceneType() const
    {
        return sceneType_.load();
    }

    CloudMediaContext(const CloudMediaContext&) = delete;
    CloudMediaContext& operator=(const CloudMediaContext&) = delete;

    void SetCloudType(const OHOS::Media::IPC::IPCContext &context)
    {
        auto headerMap = context.GetHeader();
        auto headerIt = headerMap.find(PhotoColumn::CLOUD_TYPE);
        bool isValid = headerIt != headerMap.end();
        isValid = isValid && MediaLibraryDataManagerUtils::IsNumber(headerIt->second.c_str());
        CHECK_AND_RETURN(isValid);
        int32_t cloudType = std::atoi(headerIt->second.c_str());
        this->SetCloudType(cloudType);
    }

    void SetSceneType(const OHOS::Media::IPC::IPCContext &context)
    {
        auto headerMap = context.GetHeader();
        auto headerIt = headerMap.find(SCENE_TYPE);
        bool isValid = headerIt != headerMap.end();
        isValid = isValid && MediaLibraryDataManagerUtils::IsNumber(headerIt->second.c_str());
        if (!isValid) {
            this->SetSceneType(static_cast<int32_t>(SceneType::NORMAL));
            return;
        }
        int32_t sceneType = std::atoi(headerIt->second.c_str());
        this->SetSceneType(sceneType);
    }

private:
    std::atomic<int32_t> cloudType_;
    static inline thread_local std::atomic<int32_t> sceneType_{0};
    CloudMediaContext() = default;

    void SetSceneType(int32_t sceneType)
    {
        sceneType_.store(sceneType);
    }
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CONTEXT_H