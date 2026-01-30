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

#include "media_asset_manager_ffi.h"

#include <fcntl.h>
#include <string>
#include <sys/sendfile.h>
#include <unordered_map>
#include <uuid.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "directory_ex.h"
#include "file_uri.h"
#include "image_source.h"
#include "image_source_impl.h"
#include "ipc_skeleton.h"
#include "media_call_transcode.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "moving_photo_impl.h"
#include "permission_utils.h"
#include "picture_handle_client.h"
#include "ui_extension_context.h"
#include "userfile_client.h"

using namespace std;
using namespace OHOS::FFI;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
const int32_t LOW_QUALITY_IMAGE = 1;
const int32_t HIGH_QUALITY_IMAGE = 0;
const int32_t UUID_STR_LENGTH = 37;
const int32_t MAX_URI_SIZE = 384; // 256 for display name and 128 for relative path
const int32_t REQUEST_ID_MAX_LEN = 64;

static mutex multiStagesCaptureLock;
static mutex registerTaskLock;
static map<string, shared_ptr<MultiStagesTaskObserver>> multiStagesObserverMap;
static std::map<std::string, std::map<std::string, AssetHandler*>> inProcessUriMap;
static SafeMap<string, AssetHandler*> inProcessFastRequests;
static SafeMap<std::string, AssetHandler*> onPreparedResult_;
static SafeMap<std::string, HashMapArray> onPreparedResultValue_;
static SafeMap<std::string, bool> isTranscoderMap_;
static const string HIGH_QUALITY_STRING = "high";
static const string LOW_QUALITY_STRING = "low";

bool ParseArgGetPhotoAsset(int64_t photoAssetId, int &fileId, string &uri,
    string &displayName, PhotoSubType &subType)
{
    auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(photoAssetId);
    if (photoAssetImpl == nullptr) {
        LOGE("Invalid object PhotoAssetImpl");
        return false;
    }
    fileId = photoAssetImpl->GetFileId();
    uri = photoAssetImpl->GetFileUri();
    displayName = photoAssetImpl->GetFileDisplayName();
    std::shared_ptr<FileAsset> fileAsset = photoAssetImpl->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        LOGE("Invalid object FileAsset");
        return false;
    }
    subType = static_cast<PhotoSubType>(fileAsset->GetPhotoSubType());
    return true;
}

bool ParseArgGetRequestOption(RequestOptions &requestOptions, DeliveryMode &deliveryMode, SourceMode &sourceMode)
{
    if (requestOptions.deliveryMode < static_cast<int32_t>(DeliveryMode::FAST) ||
        requestOptions.deliveryMode > static_cast<int32_t>(DeliveryMode::BALANCED_MODE)) {
        LOGE("delivery mode invalid argument.");
        return false;
    }
    deliveryMode = static_cast<DeliveryMode>(requestOptions.deliveryMode);
    sourceMode = SourceMode::EDITED_MODE; // public API just support deliveryMode.
    return true;
}

static bool HasReadPermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_READ_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

static bool IsMovingPhoto(int32_t photoSubType)
{
    return photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
}

bool MediaAssetManagerImpl::ParseRequestMediaArgs(int64_t photoAssetId,
    RequestOptions &requestOptions, unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    if (!ParseArgGetPhotoAsset(photoAssetId, asyncContext->fileId, asyncContext->photoUri,
        asyncContext->displayName, asyncContext->subType)) {
        LOGE("requestMedia ParseArgGetPhotoAsset error");
        return false;
    }
    if (!ParseArgGetRequestOption(requestOptions, asyncContext->deliveryMode,
        asyncContext->sourceMode)) {
        LOGE("requestMedia ParseArgGetRequestOption error");
        return false;
    }
    asyncContext->hasReadPermission = HasReadPermission();
    return true;
}

static string GenerateRequestId()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

MultiStagesCapturePhotoStatus MediaAssetManagerImpl::QueryPhotoStatus(int fileId,
    const string& photoUri, string &photoId, bool hasReadPermission)
{
    photoId = "";
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::vector<std::string> fetchColumn { PhotoColumn::PHOTO_QUALITY, PhotoColumn::PHOTO_ID};
    string queryUri;
    if (hasReadPermission) {
        queryUri = CONST_PAH_QUERY_PHOTO;
    } else {
        queryUri = photoUri;
        MediaFileUri::RemoveAllFragment(queryUri);
    }
    Uri uri(queryUri);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        LOGE("query resultSet is nullptr");
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    }
    int indexOfPhotoId = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_ID, indexOfPhotoId);
    resultSet->GetString(indexOfPhotoId, photoId);

    int columnIndexQuality = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_QUALITY, columnIndexQuality);
    int currentPhotoQuality = HIGH_QUALITY_IMAGE;
    resultSet->GetInt(columnIndexQuality, currentPhotoQuality);
    if (currentPhotoQuality == LOW_QUALITY_IMAGE) {
        LOGE("query photo status : lowQuality");
        return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
    }
    LOGE("query photo status quality: %{public}d", currentPhotoQuality);
    return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
}

