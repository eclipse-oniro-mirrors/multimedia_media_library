/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaAssetManagerNapi"

#include "media_asset_manager_napi.h"

#include <string>
#include <sys/sendfile.h>
#include <uuid/uuid.h>

#include "accesstoken_kit.h"
#include "adapted_vo.h"
#include "camera_character_types.h"
#include "directory_ex.h"
#include "file_uri.h"
#include "image_source_napi.h"
#include "log_cinematic_access_vo.h"
#include "media_file_uri.h"
#include "medialibrary_business_code.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_utils_ext.h"
#include "medialibrary_notify_utils.h"
#include "medialibrary_tracer.h"
#include "moving_photo_napi.h"
#include "moving_photo_call_transcoder.h"
#include "multistages_capture_on_process_observer.h"
#include "permission_utils.h"
#include "picture_handle_client.h"
#include "query_photo_vo.h"
#include "ui_extension_context.h"
#include "user_define_ipc_client.h"
#include "userfile_client.h"
#include "media_call_transcode.h"
#include "medialibrary_operation.h"
#include "media_asset_rdbstore.h"
#include "cancel_request_vo.h"
#include "get_progress_callback_vo.h"
#include "process_video_vo.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
static const std::string MEDIA_ASSET_MANAGER_CLASS = "MediaAssetManager";
static std::mutex multiStagesCaptureLock;
static std::map<std::string, std::map<std::string, AssetHandler*>> inProcessUriMap; // <uri, <requestId, handler>>
static std::mutex registerTaskLock;
static std::map<std::string,
    std::map<ObserverType, std::shared_ptr<MediaOnNotifyUserDefineObserver>>> multiStagesObserverNewMap;

const int32_t LOW_QUALITY_IMAGE = 1;
const int32_t HIGH_QUALITY_IMAGE = 0;

const int32_t UUID_STR_LENGTH = 37;
const int32_t MAX_URI_SIZE = 384; // 256 for display name and 128 for relative path
const int32_t REQUEST_ID_MAX_LEN = 64;
const int32_t PROGRESS_MAX = 100;

const std::string HIGH_TEMPERATURE = "high_temperature";
const std::string HIGH_TEMPERATURE_VIDEO = "high_temperature_video";

static const std::string URI_TYPE = "uriType";
static const std::string TYPE_PHOTOS = "1";

static const int32_t AVERAGE_FACTOR = 2;
static const int32_t SLEEP_100_MS = 100;

thread_local unique_ptr<ChangeListenerNapi> g_multiStagesRequestListObj = nullptr;
thread_local napi_ref constructor_ = nullptr;

static SafeMap<std::string, AssetHandler*> inProcessFastRequests;
static SafeMap<std::string, AssetHandler*> onPreparedResult_;
static SafeMap<std::string, napi_value> onPreparedResultValue_;
static SafeMap<std::string, bool> isTranscoderMap_;

std::unique_ptr<std::thread> MediaAssetManagerNapi::progressThread_ = nullptr;
std::mutex MediaAssetManagerNapi::runningMutex_;
std::mutex MediaAssetManagerNapi::sleepMutex_;
std::condition_variable MediaAssetManagerNapi::condition_;
std::atomic_bool MediaAssetManagerNapi::pauseFlag_ = true; // 暂停标识
std::atomic_bool MediaAssetManagerNapi::stopFlag_ = true; // 停止标识

static const std::map<MultistagesCaptureNotifyType, std::vector<ObserverType>> MATCH_NOTIFY_TO_OBSERVER = {
    { MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE, {ObserverType::REQUEST_IMAGE} },
    { MultistagesCaptureNotifyType::ON_ERROR_IMAGE, {ObserverType::REQUEST_IMAGE, ObserverType::REQUEST_QUICK_IMAGE} },
    { MultistagesCaptureNotifyType::ON_PROCESS_VIDEO_DONE, {ObserverType::REQUEST_VIDEO} },
    { MultistagesCaptureNotifyType::ON_ERROR_VIDEO, {ObserverType::REQUEST_VIDEO} },
    { MultistagesCaptureNotifyType::YUV_READY, {ObserverType::REQUEST_QUICK_IMAGE} },
};

napi_value MediaAssetManagerNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {.name = MEDIA_ASSET_MANAGER_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("requestImage", JSRequestImage),
            DECLARE_NAPI_STATIC_FUNCTION("requestImageData", JSRequestImageData),
            DECLARE_NAPI_STATIC_FUNCTION("requestMovingPhoto", JSRequestMovingPhoto),
            DECLARE_NAPI_STATIC_FUNCTION("cancelRequest", JSCancelRequest),
            DECLARE_NAPI_STATIC_FUNCTION("requestVideoFile", JSRequestVideoFile),
            DECLARE_NAPI_STATIC_FUNCTION("requestVideo", JSRequestVideo),
            DECLARE_NAPI_STATIC_FUNCTION("loadMovingPhoto", JSLoadMovingPhoto),
            DECLARE_NAPI_STATIC_FUNCTION("quickRequestImage", JSRequestEfficientIImage)
        }};
        MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
        return exports;
}

napi_value MediaAssetManagerNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    bool isConstructor = newTarget != nullptr;
    if (isConstructor) {
        napi_value thisVar = nullptr;
        unique_ptr<MediaAssetManagerNapi> obj = make_unique<MediaAssetManagerNapi>();
        CHECK_COND_WITH_MESSAGE(env, obj != nullptr, "Create MediaAssetManagerNapi failed");
        CHECK_ARGS(env,
            napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), MediaAssetManagerNapi::Destructor,
                nullptr, nullptr),
            JS_INNER_FAIL);
        obj.release();
        return thisVar;
    }
    napi_value constructor = nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, constructor_, &constructor));
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    return result;
}

void MediaAssetManagerNapi::Destructor(napi_env env, void *nativeObject, void *finalizeHint)
{
    auto* mediaAssetManager = reinterpret_cast<MediaAssetManagerNapi*>(nativeObject);
    if (mediaAssetManager != nullptr) {
        delete mediaAssetManager;
        mediaAssetManager = nullptr;
    }
}

static bool HasReadPermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_READ_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

static AssetHandler* CreateAssetHandler(const std::string &photoId, const std::string &requestId,
    const std::string &uri, const MediaAssetDataHandlerPtr &handler, napi_threadsafe_function func)
{
    AssetHandler *assetHandler = new AssetHandler(photoId, requestId, uri, handler, func);
    NAPI_DEBUG_LOG("[AssetHandler create] photoId: %{public}s, requestId: %{public}s, uri: %{public}s",
        photoId.c_str(), requestId.c_str(), uri.c_str());
    return assetHandler;
}

static void DeleteAssetHandlerSafe(AssetHandler *handler, napi_env env)
{
    if (handler != nullptr) {
        if (handler->dataHandler != nullptr) {
            handler->dataHandler->DeleteNapiReference(env);
        }
        if (handler->threadSafeFunc != nullptr) {
            napi_release_threadsafe_function(handler->threadSafeFunc, napi_tsfn_release);
            handler->threadSafeFunc = nullptr;
        }
        delete handler;
        handler = nullptr;
    }
}

static void DeleteProcessHandlerSafe(ProgressHandler *handler, napi_env env)
{
    if (handler == nullptr) {
        return;
    }
    if (handler->progressRef != nullptr && env != nullptr) {
        napi_delete_reference(env, handler->progressRef);
        handler->progressRef = nullptr;
    }
    if (handler->progressFunc != nullptr) {
        napi_release_threadsafe_function(handler->progressFunc, napi_tsfn_release);
        handler->progressFunc = nullptr;
    }
    delete handler;
    handler = nullptr;
}

static void InsertInProcessMapRecord(const std::string &requestUri, const std::string &requestId,
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

// Do not use directly
static void DeleteRecordNoLock(const std::string &requestUri, const std::string &requestId)
{
    NAPI_INFO_LOG("DeleteRecordNoLock, requestUri: %{public}s, requestId: %{public}s.",
        requestUri.c_str(), requestId.c_str());
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(requestUri);
    if (inProcessUriMap.find(uriLocal) == inProcessUriMap.end()) {
        return;
    }

    std::map<std::string, AssetHandler*> &assetHandlers = inProcessUriMap[uriLocal];
    if (assetHandlers.find(requestId) == assetHandlers.end()) {
        return;
    }
    AssetHandler* handler = assetHandlers[requestId];
    if (handler == nullptr) {
        return;
    }
    ObserverType observerType = handler->observerType;
    assetHandlers.erase(requestId);
    if (assetHandlers.empty()) {
        inProcessUriMap.erase(uriLocal);
    }

    if (multiStagesObserverNewMap.find(uriLocal) == multiStagesObserverNewMap.end()) {
        return;
    }
    auto &observerMap = multiStagesObserverNewMap[uriLocal];
    if (observerMap.find(observerType) != observerMap.end()) {
        UserFileClient::UnregisterObserverExtProvider(Uri(uriLocal),
            static_cast<std::shared_ptr<DataShare::DataShareObserver>>(observerMap[observerType]));
    }
    observerMap.erase(observerType);
    if (observerMap.empty()) {
        multiStagesObserverNewMap.erase(uriLocal);
    }
}

static void DeleteInProcessMapRecord(const std::string &requestUri, const std::string &requestId)
{
    DeleteRecordNoLock(requestUri, requestId);
}

static int32_t IsInProcessInMapRecord(const std::string &requestId, AssetHandler* &handler)
{
    for (auto record : inProcessUriMap) {
        if (record.second.find(requestId) != record.second.end()) {
            handler = record.second[requestId];
            return true;
        }
    }

    return false;
}

static AssetHandler* InsertDataHandler(NotifyMode notifyMode, napi_env env,
    MediaAssetManagerAsyncContext *asyncContext)
{
    napi_ref dataHandlerRef;
    napi_threadsafe_function threadSafeFunc;
    if (notifyMode == NotifyMode::FAST_NOTIFY) {
        dataHandlerRef = asyncContext->dataHandlerRef;
        asyncContext->dataHandlerRef = nullptr;
        threadSafeFunc = asyncContext->onDataPreparedPtr;
    } else {
        dataHandlerRef = asyncContext->dataHandlerRef2;
        asyncContext->dataHandlerRef2 = nullptr;
        threadSafeFunc = asyncContext->onDataPreparedPtr2;
    }
    std::shared_ptr<NapiMediaAssetDataHandler> mediaAssetDataHandler = make_shared<NapiMediaAssetDataHandler>(
        env, dataHandlerRef, asyncContext->returnDataType, asyncContext->photoUri, asyncContext->destUri,
        asyncContext->sourceMode);
    mediaAssetDataHandler->SetCompatibleMode(asyncContext->compatibleMode);
    mediaAssetDataHandler->SetNotifyMode(notifyMode);
    mediaAssetDataHandler->SetRequestId(asyncContext->requestId);
    mediaAssetDataHandler->SetProgressHandlerRef(asyncContext->progressHandlerRef);
    mediaAssetDataHandler->SetThreadsafeFunction(asyncContext->onProgressPtr);
    AssetHandler *assetHandler = CreateAssetHandler(asyncContext->photoId, asyncContext->requestId,
        asyncContext->photoUri, mediaAssetDataHandler, threadSafeFunc);
    assetHandler->photoQuality = asyncContext->photoQuality;
    assetHandler->needsExtraInfo = asyncContext->needsExtraInfo;
    assetHandler->observerType = asyncContext->observerType;
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} Add %{public}d, %{public}s, %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__,
        notifyMode, MediaFileUtils::DesensitizeUri(asyncContext->photoUri).c_str(), asyncContext->requestId.c_str());

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

static ProgressHandler* InsertProgressHandler(napi_env env, MediaAssetManagerAsyncContext *asyncContext,
    ProgressMode progressMode, CameraProgressMode cameraProgressMode)
{
    napi_ref dataHandlerRef;
    napi_threadsafe_function threadSafeFunc;
    dataHandlerRef = asyncContext->progressHandlerRef;
    asyncContext->progressHandlerRef = nullptr;
    threadSafeFunc = asyncContext->onProgressPtr;
    ProgressHandler *progressHandler = new ProgressHandler(env, threadSafeFunc, asyncContext->requestId,
        dataHandlerRef);
    progressHandler->progressMode = progressMode;
    progressHandler->cameraProgressMode = cameraProgressMode;

    MediaAssetManagerNapi::progressHandlerMap_.EnsureInsert(asyncContext->requestId, progressHandler);
    NAPI_DEBUG_LOG("InsertProgressHandler");
    return progressHandler;
}

static void DeleteDataHandler(NotifyMode notifyMode, const std::string &requestUri, const std::string &requestId)
{
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(requestUri);
    NAPI_INFO_LOG("Rmv %{public}d, %{public}s, %{public}s", notifyMode,
        MediaFileUtils::DesensitizeUri(requestUri).c_str(), requestId.c_str());
    if (notifyMode == NotifyMode::WAIT_FOR_HIGH_QUALITY) {
        DeleteInProcessMapRecord(uriLocal, requestId);
    }
    inProcessFastRequests.Erase(requestId);
}

