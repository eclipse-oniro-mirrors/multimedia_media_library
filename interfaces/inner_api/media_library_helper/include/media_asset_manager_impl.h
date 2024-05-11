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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_IMPL_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_IMPL_H

#include "nocopyable.h"
#include "media_asset_manager.h"
#include "media_library_manager.h"

namespace OHOS {
namespace Media {

class MediaAssetManagerImpl : public MediaAssetManager, public NoCopyable {
public:
    MediaAssetManagerImpl();
    ~MediaAssetManagerImpl();

    static MultiStagesCapturePhotoStatus QueryPhotoStatus(int32_t fileId, std::string &photoId);
    static bool NotifyImageDataPrepared(AssetHandler *assetHandler);
    std::string NativeRequestImage(const char* photoUri, const NativeRequestOptions &requestOptions,
        const char* destPath, const NativeOnDataPrepared &callback) override;
    std::string NativeRequestVideo(const char* videoUri, const NativeRequestOptions &requestOptions,
        const char* destPath, const NativeOnDataPrepared &callback) override;
    bool NativeCancelRequest(const std::string &requestId) override;

private:
    void CreateDataHelper(int32_t systemAbilityId);
    bool OnHandleRequestImage(const std::unique_ptr<RequestSourceAsyncContext> &asyncContext);
    bool OnHandleRequestVideo(const std::unique_ptr<RequestSourceAsyncContext> &asyncContext);
    bool NotifyDataPreparedWithoutRegister(const std::unique_ptr<RequestSourceAsyncContext> &asyncContext);
    void RegisterTaskObserver(const unique_ptr<RequestSourceAsyncContext> &asyncContext);
    void ProcessImage(const int fileId, const int deliveryMode, const std::string &packageName);
    static int32_t WriteFileToPath(const std::string &srcUri, const std::string &destPath, bool isSource);
    static int32_t GetFdFromSandBoxUri(const std::string &sandBoxUri);

private:
    static MediaLibraryManager* mediaLibraryManager_;
};
} // Media
} // OHOS
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_MANAGER_IMPL_H
