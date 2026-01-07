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

#define MLOG_TAG "MediaAssetManagerImpl"

#include "media_asset_manager_impl.h"

#include <safe_map.h>
#include <securec.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <uuid.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "directory_ex.h"
#include "file_uri.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "file_asset.h"
#include "oh_media_asset.h"
#include "oh_moving_photo.h"
#include "moving_photo.h"
#include "image_source_native.h"
#include "userfile_client.h"
#include "permission_utils.h"
#include "picture_handle_client.h"
#include "picture_native.h"
#include "picture_native_impl.h"
#include "userfilemgr_uri.h"

#include "medialibrary_business_code.h"
#include "user_inner_ipc_client.h"
#include "query_photo_vo.h"

using namespace OHOS::Security::AccessToken;
namespace OHOS {
namespace Media {
namespace {
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
} // namespace

using Uri = OHOS::Uri;

static const std::string MEDIA_ASSET_MANAGER_CLASS = "MediaAssetManagerImpl";
const std::string API_VERSION_STR = "api_version";
static std::mutex multiStagesCaptureLock;

const int32_t LOW_QUALITY_IMAGE = 1;

const uint32_t MAX_URI_SIZE = 384;
const std::string ERROR_REQUEST_ID = "00000000-0000-0000-0000-000000000000";

static const std::string URI_TYPE = "uriType";
static const std::string TYPE_PHOTOS = "1";

static std::map<std::string, std::shared_ptr<MultiStagesTaskObserver>> multiStagesObserverMap;
static std::map<std::string, std::map<std::string, AssetHandler*>> inProcessUriMap;
static SafeMap<std::string, AssetHandler*> inProcessFastRequests;

static std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;

MediaLibraryManager* MediaAssetManagerImpl::mediaLibraryManager_ = nullptr;

std::mutex MediaAssetManagerImpl::mutex_;

std::shared_ptr<MediaAssetManager> MediaAssetManagerFactory::CreateMediaAssetManager()
{
    std::shared_ptr<MediaAssetManager> impl = std::make_shared<MediaAssetManagerImpl>();
    CHECK_AND_PRINT_LOG(impl != nullptr, "Failed to create MediaAssetManagerImpl instance.");

    return impl;
}

MediaAssetManagerImpl::MediaAssetManagerImpl()
{
    MediaAssetManagerImpl::mediaLibraryManager_ = MediaLibraryManager::GetMediaLibraryManager();
    CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
}

MediaAssetManagerImpl::~MediaAssetManagerImpl()
{
}

static void DeleteInProcessMapRecord(const std::string &requestUri, const std::string &requestId)
{
    MEDIA_INFO_LOG("DeleteInProcessMapRecord lock multiStagesCaptureLock");
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(requestUri);
    if (inProcessUriMap.find(uriLocal) == inProcessUriMap.end()) {
        return;
    }

    std::map<std::string, AssetHandler*> assetHandlers = inProcessUriMap[uriLocal];
    if (assetHandlers.find(requestId) == assetHandlers.end()) {
        return;
    }

    assetHandlers.erase(requestId);
    if (!assetHandlers.empty()) {
        inProcessUriMap[uriLocal] = assetHandlers;
        return;
    }

    inProcessUriMap.erase(uriLocal);

    if (multiStagesObserverMap.find(uriLocal) != multiStagesObserverMap.end()) {
        sDataShareHelper_->UnregisterObserverExt(Uri(uriLocal),
            static_cast<std::shared_ptr<DataShare::DataShareObserver>>(multiStagesObserverMap[uriLocal]));
    }
    multiStagesObserverMap.erase(uriLocal);
    MEDIA_INFO_LOG("DeleteInProcessMapRecord unlock multiStagesCaptureLock");
}

static AssetHandler* CreateAssetHandler(const std::string &photoId, const std::string &requestId,
    const std::string &uri, const std::string &destUri, const MediaAssetDataHandlerPtr &handler)
{
    AssetHandler *assetHandler = new AssetHandler(photoId, requestId, uri, destUri, handler);
    MEDIA_DEBUG_LOG("[AssetHandler create] photoId: %{public}s, requestId: %{public}s, uri: %{public}s",
        photoId.c_str(), requestId.c_str(), uri.c_str());
    return assetHandler;
}

static void DeleteAssetHandlerSafe(AssetHandler *handler)
{
    if (handler != nullptr) {
        delete handler;
        handler = nullptr;
    }
}

static int32_t IsInProcessInMapRecord(const std::string &requestId, AssetHandler* &handler)
{
    MEDIA_INFO_LOG("IsInProcessInMapRecord lock multiStagesCaptureLock");
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    for (auto record : inProcessUriMap) {
        if (record.second.find(requestId) != record.second.end()) {
            handler = record.second[requestId];
            MEDIA_INFO_LOG("IsInProcessInMapRecord unlock multiStagesCaptureLock");
            return true;
        }
    }
    MEDIA_INFO_LOG("IsInProcessInMapRecord unlock multiStagesCaptureLock");
    return false;
}

static bool IsFastRequestCanceled(const std::string &requestId, std::string &photoId)
{
    AssetHandler *assetHandler = nullptr;
    if (!inProcessFastRequests.Find(requestId, assetHandler)) {
        MEDIA_ERR_LOG("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        MEDIA_ERR_LOG("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    inProcessFastRequests.Erase(requestId);
    return true;
}

static bool IsMapRecordCanceled(const std::string &requestId, std::string &photoId)
{
    AssetHandler *assetHandler = nullptr;
    if (!IsInProcessInMapRecord(requestId, assetHandler)) {
        MEDIA_ERR_LOG("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        MEDIA_ERR_LOG("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    DeleteInProcessMapRecord(assetHandler->requestUri, assetHandler->requestId);
    DeleteAssetHandlerSafe(assetHandler);
    return true;
}

static void DeleteDataHandler(NativeNotifyMode notifyMode, const std::string &requestUri, const std::string &requestId)
{
    MEDIA_INFO_LOG("Rmv %{public}d, %{public}s, %{public}s", notifyMode,
        MediaFileUtils::DesensitizeUri(requestUri).c_str(), requestId.c_str());
    if (notifyMode == NativeNotifyMode::WAIT_FOR_HIGH_QUALITY) {
        DeleteInProcessMapRecord(requestUri, requestId);
    }
    inProcessFastRequests.Erase(requestId);
}

static void InsertInProcessMapRecord(const std::string &requestUri, const std::string &requestId,
    AssetHandler *handler)
{
    MEDIA_INFO_LOG("InsertInProcessMapRecord lock multiStagesCaptureLock");
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    std::map<std::string, AssetHandler*> assetHandler;
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(requestUri);
    if (inProcessUriMap.find(uriLocal) != inProcessUriMap.end()) {
        assetHandler = inProcessUriMap[uriLocal];
        assetHandler[requestId] = handler;
        inProcessUriMap[uriLocal] = assetHandler;
    } else {
        assetHandler[requestId] = handler;
        inProcessUriMap[uriLocal] = assetHandler;
    }
    MEDIA_INFO_LOG("InsertInProcessMapRecord unlock multiStagesCaptureLock");
}

static std::string GenerateRequestId()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

static AssetHandler* InsertDataHandler(NativeNotifyMode notifyMode,
    const unique_ptr<RequestSourceAsyncContext> &asyncContext)
{
    std::shared_ptr<CapiMediaAssetDataHandler> mediaAssetDataHandler;
    if (asyncContext->returnDataType == ReturnDataType::TYPE_IMAGE_SOURCE) {
        mediaAssetDataHandler = make_shared<CapiMediaAssetDataHandler>(
            asyncContext->onRequestImageDataPreparedHandler, asyncContext->returnDataType, asyncContext->requestUri,
            asyncContext->destUri, asyncContext->requestOptions.sourceMode);
        mediaAssetDataHandler->SetPhotoQuality(static_cast<int32_t>(asyncContext->photoQuality));
    } else if (asyncContext->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO) {
        mediaAssetDataHandler = make_shared<CapiMediaAssetDataHandler>(
            asyncContext->onRequestMovingPhotoDataPreparedHandler, asyncContext->returnDataType,
            asyncContext->requestUri, asyncContext->destUri, asyncContext->requestOptions.sourceMode);
        mediaAssetDataHandler->SetPhotoQuality(static_cast<int32_t>(asyncContext->photoQuality));
    } else if (asyncContext->returnDataType == ReturnDataType::TYPE_PICTURE) {
        mediaAssetDataHandler = make_shared<CapiMediaAssetDataHandler>(
            asyncContext->onRequestQuickImageDataPreparedHandler, asyncContext->returnDataType,
            asyncContext->requestUri, asyncContext->destUri, asyncContext->requestOptions.sourceMode);
        mediaAssetDataHandler->SetPhotoQuality(static_cast<int32_t>(asyncContext->photoQuality));
    } else {
        mediaAssetDataHandler = make_shared<CapiMediaAssetDataHandler>(
        asyncContext->onDataPreparedHandler, asyncContext->returnDataType, asyncContext->requestUri,
        asyncContext->destUri, asyncContext->requestOptions.sourceMode);
    }

    mediaAssetDataHandler->SetNotifyMode(notifyMode);
    AssetHandler *assetHandler = CreateAssetHandler(asyncContext->photoId, asyncContext->requestId,
        asyncContext->requestUri, asyncContext->destUri, mediaAssetDataHandler);
    MEDIA_INFO_LOG("Add %{public}d, %{private}s, %{private}s", notifyMode,
        MediaFileUtils::DesensitizeUri(asyncContext->requestUri).c_str(), asyncContext->requestId.c_str());

    switch (notifyMode) {
        case NativeNotifyMode::FAST_NOTIFY: {
            inProcessFastRequests.EnsureInsert(asyncContext->requestId, assetHandler);
            break;
        }
        case NativeNotifyMode::WAIT_FOR_HIGH_QUALITY: {
            InsertInProcessMapRecord(asyncContext->requestUri, asyncContext->requestId, assetHandler);
            break;
        }
        default:
            break;
    }

    return assetHandler;
}

MultiStagesCapturePhotoStatus MediaAssetManagerImpl::QueryPhotoStatus(int32_t fileId, std::string &photoId)
{
    photoId = "";
    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("Get sDataShareHelper_ failed");
        return MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL;
    }

    QueryPhotoReqBody reqBody;
    QueryPhotoRespBody respBody;
    reqBody.fileId = std::to_string(fileId);
    std::unordered_map<std::string, std::string> headerMap {
        {MediaColumn::MEDIA_ID, reqBody.fileId }, {URI_TYPE, TYPE_PHOTOS}
    };
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_QUERY_PHOTO_STATUS);
    int errCode = IPC::UserInnerIPCClient().SetHeader(headerMap)
        .SetDataShareHelper(sDataShareHelper_).Call(businessCode, reqBody, respBody);
    if (errCode < 0) {
        MEDIA_ERR_LOG("UserInnerIPCClient Call errCode:%{public}d", errCode);
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    }

    photoId = respBody.photoId;
    MEDIA_ERR_LOG("Query photo status quality: %{public}d", respBody.photoQuality);
    if (respBody.photoQuality == LOW_QUALITY_IMAGE) {
        return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
    }
    return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
}

bool MediaAssetManagerImpl::NotifyImageDataPrepared(AssetHandler *assetHandler)
{
    CHECK_AND_RETURN_RET_LOG(assetHandler != nullptr, false, "assetHandler is nullptr");

    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        MEDIA_ERR_LOG("Data handler is nullptr");
        return false;
    }

    NativeNotifyMode notifyMode = dataHandler->GetNotifyMode();
    if (notifyMode == NativeNotifyMode::FAST_NOTIFY) {
        AssetHandler *tmp;
        if (!inProcessFastRequests.Find(assetHandler->requestId, tmp)) {
            MEDIA_ERR_LOG("The request has been canceled");
            return false;
        }
    }

    int32_t writeResult = E_OK;
    errno_t strncpyResult = E_OK;
    if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_TARGET_FILE) {
        writeResult = MediaAssetManagerImpl::WriteFileToPath(dataHandler->GetRequestUri(), dataHandler->GetDestUri(),
            dataHandler->GetSourceMode() == NativeSourceMode::ORIGINAL_MODE);
        Native_RequestId requestId;
        strncpyResult = strncpy_s(requestId.requestId,
            UUID_STR_LENGTH, assetHandler->requestId.c_str(), UUID_STR_LENGTH);
        CHECK_AND_RETURN_RET_LOG(strncpyResult == E_OK, false, "strncpy failed");
        if (dataHandler->onDataPreparedHandler_ != nullptr) {
            dataHandler->onDataPreparedHandler_(writeResult, requestId);
        }
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_IMAGE_SOURCE) {
        MediaLibrary_RequestId requestId;
        strncpyResult = strncpy_s(requestId.requestId,
            UUID_STR_LENGTH, assetHandler->requestId.c_str(), UUID_STR_LENGTH);
        CHECK_AND_RETURN_RET_LOG(strncpyResult == E_OK, false, "strncpy failed");
        if (dataHandler->onRequestImageDataPreparedHandler_ != nullptr) {
            int32_t photoQuality = static_cast<int32_t>(MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS);
            MediaLibrary_MediaQuality quality = (dataHandler->GetPhotoQuality() == photoQuality)
                ? MEDIA_LIBRARY_QUALITY_FULL
                : MEDIA_LIBRARY_QUALITY_FAST;
            auto imageSource = CreateImageSource(assetHandler->requestId, dataHandler->GetRequestUri());
            auto status = imageSource != nullptr ? MEDIA_LIBRARY_OK : MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            dataHandler->onRequestImageDataPreparedHandler_(status, requestId, quality,
                MEDIA_LIBRARY_COMPRESSED, imageSource);
        }
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_MOVING_PHOTO) {
        MediaLibrary_RequestId requestId;
        strncpyResult = strncpy_s(requestId.requestId,
            UUID_STR_LENGTH, assetHandler->requestId.c_str(), UUID_STR_LENGTH);
        CHECK_AND_RETURN_RET_LOG(strncpyResult == E_OK, false, "strncpy failed");
        if (dataHandler->onRequestMovingPhotoDataPreparedHandler_ != nullptr) {
            int32_t photoQuality = static_cast<int32_t>(MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS);
            MediaLibrary_MediaQuality quality = (dataHandler->GetPhotoQuality() == photoQuality)
                ? MEDIA_LIBRARY_QUALITY_FULL
                : MEDIA_LIBRARY_QUALITY_FAST;
            auto movingPhotoImpl = MovingPhotoFactory::CreateMovingPhoto(assetHandler->requestUri);
            auto movingPhoto = new OH_MovingPhoto(movingPhotoImpl);
            auto status = movingPhoto != nullptr ? MEDIA_LIBRARY_OK : MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            dataHandler->onRequestMovingPhotoDataPreparedHandler_(status, requestId, quality,
                MEDIA_LIBRARY_COMPRESSED, movingPhoto);
        }
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_PICTURE) {
        MediaLibrary_RequestId requestId;
        strncpyResult = strncpy_s(requestId.requestId,
            UUID_STR_LENGTH, assetHandler->requestId.c_str(), UUID_STR_LENGTH);
        CHECK_AND_RETURN_RET_LOG(strncpyResult == E_OK, false, "strncpy failed");
        if (dataHandler->onRequestQuickImageDataPreparedHandler_ != nullptr) {
            int32_t photoQuality = static_cast<int32_t>(MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS);
            MediaLibrary_MediaQuality quality = (dataHandler->GetPhotoQuality() == photoQuality)
                ? MEDIA_LIBRARY_QUALITY_FULL
                : MEDIA_LIBRARY_QUALITY_FAST;
            bool isPicture = true;
            OH_PictureNative* pictureNative = nullptr;
            OH_ImageSourceNative* imageSourceNative = nullptr;
            GetPictureNativeObject(
                assetHandler->requestId, dataHandler->GetRequestUri(), &pictureNative, &imageSourceNative, isPicture);
            
            MediaLibrary_ErrorCode status;
            if (isPicture) {
                status = pictureNative != nullptr ? MEDIA_LIBRARY_OK : MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            } else {
                status = imageSourceNative != nullptr ? MEDIA_LIBRARY_OK : MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            }
            dataHandler->onRequestQuickImageDataPreparedHandler_(status, requestId, quality,
                MEDIA_LIBRARY_COMPRESSED, imageSourceNative, pictureNative);
        }
    } else {
        MEDIA_ERR_LOG("Return mode type invalid %{public}d", dataHandler->GetReturnDataType());
        return false;
    }
    DeleteDataHandler(notifyMode, assetHandler->requestUri, assetHandler->requestId);
    MEDIA_DEBUG_LOG("Delete assetHandler");
    return true;
}

void MediaAssetManagerImpl::CreateDataHelper(int32_t systemAbilityId)
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("Get system ability mgr failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service Failed.");
        return;
    }
    if (sDataShareHelper_ == nullptr) {
        const sptr<IRemoteObject> &token = remoteObj;
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
        if (sDataShareHelper_ == nullptr) {
            MEDIA_ERR_LOG("Create DataShareHelper failed.");
            return;
        }
    }
    MediaAssetManagerImpl::mediaLibraryManager_->InitMediaLibraryManager(remoteObj);
    UserFileClient::Init(remoteObj);
    MEDIA_INFO_LOG("InitMediaLibraryManager success!");
}