static MultiStagesCapturePhotoStatus QueryViaSandBox(int fileId,
    const string& photoUri, std::string &photoId, bool hasReadPermission, int32_t userId)
{
    photoId = "";
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::vector<std::string> fetchColumn { PhotoColumn::PHOTO_QUALITY, PhotoColumn::PHOTO_ID};
    string queryUri;
    if (hasReadPermission) {
        queryUri = PAH_QUERY_PHOTO;
    } else {
        queryUri = photoUri;
        MediaFileUri::RemoveAllFragment(queryUri);
    }
    Uri uri(queryUri);
    int errCode = 0;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates) && userId == -1) {
        shared_ptr<DataShare::DataShareResultSet> resultSet = MediaAssetRdbStore::GetInstance()->Query(
            predicates, fetchColumn, object, errCode);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
            NAPI_ERR_LOG("query resultSet is nullptr");
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
            HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} query photo status : lowQuality",
                MLOG_TAG, __FUNCTION__, __LINE__);
            return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
        }
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} query photo status quality: %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, currentPhotoQuality);
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    } else {
        return MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL;
    }
}

MultiStagesCapturePhotoStatus MediaAssetManagerNapi::QueryPhotoStatus(int fileId,
    const string& photoUri, std::string &photoId, bool hasReadPermission, int32_t userId)
{
    MultiStagesCapturePhotoStatus status = QueryViaSandBox(fileId, photoUri, photoId, hasReadPermission, userId);
    if (status != MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL) {
        return status;
    }
    QueryPhotoReqBody reqBody;
    reqBody.fileId = std::to_string(fileId);
    QueryPhotoRespBody respBody;
    std::unordered_map<std::string, std::string> headerMap {
        {MediaColumn::MEDIA_ID, reqBody.fileId }, {URI_TYPE, TYPE_PHOTOS}};
    int ret = IPC::UserDefineIPCClient().SetUserId(userId).SetHeader(headerMap).Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_PHOTO_STATUS), reqBody, respBody);
    if (ret < 0) {
        NAPI_ERR_LOG("ret = %{public}d", ret);
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    }
    photoId = respBody.photoId;
    if (respBody.photoQuality == LOW_QUALITY_IMAGE) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} query photo status : lowQuality",
            MLOG_TAG, __FUNCTION__, __LINE__);
        return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
    }
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} query photo status quality: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, respBody.photoQuality);
    return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
}

void MediaAssetManagerNapi::ProcessImage(const int fileId, const int deliveryMode)
{
    std::string uriStr = PAH_PROCESS_IMAGE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    std::vector<std::string> columns { std::to_string(fileId), std::to_string(deliveryMode) };
    UserFileClient::Query(uri, predicates, columns, errCode);
}

void MediaAssetManagerNapi::AddImage(const int fileId, DeliveryMode deliveryMode)
{
    Uri updateAssetUri(PAH_ADD_IMAGE);
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put("deliveryMode", static_cast<int>(deliveryMode));
    UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
}

void MediaAssetManagerNapi::ProcessVideo(MediaAssetManagerAsyncContext *asyncContext)
{
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFINE_PROCESS_VIDEO);
    ProcessVideoReqBody reqBody;
    reqBody.fileId = asyncContext->fileId;
    reqBody.deliveryMode = static_cast<int32_t>(asyncContext->deliveryMode);
    reqBody.photoId = asyncContext->photoId;
    reqBody.requestId = asyncContext->requestId;
    IPC::UserDefineIPCClient().Call(businessCode, reqBody);
}

napi_status GetDeliveryMode(napi_env env, const napi_value arg, const string &propName,
    DeliveryMode& deliveryMode)
{
    bool present = false;
    napi_value property = nullptr;
    int mode = -1;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present),
        "Failed to check property name");
    if (!present) {
        NAPI_ERR_LOG("No delivery mode specified");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "No delivery mode specified");
        return napi_invalid_arg;
    }
    CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property), "Failed to get property");
    CHECK_STATUS_RET(napi_get_value_int32(env, property, &mode), "Failed to parse deliveryMode argument value");

    // delivery mode's valid range is 0 - 2
    if (mode < 0 || mode > 2) {
        NAPI_ERR_LOG("delivery mode invalid argument ");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "invalid delivery mode value");
        return napi_invalid_arg;
    }
    deliveryMode = static_cast<DeliveryMode>(mode);
    return napi_ok;
}

napi_status GetSourceMode(napi_env env, const napi_value arg, const string &propName,
    SourceMode& sourceMode)
{
    bool present = false;
    napi_value property = nullptr;
    int mode = -1;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present), "Failed to check property name");
    if (!present) {
        // use default source mode
        sourceMode = SourceMode::EDITED_MODE;
        return napi_ok;
    } else if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NAPI_ERR_LOG("Source mode is only available to system apps");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Source mode is only available to system apps");
        return napi_invalid_arg;
    }
    CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property), "Failed to get property");
    CHECK_STATUS_RET(napi_get_value_int32(env, property, &mode), "Failed to parse sourceMode argument value");

    // source mode's valid range is 0 - 1
    if (mode < 0 || mode > 1) {
        NAPI_ERR_LOG("source mode invalid");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "invalid source mode value");
        return napi_invalid_arg;
    }
    sourceMode = static_cast<SourceMode>(mode);
    return napi_ok;
}

napi_status ParseArgGetRequestOption(napi_env env, napi_value arg, DeliveryMode &deliveryMode, SourceMode &sourceMode)
{
    CHECK_STATUS_RET(GetDeliveryMode(env, arg, "deliveryMode", deliveryMode), "Failed to parse deliveryMode");
    CHECK_STATUS_RET(GetSourceMode(env, arg, "sourceMode", sourceMode), "Failed to parse sourceMode");
    return napi_ok;
}

napi_status GetCompatibleMode(napi_env env, const napi_value arg, const string &propName,
    CompatibleMode& compatibleMode)
{
    bool present = false;
    napi_value property = nullptr;
    int mode = static_cast<int>(CompatibleMode::ORIGINAL_FORMAT_MODE);
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present), "Failed to check property name");
    if (!present) {
        NAPI_INFO_LOG("compatible mode is null");
        compatibleMode = CompatibleMode::ORIGINAL_FORMAT_MODE;
        return napi_ok;
    }
    CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property), "Failed to get property");
    CHECK_STATUS_RET(napi_get_value_int32(env, property, &mode), "Failed to parse compatiblemode argument value");

    if (static_cast<CompatibleMode>(mode) < CompatibleMode::ORIGINAL_FORMAT_MODE ||
        static_cast<CompatibleMode>(mode) > CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        NAPI_ERR_LOG("delivery mode invalid argument ");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "invalid compatible mode value");
        return napi_invalid_arg;
    }
#if !defined(USE_VIDEO_PROCESSING_ENGINE) || !defined(USE_VIDEO_PROCESSING_ENGINE_EXT)
    if (static_cast<CompatibleMode>(mode) == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        NAPI_ERR_LOG("current environment not support transcoder");
        NapiError::ThrowError(env, OHOS_NOT_SUPPORT_TRANSCODER_CODE, "not support transcoder");
        return napi_invalid_arg;
    }
#endif
    compatibleMode = static_cast<CompatibleMode>(mode);
    return napi_ok;
}

napi_status GetMediaAssetProgressHandler(napi_env env, const napi_value arg, napi_value& mediaAssetProgressHandler,
    const string &propName)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, arg != nullptr, OHOS_INVALID_PARAM_CODE,
        "MediaAssetProgressHandler invalid argument", napi_invalid_arg, "MediaAssetProgressHandler  is nullptr");
    bool present = false;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present), "Failed to check property name");
    if (!present) {
        NAPI_INFO_LOG("MediaAssetProgressHandler is null");
        mediaAssetProgressHandler = nullptr;
        return napi_ok;
    }
    napi_value progressHandler;
    napi_status status = napi_get_named_property(env, arg, "mediaAssetProgressHandler", &progressHandler);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE,
        "failed to get mediaAssetProgressHandler ", napi_invalid_arg,
        "failed to get mediaAssetProgressHandler, napi status: %{public}d", static_cast<int>(status));
    napi_valuetype valueType;
    status = napi_typeof(env, progressHandler, &valueType);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid progress handler",
        napi_invalid_arg, "failed to get type of progress handler, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, valueType == napi_object, OHOS_INVALID_PARAM_CODE,
        "progress handler not an object", napi_invalid_arg, "progress handler not an object");

    napi_value onProgress;
    status = napi_get_named_property(env, progressHandler, ON_PROGRESS_FUNC, &onProgress);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE,
        "unable to get onProgress function", napi_invalid_arg,
        "failed to get onProgress function, napi status: %{public}d", static_cast<int>(status));

    status = napi_typeof(env, onProgress, &valueType);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onProgress",
        napi_invalid_arg, "failed to get type of onProgress, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, valueType == napi_function, OHOS_INVALID_PARAM_CODE,
        "onProgress not a function", napi_invalid_arg, "onProgress not a function");
    mediaAssetProgressHandler = progressHandler;
    return napi_ok;
}

napi_status ParseArgGetRequestOptionMore(napi_env env, napi_value arg, CompatibleMode &compatibleMode,
    napi_value &mediaAssetProgressHandler)
{
    NAPI_INFO_LOG("ParseArgGetRequestOptionMore start");
    CHECK_STATUS_RET(GetCompatibleMode(env, arg, "compatibleMode", compatibleMode), "Failed to parse compatibleMode");
    if (GetMediaAssetProgressHandler(env, arg, mediaAssetProgressHandler, "mediaAssetProgressHandler") != napi_ok) {
        NAPI_ERR_LOG("requestMedia GetMediaAssetProgressHandler error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia GetMediaAssetProgressHandler error");
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status ParseArgGetPhotoAsset(napi_env env, napi_value arg, int &fileId, std::string &uri,
    std::string &displayName)
{
    if (arg == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "ParseArgGetPhotoAsset failed to get photoAsset");
        return napi_invalid_arg;
    }
    FileAssetNapi *obj = nullptr;
    napi_unwrap(env, arg, reinterpret_cast<void**>(&obj));
    if (obj == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to get asset napi object");
        return napi_invalid_arg;
    }
    fileId = obj->GetFileId();
    uri = obj->GetFileUri();
    displayName = obj->GetFileDisplayName();
    return napi_ok;
}

napi_status ParseArgGetPhotoAsset(napi_env env, napi_value arg, unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    if (arg == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "ParseArgGetPhotoAsset failed to get photoAsset");
        return napi_invalid_arg;
    }
    FileAssetNapi *obj = nullptr;
    napi_unwrap(env, arg, reinterpret_cast<void**>(&obj));
    if (obj == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to get asset napi object");
        return napi_invalid_arg;
    }
    asyncContext->fileId = obj->GetFileId();
    asyncContext->photoUri = obj->GetFileUri();
    asyncContext->displayName = obj->GetFileDisplayName();
    asyncContext->userId = obj->GetFileAssetInstance()->GetUserId();
    return napi_ok;
}

napi_status ParseArgGetDestPath(napi_env env, napi_value arg, std::string &destPath)
{
    if (arg == nullptr) {
        NAPI_ERR_LOG("destPath arg is invalid");
        return napi_invalid_arg;
    }
    napi_get_print_string(env, arg, destPath);
    if (destPath.empty()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to get destPath napi object");
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status ParseArgGetEfficientImageDataHandler(napi_env env, napi_value arg, napi_value& dataHandler,
    bool& needsExtraInfo)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, arg != nullptr, OHOS_INVALID_PARAM_CODE, "efficient handler invalid argument",
        napi_invalid_arg, "efficient data handler is nullptr");

    napi_valuetype valueType;
    napi_status status = napi_typeof(env, arg, &valueType);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid efficient data handler",
        napi_invalid_arg, "failed to get type of efficient data handler, napi status: %{public}d",
        static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, valueType == napi_object, OHOS_INVALID_PARAM_CODE,
        "efficient data handler not a object", napi_invalid_arg, "efficient data handler not a object");

    dataHandler = arg;

    napi_value onDataPrepared;
    status = napi_get_named_property(env, arg, ON_DATA_PREPARED_FUNC, &onDataPrepared);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE,
        "unable to get onDataPrepared function", napi_invalid_arg,
        "failed to get type of efficient data handler, napi status: %{public}d", static_cast<int>(status));
    status = napi_typeof(env, onDataPrepared, &valueType);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onDataPrepared",
        napi_invalid_arg, "failed to get type of onDataPrepared, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, valueType == napi_function, OHOS_INVALID_PARAM_CODE,
        "onDataPrepared not a function", napi_invalid_arg, "onDataPrepared not a function");

    napi_value paramCountNapi;
    status = napi_get_named_property(env, onDataPrepared, "length", &paramCountNapi);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onDataPrepared",
        napi_invalid_arg, "get onDataPrepared arg count fail, napi status: %{public}d", static_cast<int>(status));
    int32_t paramCount = -1;
    constexpr int paramCountMin = 2;
    constexpr int paramCountMax = 3;
    status = napi_get_value_int32(env, paramCountNapi, &paramCount);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onDataPrepared",
        napi_invalid_arg, "get onDataPrepared arg count value fail, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, (paramCount >= paramCountMin && paramCount <= paramCountMax),
        OHOS_INVALID_PARAM_CODE, "onDataPrepared has wrong number of parameters",
        napi_invalid_arg, "onDataPrepared has wrong number of parameters");

    if (paramCount == ARGS_THREE) {
        needsExtraInfo = true;
    }
    return napi_ok;
}

