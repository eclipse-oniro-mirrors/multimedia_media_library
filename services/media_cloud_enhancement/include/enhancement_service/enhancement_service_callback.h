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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_SERVICE_CALLBACK_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_SERVICE_CALLBACK_H

#include <memory>
#include <string>

#include "abs_shared_result_set.h"
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
struct CloudEnhancementFileInfo {
    int32_t fileId;
    std::string filePath;
    std::string displayName;
    int32_t subtype;
    int32_t hidden;
    CloudEnhancementFileInfo() : fileId(0), filePath(""), displayName(""),
        subtype(0), hidden(0) {}
    CloudEnhancementFileInfo(int32_t fileId, std::string filePath, std::string displayName,
        int32_t subtype, int32_t hidden) : fileId(fileId), filePath(filePath),
        displayName(displayName), subtype(subtype), hidden(hidden) {}
};

class EnhancementServiceCallback {
public:
    EXPORT EnhancementServiceCallback();
    EXPORT ~EnhancementServiceCallback();

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    EXPORT static void OnSuccess(const char* photoId, MediaEnhance::MediaEnhanceBundleHandle* bundle);
    EXPORT static void OnFailed(const char* photoId, MediaEnhance::MediaEnhanceBundleHandle* bundle);
    EXPORT static void OnServiceReconnected();
    EXPORT static void DealWithSuccessedTask(CloudEnhancementThreadTask& task);
    EXPORT static void DealWithFailedTask(CloudEnhancementThreadTask& task);
    EXPORT static void UpdateAlbumsForCloudEnhancement();
#endif

private:
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    EXPORT static int32_t SaveCloudEnhancementPhoto(std::shared_ptr<CloudEnhancementFileInfo> info,
        CloudEnhancementThreadTask& task, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    EXPORT static int32_t UpdateCloudEnhancementPhotoInfo(int32_t fileId,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    EXPORT static int32_t SaveCloudEnhancementMovingPhotoVideo(shared_ptr<CloudEnhancementFileInfo> info,
        CloudEnhancementThreadTask& task, shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    EXPORT static int32_t UpdateCloudEnhancementMovingPhotoInfo(int32_t fileId,
        shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
#endif
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_SERVICE_CALLBACK_H
// LCOV_EXCL_STOP