std::string MediaAssetManagerImpl::NativeRequestImage(const char* photoUri,
    const NativeRequestOptions &requestOptions, const char* destUri, const NativeOnDataPrepared &callback)
{
    if (photoUri == nullptr || destUri == nullptr || callback == nullptr) {
        MEDIA_ERR_LOG("Request image input params are invalid.");
        return ERROR_REQUEST_ID;
    }

    MediaLibraryTracer tracer;
    tracer.Start("NativeRequestImage");

    std::unique_ptr<RequestSourceAsyncContext> asyncContext = std::make_unique<RequestSourceAsyncContext>();
    asyncContext->destUri = std::string(destUri);
    asyncContext->requestUri = std::string(photoUri);
    asyncContext->displayName = MediaFileUtils::GetFileName(asyncContext->requestUri);
    asyncContext->fileId = std::stoi(MediaFileUtils::GetIdFromUri(asyncContext->requestUri));
    asyncContext->requestOptions.deliveryMode = requestOptions.deliveryMode;
    asyncContext->requestOptions.sourceMode = NativeSourceMode::EDITED_MODE;
    asyncContext->returnDataType = ReturnDataType::TYPE_TARGET_FILE;
    asyncContext->onDataPreparedHandler = callback;

    if (asyncContext->requestUri.length() > MAX_URI_SIZE || asyncContext->destUri.length() > MAX_URI_SIZE) {
        MEDIA_ERR_LOG("Request image uri lens out of limit requestUri lens: %{public}zu, destUri lens: %{public}zu",
            asyncContext->requestUri.length(), asyncContext->destUri.length());
        return ERROR_REQUEST_ID;
    }
    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_IMAGE ||
        MediaFileUtils::GetMediaType(MediaFileUtils::GetFileName(asyncContext->destUri)) != MEDIA_TYPE_IMAGE) {
        MEDIA_ERR_LOG("Request image file type invalid");
        return ERROR_REQUEST_ID;
    }
    bool isSuccess = false;
    asyncContext->requestId = GenerateRequestId();
    isSuccess = OnHandleRequestImage(asyncContext);
    if (isSuccess) {
        MEDIA_INFO_LOG("Request image success return requestId: %{public}s", asyncContext->requestId.c_str());
        return asyncContext->requestId;
    } else {
        return ERROR_REQUEST_ID;
    }
}