napi_status ParseArgGetDataHandler(napi_env env, napi_value arg, napi_value& dataHandler, bool& needsExtraInfo)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, arg != nullptr, OHOS_INVALID_PARAM_CODE, "data handler invalid argument",
        napi_invalid_arg, "data handler is nullptr");

    napi_valuetype valueType;
    napi_status status = napi_typeof(env, arg, &valueType);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid data handler",
        napi_invalid_arg, "failed to get type of data handler, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, valueType == napi_object, OHOS_INVALID_PARAM_CODE,
        "data handler not a object", napi_invalid_arg, "data handler not a object");

    dataHandler = arg;

    napi_value onDataPrepared;
    status = napi_get_named_property(env, arg, ON_DATA_PREPARED_FUNC, &onDataPrepared);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE,
        "unable to get onDataPrepared function", napi_invalid_arg,
        "failed to get type of data handler, napi status: %{public}d", static_cast<int>(status));
    status = napi_typeof(env, onDataPrepared, &valueType);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onDataPrepared",
        napi_invalid_arg, "failed to get type of onDataPrepared, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, valueType == napi_function, OHOS_INVALID_PARAM_CODE,
        "onDataPrepared not a function", napi_invalid_arg, "onDataPrepared not a function");

    napi_value paramCountNapi;
    status = napi_get_named_property(env, onDataPrepared, "length", &paramCountNapi);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onDataPrepared",
        napi_invalid_arg, "get onDataPrepared arg count fail, napi status: %{public}d", static_cast<int>(status));
    int32_t paramCount = -1;
    constexpr int paramCountMin = 1;
    constexpr int paramCountMax = 2;
    status = napi_get_value_int32(env, paramCountNapi, &paramCount);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == napi_ok, OHOS_INVALID_PARAM_CODE, "invalid onDataPrepared",
        napi_invalid_arg, "get onDataPrepared arg count value fail, napi status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, (paramCount >= paramCountMin && paramCount <= paramCountMax),
        OHOS_INVALID_PARAM_CODE, "onDataPrepared has wrong number of parameters",
        napi_invalid_arg, "onDataPrepared has wrong number of parameters");

    if (paramCount == ARGS_TWO) {
        needsExtraInfo = true;
    }
    return napi_ok;
}

static std::string GenerateRequestId()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

void MediaAssetManagerNapi::RegisterTaskNewObserver(napi_env env, MediaAssetManagerAsyncContext *asyncContext)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");
    CHECK_IF_EQUAL(asyncContext->observerType != ObserverType::UNDEFINED,
        "In asyncContext, observerType is undefined, RegisterTaskNewObserver failed.");
    std::string uriLocal = MediaFileUtils::GetUriWithoutDisplayname(asyncContext->photoUri);

    auto observerBodyBase = std::make_shared<MultistagesCaptureOnProcessObserver>(uriLocal, asyncContext->observerType);
    auto dataObserver = std::make_shared<MediaOnNotifyUserDefineObserver>(
        NotifyUriType::USER_DEFINE_NOTIFY_URI, observerBodyBase);
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyUtils::GetUserDefineNotifyTypeAndUri(
        Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI, registerUriType, registerUri) != E_OK) {
        NAPI_ERR_LOG("GetUserDefineNotifyTypeAndUri failed.");
        return;
    }

    std::unique_lock<std::mutex> registerLock(registerTaskLock);
    if (multiStagesObserverNewMap.find(uriLocal) == multiStagesObserverNewMap.end()) {
        multiStagesObserverNewMap.insert(std::make_pair(
            uriLocal, std::map<ObserverType, std::shared_ptr<MediaOnNotifyUserDefineObserver>>{}));
    }
    auto &observerMap = multiStagesObserverNewMap.at(uriLocal);
    if (observerMap.find(asyncContext->observerType) == observerMap.end()) {
        UserFileClient::RegisterObserverExtProvider(Uri(registerUri),
            static_cast<std::shared_ptr<DataShare::DataShareObserver>>(dataObserver), false);
        observerMap.insert(std::make_pair(asyncContext->observerType, dataObserver));
    }
    registerLock.unlock();
    InsertDataHandler(NotifyMode::WAIT_FOR_HIGH_QUALITY, env, asyncContext);

    if (asyncContext->observerType == ObserverType::REQUEST_IMAGE
        || asyncContext->observerType == ObserverType::REQUEST_QUICK_IMAGE) {
        MediaAssetManagerNapi::ProcessImage(asyncContext->fileId, static_cast<int32_t>(asyncContext->deliveryMode));
    } else if (asyncContext->observerType == ObserverType::REQUEST_VIDEO) {
        MediaAssetManagerNapi::ProcessVideo(asyncContext);
    }
    NAPI_INFO_LOG("RegisterTaskNewObserver success.");
}

napi_status MediaAssetManagerNapi::ParseRequestMediaArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, asyncContext->argc, asyncContext->argv, thisVar);
    if (asyncContext->argc != ARGS_FOUR && asyncContext->argc != ARGS_FIVE) {
        NAPI_ERR_LOG("requestMedia argc error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia argc invalid");
        return napi_invalid_arg;
    }
    if (ParseArgGetPhotoAsset(env, asyncContext->argv[PARAM1], asyncContext) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return napi_invalid_arg;
    }
    if (ParseArgGetRequestOption(env, asyncContext->argv[PARAM2], asyncContext->deliveryMode,
        asyncContext->sourceMode) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetRequestOption error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOption error");
        return napi_invalid_arg;
    }
    if (ParseArgGetRequestOptionMore(env, asyncContext->argv[PARAM2], asyncContext->compatibleMode,
        asyncContext->mediaAssetProgressHandler) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetRequestOptionMore error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOptionMore error");
        return napi_invalid_arg;
    }
    if (asyncContext->argc == ARGS_FOUR) {
        if (ParseArgGetDataHandler(env, asyncContext->argv[PARAM3], asyncContext->dataHandler,
            asyncContext->needsExtraInfo) != napi_ok) {
            NAPI_ERR_LOG("requestMedia ParseArgGetDataHandler error");
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetDataHandler error");
            return napi_invalid_arg;
        }
    } else if (asyncContext->argc == ARGS_FIVE) {
        if (ParseArgGetDestPath(env, asyncContext->argv[PARAM3], asyncContext->destUri) != napi_ok) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetDestPath error");
            return napi_invalid_arg;
        }
        if (ParseArgGetDataHandler(env, asyncContext->argv[PARAM4], asyncContext->dataHandler,
            asyncContext->needsExtraInfo) != napi_ok) {
            NAPI_ERR_LOG("requestMedia ParseArgGetDataHandler error");
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetDataHandler error");
            return napi_invalid_arg;
        }
    }
    asyncContext->hasReadPermission = HasReadPermission();
    return napi_ok;
}

napi_status MediaAssetManagerNapi::ParseEfficentRequestMediaArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, asyncContext->argc, asyncContext->argv, thisVar);
    if (asyncContext->argc != ARGS_FOUR && asyncContext->argc != ARGS_FIVE) {
        NAPI_ERR_LOG("requestMedia argc error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia argc invalid");
        return napi_invalid_arg;
    }

    if (ParseArgGetPhotoAsset(env, asyncContext->argv[PARAM1], asyncContext) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return napi_invalid_arg;
    }
    if (ParseArgGetRequestOption(env, asyncContext->argv[PARAM2], asyncContext->deliveryMode,
        asyncContext->sourceMode) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetRequestOption error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOption error");
        return napi_invalid_arg;
    }
    if (asyncContext->argc == ARGS_FOUR) {
        if (ParseArgGetEfficientImageDataHandler(env, asyncContext->argv[PARAM3], asyncContext->dataHandler,
            asyncContext->needsExtraInfo) != napi_ok) {
            NAPI_ERR_LOG("requestMedia ParseArgGetEfficientImageDataHandler error");
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE,
                "requestMedia ParseArgGetEfficientImageDataHandler error");
            return napi_invalid_arg;
        }
    } else if (asyncContext->argc == ARGS_FIVE) {
        if (ParseArgGetDestPath(env, asyncContext->argv[PARAM3], asyncContext->destUri) != napi_ok) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetDestPath error");
            return napi_invalid_arg;
        }
        if (ParseArgGetEfficientImageDataHandler(env, asyncContext->argv[PARAM4], asyncContext->dataHandler,
            asyncContext->needsExtraInfo) != napi_ok) {
            NAPI_ERR_LOG("requestMedia ParseArgGetEfficientImageDataHandler error");
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE,
                "requestMedia ParseArgGetEfficientImageDataHandler error");
            return napi_invalid_arg;
        }
    }
    asyncContext->hasReadPermission = HasReadPermission();
    return napi_ok;
}

bool MediaAssetManagerNapi::InitUserFileClient(napi_env env, napi_callback_info info, const int32_t userId)
{
    if (UserFileClient::IsValid(userId)) {
        return true;
    }

    std::unique_lock<std::mutex> helperLock(MediaLibraryNapi::sUserFileClientMutex_);
    if (!UserFileClient::IsValid(userId)) {
        UserFileClient::Init(env, info, userId);
    }
    helperLock.unlock();
    return UserFileClient::IsValid(userId);
}

static int32_t GetPhotoSubtype(napi_env env, napi_value photoAssetArg)
{
    if (photoAssetArg == nullptr) {
        NAPI_ERR_LOG(
            "Dfx adaptation to moving photo collector error: failed to get photo subtype, photo asset is null");
        return -1;
    }
    FileAssetNapi *obj = nullptr;
    napi_unwrap(env, photoAssetArg, reinterpret_cast<void**>(&obj));
    if (obj == nullptr) {
        NAPI_ERR_LOG("Dfx adaptation to moving photo collector error: failed to unwrap file asset");
        return -1;
    }
    return obj->GetFileAssetInstance()->GetPhotoSubType();
}

napi_value MediaAssetManagerNapi::JSRequestImageData(napi_env env, napi_callback_info info)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} Begin JSRequestImageData",
        MLOG_TAG, __FUNCTION__, __LINE__);
    if (env == nullptr || info == nullptr) {
        NAPI_ERR_LOG("JSRequestImageData js arg invalid");
        NapiError::ThrowError(env, JS_INNER_FAIL, "JSRequestImageData js arg invalid");
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("JSRequestImageData");
    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_ARRAY_BUFFER;
    if (ParseRequestMediaArgs(env, info, asyncContext) != napi_ok) {
        NAPI_ERR_LOG("failed to parse requestImagedata args");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to parse requestImagedata args");
        return nullptr;
    }
    if (!InitUserFileClient(env, info, asyncContext->userId)) {
        NAPI_ERR_LOG("JSRequestEfficientIImage init user file client failed");
        NapiError::ThrowError(env, JS_INNER_FAIL, "handler is invalid");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef2) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr2) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }

    asyncContext->requestId = GenerateRequestId();
    asyncContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asyncContext->argv[PARAM1]));
    asyncContext->observerType = ObserverType::REQUEST_IMAGE;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestImageData", JSRequestExecute,
        JSRequestComplete);
}

napi_value MediaAssetManagerNapi::JSRequestImage(napi_env env, napi_callback_info info)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} Begin JSRequestImage",
        MLOG_TAG, __FUNCTION__, __LINE__);
    if (env == nullptr || info == nullptr) {
        NAPI_ERR_LOG("JSRequestImage js arg invalid");
        NapiError::ThrowError(env, JS_INNER_FAIL, "JSRequestImage js arg invalid");
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("JSRequestImage");

    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_IMAGE_SOURCE;
    if (ParseRequestMediaArgs(env, info, asyncContext) != napi_ok) {
        NAPI_ERR_LOG("failed to parse requestImage args");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to parse requestImage args");
        return nullptr;
    }
    if (!InitUserFileClient(env, info, asyncContext->userId)) {
        NAPI_ERR_LOG("JSRequestImage init user file client failed");
        NapiError::ThrowError(env, JS_INNER_FAIL, "handler is invalid");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef2) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr2) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }

    asyncContext->requestId = GenerateRequestId();
    asyncContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asyncContext->argv[PARAM1]));
    asyncContext->observerType = ObserverType::REQUEST_IMAGE;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestImage", JSRequestExecute,
        JSRequestComplete);
}

