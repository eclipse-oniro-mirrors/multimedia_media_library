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
 
#ifndef INTERFACES_INNERKITS_MEDIA_LIBRARY_CAMERA_MANAGER_H_
#define INTERFACES_INNERKITS_MEDIA_LIBRARY_CAMERA_MANAGER_H_

#include <mutex>

#include "datashare_helper.h"
#include "media_low_quality_memory_callback.h"
#include "media_photo_asset_proxy.h"
 
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
 
class MediaLibraryCameraManager {
public:
    EXPORT static MediaLibraryCameraManager *GetMediaLibraryCameraManager();
    EXPORT void InitMediaLibraryCameraManager(const sptr<IRemoteObject> &token);

    EXPORT std::shared_ptr<PhotoAssetProxy> CreatePhotoAssetProxy(
        const PhotoAssetProxyCallerInfo &callerInfo, CameraShotType cameraShotType, int32_t videoCount = 1);
    EXPORT int32_t OpenAsset(std::string &uri, const std::string &openMode);

    EXPORT int32_t RegisterPhotoStateCallback(const LowQualityMemoryNumHandler &func);
    EXPORT int32_t UnregisterPhotoStateCallback();

private:
    EXPORT MediaLibraryCameraManager() = default;
    EXPORT virtual ~MediaLibraryCameraManager() = default;

    std::mutex mutex_;
    std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    sptr<IRemoteObject> token_;

    // 三方拍照需要控速，防止峰值内存超基线
    sptr<MediaLowQualityMemoryCallback> callback_ = nullptr;
};
} // namespace Media
} // namespace OHOS
 
#endif  // INTERFACES_INNERKITS_MEDIA_LIBRARY_CAMERA_MANAGER_H_