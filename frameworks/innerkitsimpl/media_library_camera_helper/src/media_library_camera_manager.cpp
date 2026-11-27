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
#include "get_deferred_picture_info_vo.h"
#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_business_code.h"
#include "medialibrary_errno.h"
#include "user_inner_ipc_client.h"
#include "media_common_client.h"

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

void MediaLibraryCameraManager::InitMediaLibraryCameraManager(const sptr<IRemoteObject> &token, int32_t userId)
{
    std::unique_lock<std::mutex> locker(mutex_);
    token_ = token;
    IPC::MediaCommonClient::GetInstance().Init(token_, userId);
}

std::shared_ptr<DataShare::DataShareHelper> MediaLibraryCameraManager::GetDataShareHelper(int32_t userId)
{
    IPC::MediaCommonClient::GetInstance().Init(token_, userId);
    return IPC::MediaCommonClient::GetInstance().GetDataShareHelper(userId);
}

std::shared_ptr<PhotoAssetProxy> MediaLibraryCameraManager::CreatePhotoAssetProxy(
    const PhotoAssetProxyCallerInfo &callerInfo, CameraShotType cameraShotType, int32_t videoCount)
{
    std::unique_lock<std::mutex> locker(mutex_);
    auto dataShareHelper = GetDataShareHelper(callerInfo.userId);
    MEDIA_INFO_LOG("dataShareHelper is ready, ret = %{public}d, userId = %{public}d.",
        dataShareHelper != nullptr, callerInfo.userId);
    std::shared_ptr<PhotoAssetProxy> photoAssetProxy = std::make_shared<PhotoAssetProxy>(
        dataShareHelper, callerInfo, cameraShotType, videoCount);
    return photoAssetProxy;
}

std::shared_ptr<PhotoAssetProxy> MediaLibraryCameraManager::CreatePhotoAssetProxy(
    const PhotoAssetProxyCallerInfo &callerInfo, const CameraPresetPara &presetPara)
{
    std::unique_lock<std::mutex> locker(mutex_);
    auto dataShareHelper = GetDataShareHelper(callerInfo.userId);
    MEDIA_INFO_LOG("dataShareHelper is ready, ret = %{public}d, userId = %{public}d.",
        dataShareHelper != nullptr, callerInfo.userId);
    std::shared_ptr<PhotoAssetProxy> photoAssetProxy = std::make_shared<PhotoAssetProxy>(
        dataShareHelper, callerInfo, presetPara);
    return photoAssetProxy;
}

DeferredPictureInfo MediaLibraryCameraManager::GetDeferredPictureInfo(const std::string& photoId, int32_t userId)
{
    MEDIA_INFO_LOG("GetEditData begin, photoId: %{public}s.", photoId.c_str());
    DeferredPictureInfo pictureInfo;

    std::unique_lock<std::mutex> locker(mutex_);
    GetDeferredPictureInfoReqBody reqBody;
    reqBody.photoId = photoId;

    GetDeferredPictureInfoRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFERRED_PICTURE_INFO);
    IPC::UserInnerIPCClient().SetUserId(userId).Call(businessCode, reqBody, respBody);

    pictureInfo.editData = respBody.editData;
    pictureInfo.mimeType = respBody.mimeType;
    pictureInfo.enableC2PA = respBody.enableC2PA;
    pictureInfo.authorId = respBody.authorId;
    pictureInfo.authorName = respBody.authorName;
    MEDIA_INFO_LOG("%{public}s:{%{public}s:%{public}d} GetEditData done, photoId: %{public}s, "
        "hasEditData: %{public}d, mimeType: %{public}s [c2pa][%{public}d][%{public}s][%{public}s].",
        MLOG_TAG, __FUNCTION__, __LINE__, photoId.c_str(), !pictureInfo.editData.empty(),
        pictureInfo.mimeType.c_str(), static_cast<int32_t>(pictureInfo.enableC2PA),
        pictureInfo.authorId.c_str(), pictureInfo.authorName.c_str());
    return pictureInfo;
}

int32_t MediaLibraryCameraManager::OpenAsset(std::string &uri, const std::string &openMode, int32_t userId)
{
    CHECK_AND_RETURN_RET(!openMode.empty(), E_ERR);
    CHECK_AND_RETURN_RET_LOG(MediaUriUtils::CheckUri(uri), E_ERR, "invalid uri");
    std::string originOpenMode = openMode;
    std::transform(originOpenMode.begin(), originOpenMode.end(),
        originOpenMode.begin(), [](unsigned char c) {return std::tolower(c);});
    if (!MEDIA_OPEN_MODES.count(originOpenMode)) {
        return E_ERR;
    }

    std::unique_lock<std::mutex> locker(mutex_);
    auto dataShareHelper = GetDataShareHelper(userId);
    if (dataShareHelper == nullptr) {
        MEDIA_ERR_LOG("Failed to open Asset, datashareHelper is nullptr");
        return E_ERR;
    }
    Uri openUri(uri);
    openUri = MediaUriUtils::GetMultiUri(openUri, userId);
    return dataShareHelper->OpenFile(openUri, openMode);
}

int32_t MediaLibraryCameraManager::RegisterPhotoStateCallback(const LowQualityMemoryNumHandler &func, int32_t userId)
{
    MEDIA_INFO_LOG("RegisterPhotoStateCallback begin.");
    std::unique_lock<std::mutex> locker(mutex_);
    if (callback_ == nullptr) {
        callback_ = new MediaLowQualityMemoryCallback();
        CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, E_ERR, "failed to get MediaLibraryCameraCallback.");
    }
    auto dataShareHelper = GetDataShareHelper(userId);
    return callback_->RegisterPhotoStateCallback(dataShareHelper, func);
}

int32_t MediaLibraryCameraManager::UnregisterPhotoStateCallback(int32_t userId)
{
    MEDIA_INFO_LOG("UnregisterPhotoStateCallback begin.");
    std::unique_lock<std::mutex> locker(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback_ != nullptr, E_ERR, "failed to get MediaLibraryCameraCallback.");
    auto dataShareHelper = GetDataShareHelper(userId);
    int32_t ret = callback_->UnregisterPhotoStateCallback(dataShareHelper);
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