napi_value MediaAssetManagerNapi::JSRequestEfficientIImage(napi_env env, napi_callback_info info)
{
    NAPI_DEBUG_LOG("JSRequestEfficientIImage");
    if (env == nullptr || info == nullptr) {
        NAPI_ERR_LOG("JSRequestEfficientIImage js arg invalid");
        NapiError::ThrowError(env, JS_INNER_FAIL, "JSRequestEfficientIImage js arg invalid");
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("JSRequestEfficientIImage");

    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_PICTURE;
    if (ParseEfficentRequestMediaArgs(env, info, asyncContext) != napi_ok) {
        NAPI_ERR_LOG("failed to parse JSRequestEfficientIImage args");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to parse JSRequestEfficientIImage args");
        return nullptr;
    }
    if (!InitUserFileClient(env, info, asyncContext->userId)) {
        NAPI_ERR_LOG("JSRequestEfficientIImage init user file client failed");
        NapiError::ThrowError(env, JS_INNER_FAIL, "handler is invalid");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef2) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr2) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }

    asyncContext->requestId = GenerateRequestId();
    asyncContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asyncContext->argv[PARAM1]));
    asyncContext->observerType = ObserverType::REQUEST_QUICK_IMAGE;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestEfficientIImage", JSRequestExecute,
        JSRequestComplete);
}

void MediaAssetManagerNapi::ReleaseSafeFunc(napi_threadsafe_function &threadSafeFunc)
{
    if (threadSafeFunc == nullptr) {
        return;
    }
    napi_release_threadsafe_function(threadSafeFunc, napi_tsfn_release);
    threadSafeFunc = nullptr;
}

bool MediaAssetManagerNapi::CreateOnProgressHandlerInfo(napi_env env,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    if (asyncContext->compatibleMode != CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        return true;
    }
    if (asyncContext->mediaAssetProgressHandler == nullptr) {
        if (CreateOnProgressThreadSafeFunc(env, asyncContext, asyncContext->onProgressPtr) != napi_ok) {
            NAPI_ERR_LOG("CreateOnProgressThreadSafeFunc failed");
            return false;
        }
        return true;
    }
    if (CreateProgressHandlerRef(env, asyncContext, asyncContext->progressHandlerRef) != napi_ok ||
        CreateOnProgressThreadSafeFunc(env, asyncContext, asyncContext->onProgressPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateProgressHandlerRef or CreateOnProgressThreadSafeFunc failed");
        return false;
    }

    MEDIA_INFO_LOG("CreateOnProgressHandlerInfo create progress callback success.");
    return true;
}

static bool CheckBaseParams(napi_env env, napi_callback_info info, const std::string &funcName)
{
    if (env == nullptr || info == nullptr) {
        NAPI_ERR_LOG("%{public}s js arg invalid", funcName.c_str());
        NapiError::ThrowError(env, JS_INNER_FAIL, "js arg invalid");
        return false;
    }
    return true;
}

bool MediaAssetManagerNapi::ParseAndCheckArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaAssetManagerAsyncContext>& asyncContext)
{
    if (ParseRequestMediaArgs(env, info, asyncContext) != napi_ok) {
        NAPI_ERR_LOG("failed to parse requestVideo args");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to parse requestVideo args");
        return false;
    }

    if (!InitUserFileClient(env, info, asyncContext->userId)) {
        NAPI_ERR_LOG("JSRequestVideoFile init user file client failed, userId is %{public}d", asyncContext->userId);
        NapiError::ThrowError(env, JS_INNER_FAIL, "handler is invalid");
        return false;
    }

    if (asyncContext->photoUri.length() > MAX_URI_SIZE || asyncContext->destUri.length() > MAX_URI_SIZE) {
        NAPI_ERR_LOG("request video file uri lens out of limit photoUri lens: %{public}zu, destUri lens: %{public}zu",
            asyncContext->photoUri.length(), asyncContext->destUri.length());
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "request video file uri lens out of limit");
        return false;
    }

    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_VIDEO ||
        MediaFileUtils::GetMediaType(MediaFileUtils::GetFileName(asyncContext->destUri)) != MEDIA_TYPE_VIDEO) {
        NAPI_ERR_LOG("request video file type invalid");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "request video file type invalid");
        return false;
    }
    return true;
}

bool MediaAssetManagerNapi::CheckDataHandler(napi_env env, unique_ptr<MediaAssetManagerAsyncContext>& asyncContext)
{
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return false;
    }

    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef2) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr2) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return false;
    }

    return true;
}

napi_value MediaAssetManagerNapi::JSRequestVideoFile(napi_env env, napi_callback_info info)
{
    if (!CheckBaseParams(env, info, "JSRequestVideoFile")) {
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JSRequestVideoFile");

    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_TARGET_PATH;
    if (!ParseAndCheckArgs(env, info, asyncContext)) {
        return nullptr;
    }

    if (!CheckDataHandler(env, asyncContext)) {
        return nullptr;
    }

    if (!CreateOnProgressHandlerInfo(env, asyncContext)) {
        NAPI_ERR_LOG("CreateOnProgressHandlerInfo failed");
        return nullptr;
    }

    asyncContext->requestId = GenerateRequestId();
    asyncContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asyncContext->argv[PARAM1]));
    asyncContext->observerType = ObserverType::REQUEST_VIDEO;
    NAPI_INFO_LOG("JSRequestVideoFile requestId: %{public}s, subtype: %{public}d.",
        asyncContext->requestId.c_str(), static_cast<int32_t>(asyncContext->subType));
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestVideoFile",
        JSRequestVideoFileExecute, JSRequestComplete);
}

napi_value MediaAssetManagerNapi::JSRequestVideo(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("Beigin JSRequestVideo");
    if (!CheckBaseParams(env, info, "JSRequestVideo")) {
        return nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("JSRequestVideo");

    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    asyncContext->returnDataType = ReturnDataType::TYPE_READY;

    if (ParseRequestVideoArgs(env, info, asyncContext) != napi_ok) {
        NAPI_ERR_LOG("failed to parse requestVideo args");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "failed to parse requestVideo args");
        return nullptr;
    }
    if (!InitUserFileClient(env, info, asyncContext->userId)) {
        NAPI_ERR_LOG("JSRequestVideo init user file client failed, userId is %{public}d", asyncContext->userId);
        NapiError::ThrowError(env, JS_INNER_FAIL, "handler is invalid");
        return nullptr;
    }

    if (asyncContext->photoUri.length() > MAX_URI_SIZE) {
        NAPI_ERR_LOG("request video file uri lens out of limit photoUri lens: %{public}zu",
            asyncContext->photoUri.length());
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "request video file uri lens out of limit");
        return nullptr;
    }
    if (MediaFileUtils::GetMediaType(asyncContext->displayName) != MEDIA_TYPE_VIDEO) {
        NAPI_ERR_LOG("request video file type invalid");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "request video file type invalid");
        return nullptr;
    }

    if (!CheckDataHandler(env, asyncContext)) {
        return nullptr;
    }

    if (!CreateOnProgressHandlerInfoForRequestVideo(env, asyncContext)) {
        NAPI_ERR_LOG("CreateOnProgressHandlerInfoForRequestVideo failed");
        return nullptr;
    }

    asyncContext->requestId = GenerateRequestId();
    asyncContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asyncContext->argv[PARAM1]));
    asyncContext->observerType = ObserverType::REQUEST_VIDEO;
    NAPI_INFO_LOG("JSRequestVideo requestId: %{public}s, subtype: %{public}d.",
        asyncContext->requestId.c_str(), static_cast<int32_t>(asyncContext->subType));
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestVideo",
        JSRequestVideoFileExecute, JSRequestComplete);
}

napi_status MediaAssetManagerNapi::ParseRequestVideoArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, asyncContext->argc, asyncContext->argv, thisVar);
    if (asyncContext->argc != ARGS_FOUR) {
        NAPI_ERR_LOG("requestMedia argc error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia argc invalid");
        return napi_invalid_arg;
    }
    if (ParseArgGetPhotoAsset(env, asyncContext->argv[PARAM1], asyncContext) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return napi_invalid_arg;
    }
    if (ParseArgGetRequestOption(env, asyncContext->argv[PARAM2], asyncContext->deliveryMode,
        asyncContext->sourceMode) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetRequestOption error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOption error");
        return napi_invalid_arg;
    }
    if (ParseArgGetRequestOptionMore(env, asyncContext->argv[PARAM2], asyncContext->compatibleMode,
        asyncContext->mediaAssetProgressHandler) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetRequestOptionMore error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOptionMore error");
        return napi_invalid_arg;
    }

    if (asyncContext->compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        NAPI_ERR_LOG("This compatibleMode is not supported.");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOptionMore error");
        return napi_invalid_arg;
    }

    if (ParseArgGetDataHandler(env, asyncContext->argv[PARAM3], asyncContext->dataHandler,
        asyncContext->needsExtraInfo) != napi_ok) {
        NAPI_ERR_LOG("requestMedia ParseArgGetDataHandler error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetDataHandler error");
        return napi_invalid_arg;
    }
    asyncContext->hasReadPermission = HasReadPermission();
    return napi_ok;
}

bool MediaAssetManagerNapi::CreateOnProgressHandlerInfoForRequestVideo(napi_env env,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    if (asyncContext->mediaAssetProgressHandler == nullptr) {
        MEDIA_INFO_LOG("mediaAssetProgressHandler is nullptr, there is no progress callback.");
        return true;
    }
    if (CreateProgressHandlerRef(env, asyncContext, asyncContext->progressHandlerRef) != napi_ok ||
        CreateOnProgressThreadSafeFunc(env, asyncContext, asyncContext->onProgressPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateProgressHandlerRef or CreateOnProgressThreadSafeFunc failed");
        return false;
    }
    MEDIA_INFO_LOG("ForRequestVideo create progress callback success.");
    return true;
}

void MediaAssetManagerNapi::OnHandleRequestImage(napi_env env, MediaAssetManagerAsyncContext *asyncContext)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} OnHandleRequestImage mode: %{public}d.",
        MLOG_TAG, __FUNCTION__, __LINE__, static_cast<int32_t>(asyncContext->deliveryMode));
    MultiStagesCapturePhotoStatus status = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    switch (asyncContext->deliveryMode) {
        case DeliveryMode::FAST:
            if (asyncContext->needsExtraInfo) {
                asyncContext->photoQuality =
                    MediaAssetManagerNapi::QueryPhotoStatus(asyncContext->fileId, asyncContext->photoUri,
                    asyncContext->photoId, asyncContext->hasReadPermission, asyncContext->userId);
            }
            MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(env, asyncContext);
            ReleaseSafeFunc(asyncContext->onDataPreparedPtr2);
            break;
        case DeliveryMode::HIGH_QUALITY:
            status = MediaAssetManagerNapi::QueryPhotoStatus(asyncContext->fileId,
                asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission, asyncContext->userId);
            asyncContext->photoQuality = status;
            if (status == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
                MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(env, asyncContext);
                ReleaseSafeFunc(asyncContext->onDataPreparedPtr2);
            } else {
                RegisterTaskNewObserver(env, asyncContext);
                ReleaseSafeFunc(asyncContext->onDataPreparedPtr);
            }
            break;
        case DeliveryMode::BALANCED_MODE:
            status = MediaAssetManagerNapi::QueryPhotoStatus(asyncContext->fileId,
                asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission, asyncContext->userId);
            asyncContext->photoQuality = status;
            MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(env, asyncContext);
            if (status == MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
                RegisterTaskNewObserver(env, asyncContext);
            } else {
                ReleaseSafeFunc(asyncContext->onDataPreparedPtr2);
            }
            break;
        default: {
            NAPI_ERR_LOG("invalid delivery mode");
            return;
        }
    }
}

static void DfxLogCinematicVideo(bool isHighQaulity, const int32_t &userId)
{
    CinematicVideoAccessReqBody reqBody;
    reqBody.isHighQaulity = isHighQaulity;
    int32_t ret = IPC::UserDefineIPCClient().SetUserId(userId).Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_CINEMATIC_VIDEO), reqBody);
    if (ret != E_OK) {
        NAPI_ERR_LOG("cinematic video access log failed, ret: %{public}d", ret);
    }
}

void MediaAssetManagerNapi::RequestVidoForFastMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");

    if (asyncContext->needsExtraInfo) {
        asyncContext->photoQuality = MediaAssetManagerNapi::QueryPhotoStatus(asyncContext->fileId,
            asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission, asyncContext->userId);
        if (asyncContext->subType == PhotoSubType::CINEMATIC_VIDEO) {
            DfxLogCinematicVideo(asyncContext->photoQuality == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS,
                asyncContext->userId);
        }
    }
    MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(env, asyncContext);
    ReleaseSafeFunc(asyncContext->onDataPreparedPtr2);
}

void MediaAssetManagerNapi::RequestVideoForHighQualityMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");

    MultiStagesCapturePhotoStatus status = MediaAssetManagerNapi::QueryPhotoStatus(asyncContext->fileId,
        asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission, asyncContext->userId);
    asyncContext->photoQuality = status;
    if (asyncContext->subType == PhotoSubType::CINEMATIC_VIDEO) {
        if (status == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
            MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(env, asyncContext);
            ReleaseSafeFunc(asyncContext->onDataPreparedPtr2);
        } else {
            RegisterTaskNewObserver(env, asyncContext);
            ReleaseSafeFunc(asyncContext->onDataPreparedPtr);
        }
        DfxLogCinematicVideo(true, asyncContext->userId);
    } else {
        MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(env, asyncContext);
        ReleaseSafeFunc(asyncContext->onDataPreparedPtr2);
    }
}