static void InsertInProcessMapRecord(const string &requestUri, const string &requestId,
    AssetHandler *handler)
{
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
}

static AssetHandler* InsertDataHandler(NotifyMode notifyMode,
    unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    int64_t dataHandlerRef = asyncContext->dataHandler;
    AssetHandler *assetHandler = new AssetHandler(asyncContext->photoId, asyncContext->requestId,
        asyncContext->photoUri, dataHandlerRef, asyncContext->returnDataType);
    if (assetHandler == nullptr) {
        LOGE("assetHandler is nullptr");
        return nullptr;
    }
    
    assetHandler->photoQuality = asyncContext->photoQuality;
    assetHandler->needsExtraInfo = asyncContext->needsExtraInfo;
    assetHandler->notifyMode = notifyMode;
    assetHandler->sourceMode = asyncContext->sourceMode;
    assetHandler->compatibleMode = asyncContext->compatibleMode;
    assetHandler->destUri = asyncContext->destUri;
    switch (notifyMode) {
        case NotifyMode::FAST_NOTIFY: {
            inProcessFastRequests.EnsureInsert(asyncContext->requestId, assetHandler);
            break;
        }
        case NotifyMode::WAIT_FOR_HIGH_QUALITY: {
            InsertInProcessMapRecord(asyncContext->photoUri, asyncContext->requestId, assetHandler);
            break;
        }
        default:
            break;
    }
    return assetHandler;
}

void MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    AssetHandler *assetHandler = InsertDataHandler(NotifyMode::FAST_NOTIFY, asyncContext);
    if (assetHandler == nullptr) {
        LOGE("assetHandler is nullptr");
        return;
    }
    asyncContext->assetHandler = assetHandler;
}

void MediaAssetManagerImpl::ProcessImage(const int fileId, const int deliveryMode)
{
    string uriStr = CONST_PAH_PROCESS_IMAGE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    vector<string> columns { to_string(fileId), to_string(deliveryMode) };
    UserFileClient::Query(uri, predicates, columns, errCode);
}

void MediaAssetManagerImpl::RegisterTaskObserver(unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    auto dataObserver = make_shared<MultiStagesTaskObserver>(asyncContext->fileId);
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(asyncContext->photoUri);
    Uri uri(asyncContext->photoUri);
    std::unique_lock<std::mutex> registerLock(registerTaskLock);
    if (multiStagesObserverMap.find(uriLocal) == multiStagesObserverMap.end()) {
        UserFileClient::RegisterObserverExt(Uri(uriLocal),
            static_cast<shared_ptr<DataShare::DataShareObserver>>(dataObserver), false);
        multiStagesObserverMap.insert(make_pair(uriLocal, dataObserver));
    }
    registerLock.unlock();
    InsertDataHandler(NotifyMode::WAIT_FOR_HIGH_QUALITY, asyncContext);
    MediaAssetManagerImpl::ProcessImage(asyncContext->fileId, static_cast<int32_t>(asyncContext->deliveryMode));
}

void MediaAssetManagerImpl::OnHandleRequestImage(unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    MultiStagesCapturePhotoStatus status = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    switch (asyncContext->deliveryMode) {
        case DeliveryMode::FAST:
            if (asyncContext->needsExtraInfo) {
                asyncContext->photoQuality = MediaAssetManagerImpl::QueryPhotoStatus(asyncContext->fileId,
                    asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission);
            }
            MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        case DeliveryMode::HIGH_QUALITY:
            status = MediaAssetManagerImpl::QueryPhotoStatus(asyncContext->fileId,
                asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission);
            asyncContext->photoQuality = status;
            if (status == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
                MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(asyncContext);
            } else {
                RegisterTaskObserver(asyncContext);
            }
            break;
        case DeliveryMode::BALANCED_MODE:
            status = MediaAssetManagerImpl::QueryPhotoStatus(asyncContext->fileId,
                asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission);
            asyncContext->photoQuality = status;
            MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(asyncContext);
            if (status == MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
                RegisterTaskObserver(asyncContext);
            }
            break;
        default:
            LOGE("invalid delivery mode");
            return;
    }
}

static void DeleteAssetHandlerSafe(AssetHandler *handler)
{
    if (handler != nullptr) {
        delete handler;
        handler = nullptr;
    }
}