std::string MediaAssetManagerImpl::NativeRequestVideo(const char* videoUri,
    const NativeRequestOptions &requestOptions, const char* destUri, const NativeOnDataPrepared &callback)
{
    if (videoUri == nullptr || destUri == nullptr || callback == nullptr) {
        MEDIA_ERR_LOG("Request video input params are invalid.");
        return ERROR_REQUEST_ID;
    }
    MediaLibraryTracer tracer;
    tracer.Start("NativeRequestVideo");

    std::unique_ptr<RequestSourceAsyncContext> asyncContext = std::make_unique<RequestSourceAsyncContext>();
    asyncContext->destUri = std::string(destUri);
    asyncContext->requestUri = std::string(videoUri);
    asyncContext->displayName = MediaFileUtils::GetFileName(asyncContext->requestUri);
    asyncContext->fileId = std::stoi(MediaFileUtils::GetIdFromUri(asyncContext->requestUri));
    asyncContext->requestOptions.deliveryMode = requestOptions.deliveryMode;
    asyncContext->requestOptions.sourceMode = NativeSourceMode::EDITED_MODE;
    asyncContext->returnDataType = ReturnDataType::TYPE_TARGET_FILE;
    asyncContext->onDataPreparedHandler = callback;

    if (asyncContext->requestUri.length() > MAX_URI_SIZE || asyncContext->destUri.length() > MAX_URI_SIZE) {
        MEDIA_ERR_LOG("Request video uri lens out of limit requestUri lens: %{public}zu, destUri lens: %{public}zu",
            asyncContext->requestUri.length(), asyncContext->destUri.length());
        return ERROR_REQUEST_ID;
    }
    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_VIDEO ||
        MediaFileUtils::GetMediaType(MediaFileUtils::GetFileName(asyncContext->destUri)) != MEDIA_TYPE_VIDEO) {
        MEDIA_ERR_LOG("Request video file type invalid");
        return ERROR_REQUEST_ID;
    }
    bool isSuccess = false;
    asyncContext->requestId = GenerateRequestId();
    isSuccess = OnHandleRequestVideo(asyncContext);
    if (isSuccess) {
        MEDIA_ERR_LOG("Request video success return requestId: %{public}s", asyncContext->requestId.c_str());
        return asyncContext->requestId;
    } else {
        return ERROR_REQUEST_ID;
    }
}