void MediaAssetManagerNapi::OnHandleRequestVideo(napi_env env, MediaAssetManagerAsyncContext *asyncContext)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");

    NAPI_INFO_LOG("OnHandleRequestVideo mode: %{public}d.", asyncContext->deliveryMode);
    switch (asyncContext->deliveryMode) {
        case DeliveryMode::FAST:
        case DeliveryMode::BALANCED_MODE:
            MediaAssetManagerNapi::RequestVidoForFastMode(env, asyncContext);
            break;
        case DeliveryMode::HIGH_QUALITY:
            MediaAssetManagerNapi::RequestVideoForHighQualityMode(env, asyncContext);
            break;
        default: {
            NAPI_ERR_LOG("invalid delivery mode");
            return;
        }
    }
}

void MediaAssetManagerNapi::NotifyDataPreparedWithoutRegister(napi_env env,
    MediaAssetManagerAsyncContext *asyncContext)
{
    AssetHandler *assetHandler = InsertDataHandler(NotifyMode::FAST_NOTIFY, env, asyncContext);
    if (assetHandler == nullptr) {
        NAPI_ERR_LOG("assetHandler is nullptr");
        return;
    }
    asyncContext->assetHandler = assetHandler;
}

void MediaAssetManagerNapi::OnHandleProgress(napi_env env, MediaAssetManagerAsyncContext *asyncContext,
    ProgressMode progressMode, CameraProgressMode cameraProgressMode)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");
    MEDIA_INFO_LOG("OnHandleProgress, progressMode: %{public}d, cameraProgressMode: %{public}d.",
        static_cast<int32_t>(progressMode), static_cast<int32_t>(cameraProgressMode));
    ProgressHandler *progressHandler = InsertProgressHandler(env, asyncContext, progressMode, cameraProgressMode);
    if (progressHandler == nullptr) {
        NAPI_ERR_LOG("progressHandler is nullptr");
        return;
    }
    asyncContext->progressHandler = progressHandler;
}

static string PhotoQualityToString(MultiStagesCapturePhotoStatus photoQuality)
{
    static const string HIGH_QUALITY_STRING = "high";
    static const string LOW_QUALITY_STRING = "low";
    if (photoQuality != MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS &&
        photoQuality != MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
        NAPI_ERR_LOG("Invalid photo quality: %{public}d", static_cast<int>(photoQuality));
        return HIGH_QUALITY_STRING;
    }

    return (photoQuality == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) ? HIGH_QUALITY_STRING :
        LOW_QUALITY_STRING;
}

static napi_value GetInfoMapNapiValue(napi_env env, AssetHandler* assetHandler)
{
    napi_status status;
    napi_value mapNapiValue {nullptr};
    status = napi_create_map(env, &mapNapiValue);
    CHECK_COND_RET(status == napi_ok && mapNapiValue != nullptr, nullptr,
        "Failed to create map napi value, napi status: %{public}d", static_cast<int>(status));

    napi_value qualityInfo {nullptr};
    status = napi_create_string_utf8(env, PhotoQualityToString(assetHandler->photoQuality).c_str(),
        NAPI_AUTO_LENGTH, &qualityInfo);
    CHECK_COND_RET(status == napi_ok && qualityInfo != nullptr, nullptr,
        "Failed to create quality string, napi status: %{public}d", static_cast<int>(status));

    status = napi_set_named_property(env, mapNapiValue, "quality", qualityInfo);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to set quality property, napi status: %{public}d",
        static_cast<int>(status));

    status = napi_map_set_named_property(env, mapNapiValue, "quality", qualityInfo);
    CHECK_COND_RET(status == napi_ok, nullptr, "Failed to set quality map key-value, napi status: %{public}d",
        static_cast<int>(status));

    return mapNapiValue;
}

static napi_value GetNapiValueOfMedia(napi_env env, const std::shared_ptr<NapiMediaAssetDataHandler>& dataHandler,
    bool& isPicture)
{
    NAPI_DEBUG_LOG("GetNapiValueOfMedia");
    napi_value napiValueOfMedia = nullptr;
    if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_ARRAY_BUFFER) {
        MediaAssetManagerNapi::GetByteArrayNapiObject(dataHandler->GetRequestUri(), napiValueOfMedia,
            dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE, env);
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_IMAGE_SOURCE) {
        MediaAssetManagerNapi::GetImageSourceNapiObject(dataHandler->GetRequestUri(), napiValueOfMedia,
            dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE, env);
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_TARGET_PATH) {
        WriteData param;
        param.compatibleMode = dataHandler->GetCompatibleMode();
        param.destUri = dataHandler->GetDestUri();
        param.requestUri = dataHandler->GetRequestUri();
        param.env = env;
        param.isSource = dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE;
        MediaAssetManagerNapi::WriteDataToDestPath(param, napiValueOfMedia, dataHandler->GetRequestId());
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_MOVING_PHOTO) {
        MovingPhotoParam movingPhotoParam;
        movingPhotoParam.compatibleMode =  dataHandler->GetCompatibleMode();
        movingPhotoParam.requestId = dataHandler->GetRequestId();
        movingPhotoParam.progressHandlerRef = dataHandler->GetProgressHandlerRef();
        movingPhotoParam.threadsafeFunction = dataHandler->GetThreadsafeFunction();
        napiValueOfMedia = MovingPhotoNapi::NewMovingPhotoNapi(env, dataHandler->GetRequestUri(),
            dataHandler->GetSourceMode(), movingPhotoParam);
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_PICTURE) {
        MediaAssetManagerNapi::GetPictureNapiObject(dataHandler->GetRequestUri(), napiValueOfMedia,
            dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE, env, isPicture);
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_READY) {
        napi_get_boolean(env, dataHandler->GetCinematicResult(), &napiValueOfMedia);
    } else {
        NAPI_ERR_LOG("source mode type invalid");
    }
    return napiValueOfMedia;
}

void MediaAssetManagerNapi::SetCinematicResult(const std::shared_ptr<NapiMediaAssetDataHandler>& dataHandler,
    AssetHandler *assetHandler)
{
    if (dataHandler->GetReturnDataType() != ReturnDataType::TYPE_READY) {
        return;
    }
    dataHandler->SetCinematicResult(!assetHandler->isCinematicVideoError);
    NAPI_INFO_LOG("isCinematicVideoError: %{public}d, CinematicResult: %{public}d",
        assetHandler->isCinematicVideoError, dataHandler->GetCinematicResult());
}

bool IsSaveCallbackInfoByTranscoder(napi_value napiValueOfMedia, napi_env env, AssetHandler *assetHandler,
    napi_value napiValueOfInfoMap)
{
    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        NAPI_ERR_LOG("data handler is nullptr");
        return false;
    }
    if (napiValueOfMedia == nullptr) {
        napi_get_undefined(env, &napiValueOfMedia);
    }
    bool isTranscoder;
    if (!isTranscoderMap_.Find(assetHandler->requestId, isTranscoder)) {
        NAPI_INFO_LOG("not find key from map");
        isTranscoder = false;
    }
    NAPI_INFO_LOG("IsSaveCallbackInfoByTranscoder isTranscoder_ %{public}d", isTranscoder);
    if (isTranscoder) {
        onPreparedResult_.EnsureInsert(assetHandler->requestId, assetHandler);
        onPreparedResultValue_.EnsureInsert(assetHandler->requestId, napiValueOfMedia);
        return true;
    }
    dataHandler->JsOnDataPrepared(env, napiValueOfMedia, napiValueOfInfoMap);
    return false;
}

static void SavePicture(const std::shared_ptr<NapiMediaAssetDataHandler>& dataHandler)
{
    if (dataHandler->GetReturnDataType() != ReturnDataType::TYPE_ARRAY_BUFFER &&
        dataHandler->GetReturnDataType() != ReturnDataType::TYPE_IMAGE_SOURCE) {
        return;
    }
    string fileUri = dataHandler->GetRequestUri();
    std::string uriStr = PATH_SAVE_PICTURE;
    std::string tempStr = fileUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length());
    std::size_t index = tempStr.find("/");
    std::string fileId = tempStr.substr(0, index);
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_ID, fileId);
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, IMAGE_FILE_TYPE, "1");
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, "uri", fileUri);
    Uri uri(uriStr);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    DataShare::DataSharePredicates predicate;
    UserFileClient::Update(uri, predicate, valuesBucket);
}

void MediaAssetManagerNapi::OnDataPrepared(napi_env env, napi_value cb, void *context, void *data)
{
    NAPI_INFO_LOG("Begin OnDataPrepared.");
    AssetHandler *assetHandler = reinterpret_cast<AssetHandler *>(data);
    CHECK_NULL_PTR_RETURN_VOID(assetHandler, "assetHandler is nullptr");
    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        NAPI_ERR_LOG("data handler is nullptr");
        DeleteAssetHandlerSafe(assetHandler, env);
        return;
    }

    NotifyMode notifyMode = dataHandler->GetNotifyMode();
    if (notifyMode == NotifyMode::FAST_NOTIFY) {
        AssetHandler *tmp;
        if (!inProcessFastRequests.Find(assetHandler->requestId, tmp)) {
            NAPI_ERR_LOG("The request has been canceled");
            DeleteAssetHandlerSafe(assetHandler, env);
            return;
        }
    }

    napi_value napiValueOfInfoMap = nullptr;
    if (assetHandler->needsExtraInfo) {
        napiValueOfInfoMap = GetInfoMapNapiValue(env, assetHandler);
        if (napiValueOfInfoMap == nullptr) {
            NAPI_ERR_LOG("Failed to get info map");
            napi_get_undefined(env, &napiValueOfInfoMap);
        }
    }
    bool isPicture = true;
    SavePicture(dataHandler);
    SetCinematicResult(dataHandler, assetHandler);
    napi_value napiValueOfMedia = assetHandler->isError ? nullptr : GetNapiValueOfMedia(env, dataHandler, isPicture);
    if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_PICTURE) {
        if (isPicture) {
            dataHandler->JsOnDataPrepared(env, napiValueOfMedia, nullptr, napiValueOfInfoMap);
        } else {
            if (napiValueOfMedia == nullptr) {
                napi_get_undefined(env, &napiValueOfMedia);
            }
            dataHandler->JsOnDataPrepared(env, nullptr, napiValueOfMedia, napiValueOfInfoMap);
        }
    } else {
        if (IsSaveCallbackInfoByTranscoder(napiValueOfMedia, env, assetHandler, napiValueOfInfoMap)) {
            return;
        }
    }
    DeleteDataHandler(notifyMode, assetHandler->requestUri, assetHandler->requestId);
    NAPI_DEBUG_LOG("delete assetHandler");
    DeleteAssetHandlerSafe(assetHandler, env);
}

void CallPreparedCallbackAfterProgress(napi_env env, ProgressHandler *progressHandler, napi_value napiValueOfMedia)
{
    MediaCallTranscode::CallTranscodeRelease(progressHandler->requestId);
    MediaAssetManagerNapi::progressHandlerMap_.Erase(progressHandler->requestId);
    AssetHandler *assetHandler = nullptr;
    if (!onPreparedResult_.Find(progressHandler->requestId, assetHandler)) {
        NAPI_ERR_LOG("not find key from map");
        return;
    }
    onPreparedResult_.Erase(progressHandler->requestId);
    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        NAPI_ERR_LOG("data handler is nullptr");
        DeleteAssetHandlerSafe(assetHandler, env);
        return;
    }

    NotifyMode notifyMode = dataHandler->GetNotifyMode();
    napi_value napiValueOfInfoMap = nullptr;
    if (assetHandler->needsExtraInfo) {
        napiValueOfInfoMap = GetInfoMapNapiValue(env, assetHandler);
        if (napiValueOfInfoMap == nullptr) {
            NAPI_ERR_LOG("Failed to get info map");
            napi_get_undefined(env, &napiValueOfInfoMap);
        }
    }
    dataHandler->JsOnDataPrepared(env, napiValueOfMedia, napiValueOfInfoMap);
    NAPI_INFO_LOG("delete assetHandler");
    DeleteProcessHandlerSafe(progressHandler, env);
    DeleteDataHandler(notifyMode, assetHandler->requestUri, assetHandler->requestId);
    DeleteAssetHandlerSafe(assetHandler, env);
}