static string PhotoQualityToString(MultiStagesCapturePhotoStatus photoQuality)
{
    if (photoQuality != MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS &&
        photoQuality != MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
        LOGE("Invalid photo quality: %{public}d", static_cast<int>(photoQuality));
        return HIGH_QUALITY_STRING;
    }

    return (photoQuality == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) ? HIGH_QUALITY_STRING :
        LOW_QUALITY_STRING;
}

static void GetInfoMapValue(AssetHandler* assetHandler, HashMapArray &valueOfInfoMap)
{
    int64_t mapValueLength = 1; // only support quality
    KeyValue* head = static_cast<KeyValue*>(malloc(sizeof(KeyValue) * mapValueLength));
    if (head != nullptr) {
        string quality = "quality";
        string qualityInfo = PhotoQualityToString(assetHandler->photoQuality);
        for (int64_t i = 0; i < mapValueLength; i++) {
            head[i].key = MallocCString(quality);
            head[i].value = MallocCString(qualityInfo);
        }
        valueOfInfoMap.head = head;
        valueOfInfoMap.size = mapValueLength;
    } else {
        LOGE("malloc KeyValue failed.");
    }
}

static void OnDataPrepared(MediaObject &mediaObject, AssetHandler* assetHandler, HashMapArray &valueOfInfoMap)
{
    if (mediaObject.returnDataType == ReturnDataType::TYPE_ARRAY_BUFFER) {
        if (mediaObject.imageData.head == nullptr) {
            LOGE("ArrayBuffer is null.");
            return;
        }
        auto func = reinterpret_cast<void(*)(CArrUI8, HashMapArray)>(assetHandler->dataHandler);
        auto callbackRef = CJLambda::Create(func);
        if (callbackRef == nullptr) {
            LOGE("OnDataPrepared create callbackRef of ArrayBuffer failed.");
            return;
        }
        callbackRef(mediaObject.imageData, valueOfInfoMap);
    } else if (mediaObject.returnDataType == ReturnDataType::TYPE_IMAGE_SOURCE) {
        if (mediaObject.imageId == -1) {
            LOGE("get ImageSource failed.");
            return;
        }
        auto func = reinterpret_cast<void(*)(int64_t, HashMapArray)>(assetHandler->dataHandler);
        auto callbackRef = CJLambda::Create(func);
        if (callbackRef == nullptr) {
            LOGE("OnDataPrepared create callbackRef of ImageSource failed.");
            return;
        }
        callbackRef(mediaObject.imageId, valueOfInfoMap);
    } else if (mediaObject.returnDataType == ReturnDataType::TYPE_TARGET_PATH) {
        auto func = reinterpret_cast<void(*)(bool, HashMapArray)>(assetHandler->dataHandler);
        auto callbackRef = CJLambda::Create(func);
        if (callbackRef == nullptr) {
            LOGE("OnDataPrepared create callbackRef of videoFile failed.");
            return;
        }
        callbackRef(mediaObject.videoFile, valueOfInfoMap);
    } else if (mediaObject.returnDataType == ReturnDataType::TYPE_MOVING_PHOTO) {
        if (mediaObject.movingPhotoId == -1) {
            LOGE("get MovingPhoto failed.");
            return;
        }
        auto func = reinterpret_cast<void(*)(int64_t, HashMapArray)>(assetHandler->dataHandler);
        auto callbackRef = CJLambda::Create(func);
        if (callbackRef == nullptr) {
            LOGE("OnDataPrepared create callbackRef of MovingPhoto failed.");
            return;
        }
        callbackRef(mediaObject.movingPhotoId, valueOfInfoMap);
    } else {
        LOGE("source mode type invalid");
    }
}

bool IsSaveCallbackInfoByTranscoder(MediaObject &mediaObject,
    AssetHandler* assetHandler, HashMapArray &valueOfInfoMap)
{
    int64_t dataHandler = assetHandler->dataHandler;
    if (dataHandler == -1) {
        LOGE("data handler is error");
        DeleteAssetHandlerSafe(assetHandler);
        return false;
    }
    bool isTranscoder;
    if (!isTranscoderMap_.Find(assetHandler->requestId, isTranscoder)) {
        LOGI("not find key from map");
        isTranscoder = false;
    }
    if (isTranscoder) {
        onPreparedResult_.EnsureInsert(assetHandler->requestId, assetHandler);
        onPreparedResultValue_.EnsureInsert(assetHandler->requestId, valueOfInfoMap);
        return true;
    }
    OnDataPrepared(mediaObject, assetHandler, valueOfInfoMap);
    return false;
}

