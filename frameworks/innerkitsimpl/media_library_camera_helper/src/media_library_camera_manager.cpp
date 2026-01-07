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

#define MLOG_TAG "MediaLibraryCameraManager"

#include "media_library_camera_manager.h"

#include <unordered_set>

#include "base_data_uri.h"
#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
const std::string MEDIA_FILEMODE_READONLY = "r";
const std::string MEDIA_FILEMODE_WRITEONLY = "w";
const std::string MEDIA_FILEMODE_READWRITE = "rw";
const std::string MEDIA_FILEMODE_WRITETRUNCATE = "wt";
const std::string MEDIA_FILEMODE_WRITEAPPEND = "wa";
const std::string MEDIA_FILEMODE_READWRITETRUNCATE = "rwt";
const std::string MEDIA_FILEMODE_READWRITEAPPEND = "rwa";
const std::unordered_set<std::string> MEDIA_OPEN_MODES = {
    MEDIA_FILEMODE_READONLY,
    MEDIA_FILEMODE_WRITEONLY,
    MEDIA_FILEMODE_READWRITE,
    MEDIA_FILEMODE_WRITETRUNCATE,
    MEDIA_FILEMODE_WRITEAPPEND,
    MEDIA_FILEMODE_READWRITETRUNCATE,
    MEDIA_FILEMODE_READWRITEAPPEND
};

MediaLibraryCameraManager *MediaLibraryCameraManager::GetMediaLibraryCameraManager()
{
    static MediaLibraryCameraManager mediaLibMgr;
    return &mediaLibMgr;
}

void MediaLibraryCameraManager::InitMediaLibraryCameraManager(const sptr<IRemoteObject> &token)
{
    std::unique_lock<std::mutex> locker(mutex_);
    token_ = token;
    CHECK_AND_EXECUTE(sDataShareHelper_ != nullptr,
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI));
}

std::shared_ptr<PhotoAssetProxy> MediaLibraryCameraManager::CreatePhotoAssetProxy(
    const PhotoAssetProxyCallerInfo &callerInfo, CameraShotType cameraShotType, int32_t videoCount)
{
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} dataShareHelper is ready, ret = %{public}d.",
        MLOG_TAG, __FUNCTION__, __LINE__, dataShareHelper != nullptr);
    std::shared_ptr<PhotoAssetProxy> photoAssetProxy = std::make_shared<PhotoAssetProxy>(
        dataShareHelper, callerInfo, cameraShotType, videoCount);
    return photoAssetProxy;
}

int32_t MediaLibraryCameraManager::OpenAsset(std::string &uri, const std::string &openMode)
{
    CHECK_AND_RETURN_RET(!openMode.empty(), E_ERR);
    CHECK_AND_RETURN_RET_LOG(MediaUriUtils::CheckUri(uri), E_ERR, "invalid uri");
    std::string originOpenMode = openMode;
    std::transform(originOpenMode.begin(), originOpenMode.end(),
        originOpenMode.begin(), [](unsigned char c) {return std::tolower(c);});
    if (!MEDIA_OPEN_MODES.count(originOpenMode)) {
        return E_ERR;
    }

    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("Failed to open Asset, datashareHelper is nullptr");
        return E_ERR;
    }
    Uri openUri(uri);
    return sDataShareHelper_->OpenFile(openUri, openMode);
}

int32_t MediaLibraryCameraManager::RegisterPhotoStateCallback(const LowQualityMemoryNumHandler &func)
{
    MEDIA_INFO_LOG("RegisterPhotoStateCallback begin.");
    if (callback_ == nullptr) {
        callback_ = new MediaLowQualityMemoryCallback();
        CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, E_ERR, "failed to get MediaLibraryCameraCallback.");
    }
    return callback_->RegisterPhotoStateCallback(sDataShareHelper_, func);
}
 
int32_t MediaLibraryCameraManager::UnregisterPhotoStateCallback()
{
    MEDIA_INFO_LOG("UnregisterPhotoStateCallback begin.");
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, E_ERR, "failed to get MediaLibraryCameraCallback.");
    int32_t ret = callback_->UnregisterPhotoStateCallback(sDataShareHelper_);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("failed to UnregisterPhotoStateCallback.");
        return ret;
    }
    callback_ = nullptr;
    MEDIA_INFO_LOG("UnregisterPhotoStateCallback success.");
    return E_OK;
}
} // namespace Media
} // namespace OHOS