void CallProgressCallback(napi_env env, ProgressHandler* progressHandler, int32_t process)
{
    if (progressHandler == nullptr) {
        NAPI_ERR_LOG("progressHandler is nullptr");
        return;
    }
    napi_value result;
    napi_status status = napi_create_int32(env, process, &result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("OnProgress napi_create_int32 fail");
    }
    napi_value callback;
    if (process < 0 || process > PROGRESS_MAX || progressHandler->progressRef == nullptr) {
        NAPI_ERR_LOG("progressHandler->progressRef is nullptr or process out of range");
        return;
    }
    status = napi_get_reference_value(env, progressHandler->progressRef, &callback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("OnProgress napi_get_reference_value fail, napi status: %{public}d",
            static_cast<int>(status));
        DeleteProcessHandlerSafe(progressHandler, env);
        return;
    }
    napi_value jsOnProgress;
    status = napi_get_named_property(env, callback, ON_PROGRESS_FUNC, &jsOnProgress);
    if (status != napi_ok) {
        NAPI_ERR_LOG("jsOnProgress napi_get_named_property fail, napi status: %{public}d",
            static_cast<int>(status));
        DeleteProcessHandlerSafe(progressHandler, env);
        return;
    }
    constexpr size_t maxArgs = 1;
    napi_value argv[maxArgs];
    size_t argc = ARGS_ONE;
    argv[PARAM0] = result;
    argc = ARGS_ONE;
    napi_value promise;
    status = napi_call_function(env, nullptr, jsOnProgress, argc, argv, &promise);
    if (status != napi_ok) {
        NAPI_ERR_LOG("call js function failed %{public}d", static_cast<int32_t>(status));
        NapiError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
    }
    NAPI_INFO_LOG("CallProgressCallback process %{public}d", process);
}

void MediaAssetManagerNapi::OnProgress(napi_env env, napi_value cb, void *context, void *data)
{
    ProgressHandler *progressHandler = reinterpret_cast<ProgressHandler *>(data);
    if (progressHandler == nullptr) {
        NAPI_ERR_LOG("progressHandler handler is nullptr");
        DeleteProcessHandlerSafe(progressHandler, env);
        return;
    }
    double process = progressHandler->outputResult;
    int32_t type = progressHandler->retProgressValue.type;

    MEDIA_INFO_LOG("OnProgress begin, type: %{public}d, requestId: %{public}s.",
        type, progressHandler->requestId.c_str());
    if (type == INFO_TYPE_TRANSCODER_COMPLETED || type == INFO_TYPE_ERROR) {
        bool isTranscoder;
        if (isTranscoderMap_.Find(progressHandler->requestId, isTranscoder)) {
            isTranscoderMap_.Erase(progressHandler->requestId);
        }

        napi_value napiValueOfMedia;
        if (onPreparedResultValue_.Find(progressHandler->requestId, napiValueOfMedia)) {
            onPreparedResultValue_.Erase(progressHandler->requestId);
        }
        if (type == INFO_TYPE_ERROR) {
            napi_get_boolean(env, false, &napiValueOfMedia);
        }
        NAPI_INFO_LOG("CallPreparedCallbackAfterProgress type %{public}d, requestId: %{public}s.",
            type, progressHandler->requestId.c_str());
        CallPreparedCallbackAfterProgress(env, progressHandler, napiValueOfMedia);
        return;
    }
    if (progressHandler->progressRef == nullptr) {
        NAPI_INFO_LOG("progressHandler->progressRef == nullptr");
        return;
    }
    CallProgressCallback(env, progressHandler, process);
}

void MediaAssetManagerNapi::NotifyMediaDataPrepared(AssetHandler *assetHandler)
{
    napi_status status = napi_call_threadsafe_function(assetHandler->threadSafeFunc, (void *)assetHandler,
        napi_tsfn_blocking);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_call_threadsafe_function fail, %{public}d", static_cast<int32_t>(status));
        napi_release_threadsafe_function(assetHandler->threadSafeFunc, napi_tsfn_release);
        DeleteAssetHandlerSafe(assetHandler, nullptr);
    }
}

std::shared_ptr<MultistagesCaptureNotifyServerInfo> MultistagesCaptureOnProcessObserver::ConvertWrapperToNotifyInfo(
    const UserDefineCallbackWrapper &wrapper)
{
    if (wrapper.userDefineInfo_ == nullptr ||
        wrapper.userDefineInfo_->notifyUserDefineType_ != NotifyForUserDefineType::MULTISTAGES_CAPTURE) {
        NAPI_WARN_LOG("Wrapper is invalid.");
        return nullptr;
    }
 
    auto notifyBody = wrapper.userDefineInfo_->GetUserDefineNotifyBody();
    if (notifyBody == nullptr) {
        NAPI_ERR_LOG("NotifyBody is nullptr.");
        return nullptr;
    }
 
    auto notifyInfo = static_pointer_cast<MultistagesCaptureNotifyServerInfo>(notifyBody);
    if (notifyInfo == nullptr) {
        NAPI_ERR_LOG("NotifyInfo is nullptr.");
        return nullptr;
    }
 
    int32_t notifyType = static_cast<int32_t>(notifyInfo->notifyType_);
    if (notifyType <= static_cast<int32_t>(MultistagesCaptureNotifyType::UNDEFINED) ||
        notifyType >= static_cast<int32_t>(MultistagesCaptureNotifyType::NOTIFY_END)) {
        NAPI_ERR_LOG("NotifyType is invalid.");
        return nullptr;
    }
    return notifyInfo;
}
 
bool MultistagesCaptureOnProcessObserver::MatchNotifyToObserver(
    const MultistagesCaptureNotifyType &notifyType, const ObserverType &observerType)
{
    if (MATCH_NOTIFY_TO_OBSERVER.find(notifyType) == MATCH_NOTIFY_TO_OBSERVER.end()) {
        NAPI_ERR_LOG("NotifyType is invalid.");
        return false;
    }
    std::vector<ObserverType> observers = MATCH_NOTIFY_TO_OBSERVER.at(notifyType);
    for (const auto observer : observers) {
        if (observer == observerType) {
            NAPI_INFO_LOG("Need notify.");
            return true;
        }
    }
    return false;
}

void MultistagesCaptureOnProcessObserver::OnChange(const UserDefineCallbackWrapper &wrapper)
{
    NAPI_INFO_LOG("MultistagesCapture, OnChange called, %{public}s.", ToString().c_str());
    std::shared_ptr<MultistagesCaptureNotifyServerInfo> notifyInfo = ConvertWrapperToNotifyInfo(wrapper);
    if (notifyInfo == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    if (inProcessUriMap.find(notifyInfo->uri_) == inProcessUriMap.end()) {
        NAPI_ERR_LOG("Current uri does not in process, uri: %{public}s", uri_.c_str());
        return;
    }
    std::map<std::string, AssetHandler *> assetHandlers = inProcessUriMap[notifyInfo->uri_];
    for (auto handler : assetHandlers) {
        auto assetHandler = handler.second;
        if (!MatchNotifyToObserver(notifyInfo->notifyType_, assetHandler->observerType)) {
            continue;
        }
        assetHandler->photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
        if (notifyInfo->notifyType_ == MultistagesCaptureNotifyType::ON_ERROR_IMAGE ||
            notifyInfo->notifyType_ == MultistagesCaptureNotifyType::ON_ERROR_VIDEO) {
            NAPI_WARN_LOG("OnChange receive high_temperature");
            assetHandler->isError = true;
        }
        MediaAssetManagerNapi::NotifyMediaDataPrepared(assetHandler);
        DeleteRecordNoLock(handler.second->requestUri, handler.second->requestId);
    }
}

bool OnHandleProgressOutputResult(int32_t type, int32_t progress, ProgressHandler *progressHandler)
{
    if (progressHandler == nullptr) {
        MEDIA_ERR_LOG("ProgressHandler is nullptr.");
        return false;
    }

    MEDIA_INFO_LOG("OnHandleProgressOutputResult type: %{public}d, progress: %{public}d.", type, progress);
    progressHandler->retProgressValue.type = type;
    if (type == static_cast<int32_t>(ProgressReturnInfoType::CAMERA_PROGRESS)) {
        if (progressHandler->progressMode == ProgressMode::ONLY_FOR_CAMERA) {
            progressHandler->cameraProgress = progress;
            progressHandler->outputResult = progress;
        } else if (progressHandler->progressMode == ProgressMode::CAMERA_AND_TRANSCODING) {
            progressHandler->cameraProgress = progress;
            progressHandler->outputResult =
                (progressHandler->retProgressValue.progress + progressHandler->cameraProgress) / AVERAGE_FACTOR;
        } else {
            MEDIA_ERR_LOG("Failed to OnHandleProgressOutputResult, type: %{public}d.", type);
            return false;
        }
    } else {
        if (progressHandler->progressMode == ProgressMode::ONLY_FOR_TRANSCODING) {
            progressHandler->retProgressValue.progress = progress;
            progressHandler->outputResult = progress;
        } else if (progressHandler->progressMode == ProgressMode::CAMERA_AND_TRANSCODING) {
            progressHandler->retProgressValue.progress = progress;
            progressHandler->outputResult =
                (progressHandler->retProgressValue.progress + progressHandler->cameraProgress) / AVERAGE_FACTOR;
        } else {
            MEDIA_ERR_LOG("Failed to OnHandleProgressOutputResult, type: %{public}d.", type);
            return false;
        }
    }
    return true;
}

void MediaAssetManagerNapi::NotifyOnProgress(int32_t type, int32_t progress, std::string requestId)
{
    NAPI_DEBUG_LOG("NotifyOnProgress start %{public}d, type:%{public}d, requestId:%{public}s", progress, type,
        requestId.c_str());
    ProgressHandler *progressHandler = nullptr;
    if (!MediaAssetManagerNapi::progressHandlerMap_.Find(requestId, progressHandler)) {
        NAPI_ERR_LOG("not find key from map");
        return;
    }
    if (progressHandler == nullptr) {
        NAPI_ERR_LOG("ProgressHandler is nullptr.");
        return;
    }

    if (!OnHandleProgressOutputResult(type, progress, progressHandler)) {
        return;
    }
    napi_status status = napi_acquire_threadsafe_function(progressHandler->progressFunc);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_acquire_threadsafe_function fail, status: %{public}d", static_cast<int32_t>(status));
        return;
    }
    status = napi_call_threadsafe_function(progressHandler->progressFunc, (void *)progressHandler, napi_tsfn_blocking);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_call_threadsafe_function fail, status: %{public}d", static_cast<int32_t>(status));
        DeleteProcessHandlerSafe(progressHandler, progressHandler->env);
        return;
    }
}

void MediaAssetManagerNapi::GetImageSourceNapiObject(const std::string &fileUri, napi_value &imageSourceNapiObj,
    bool isSource, napi_env env)
{
    if (env == nullptr) {
        NAPI_ERR_LOG(" create image source object failed, need to initialize js env");
        return;
    }
    napi_value tempImageSourceNapi;
    ImageSourceNapi::CreateImageSourceNapi(env, &tempImageSourceNapi);
    ImageSourceNapi* imageSourceNapi = nullptr;
    napi_unwrap(env, tempImageSourceNapi, reinterpret_cast<void**>(&imageSourceNapi));
    if (imageSourceNapi == nullptr) {
        NAPI_ERR_LOG("unwrap image napi object failed");
        return;
    }
    std::string tmpUri = fileUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
        NAPI_INFO_LOG("request source image's imageSource");
    }
    MediaFileUtils::UriAppendKeyValue(tmpUri, PHOTO_TRANSCODE_OPERATION, OPRN_TRANSCODE_HEIF);
    Uri uri(tmpUri);
    int fd = UserFileClient::OpenFile(uri, "r");
    if (fd < 0) {
        NAPI_ERR_LOG("get image fd failed, errno: %{public}d", errno);
        return;
    }

    SourceOptions opts;
    uint32_t errCode = 0;
    auto nativeImageSourcePtr = ImageSource::CreateImageSource(fd, opts, errCode);
    close(fd);
    if (nativeImageSourcePtr == nullptr) {
        NAPI_ERR_LOG("get ImageSource::CreateImageSource failed nullptr, errCode:%{public}d", errCode);
        return;
    }
    imageSourceNapi->SetNativeImageSource(std::move(nativeImageSourcePtr));
    imageSourceNapiObj = tempImageSourceNapi;
}

void MediaAssetManagerNapi::GetPictureNapiObject(const std::string &fileUri, napi_value &pictureNapiObj,
    bool isSource, napi_env env,  bool& isPicture)
{
    if (env == nullptr) {
        NAPI_ERR_LOG(" create image source object failed, need to initialize js env");
        return;
    }
    NAPI_DEBUG_LOG("GetPictureNapiObject");

    std::string tempStr = fileUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length());
    std::size_t index = tempStr.find("/");
    std::string fileId = tempStr.substr(0, index);
    auto pic = PictureHandlerClient::RequestPicture(std::atoi(fileId.c_str()));
    if (pic == nullptr) {
        NAPI_ERR_LOG("picture is null");
        isPicture = false;
        GetImageSourceNapiObject(fileUri, pictureNapiObj, isSource, env);
        return;
    }
    NAPI_ERR_LOG("picture is not null");
    napi_value tempPictureNapi;
    PictureNapi::CreatePictureNapi(env, &tempPictureNapi);
    PictureNapi* pictureNapi = nullptr;
    napi_unwrap(env, tempPictureNapi, reinterpret_cast<void**>(&pictureNapi));
    if (pictureNapi == nullptr) {
        NAPI_ERR_LOG("GetPictureNapiObject unwrap image napi object failed");
        return;
    }
    pictureNapi->SetNativePicture(pic);
    pictureNapiObj = tempPictureNapi;
}