static void SavePicture(std::string &fileUri)
{
    std::string uriStr = CONST_PATH_SAVE_PICTURE;
    std::string tempStr = fileUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length());
    std::size_t index = tempStr.find("/");
    std::string fileId = tempStr.substr(0, index);
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_ID, fileId);
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, CONST_IMAGE_FILE_TYPE, "1");
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, "uri", fileUri);
    Uri uri(uriStr);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    DataShare::DataSharePredicates predicate;
    UserFileClient::Update(uri, predicate, valuesBucket);
}

void MediaAssetManagerImpl::GetByteArrayObject(const string &requestUri,
    MediaObject &mediaObject, bool isSource)
{
    std::string tmpUri = requestUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, CONST_MEDIA_OPERN_KEYWORD, CONST_SOURCE_REQUEST);
    }
    Uri uri(tmpUri);
    int imageFd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    if (imageFd < 0) {
        LOGE("get image fd failed, %{public}d", errno);
        return;
    }
    ssize_t imgLen = lseek(imageFd, 0, SEEK_END);
    if (imgLen <= 0) {
        LOGE("imgLen is error");
        close(imageFd);
        return;
    }
    void* buffer = malloc(imgLen);
    if (buffer == nullptr) {
        LOGE("malloc buffer failed");
        close(imageFd);
        return;
    }
    lseek(imageFd, 0, SEEK_SET);
    ssize_t readRet = read(imageFd, buffer, imgLen);
    close(imageFd);
    if (readRet != imgLen) {
        LOGE("read image failed");
        free(buffer);
        return;
    }
    mediaObject.imageData.head = static_cast<uint8_t*>(buffer);
    mediaObject.imageData.size = static_cast<int64_t>(readRet);
}

void MediaAssetManagerImpl::GetImageSourceObject(const std::string &fileUri,
    MediaObject &mediaObject, bool isSource)
{
    std::string tmpUri = fileUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, CONST_MEDIA_OPERN_KEYWORD, CONST_SOURCE_REQUEST);
        LOGI("request source image's imageSource");
    }
    Uri uri(tmpUri);
    int fd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    if (fd < 0) {
        LOGE("get image fd failed, errno: %{public}d", errno);
        return;
    }
    SourceOptions opts;
    uint32_t errCode = 0;
    auto nativeImageSourcePtr = ImageSource::CreateImageSource(fd, opts, errCode);
    close(fd);
    if (nativeImageSourcePtr == nullptr) {
        LOGE("get ImageSource::CreateImageSource failed nullptr");
        return;
    }
    auto nativeImageSource = FFIData::Create<ImageSourceImpl>(move(nativeImageSourcePtr));
    if (!nativeImageSource) {
        LOGE("get ImageSourceImpl::Create failed");
        return;
    }
    mediaObject.imageId = nativeImageSource->GetID();
}