void UriAppendKeyValue(string &uri, const string &key, const string &value)
{
    string uriKey = key + '=';
    if (uri.find(uriKey) != string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == string::npos) ? '?' : '&';
    string append = queryMark + key + '=' + value;

    size_t posJ = uri.find('#');
    if (posJ == string::npos) {
        uri += append;
    } else {
        uri.insert(posJ, append);
    }
}

bool MediaAssetManagerImpl::NativeCancelRequest(const std::string &requestId)
{
    if (requestId.empty()) {
        MEDIA_ERR_LOG("NativeCancel request id is empty.");
        return false;
    }
    if (sDataShareHelper_ == nullptr) {
        CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ == nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "sDataShareHelper_ is null");
    }

    std::string photoId = "";
    bool hasFastRequestInProcess = IsFastRequestCanceled(requestId, photoId);
    bool hasMapRecordInProcess = IsMapRecordCanceled(requestId, photoId);
    if (hasFastRequestInProcess || hasMapRecordInProcess) {
        std::string uriStr = PAH_CANCEL_PROCESS_IMAGE;
        UriAppendKeyValue(uriStr, API_VERSION_STR, to_string(MEDIA_API_VERSION_V10));
        Uri updateAssetUri(uriStr);
        DataShare::DataSharePredicates predicates;
        DataShare::DatashareBusinessError errCode;
        std::vector<std::string> columns { photoId };
        sDataShareHelper_->Query(updateAssetUri, predicates, columns, &errCode);
    } else {
        MEDIA_ERR_LOG("NativeCancel requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }
    return true;
}

MediaLibrary_ErrorCode MediaAssetManagerImpl::NativeRequestImageSource(OH_MediaAsset* mediaAsset,
    NativeRequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnImageDataPrepared callback)
{
    MEDIA_INFO_LOG("MediaAssetManagerImpl::NativeRequestImageSource Called");
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr && mediaAsset->mediaAsset_ != nullptr,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "mediaAsset or mediaAsset_ is null");
    std::shared_ptr<FileAsset> fileAsset_ = mediaAsset->mediaAsset_->GetFileAssetInstance();
    MediaLibraryTracer tracer;
    tracer.Start("NativeRequestImageSource");

    std::unique_ptr<RequestSourceAsyncContext> asyncContext = std::make_unique<RequestSourceAsyncContext>();
    asyncContext->requestUri = fileAsset_->GetUri();
    asyncContext->displayName = fileAsset_->GetDisplayName();
    asyncContext->fileId = fileAsset_->GetId();
    asyncContext->requestOptions.deliveryMode = requestOptions.deliveryMode;
    asyncContext->requestOptions.sourceMode = NativeSourceMode::EDITED_MODE;
    asyncContext->returnDataType = ReturnDataType::TYPE_IMAGE_SOURCE;
    asyncContext->needsExtraInfo = true;
    asyncContext->onRequestImageDataPreparedHandler = callback;

    if (sDataShareHelper_ == nullptr) {
        CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ == nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "sDataShareHelper_ is null");
    }

    if (asyncContext->requestUri.length() > MAX_URI_SIZE) {
        MEDIA_ERR_LOG("Request image uri lens out of limit requestUri lens: %{public}zu",
            asyncContext->requestUri.length());
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_PARAMETER_ERROR;
    }

    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_IMAGE) {
        MEDIA_ERR_LOG("Request image file type invalid");
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_PARAMETER_ERROR;
    }

    bool isSuccess = false;
    asyncContext->requestId = GenerateRequestId();
    isSuccess = OnHandleRequestImage(asyncContext);
    if (isSuccess) {
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (asyncContext->requestId.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OK;
    } else {
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED;
    }
}