void MediaAssetManagerNapi::GetByteArrayNapiObject(const std::string &requestUri, napi_value &arrayBuffer,
    bool isSource, napi_env env)
{
    if (env == nullptr) {
        NAPI_ERR_LOG("create byte array object failed, need to initialize js env");
        return;
    }
    
    std::string tmpUri = requestUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    Uri uri(tmpUri);
    int imageFd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    if (imageFd < 0) {
        NAPI_ERR_LOG("get image fd failed, %{public}d", errno);
        return;
    }
    ssize_t imgLen = lseek(imageFd, 0, SEEK_END);
    void* buffer = nullptr;
    if (napi_create_arraybuffer(env, imgLen, &buffer, &arrayBuffer) != napi_ok) {
        NAPI_ERR_LOG("create napi arraybuffer failed");
        close(imageFd);
        return;
    }
    lseek(imageFd, 0, SEEK_SET);
    ssize_t readRet = read(imageFd, buffer, imgLen);
    close(imageFd);
    if (readRet != imgLen) {
        NAPI_ERR_LOG("read image failed");
        return;
    }
}

bool IsMovingPhoto(int32_t photoSubType, int32_t effectMode, int32_t sourceMode)
{
    return photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        (MediaLibraryNapiUtils::IsSystemApp() && sourceMode == static_cast<int32_t>(SourceMode::ORIGINAL_MODE) &&
        effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}

static napi_value ParseArgsForRequestMovingPhoto(napi_env env, size_t argc, const napi_value argv[],
    unique_ptr<MediaAssetManagerAsyncContext> &context)
{
    CHECK_COND_WITH_MESSAGE(env, (argc == ARGS_FOUR), "Invalid number of arguments");

    FileAssetNapi *fileAssetNapi = nullptr;
    CHECK_COND_WITH_MESSAGE(env,
        (napi_unwrap(env, argv[PARAM1], reinterpret_cast<void**>(&fileAssetNapi)) == napi_ok),
        "Failed to parse photo asset");
    CHECK_COND_WITH_MESSAGE(env, fileAssetNapi != nullptr, "Failed to parse photo asset");
    auto fileAssetPtr = fileAssetNapi->GetFileAssetInstance();
    CHECK_COND_WITH_MESSAGE(env, fileAssetPtr != nullptr, "fileAsset is null");
    context->photoUri = fileAssetPtr->GetUri();
    context->fileId = fileAssetPtr->GetId();
    context->returnDataType = ReturnDataType::TYPE_MOVING_PHOTO;
    context->hasReadPermission = HasReadPermission();
    context->subType = PhotoSubType::MOVING_PHOTO;
    context->userId = fileAssetPtr->GetUserId();

    CHECK_COND_WITH_MESSAGE(env,
        ParseArgGetRequestOption(env, argv[PARAM2], context->deliveryMode, context->sourceMode) == napi_ok,
        "Failed to parse request option");
    CHECK_COND_WITH_MESSAGE(env,
        ParseArgGetRequestOptionMore(env, argv[PARAM2], context->compatibleMode,
        context->mediaAssetProgressHandler) == napi_ok, "Failed to parse request option more");
    CHECK_COND_WITH_MESSAGE(env, IsMovingPhoto(fileAssetPtr->GetPhotoSubType(),
        fileAssetPtr->GetMovingPhotoEffectMode(), static_cast<int32_t>(context->sourceMode)),
        "Asset is not a moving photo");
    if (fileAssetPtr->GetUserId() != -1) {
        MediaFileUtils::UriAppendKeyValue(context->photoUri, "user", to_string(fileAssetPtr->GetUserId()));
    }
    if (ParseArgGetDataHandler(env, argv[PARAM3], context->dataHandler, context->needsExtraInfo) != napi_ok) {
        NAPI_ERR_LOG("requestMovingPhoto ParseArgGetDataHandler error");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMovingPhoto ParseArgGetDataHandler error");
        return nullptr;
    }

    RETURN_NAPI_TRUE(env);
}

napi_status MediaAssetManagerNapi::CreateMovingPhotoThreadSafeFunc(napi_env env,
    unique_ptr<MediaAssetManagerAsyncContext> &context, napi_threadsafe_function &progressFunc)
{
    if (env == nullptr || context == nullptr) {
        NAPI_ERR_LOG("env or context is nullptr");
        return napi_invalid_arg;
    }
    napi_value workName = nullptr;
    napi_create_string_utf8(env, "ProgressThread", NAPI_AUTO_LENGTH, &workName);
    napi_status status = napi_ok;
    status = napi_create_threadsafe_function(env, context->mediaAssetProgressHandler, NULL, workName, 0, 1,
        NULL, NULL, NULL, MovingPhotoCallTranscoder::OnProgress, &progressFunc);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_create_threadsafe_function fail");
        progressFunc = nullptr;
        NapiError::ThrowError(env, JS_INNER_FAIL, "napi_create_threadsafe_function fail");
    }
    return status;
}

bool MediaAssetManagerNapi::CreateMovingPhotoHandlerInfo(napi_env env,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext)
{
    if (env == nullptr || asyncContext == nullptr) {
        NAPI_ERR_LOG("env or asyncContext is nullptr");
        return false;
    }
    if (asyncContext->compatibleMode != CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        return true;
    }
    if (asyncContext->mediaAssetProgressHandler == nullptr) {
        if (CreateMovingPhotoThreadSafeFunc(env, asyncContext, asyncContext->onProgressPtr) != napi_ok) {
            NAPI_ERR_LOG("CreateMovingPhotoThreadSafeFunc failed");
            return false;
        }
        return true;
    }
    if (CreateProgressHandlerRef(env, asyncContext, asyncContext->progressHandlerRef) != napi_ok ||
        CreateMovingPhotoThreadSafeFunc(env, asyncContext, asyncContext->onProgressPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateProgressHandlerRef or CreateMovingPhotoThreadSafeFunc failed");
        return false;
    }
    return true;
}

napi_value MediaAssetManagerNapi::JSRequestMovingPhoto(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSRequestMovingPhoto");

    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    CHECK_ARGS(env, napi_get_cb_info(env, info, &(asyncContext->argc), asyncContext->argv, nullptr, nullptr),
        JS_INNER_FAIL);
    CHECK_NULLPTR_RET(ParseArgsForRequestMovingPhoto(env, asyncContext->argc, asyncContext->argv, asyncContext));
    CHECK_COND(env, InitUserFileClient(env, info, asyncContext->userId), JS_INNER_FAIL);
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, asyncContext, asyncContext->dataHandlerRef2) != napi_ok
            || CreateOnDataPreparedThreadSafeFunc(env, asyncContext, asyncContext->onDataPreparedPtr2) != napi_ok) {
        NAPI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (!CreateMovingPhotoHandlerInfo(env, asyncContext)) {
        NAPI_ERR_LOG("CreateMovingPhotoHandlerInfo failed");
        return nullptr;
    }
    asyncContext->requestId = GenerateRequestId();
    asyncContext->observerType = ObserverType::REQUEST_IMAGE;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSRequestMovingPhoto", JSRequestExecute,
        JSRequestComplete);
}

void MediaAssetManagerNapi::WriteDataToDestPath(WriteData &writeData, napi_value &resultNapiValue,
    std::string requestId)
{
    if (writeData.env == nullptr) {
        NAPI_ERR_LOG("create byte array object failed, need to initialize js env");
        return;
    }
    if (writeData.requestUri.empty() || writeData.destUri.empty()) {
        napi_get_boolean(writeData.env, false, &resultNapiValue);
        NAPI_ERR_LOG("requestUri or responseUri is nullptr");
        return;
    }
    std::string tmpUri = writeData.requestUri;
    if (writeData.isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    Uri srcUri(tmpUri);
    int srcFd = UserFileClient::OpenFile(srcUri, MEDIA_FILEMODE_READONLY);
    if (srcFd < 0) {
        napi_get_boolean(writeData.env, false, &resultNapiValue);
        NAPI_ERR_LOG("get source file fd failed %{public}d", srcFd);
        return;
    }
    UniqueFd uniqueSrcFd(srcFd);
    struct stat statSrc;
    if (fstat(uniqueSrcFd.Get(), &statSrc) == -1) {
        napi_get_boolean(writeData.env, false, &resultNapiValue);
        NAPI_DEBUG_LOG("File get stat failed, %{public}d", errno);
        return;
    }
    int destFd = GetFdFromSandBoxUri(writeData.destUri);
    if (destFd < 0) {
        napi_get_boolean(writeData.env, false, &resultNapiValue);
        NAPI_ERR_LOG("get dest fd failed %{public}d", destFd);
        return;
    }
    UniqueFd uniqueDestFd(destFd);
    NAPI_INFO_LOG("WriteDataToDestPath compatibleMode %{public}d", writeData.compatibleMode);
    if (writeData.compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        isTranscoderMap_.Insert(requestId, true);
        MediaCallTranscode::RegisterCallback(NotifyOnProgress);
        MediaCallTranscode::CallTranscodeHandle(writeData.env, uniqueSrcFd.Get(), uniqueDestFd.Get(), resultNapiValue,
            statSrc.st_size, requestId);
    } else {
        SendFile(writeData.env, uniqueSrcFd.Get(), uniqueDestFd.Get(), resultNapiValue, statSrc.st_size);
    }
    return;
}

void MediaAssetManagerNapi::SendFile(napi_env env, int srcFd, int destFd, napi_value &result, off_t fileSize)
{
    if (srcFd < 0 || destFd < 0) {
        NAPI_ERR_LOG("srcFd or destFd is invalid");
        napi_get_boolean(env, false, &result);
        return;
    }
    if (sendfile(destFd, srcFd, nullptr, fileSize) == -1) {
        close(srcFd);
        close(destFd);
        napi_get_boolean(env, false, &result);
        NAPI_ERR_LOG("send file failed, %{public}d", errno);
        return;
    }
    napi_get_boolean(env, true, &result);
}

static bool IsFastRequestCanceled(const std::string &requestId, std::string &photoId)
{
    AssetHandler *assetHandler = nullptr;
    if (!inProcessFastRequests.Find(requestId, assetHandler)) {
        NAPI_ERR_LOG("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        NAPI_ERR_LOG("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    inProcessFastRequests.Erase(requestId);
    return true;
}

static bool IsMapRecordCanceled(const std::string &requestId, std::string &photoId,
    std::string &requestUri, napi_env env)
{
    AssetHandler *assetHandler = nullptr;
    std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
    if (!IsInProcessInMapRecord(requestId, assetHandler)) {
        NAPI_ERR_LOG("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        NAPI_ERR_LOG("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    requestUri = assetHandler->requestUri;
    DeleteInProcessMapRecord(assetHandler->requestUri, assetHandler->requestId);
    DeleteAssetHandlerSafe(assetHandler, env);
    return true;
}

void MediaAssetManagerNapi::DeleteProgressHandler(const std::string &requestId)
{
    ProgressHandler *progressHandler = nullptr;
    if (MediaAssetManagerNapi::progressHandlerMap_.Find(requestId, progressHandler)) {
        MediaAssetManagerNapi::progressHandlerMap_.Erase(requestId);
    }
}

napi_value MediaAssetManagerNapi::JSCancelRequest(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO];
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_TWO), "requires 2 paramters");

    string requestId;
    CHECK_ARGS_THROW_INVALID_PARAM(env,
        MediaLibraryNapiUtils::GetParamStringWithLength(env, argv[ARGS_ONE], REQUEST_ID_MAX_LEN, requestId));
    std::string photoId = "";
    std::string requestUri = "";
    bool hasFastRequestInProcess = IsFastRequestCanceled(requestId, photoId);
    bool hasMapRecordInProcess = IsMapRecordCanceled(requestId, photoId, requestUri, env);
    if (hasMapRecordInProcess) {
        DeleteProgressHandler(requestId);
    }

    if (hasFastRequestInProcess || hasMapRecordInProcess) {
        unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
        asyncContext->photoId = photoId;
        asyncContext->photoUri = requestUri;
        asyncContext->requestId = requestId;
        return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSCancelRequest", JSCancelRequestExecute,
            JSCancelRequestComplete);
    }
    RETURN_NAPI_UNDEFINED(env);
}

static napi_value ParseArgsForLoadMovingPhoto(napi_env env, size_t argc, const napi_value argv[],
    unique_ptr<MediaAssetManagerAsyncContext> &context)
{
    CHECK_COND_WITH_MESSAGE(env, (argc == ARGS_THREE), "Invalid number of arguments");

    std::string imageFileUri;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], imageFileUri) == napi_ok,
        "Failed to parse image file uri");
    std::string videoFileUri;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM2], videoFileUri) == napi_ok,
        "Failed to parse video file uri");
    std::string uri(imageFileUri + MOVING_PHOTO_URI_SPLIT + videoFileUri);
    context->photoUri = uri;
    RETURN_NAPI_TRUE(env);
}