int32_t MediaAssetManagerImpl::GetFdFromSandBoxUri(const std::string &sandBoxUri)
{
    AppFileService::ModuleFileUri::FileUri destUri(sandBoxUri);
    string destPath = destUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        LOGE("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    string absDestPath;
    if (!PathToRealPath(destPath, absDestPath)) {
        LOGE("PathToRealPath failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    return MediaFileUtils::OpenFile(absDestPath, MEDIA_FILEMODE_WRITETRUNCATE);
}

void MediaAssetManagerImpl::WriteDataToDestPath(WriteData &writeData, MediaObject &mediaObject, std::string requestId)
{
    if (writeData.requestUri.empty() || writeData.destUri.empty()) {
        mediaObject.videoFile = false;
        LOGE("requestUri or responseUri is nullptr");
        return;
    }
    std::string tmpUri = writeData.requestUri;
    if (writeData.isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, CONST_MEDIA_OPERN_KEYWORD, CONST_SOURCE_REQUEST);
    }
    Uri srcUri(tmpUri);
    int srcFd = UserFileClient::OpenFile(srcUri, MEDIA_FILEMODE_READONLY);
    if (srcFd < 0) {
        mediaObject.videoFile = false;
        LOGE("get source file fd failed %{public}d", srcFd);
        return;
    }
    struct stat statSrc;
    if (fstat(srcFd, &statSrc) == -1) {
        close(srcFd);
        mediaObject.videoFile = false;
        LOGE("File get stat failed, %{public}d", errno);
        return;
    }
    int32_t destFd = GetFdFromSandBoxUri(writeData.destUri);
    if (destFd < 0) {
        close(srcFd);
        mediaObject.videoFile = false;
        LOGE("get dest fd failed %{public}d", destFd);
        return;
    }
    if (writeData.compatibleMode == CompatibleMode::ORIGINAL_FORMAT_MODE) {
        SendFile(mediaObject, writeData, srcFd, destFd, statSrc.st_size);
    }
    close(srcFd);
    close(destFd);
}

void MediaAssetManagerImpl::SendFile(MediaObject &mediaObject, WriteData &writeData,
    int srcFd, int destFd, off_t fileSize)
{
    if (srcFd < 0 || destFd < 0) {
        LOGE("srcFd or destFd is invalid");
        mediaObject.videoFile = false;
        return;
    }
    if (sendfile(destFd, srcFd, nullptr, fileSize) == -1) {
        mediaObject.videoFile = false;
        LOGE("send file failed, %{public}d", errno);
        return;
    }
    mediaObject.videoFile = true;
}

void MediaAssetManagerImpl::GetMovingPhotoObject(const string &requestUri,
    SourceMode sourceMode, MediaObject &mediaObject)
{
    auto nativeMovingPhoto = FFIData::Create<FfiMovingPhotoImpl>(requestUri, sourceMode);
    if (!nativeMovingPhoto) {
        LOGE("get nativeMovingPhoto failed");
        return;
    }
    mediaObject.movingPhotoId = nativeMovingPhoto->GetID();
}

static void GetValueOfMedia(AssetHandler *assetHandler, MediaObject &mediaObject)
{
    mediaObject.returnDataType = assetHandler->returnDataType;
    if (assetHandler->returnDataType == ReturnDataType::TYPE_ARRAY_BUFFER) {
        MediaAssetManagerImpl::GetByteArrayObject(assetHandler->requestUri, mediaObject,
            assetHandler->sourceMode == SourceMode::ORIGINAL_MODE);
    } else if (assetHandler->returnDataType == ReturnDataType::TYPE_IMAGE_SOURCE) {
        MediaAssetManagerImpl::GetImageSourceObject(assetHandler->requestUri, mediaObject,
            assetHandler->sourceMode == SourceMode::ORIGINAL_MODE);
    } else if (assetHandler->returnDataType == ReturnDataType::TYPE_TARGET_PATH) {
        WriteData param;
        param.compatibleMode = assetHandler->compatibleMode;
        param.destUri = assetHandler->destUri;
        param.requestUri = assetHandler->requestUri;
        param.isSource = assetHandler->sourceMode == SourceMode::ORIGINAL_MODE;
        MediaAssetManagerImpl::WriteDataToDestPath(param, mediaObject, assetHandler->requestId);
    } else if (assetHandler->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO) {
        MediaAssetManagerImpl::GetMovingPhotoObject(assetHandler->requestUri, assetHandler->sourceMode, mediaObject);
    } else {
        LOGE("source mode type invalid");
    }
}

static void DeleteInProcessMapRecord(const string &requestUri, const string &requestId)
{
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
        UserFileClient::UnregisterObserverExt(Uri(uriLocal),
            static_cast<std::shared_ptr<DataShare::DataShareObserver>>(multiStagesObserverMap[uriLocal]));
    }
    multiStagesObserverMap.erase(uriLocal);
}

static void DeleteDataHandler(NotifyMode notifyMode, const string &requestUri, const string &requestId)
{
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(requestUri);
    LOGI("Rmv %{public}d, %{public}s, %{public}s", notifyMode, requestUri.c_str(), requestId.c_str());
    if (notifyMode == NotifyMode::WAIT_FOR_HIGH_QUALITY) {
        DeleteInProcessMapRecord(uriLocal, requestId);
    }
    inProcessFastRequests.Erase(requestId);
}

void FreeArrAndMap(CArrUI8 &arr, HashMapArray &map)
{
    if (arr.head != nullptr) {
        free(arr.head);
        arr.size = 0;
    }
    if (map.head != nullptr) {
        for (int64_t i = 0; i < map.size; i++) {
            free(map.head[i].key);
            free(map.head[i].value);
        }
        free(map.head);
        map.size = 0;
    }
}

void MediaAssetManagerImpl::NotifyMediaDataPrepared(AssetHandler *assetHandler)
{
    if (assetHandler->dataHandler == -1) {
        LOGE("data handler is error");
        DeleteAssetHandlerSafe(assetHandler);
        return;
    }
    if (assetHandler->notifyMode == NotifyMode::FAST_NOTIFY) {
        AssetHandler *tmp;
        if (!inProcessFastRequests.Find(assetHandler->requestId, tmp)) {
            LOGE("The request has been canceled");
            DeleteAssetHandlerSafe(assetHandler);
            return;
        }
    }
    HashMapArray valueOfInfoMap = { .head = nullptr, .size = 0};
    if (assetHandler->needsExtraInfo) {
        GetInfoMapValue(assetHandler, valueOfInfoMap);
        if (valueOfInfoMap.head == nullptr || valueOfInfoMap.size == 0) {
            LOGE("Failed to get info map");
        }
    }
    if (assetHandler->returnDataType == ReturnDataType::TYPE_ARRAY_BUFFER ||
        assetHandler->returnDataType == ReturnDataType::TYPE_IMAGE_SOURCE) {
        string uri = assetHandler->requestUri;
        SavePicture(uri);
    }
    MediaObject mediaObject;
    GetValueOfMedia(assetHandler, mediaObject);
    if (IsSaveCallbackInfoByTranscoder(mediaObject, assetHandler, valueOfInfoMap)) {
        FreeArrAndMap(mediaObject.imageData, valueOfInfoMap);
        return;
    }
    FreeArrAndMap(mediaObject.imageData, valueOfInfoMap);
    if (assetHandler != nullptr) {
        DeleteDataHandler(assetHandler->notifyMode, assetHandler->requestUri, assetHandler->requestId);
    }
    DeleteAssetHandlerSafe(assetHandler);
}

void MultiStagesTaskObserver::OnChange(const ChangeInfo &changeInfo)
{
    if (changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_UPDATE)) {
        LOGE("ignore notify change, type: %{public}d", changeInfo.changeType_);
        return;
    }
    for (auto &uri : changeInfo.uris_) {
        string uriString = uri.ToString();
        string photoId = "";
        if (MediaAssetManagerImpl::QueryPhotoStatus(fileId_, uriString, photoId, true) !=
            MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
            LOGE("requested data not prepared");
            continue;
        }

        std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
        if (inProcessUriMap.find(uriString) == inProcessUriMap.end()) {
            LOGI("current uri does not in process, uri: %{public}s", uriString.c_str());
            return;
        }
        std::map<std::string, AssetHandler *> assetHandlers = inProcessUriMap[uriString];
        for (auto handler : assetHandlers) {
            auto assetHandler = handler.second;
            if (assetHandler == nullptr) {
                continue;
            }
            assetHandler->photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
            MediaAssetManagerImpl::NotifyMediaDataPrepared(assetHandler);
        }
    }
}