MediaLibrary_ErrorCode MediaAssetManagerImpl::NativeRequestMovingPhoto(OH_MediaAsset* mediaAsset,
    NativeRequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnMovingPhotoDataPrepared callback)
{
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr && mediaAsset->mediaAsset_ != nullptr,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "mediaAsset or mediaAsset_ is null");
    std::shared_ptr<FileAsset> fileAsset_ = mediaAsset->mediaAsset_->GetFileAssetInstance();
    MediaLibraryTracer tracer;
    tracer.Start("NativeRequestMovingPhoto");

    std::unique_ptr<RequestSourceAsyncContext> asyncContext = std::make_unique<RequestSourceAsyncContext>();
    asyncContext->requestUri = fileAsset_->GetUri();
    asyncContext->fileId = fileAsset_->GetId();
    asyncContext->displayName = fileAsset_->GetDisplayName();
    asyncContext->requestOptions.deliveryMode = requestOptions.deliveryMode;
    asyncContext->requestOptions.sourceMode = NativeSourceMode::EDITED_MODE;
    asyncContext->returnDataType = ReturnDataType::TYPE_MOVING_PHOTO;
    asyncContext->onRequestMovingPhotoDataPreparedHandler = callback;

    if (sDataShareHelper_ == nullptr) {
        CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ == nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "sDataShareHelper_ is null");
    }

    if (asyncContext->requestUri.length() > MAX_URI_SIZE) {
        MEDIA_ERR_LOG("Request image uri lens out of limit requestUri lens: %{public}zu",
            asyncContext->requestUri.length());
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_PARAMETER_ERROR;
    }

    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_IMAGE) {
        MEDIA_ERR_LOG("Request image file type invalid");
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_PARAMETER_ERROR;
    }

    bool isSuccess = false;
    asyncContext->requestId = GenerateRequestId();
    isSuccess = OnHandleRequestImage(asyncContext);
    string uri = LOG_MOVING_PHOTO;
    Uri logMovingPhotoUri(uri);
    DataShare::DataShareValuesBucket valuesBucket;
    string result;
    valuesBucket.Put("package_name", asyncContext->callingPkgName);
    valuesBucket.Put("adapted", asyncContext->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO);
    UserFileClient::InsertExt(logMovingPhotoUri, valuesBucket, result);
    if (isSuccess) {
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (asyncContext->requestId.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OK;
    } else {
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED;
    }
}