static void JSLoadMovingPhotoComplete(napi_env env, napi_status status, void *data)
{
    MediaAssetManagerAsyncContext *context = static_cast<MediaAssetManagerAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    MediaLibraryTracer tracer;
    tracer.Start("JSLoadMovingPhotoComplete");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        MovingPhotoParam movingPhotoParam;
        movingPhotoParam.compatibleMode = CompatibleMode::ORIGINAL_FORMAT_MODE;
        movingPhotoParam.requestId = context->requestId;
        napi_value movingPhoto = MovingPhotoNapi::NewMovingPhotoNapi(env, context->photoUri,
            SourceMode::EDITED_MODE, movingPhotoParam);
        jsContext->data = movingPhoto;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static void JSLoadMovingPhotoExecute(napi_env env, void *data)
{
}

napi_value MediaAssetManagerNapi::JSLoadMovingPhoto(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaAssetManagerAsyncContext> asyncContext = make_unique<MediaAssetManagerAsyncContext>();
    CHECK_ARGS(env, napi_get_cb_info(env, info, &(asyncContext->argc), asyncContext->argv, nullptr, nullptr),
        JS_INNER_FAIL);
    CHECK_NULLPTR_RET(ParseArgsForLoadMovingPhoto(env, asyncContext->argc, asyncContext->argv, asyncContext));
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSLoadMovingPhoto", JSLoadMovingPhotoExecute,
        JSLoadMovingPhotoComplete);
}

int32_t MediaAssetManagerNapi::GetFdFromSandBoxUri(const std::string &sandBoxUri)
{
    AppFileService::ModuleFileUri::FileUri destUri(sandBoxUri);
    string destPath = destUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        NAPI_DEBUG_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    string absDestPath;
    if (!PathToRealPath(destPath, absDestPath)) {
        NAPI_DEBUG_LOG("PathToRealPath failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    return MediaFileUtils::OpenFile(absDestPath, MEDIA_FILEMODE_WRITETRUNCATE);
}

napi_status MediaAssetManagerNapi::CreateDataHandlerRef(napi_env env,
    const unique_ptr<MediaAssetManagerAsyncContext> &context, napi_ref &dataHandlerRef)
{
    napi_status status = napi_create_reference(env, context->dataHandler, PARAM1, &dataHandlerRef);
    if (status != napi_ok) {
        dataHandlerRef = nullptr;
        NAPI_ERR_LOG("napi_create_reference failed");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "napi_create_reference fail");
    }
    return status;
}

napi_status MediaAssetManagerNapi::CreateProgressHandlerRef(napi_env env,
    const unique_ptr<MediaAssetManagerAsyncContext> &context, napi_ref &dataHandlerRef)
{
    napi_status status = napi_create_reference(env, context->mediaAssetProgressHandler, PARAM1, &dataHandlerRef);
    if (status != napi_ok) {
        dataHandlerRef = nullptr;
        NAPI_ERR_LOG("napi_create_reference failed");
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "napi_create_reference fail");
    }
    return status;
}

napi_status MediaAssetManagerNapi::CreateOnDataPreparedThreadSafeFunc(napi_env env,
    const unique_ptr<MediaAssetManagerAsyncContext> &context, napi_threadsafe_function &threadSafeFunc)
{
    NAPI_DEBUG_LOG("CreateOnDataPreparedThreadSafeFunc");
    napi_value workName = nullptr;
    napi_create_string_utf8(env, "Data Prepared", NAPI_AUTO_LENGTH, &workName);
    napi_status status = napi_create_threadsafe_function(env, context->dataHandler, NULL, workName, 0, 1,
        NULL, NULL, NULL, MediaAssetManagerNapi::OnDataPrepared, &threadSafeFunc);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_create_threadsafe_function fail");
        threadSafeFunc = nullptr;
        NapiError::ThrowError(env, JS_INNER_FAIL, "napi_create_threadsafe_function fail");
    }
    return status;
}

napi_status MediaAssetManagerNapi::CreateOnProgressThreadSafeFunc(napi_env env,
    unique_ptr<MediaAssetManagerAsyncContext> &context, napi_threadsafe_function &progressFunc)
{
    napi_value workName = nullptr;
    napi_create_string_utf8(env, "ProgressThread", NAPI_AUTO_LENGTH, &workName);
    napi_status status = napi_ok;
    status = napi_create_threadsafe_function(env, context->mediaAssetProgressHandler, NULL, workName, 0, 1,
        NULL, NULL, NULL, MediaAssetManagerNapi::OnProgress, &progressFunc);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_create_threadsafe_function fail");
        progressFunc = nullptr;
        NapiError::ThrowError(env, JS_INNER_FAIL, "napi_create_threadsafe_function fail");
    }
    return status;
}

void MediaAssetManagerNapi::JSRequestExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSRequestExecute");
    MediaAssetManagerAsyncContext *context = static_cast<MediaAssetManagerAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    OnHandleRequestImage(env, context);
    if (context->subType == PhotoSubType::MOVING_PHOTO) {
        string uri = LOG_MOVING_PHOTO;
        Uri logMovingPhotoUri(uri);
        DataShare::DataShareValuesBucket valuesBucket;
        string result;
        valuesBucket.Put("adapted", context->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO);
        AdaptedReqBody reqBody;
        reqBody.adapted = context->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO;
        IPC::UserDefineIPCClient().SetUserId(context->userId).Call(
            static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_MOVING_PHOTO), reqBody);
    }
}

ProgressMode GetProgressMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext)
{
    if (asyncContext->compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        if (asyncContext->subType == PhotoSubType::CINEMATIC_VIDEO) {
            if (asyncContext->sourceMode == SourceMode::EDITED_MODE) {
                return ProgressMode::CAMERA_AND_TRANSCODING;
            }
            return ProgressMode::ONLY_FOR_CAMERA;
        } else {
            return ProgressMode::ONLY_FOR_TRANSCODING;
        }
    } else {
        if (asyncContext->subType == PhotoSubType::CINEMATIC_VIDEO) {
            return ProgressMode::ONLY_FOR_CAMERA;
        }
        if (asyncContext->subType == PhotoSubType::SLOW_MOTION_VIDEO) {
            return ProgressMode::ONLY_FOR_TRANSCODING;
        }
    }
    return ProgressMode::UNDEFINED;
}

static void CheckProgressMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext,
    ProgressMode &progressMode)
{
    if (asyncContext->photoQuality == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS &&
            progressMode == ProgressMode::CAMERA_AND_TRANSCODING) {
        progressMode = ProgressMode::ONLY_FOR_TRANSCODING;
    }
}

CameraProgressMode GetCameraProgressMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext,
    ProgressMode progressMode)
{
    if (progressMode == ProgressMode::ONLY_FOR_TRANSCODING || progressMode == ProgressMode::UNDEFINED) {
        return CameraProgressMode::UNDEFINED;
    } else if (progressMode == ProgressMode::ONLY_FOR_CAMERA || progressMode == ProgressMode::CAMERA_AND_TRANSCODING) {
        switch (asyncContext->deliveryMode) {
            case DeliveryMode::FAST:
            case DeliveryMode::BALANCED_MODE:
                return CameraProgressMode::IMMEDIATELY_MODE;
            case DeliveryMode::HIGH_QUALITY:
                if (asyncContext->photoQuality == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
                    return CameraProgressMode::IMMEDIATELY_MODE;
                }
                return CameraProgressMode::WAIT_FOR_POLLOT;
        }
    }
    return CameraProgressMode::UNDEFINED;
}

void MediaAssetManagerNapi::OnHandleProgressForRequest(
    napi_env env, MediaAssetManagerAsyncContext *asyncContext, CameraProgressMode &cameraProgressMode)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");
    MultiStagesCapturePhotoStatus status = MediaAssetManagerNapi::QueryPhotoStatus(asyncContext->fileId,
        asyncContext->photoUri, asyncContext->photoId, asyncContext->hasReadPermission, asyncContext->userId);
    asyncContext->photoQuality = status;
    NAPI_INFO_LOG("OnHandleProgressForRequest mode: %{public}d.", asyncContext->deliveryMode);
    ProgressMode progressMode = GetProgressMode(env, asyncContext);
    CheckProgressMode(env, asyncContext, progressMode);
    cameraProgressMode = GetCameraProgressMode(env, asyncContext, progressMode);

    MediaAssetManagerNapi::OnHandleProgress(env, asyncContext, progressMode, cameraProgressMode);
}

void MediaAssetManagerNapi::JSRequestVideoFileExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSRequestVideoFileExecute");
    MediaAssetManagerAsyncContext *context = static_cast<MediaAssetManagerAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    OnHandleRequestVideo(env, context);
    CameraProgressMode cameraProgressMode = CameraProgressMode::UNDEFINED;
    OnHandleProgressForRequest(env, context, cameraProgressMode);
    if (cameraProgressMode == CameraProgressMode::WAIT_FOR_POLLOT && context->onProgressPtr != nullptr) {
        StartThreadForProgress();
    }
}

void MediaAssetManagerNapi::Run()
{
    MEDIA_INFO_LOG("enter thread run:");
    string name("VideoProgressThread");
    pthread_setname_np(pthread_self(), name.c_str());
    while (!stopFlag_) {
        {
            std::unique_lock<mutex> locker(sleepMutex_);
            condition_.wait_for(locker, std::chrono::milliseconds(SLEEP_100_MS)); // 实际100 ms扫描一次
        }
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFINE_GET_PROGRESS_CALLBACK);
        GetProgressCallbackReqBody reqBody;
        GetProgressCallbackRespBody respBody;
        IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
        if (respBody.progressMap.empty()) {
            pauseFlag_ = true;
            MEDIA_INFO_LOG("VideoProgressThread end.");
            return;
        }
        for (auto [requestId, progress] : respBody.progressMap) {
            MEDIA_INFO_LOG("VideoProgressThread requestId: %{public}s, progress: %{public}f",
                requestId.c_str(), progress);
            NotifyOnProgress(static_cast<int32_t>(ProgressReturnInfoType::CAMERA_PROGRESS),
                static_cast<int32_t>(progress * PROGRESS_MAX), requestId);
        }
    }
}

void MediaAssetManagerNapi::StopThreadForProgress()
{
    MEDIA_INFO_LOG("enter StopThreadForProgress");
    if (progressThread_ != nullptr) {
        pauseFlag_ = false;
        stopFlag_ = true;
        condition_.notify_all();  // Notify one waiting thread, if there is one.
        if (progressThread_->joinable()) {
            progressThread_->join(); // wait for thread finished
        }
        progressThread_ = nullptr;
    }
}

void MediaAssetManagerNapi::StartThreadForProgress()
{
    std::unique_lock<mutex> locker(runningMutex_);
    if (pauseFlag_) {
        StopThreadForProgress();
    }
    if (progressThread_ == nullptr) {
        pauseFlag_ = false;
        stopFlag_ = false;
        progressThread_ = std::make_unique<std::thread>(&MediaAssetManagerNapi::Run);
    }
}

void MediaAssetManagerNapi::JSRequestComplete(napi_env env, napi_status, void *data)
{
    MediaAssetManagerAsyncContext *context = static_cast<MediaAssetManagerAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    if (context->dataHandlerRef != nullptr) {
        napi_delete_reference(env, context->dataHandlerRef);
        context->dataHandlerRef = nullptr;
    }
    if (context->dataHandlerRef2 != nullptr) {
        napi_delete_reference(env, context->dataHandlerRef2);
        context->dataHandlerRef2 = nullptr;
    }

    MediaLibraryTracer tracer;
    tracer.Start("JSRequestComplete");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->progressHandler &&
        context->progressHandler->cameraProgressMode == CameraProgressMode::IMMEDIATELY_MODE) {
        NotifyOnProgress(static_cast<int32_t>(
            ProgressReturnInfoType::CAMERA_PROGRESS), PROGRESS_MAX, context->requestId);
        context->progressHandler = nullptr;
    }
    if (context->assetHandler) {
        NotifyMediaDataPrepared(context->assetHandler);
        context->assetHandler = nullptr;
    }
    if (context->error == ERR_DEFAULT) {
        napi_value requestId;
        napi_create_string_utf8(env, context->requestId.c_str(), NAPI_AUTO_LENGTH, &requestId);
        jsContext->data = requestId;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

void MediaAssetManagerNapi::JSCancelRequestExecute(napi_env env, void *data)
{
    MediaAssetManagerAsyncContext *context = static_cast<MediaAssetManagerAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    int32_t mediaType = static_cast<int32_t>(MediaFileUtils::GetMediaType(
        MediaFileUtils::GetFileName(context->photoUri)));
    NAPI_INFO_LOG("JSCancelRequestExecute requestId: %{public}s", (context->requestId).c_str());
    CancelRequestReqBody reqBody;
    reqBody.photoId = context->photoId;
    reqBody.mediaType = mediaType;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_MAM_CANCEL_PROCESS);
    IPC::UserDefineIPCClient().Call(businessCode, reqBody);
}

void MediaAssetManagerNapi::JSCancelRequestComplete(napi_env env, napi_status, void *data)
{
    MediaAssetManagerAsyncContext *context = static_cast<MediaAssetManagerAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    MediaLibraryTracer tracer;
    tracer.Start("JSCancelRequestComplete");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    napi_get_undefined(env, &jsContext->data);

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}
} // namespace Media
} // namespace OHOS