char* MediaAssetManagerImpl::RequestImage(int64_t contextId, int64_t photoAssetId,
    RequestOptions requestOptions, int64_t funcId, int32_t &errCode)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (context == nullptr) {
        LOGE("get context failed.");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    sptr<IRemoteObject> token = context->GetToken();
    if (!PhotoAccessHelperImpl::CheckWhetherInitSuccess(token)) {
        LOGE("RequestImage init user file client failed");
        errCode = JS_INNER_FAIL;
        return nullptr;
    }
    unique_ptr<MediaAssetManagerContext> asyncContext = make_unique<MediaAssetManagerContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_IMAGE_SOURCE;
    if (!ParseRequestMediaArgs(photoAssetId, requestOptions, asyncContext)) {
        LOGE("failed to parse requestImage args");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    if (funcId == -1) {
        LOGE("requestMedia ParseArgGetDataHandler error");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    asyncContext->dataHandler = funcId;
    asyncContext->requestId = GenerateRequestId();
    OnHandleRequestImage(asyncContext);
    if (asyncContext->subType == PhotoSubType::MOVING_PHOTO) {
        string uri = CONST_LOG_MOVING_PHOTO;
        Uri logMovingPhotoUri(uri);
        DataShare::DataShareValuesBucket valuesBucket;
        string result;
        valuesBucket.Put("adapted", asyncContext->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO);
        UserFileClient::InsertExt(logMovingPhotoUri, valuesBucket, result);
    }
    if (asyncContext->assetHandler) {
        NotifyMediaDataPrepared(asyncContext->assetHandler);
        asyncContext->assetHandler = nullptr;
    }
    if (errCode == ERR_DEFAULT) {
        char* result = MallocCString(asyncContext->requestId);
        return result;
    } else {
        return nullptr;
    }
}

char* MediaAssetManagerImpl::RequestImageData(int64_t contextId, int64_t photoAssetId,
    RequestOptions requestOptions, int64_t funcId, int32_t &errCode)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (context == nullptr) {
        LOGE("get context failed.");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    sptr<IRemoteObject> token = context->GetToken();
    if (!PhotoAccessHelperImpl::CheckWhetherInitSuccess(token)) {
        LOGE("RequestImage init user file client failed");
        errCode = JS_INNER_FAIL;
        return nullptr;
    }
    unique_ptr<MediaAssetManagerContext> asyncContext = make_unique<MediaAssetManagerContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_ARRAY_BUFFER;
    if (!ParseRequestMediaArgs(photoAssetId, requestOptions, asyncContext)) {
        LOGE("failed to parse requestImage args");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    if (funcId == -1) {
        LOGE("requestMedia ParseArgGetDataHandler error");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    asyncContext->dataHandler = funcId;
    asyncContext->requestId = GenerateRequestId();
    MediaAssetManagerImpl::OnHandleRequestImage(asyncContext);
    if (asyncContext->subType == PhotoSubType::MOVING_PHOTO) {
        string uri = CONST_LOG_MOVING_PHOTO;
        Uri logMovingPhotoUri(uri);
        DataShare::DataShareValuesBucket valuesBucket;
        string result;
        valuesBucket.Put("adapted", asyncContext->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO);
        UserFileClient::InsertExt(logMovingPhotoUri, valuesBucket, result);
    }
    if (asyncContext->assetHandler) {
        NotifyMediaDataPrepared(asyncContext->assetHandler);
        asyncContext->assetHandler = nullptr;
    }
    if (errCode == ERR_DEFAULT) {
        char* result = MallocCString(asyncContext->requestId);
        return result;
    } else {
        return nullptr;
    }
}

char* MediaAssetManagerImpl::RequestMovingPhoto(int64_t contextId, int64_t photoAssetId,
    RequestOptions requestOptions, int64_t funcId, int32_t &errCode)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (context == nullptr) {
        LOGE("get context failed.");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    sptr<IRemoteObject> token = context->GetToken();
    if (!PhotoAccessHelperImpl::CheckWhetherInitSuccess(token)) {
        LOGE("RequestImage init user file client failed");
        errCode = JS_INNER_FAIL;
        return nullptr;
    }
    unique_ptr<MediaAssetManagerContext> asyncContext = make_unique<MediaAssetManagerContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_MOVING_PHOTO;
    if (!ParseRequestMediaArgs(photoAssetId, requestOptions, asyncContext)) {
        LOGE("failed to parse requestImage args");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    if (!IsMovingPhoto(static_cast<int32_t>(asyncContext->subType))) {
        LOGE("Asset is not a moving photo");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    asyncContext->dataHandler = funcId;
    asyncContext->requestId = GenerateRequestId();
    OnHandleRequestImage(asyncContext);
    string uri = CONST_LOG_MOVING_PHOTO;
    Uri logMovingPhotoUri(uri);
    DataShare::DataShareValuesBucket valuesBucket;
    string result;
    valuesBucket.Put("adapted", asyncContext->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO);
    UserFileClient::InsertExt(logMovingPhotoUri, valuesBucket, result);
    if (asyncContext->assetHandler) {
        NotifyMediaDataPrepared(asyncContext->assetHandler);
        asyncContext->assetHandler = nullptr;
    }
    if (errCode == ERR_DEFAULT) {
        char* result = MallocCString(asyncContext->requestId);
        return result;
    } else {
        return nullptr;
    }
}

static bool ParseArgGetDestPath(char* fileUri, unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    string destPath(fileUri);
    if (destPath.empty()) {
        LOGE("failed to get destPath.");
        return false;
    }
    asyncContext->destUri = destPath;
    if (asyncContext->photoUri.length() > MAX_URI_SIZE || asyncContext->destUri.length() > MAX_URI_SIZE) {
        LOGE("request video file uri lens out of limit photoUri lens: %{public}zu, destUri lens: %{public}zu",
            asyncContext->photoUri.length(), asyncContext->destUri.length());
        return false;
    }
    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_VIDEO ||
        MediaFileUtils::GetMediaType(MediaFileUtils::GetFileName(asyncContext->destUri)) != MEDIA_TYPE_VIDEO) {
        LOGE("request video file type invalid");
        return false;
    }
    return true;
}

void MediaAssetManagerImpl::OnHandleRequestVideo(unique_ptr<MediaAssetManagerContext> &asyncContext)
{
    switch (asyncContext->deliveryMode) {
        case DeliveryMode::FAST:
            MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        case DeliveryMode::HIGH_QUALITY:
            MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        case DeliveryMode::BALANCED_MODE:
            MediaAssetManagerImpl::NotifyDataPreparedWithoutRegister(asyncContext);
            break;
        default: {
            LOGE("invalid delivery mode");
            return;
        }
    }
}

char* MediaAssetManagerImpl::RequestVideoFile(int64_t contextId, int64_t photoAssetId,
    RequestOptions requestOptions, char* fileUri, int64_t funcId, int32_t &errCode)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (context == nullptr) {
        LOGE("get context failed.");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    sptr<IRemoteObject> token = context->GetToken();
    if (!PhotoAccessHelperImpl::CheckWhetherInitSuccess(token)) {
        LOGE("RequestImage init user file client failed");
        errCode = JS_INNER_FAIL;
        return nullptr;
    }
    unique_ptr<MediaAssetManagerContext> asyncContext = make_unique<MediaAssetManagerContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_TARGET_PATH;
    if (!ParseRequestMediaArgs(photoAssetId, requestOptions, asyncContext)) {
        LOGE("failed to parse requestVideo args");
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    if (!ParseArgGetDestPath(fileUri, asyncContext)) {
        errCode = OHOS_INVALID_PARAM_CODE;
        return nullptr;
    }
    asyncContext->dataHandler = funcId;
    asyncContext->requestId = GenerateRequestId();
    OnHandleRequestVideo(asyncContext);
    if (asyncContext->assetHandler) {
        NotifyMediaDataPrepared(asyncContext->assetHandler);
        asyncContext->assetHandler = nullptr;
    }
    if (errCode == ERR_DEFAULT) {
        char* result = MallocCString(asyncContext->requestId);
        return result;
    } else {
        return nullptr;
    }
}

static bool IsFastRequestCanceled(const std::string &requestId, std::string &photoId)
{
    AssetHandler *assetHandler = nullptr;
    if (!inProcessFastRequests.Find(requestId, assetHandler)) {
        LOGE("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        LOGE("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    inProcessFastRequests.Erase(requestId);
    return true;
}

static int32_t IsInProcessInMapRecord(const string &requestId, AssetHandler* &handler)
{
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    for (auto record : inProcessUriMap) {
        if (record.second.find(requestId) != record.second.end()) {
            handler = record.second[requestId];
            return true;
        }
    }
    return false;
}

static bool IsMapRecordCanceled(const std::string &requestId, std::string &photoId)
{
    AssetHandler *assetHandler = nullptr;
    if (!IsInProcessInMapRecord(requestId, assetHandler)) {
        LOGE("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        LOGE("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    DeleteInProcessMapRecord(assetHandler->requestUri, assetHandler->requestId);
    DeleteAssetHandlerSafe(assetHandler);
    return true;
}

void MediaAssetManagerImpl::CancelProcessImage(const string &photoId)
{
    string uriStr = CONST_PAH_CANCEL_PROCESS_IMAGE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    std::vector<std::string> columns { photoId };
    UserFileClient::Query(uri, predicates, columns, errCode);
}

void MediaAssetManagerImpl::CancelRequest(int64_t contextId, char* cRequestId, int32_t &errCode)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (context == nullptr) {
        LOGE("get context failed.");
        errCode = OHOS_INVALID_PARAM_CODE;
        return;
    }
    sptr<IRemoteObject> token = context->GetToken();
    if (!PhotoAccessHelperImpl::CheckWhetherInitSuccess(token)) {
        LOGE("RequestImage init user file client failed");
        errCode = JS_INNER_FAIL;
        return;
    }
    string requestId(cRequestId);
    if (requestId.size() > REQUEST_ID_MAX_LEN) {
        requestId = requestId.substr(0, REQUEST_ID_MAX_LEN);
    }
    string photoId = "";
    bool hasFastRequestInProcess = IsFastRequestCanceled(requestId, photoId);
    bool hasMapRecordInProcess = IsMapRecordCanceled(requestId, photoId);
    if (hasFastRequestInProcess || hasMapRecordInProcess) {
        unique_ptr<MediaAssetManagerContext> asyncContext = make_unique<MediaAssetManagerContext>();
        asyncContext->photoId = photoId;
        MediaAssetManagerImpl::CancelProcessImage(asyncContext->photoId);
    }
}

int64_t MediaAssetManagerImpl::LoadMovingPhoto(int64_t contextId,
    char* cImageFileUri, char* cVideoFileUri, int32_t &errCode)
{
    if (contextId == -1) {
        LOGE("Get context failed.");
        errCode = OHOS_INVALID_PARAM_CODE;
        return -1;
    }
    string imageFileUri(cImageFileUri);
    string videoFileUri(cVideoFileUri);
    string uri(imageFileUri + MOVING_PHOTO_URI_SPLIT + videoFileUri);
    auto nativeMovingPhoto = FFIData::Create<FfiMovingPhotoImpl>(uri, SourceMode::EDITED_MODE);
    if (!nativeMovingPhoto) {
        LOGE("get nativeMovingPhoto failed");
        errCode = JS_INNER_FAIL;
        return -1;
    }
    return nativeMovingPhoto->GetID();
}
}
}