OH_ImageSourceNative* MediaAssetManagerImpl::CreateImageSource(const std::string requestId,
    const std::string requestUri)
{
    MEDIA_INFO_LOG("Request image success requestId: %{public}s, uri: %{public}s",
        requestId.c_str(), requestUri.c_str());

    std::string tmpUri = requestUri;
    MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    MediaFileUtils::UriAppendKeyValue(tmpUri, PHOTO_TRANSCODE_OPERATION, OPRN_TRANSCODE_HEIF);
    Uri uri(tmpUri);
    int fd = UserFileClient::OpenFile(uri, "r");
    CHECK_AND_RETURN_RET_LOG(fd >= 0, nullptr, "get image fd failed");

    struct OH_ImageSourceNative *imageSource;
    OH_ImageSourceNative_CreateFromFd(fd, &imageSource);
    close(fd);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "new OH_ImageSourceNative failed");

    return imageSource;
}

void MediaAssetManagerImpl::GetPictureNativeObject(const std::string requestId, const std::string fileUri,
    OH_PictureNative** pictureNative, OH_ImageSourceNative** imageSourceNative, bool &isPicture)
{
    MEDIA_INFO_LOG("GetPictureNativeObject");
    std::string tempStr = fileUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length());
    std::size_t index = tempStr.find("/");
    std::string fileId = tempStr.substr(0, index);

    auto picture = PictureHandlerClient::RequestPicture(std::atoi(fileId.c_str()));
    if (picture == nullptr) {
        MEDIA_INFO_LOG("picture is null");
        isPicture = false;
        *imageSourceNative = CreateImageSource(requestId, fileUri);
        return;
    }

    MEDIA_INFO_LOG("picture is not null.");
    *pictureNative = new OH_PictureNative(picture);
    if (*pictureNative == nullptr) {
        MEDIA_ERR_LOG("Failed to create OH_PictureNative.");
        return;
    }
}

bool MediaAssetManagerImpl::OnHandleRequestImage(
    const std::unique_ptr<RequestSourceAsyncContext> &asyncContext)
{
    MultiStagesCapturePhotoStatus status = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    bool result = false;
    switch (asyncContext->requestOptions.deliveryMode) {
        case NativeDeliveryMode::FAST_MODE:
            if (asyncContext->needsExtraInfo) {
                asyncContext->photoQuality = QueryPhotoStatus(asyncContext->fileId, asyncContext->photoId);
                MEDIA_DEBUG_LOG("OnHandleRequestImage photoQuality: %{public}d", asyncContext->photoQuality);
            }
            result = NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        case NativeDeliveryMode::HIGH_QUALITY_MODE:
            status = QueryPhotoStatus(asyncContext->fileId, asyncContext->photoId);
            asyncContext->photoQuality = status;
            MEDIA_DEBUG_LOG("OnHandleRequestImage photoQuality: %{public}d", asyncContext->photoQuality);
            if (status == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
                result = NotifyDataPreparedWithoutRegister(asyncContext);
            } else {
                RegisterTaskObserver(asyncContext);
                result = true;
            }
            break;
        case NativeDeliveryMode::BALANCED_MODE:
            status = QueryPhotoStatus(asyncContext->fileId, asyncContext->photoId);
            asyncContext->photoQuality = status;
            MEDIA_DEBUG_LOG("OnHandleRequestImage photoQuality: %{public}d", asyncContext->photoQuality);
            result = NotifyDataPreparedWithoutRegister(asyncContext);
            if (status == MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
                RegisterTaskObserver(asyncContext);
            }
            break;
        default: {
            MEDIA_ERR_LOG("Invalid delivery mode %{public}d", asyncContext->requestOptions.deliveryMode);
            return result;
        }
    }
    return result;
}

bool MediaAssetManagerImpl::OnHandleRequestVideo(
    const std::unique_ptr<RequestSourceAsyncContext> &asyncContext)
{
    bool result = false;
    switch (asyncContext->requestOptions.deliveryMode) {
        case NativeDeliveryMode::FAST_MODE:
            result = NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        case NativeDeliveryMode::HIGH_QUALITY_MODE:
            result = NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        case NativeDeliveryMode::BALANCED_MODE:
            result = NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        default: {
            MEDIA_ERR_LOG("Invalid delivery mode %{public}d", asyncContext->requestOptions.deliveryMode);
            return result;
        }
    }
    return result;
}

bool MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(
    const std::unique_ptr<RequestSourceAsyncContext> &asyncContext)
{
    bool ret = false;
    AssetHandler *assetHandler = InsertDataHandler(NativeNotifyMode::FAST_NOTIFY, asyncContext);
    if (assetHandler == nullptr) {
        MEDIA_ERR_LOG("assetHandler is nullptr");
        return ret;
    }

    {
        std::lock_guard<mutex> lock(MediaAssetManagerImpl::mutex_);
        ret = NotifyImageDataPrepared(assetHandler);
        DeleteAssetHandlerSafe(assetHandler);
    }
    return ret;
}

void MediaAssetManagerImpl::RegisterTaskObserver(const unique_ptr<RequestSourceAsyncContext> &asyncContext)
{
    auto dataObserver = std::make_shared<MultiStagesTaskObserver>(asyncContext->fileId);
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(asyncContext->requestUri);
    if (multiStagesObserverMap.find(uriLocal) == multiStagesObserverMap.end()) {
        sDataShareHelper_->RegisterObserverExt(Uri(uriLocal),
            static_cast<std::shared_ptr<DataShare::DataShareObserver>>(dataObserver), false);
        multiStagesObserverMap.insert(std::make_pair(uriLocal, dataObserver));
    }

    InsertDataHandler(NativeNotifyMode::WAIT_FOR_HIGH_QUALITY, asyncContext);

    ProcessImage(asyncContext->fileId, static_cast<int32_t>(asyncContext->requestOptions.deliveryMode));
}

void MediaAssetManagerImpl::ProcessImage(const int fileId, const int deliveryMode)
{
    CHECK_AND_RETURN_LOG(sDataShareHelper_ != nullptr, "Get sDataShareHelper_ failed");
    std::string uriStr = PAH_PROCESS_IMAGE;
    MediaFileUtils::UriAppendKeyValue(uriStr, API_VERSION_STR, std::to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    DataShare::DatashareBusinessError errCode;
    std::vector<std::string> columns { std::to_string(fileId), std::to_string(deliveryMode) };
    sDataShareHelper_->Query(uri, predicates, columns, &errCode);
    MEDIA_INFO_LOG("MediaAssetManagerImpl::ProcessImage Called");
}

void MultiStagesTaskObserver::OnChange(const ChangeInfo &changeInfo)
{
    if (changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_UPDATE)) {
        MEDIA_DEBUG_LOG("Ignore notify change, type: %{public}d", changeInfo.changeType_);
        return;
    }
    std::string photoId = "";
    if (MediaAssetManagerImpl::QueryPhotoStatus(fileId_, photoId) !=
        MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
        MEDIA_ERR_LOG("Requested data not prepared");
        return;
    }

    MEDIA_INFO_LOG("MultiStagesTaskObserver::OnChange Called");
    for (auto &uri : changeInfo.uris_) {
        string uriString = uri.ToString();
        std::map<std::string, AssetHandler *> assetHandlers = GetAssetHandlers(uriString);
        for (auto handler : assetHandlers) {
            auto assetHandler = handler.second;
            auto dataHandler = assetHandler->dataHandler;
            if (dataHandler != nullptr) {
                int32_t quality = static_cast<int32_t>(MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS);
                dataHandler->SetPhotoQuality(quality);
            }
            {
                std::lock_guard<mutex> lock(MediaAssetManagerImpl::mutex_);
                MediaAssetManagerImpl::NotifyImageDataPrepared(assetHandler);
                DeleteAssetHandlerSafe(assetHandler);
            }
        }
    }
}

std::map<std::string, AssetHandler *> MultiStagesTaskObserver::GetAssetHandlers(const std::string uriString)
{
    MEDIA_INFO_LOG("GetAssetHandlers lock multiStagesCaptureLock");
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    if (inProcessUriMap.find(uriString) == inProcessUriMap.end()) {
        MEDIA_INFO_LOG("current uri does not in process, uri: %{public}s", uriString.c_str());
        MEDIA_INFO_LOG("GetAssetHandlers unlock multiStagesCaptureLock");
        return std::map<std::string, AssetHandler*>();
    }
    MEDIA_INFO_LOG("GetAssetHandlers unlock multiStagesCaptureLock");
    return inProcessUriMap[uriString];
}

int32_t MediaAssetManagerImpl::WriteFileToPath(const std::string &srcUri, const std::string &destUri,
    bool isSource)
{
    if (srcUri.empty() || destUri.empty()) {
        MEDIA_ERR_LOG("srcUri or destUri is empty");
        return E_INVALID_URI;
    }
    std::string tmpSrcUri = srcUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpSrcUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    int srcFd = MediaAssetManagerImpl::mediaLibraryManager_->OpenAsset(tmpSrcUri, MEDIA_FILEMODE_READONLY);
    if (srcFd < 0) {
        MEDIA_ERR_LOG("Get source %{public}s fd error: %{public}d", tmpSrcUri.c_str(), srcFd);
        return srcFd;
    }
    struct stat statSrc;
    if (fstat(srcFd, &statSrc) != E_SUCCESS) {
        MediaAssetManagerImpl::mediaLibraryManager_->CloseAsset(tmpSrcUri, srcFd);
        MEDIA_ERR_LOG("File get stat failed, %{public}d", errno);
        return E_FILE_OPER_FAIL;
    }
    int destFd = GetFdFromSandBoxUri(destUri);
    if (destFd < 0) {
        MEDIA_ERR_LOG("Get destination %{public}s fd error: %{public}d", destUri.c_str(), destFd);
        MediaAssetManagerImpl::mediaLibraryManager_->CloseAsset(tmpSrcUri, srcFd);
        return destFd;
    }
    if (sendfile(destFd, srcFd, nullptr, statSrc.st_size) == -1) {
        MediaAssetManagerImpl::mediaLibraryManager_->CloseAsset(tmpSrcUri, srcFd);
        close(destFd);
        MEDIA_ERR_LOG("Sendfile failed, %{public}d", errno);
        return E_FILE_OPER_FAIL;
    }
    MediaAssetManagerImpl::mediaLibraryManager_->CloseAsset(tmpSrcUri, srcFd);
    close(destFd);
    return E_SUCCESS;
}

int32_t MediaAssetManagerImpl::GetFdFromSandBoxUri(const std::string &sandBoxUri)
{
    AppFileService::ModuleFileUri::FileUri destUri(sandBoxUri);
    string destPath = destUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        MEDIA_ERR_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_FILE_OPER_FAIL;
    }
    string absDestPath;
    if (!PathToRealPath(destPath, absDestPath)) {
        MEDIA_ERR_LOG("PathToRealPath failed, path:%{private}s", destPath.c_str());
        return E_FILE_OPER_FAIL;
    }
    return MediaFileUtils::OpenFile(absDestPath, MEDIA_FILEMODE_WRITETRUNCATE);
}

static bool HasReadPermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_READ_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

MediaLibrary_ErrorCode MediaAssetManagerImpl::NativeQuickRequestImage(OH_MediaAsset* mediaAsset,
    NativeRequestOptions requestOptions, MediaLibrary_RequestId* requestId,
    OH_MediaLibrary_OnQuickImageDataPrepared callback)
{
    MEDIA_INFO_LOG("MediaAssetManagerImpl::NativeQuickRequestImage Called");
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr && mediaAsset->mediaAsset_ != nullptr,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "mediaAsset or mediaAsset_ is null");
    CHECK_AND_RETURN_RET_LOG(HasReadPermission(), MEDIA_LIBRARY_PERMISSION_DENIED, "permission denied.");
    std::shared_ptr<FileAsset> fileAsset_ = mediaAsset->mediaAsset_->GetFileAssetInstance();
    MediaLibraryTracer tracer;
    tracer.Start("NativeQuickRequestImage");

    std::unique_ptr<RequestSourceAsyncContext> asyncContext = std::make_unique<RequestSourceAsyncContext>();
    asyncContext->requestUri = fileAsset_->GetUri();
    asyncContext->displayName = fileAsset_->GetDisplayName();
    asyncContext->fileId = fileAsset_->GetId();
    asyncContext->requestOptions.deliveryMode = requestOptions.deliveryMode;
    asyncContext->requestOptions.sourceMode = NativeSourceMode::EDITED_MODE;
    asyncContext->returnDataType = ReturnDataType::TYPE_PICTURE;
    asyncContext->needsExtraInfo = true;
    asyncContext->onRequestQuickImageDataPreparedHandler = callback;

    if (sDataShareHelper_ == nullptr) {
        CreateDataHelper(STORAGE_MANAGER_MANAGER_ID);
        CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ == nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "sDataShareHelper_ is null");
    }

    if (asyncContext->requestUri.length() > MAX_URI_SIZE) {
        MEDIA_ERR_LOG("Request image uri lens out of limit requestUri lens: %{public}zu",
            asyncContext->requestUri.length());
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED;
    }

    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_IMAGE) {
        MEDIA_ERR_LOG("Request image file type invalid");
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED;
    }

    bool isSuccess = false;
    asyncContext->requestId = GenerateRequestId();
    isSuccess = OnHandleRequestImage(asyncContext);
    if (isSuccess) {
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (asyncContext->requestId.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OK;
    } else {
        strncpy_s(requestId->requestId, UUID_STR_LENGTH, (ERROR_REQUEST_ID.c_str()), UUID_STR_LENGTH);
        return MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED;
    }
}
} // namespace Media
} // namespace OHOS