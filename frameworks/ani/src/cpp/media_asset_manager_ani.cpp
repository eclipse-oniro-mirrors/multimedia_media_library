/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "media_asset_manager_ani.h"

#include <fcntl.h>
#include <string>
#include <sys/sendfile.h>
#include <unordered_map>
#include <uuid.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "ani_class_name.h"
#include "dataobs_mgr_client.h"
#include "directory_ex.h"
#include "file_asset_ani.h"
#include "file_uri.h"
#include "image_ani_utils.h"
#include "image_source.h"
#include "image_source_ani.h"
#include "ipc_skeleton.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_ani_utils_ext.h"
#include "medialibrary_tracer.h"
#include "moving_photo_ani.h"
#include "permission_utils.h"
#include "picture_handle_client.h"
#include "ui_extension_context.h"
#include "userfile_client.h"
#include "media_call_transcode.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS::Media {
static std::mutex multiStagesCaptureLock;
static std::mutex registerTaskLock;

const int32_t LOW_QUALITY_IMAGE = 1;
const int32_t HIGH_QUALITY_IMAGE = 0;

const int32_t UUID_STR_LENGTH = 37;
const int32_t REQUEST_ID_MAX_LEN = 64;

static std::map<std::string, std::shared_ptr<MultiStagesTaskObserver>> multiStagesObserverMap;
static std::map<std::string, std::map<std::string, AssetHandler*>> inProcessUriMap;
static SafeMap<std::string, AssetHandler*> inProcessFastRequests;
static SafeMap<std::string, AssetHandler*> onPreparedResult_;
static SafeMap<std::string, ani_object> onPreparedResultValue_;
static SafeMap<std::string, bool> isTranscoderMap_;

ani_status MediaAssetManagerAni::Init(ani_env *env)
{
    static const char *className = PAH_ANI_CLASS_MEDIA_MANAGER.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array staticMethods = {
        ani_native_function {"requestImageInner", nullptr, reinterpret_cast<void *>(RequestImage)},
        ani_native_function {"requestImageDataInner", nullptr, reinterpret_cast<void *>(RequestImageData)},
        ani_native_function {"requestMovingPhotoInner", nullptr, reinterpret_cast<void *>(RequestMovingPhoto)},
        ani_native_function {"cancelRequestInner", nullptr, reinterpret_cast<void *>(CancelRequest)},
        ani_native_function {"quickRequestImageInner", nullptr, reinterpret_cast<void *>(RequestEfficientImage)},
    };

    status = env->Class_BindNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native staticMethods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

static bool HasReadPermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_READ_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

static AssetHandler* CreateAssetHandler(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context,
    const std::shared_ptr<AniMediaAssetDataHandler> &handler, ThreadFuncitonOnData func)
{
    AssetHandler *assetHandler =
        new AssetHandler(env, context->photoId, context->requestId, context->photoUri, handler, func);
    ANI_DEBUG_LOG("[AssetHandler create] photoId: %{public}s, requestId: %{public}s, uri: %{public}s, %{public}p",
        context->photoId.c_str(), context->requestId.c_str(), context->photoUri.c_str(), assetHandler);
    return assetHandler;
}

static void DeleteAssetHandlerSafe(AssetHandler *handler, ani_env *env)
{
    if (handler != nullptr) {
        ANI_DEBUG_LOG("[AssetHandler delete] %{public}p.", handler);
        if (handler->dataHandler != nullptr) {
            handler->dataHandler->DeleteAniReference(env);
        }
        delete handler;
        handler = nullptr;
    }
}

static void DeleteProcessHandlerSafe(ProgressHandler *handler, ani_env *env)
{
    if (handler == nullptr) {
        return;
    }
    ANI_DEBUG_LOG("[ProgressHandler delete] %{public}p.", handler);
    if (handler->progressRef != nullptr && env != nullptr) {
        env->GlobalReference_Delete(handler->progressRef);
        handler->progressRef = nullptr;
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

static void DeleteInProcessMapRecord(const std::string &requestUri, const std::string &requestId)
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

static int32_t IsInProcessInMapRecord(const std::string &requestId, AssetHandler* &handler)
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

static AssetHandler* InsertDataHandler(NotifyMode notifyMode, ani_env *env,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    ani_ref dataHandlerRef;
    ThreadFuncitonOnData threadSafeFunc;
    if (notifyMode == NotifyMode::FAST_NOTIFY) {
        dataHandlerRef = context->dataHandlerRef;
        context->dataHandlerRef = nullptr;
        threadSafeFunc = context->onDataPreparedPtr;
    } else {
        dataHandlerRef = context->dataHandlerRef2;
        context->dataHandlerRef2 = nullptr;
        threadSafeFunc = context->onDataPreparedPtr2;
    }
    std::shared_ptr<AniMediaAssetDataHandler> mediaAssetDataHandler = make_shared<AniMediaAssetDataHandler>(
        env, dataHandlerRef, context->returnDataType, context->photoUri, context->destUri,
        context->sourceMode);
    mediaAssetDataHandler->SetCompatibleMode(context->compatibleMode);
    mediaAssetDataHandler->SetNotifyMode(notifyMode);
    mediaAssetDataHandler->SetRequestId(context->requestId);

    AssetHandler *assetHandler = CreateAssetHandler(env, context, mediaAssetDataHandler, threadSafeFunc);
    assetHandler->photoQuality = context->photoQuality;
    assetHandler->needsExtraInfo = context->needsExtraInfo;
    ANI_INFO_LOG("Add %{public}d, %{public}s, %{public}s, %{public}p", notifyMode, context->photoUri.c_str(),
        context->requestId.c_str(), assetHandler);

    switch (notifyMode) {
        case NotifyMode::FAST_NOTIFY: {
            inProcessFastRequests.EnsureInsert(context->requestId, assetHandler);
            break;
        }
        case NotifyMode::WAIT_FOR_HIGH_QUALITY: {
            InsertInProcessMapRecord(context->photoUri, context->requestId, assetHandler);
            break;
        }
        default:
            break;
    }

    return assetHandler;
}

static void DeleteDataHandler(NotifyMode notifyMode, const std::string &requestUri, const std::string &requestId)
{
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(requestUri);
    ANI_INFO_LOG("Rmv %{public}d, %{public}s, %{public}s", notifyMode, requestUri.c_str(), requestId.c_str());
    if (notifyMode == NotifyMode::WAIT_FOR_HIGH_QUALITY) {
        DeleteInProcessMapRecord(uriLocal, requestId);
    }
    inProcessFastRequests.Erase(requestId);
}

static bool IsMovingPhoto(int32_t photoSubType, int32_t effectMode, int32_t sourceMode)
{
    return photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        (MediaLibraryAniUtils::IsSystemApp() && sourceMode == static_cast<int32_t>(SourceMode::ORIGINAL_MODE) &&
        effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}

static ani_status ParseArgGetPhotoAsset(ani_env *env, ani_object photoAsset,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_COND_WITH_RET_MESSAGE(env, photoAsset != nullptr, ANI_INVALID_ARGS,
        "ParseArgGetPhotoAsset failed to get photoAsset");
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_INVALID_ARGS, "context is nullptr");

    FileAssetAni *obj = FileAssetAni::Unwrap(env, photoAsset);
    CHECK_COND_WITH_RET_MESSAGE(env, obj != nullptr, ANI_INVALID_ARGS, "Failed to parse photo asset");
    context->fileId = obj->GetFileId();
    context->photoUri = obj->GetFileUri();
    context->displayName = obj->GetFileDisplayName();
    return ANI_OK;
}

static ani_status GetDeliveryMode(ani_env *env, ani_object requestOptions, DeliveryMode &deliveryMode)
{
    ani_object deliveryModeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, requestOptions, "deliveryMode", deliveryModeAni),
        "Failed to check deliveryMode");
    if (MediaLibraryAniUtils::IsUndefined(env, deliveryModeAni) == ANI_TRUE) {
        ANI_ERR_LOG("No delivery mode specified");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "No delivery mode specified");
        return ANI_INVALID_ARGS;
    }
    int mode = -1;
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(deliveryModeAni), mode),
        "Failed to parse deliveryMode argument value");

    // source mode's valid range is 0 - 2
    if (mode < 0 || mode > 2) {
        ANI_ERR_LOG("delivery mode invalid argument");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "delivery mode invalid argument");
        return ANI_INVALID_ARGS;
    }
    deliveryMode = static_cast<DeliveryMode>(mode);
    return ANI_OK;
}

static ani_status GetSourceMode(ani_env *env, ani_object requestOptions, SourceMode &sourceMode)
{
    ani_object sourceModeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, requestOptions, "sourceMode", sourceModeAni),
        "Failed to check sourceMode");
    if (MediaLibraryAniUtils::IsUndefined(env, sourceModeAni) == ANI_TRUE) {
        // use default source mode
        sourceMode = SourceMode::EDITED_MODE;
        return ANI_OK;
    } else if (!MediaLibraryAniUtils::IsSystemApp()) {
        ANI_ERR_LOG("Source mode is only available to system apps");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Source mode is only available to system apps");
        return ANI_INVALID_ARGS;
    }
    int mode = -1;
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(sourceModeAni), mode),
        "Failed to parse sourceMode argument value");

    // source mode's valid range is 0 - 1
    if (mode < 0 || mode > 1) {
        ANI_ERR_LOG("source mode invalid");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "invalid source mode value");
        return ANI_INVALID_ARGS;
    }
    sourceMode = static_cast<SourceMode>(mode);
    return ANI_OK;
}

static ani_status ParseArgGetRequestOption(ani_env *env, ani_object requestOptions,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_STATUS_RET(GetDeliveryMode(env, requestOptions, context->deliveryMode), "Failed to parse deliveryMode");
    CHECK_STATUS_RET(GetSourceMode(env, requestOptions, context->sourceMode), "Failed to parse sourceMode");
    return ANI_OK;
}

static ani_status GetCompatibleMode(ani_env *env, ani_object requestOptions, CompatibleMode& compatibleMode)
{
    ani_object compatibleModeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, requestOptions, "compatibleMode", compatibleModeAni),
        "Failed to check compatibleMode");
    if (MediaLibraryAniUtils::IsUndefined(env, compatibleModeAni) == ANI_TRUE) {
        ANI_INFO_LOG("compatible mode is null");
        compatibleMode = CompatibleMode::ORIGINAL_FORMAT_MODE;
        return ANI_OK;
    }
    int mode = static_cast<int>(CompatibleMode::ORIGINAL_FORMAT_MODE);
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(compatibleModeAni), mode),
        "Failed to parse compatibleMode argument value");

    if (static_cast<CompatibleMode>(mode) < CompatibleMode::ORIGINAL_FORMAT_MODE ||
        static_cast<CompatibleMode>(mode) > CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        ANI_ERR_LOG("compatible mode invalid argument ");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "invalid compatible mode value");
        return ANI_INVALID_ARGS;
    }
    compatibleMode = static_cast<CompatibleMode>(mode);
    return ANI_OK;
}

static ani_status GetMediaAssetProgressHandler(ani_env *env, ani_object requestOptions,
    ani_object &mediaAssetProgressHandler)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, requestOptions != nullptr, OHOS_INVALID_PARAM_CODE,
        "MediaAssetProgressHandler invalid argument", ANI_INVALID_ARGS, "MediaAssetProgressHandler is nullptr");

    ani_object progressHandlerAni {};
    ani_status status =
        MediaLibraryAniUtils::GetProperty(env, requestOptions, "mediaAssetProgressHandler", progressHandlerAni);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == ANI_OK, OHOS_INVALID_PARAM_CODE,
        "failed to get mediaAssetProgressHandler ", ANI_INVALID_ARGS,
        "failed to get mediaAssetProgressHandler, ani status: %{public}d", static_cast<int>(status));
    if (MediaLibraryAniUtils::IsUndefined(env, progressHandlerAni) == ANI_TRUE) {
        ANI_INFO_LOG("MediaAssetProgressHandler is null");
        mediaAssetProgressHandler = nullptr;
        return ANI_OK;
    }

    ani_method onProgress {};
    status = MediaLibraryAniUtils::FindClassMethod(env, PAH_ANI_CLASS_MEDIA_PROGRESS_HANDLER,
        std::string(ON_PROGRESS_FUNC), &onProgress);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == ANI_OK, OHOS_INVALID_PARAM_CODE,
        "unable to get onProgress function", ANI_INVALID_ARGS,
        "failed to get onProgress function, ani status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, onProgress != nullptr, OHOS_INVALID_PARAM_CODE,
        "invalid onProgress", ANI_INVALID_ARGS, "onProgress is nullptr");
    mediaAssetProgressHandler = progressHandlerAni;
    return ANI_OK;
}

static ani_status ParseArgGetRequestOptionMore(ani_env *env, ani_object requestOptions,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_STATUS_RET(GetCompatibleMode(env, requestOptions, context->compatibleMode),
        "Failed to parse compatibleMode");
    if (GetMediaAssetProgressHandler(env, requestOptions, context->mediaAssetProgressHandler) != ANI_OK) {
        ANI_ERR_LOG("requestMedia GetMediaAssetProgressHandler error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia GetMediaAssetProgressHandler error");
        return ANI_INVALID_ARGS;
    }
    return ANI_OK;
}

static ani_status ParseArgGetDataHandler(ani_env *env, ani_object dataHandler,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, MediaLibraryAniUtils::IsUndefined(env, dataHandler) == ANI_FALSE,
        OHOS_INVALID_PARAM_CODE, "invalid data handler", ANI_INVALID_ARGS, "data handler is nullptr");
    context->dataHandler = dataHandler;

    ani_method onDataPrepared {};
    ani_status status = MediaLibraryAniUtils::FindClassMethod(env, PAH_ANI_CLASS_MEDIA_DATA_HANDLER,
        std::string(ON_DATA_PREPARED_FUNC), &onDataPrepared);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == ANI_OK, OHOS_INVALID_PARAM_CODE,
        "unable to get onDataPrepared function", ANI_INVALID_ARGS,
        "failed to get property onDataPrepared, ani status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, onDataPrepared != nullptr, OHOS_INVALID_PARAM_CODE,
        "invalid onDataPrepared", ANI_INVALID_ARGS, "onDataPrepared is nullptr");
    context->needsExtraInfo = true;
    return ANI_OK;
}

static ani_status ParseArgGetEfficientImageDataHandler(ani_env *env, ani_object dataHandler,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, MediaLibraryAniUtils::IsUndefined(env, dataHandler) == ANI_FALSE,
        OHOS_INVALID_PARAM_CODE, "efficient data handler invalid argument",
        ANI_INVALID_ARGS, "efficient data handler is nullptr");
    context->dataHandler = dataHandler;

    ani_method onDataPrepared {};
    ani_status status = MediaLibraryAniUtils::FindClassMethod(env, PAH_ANI_CLASS_MEDIA_DATA_HANDLER,
        std::string(ON_DATA_PREPARED_FUNC), &onDataPrepared);
    CHECK_COND_LOG_THROW_RETURN_RET(env, status == ANI_OK, OHOS_INVALID_PARAM_CODE,
        "unable to get onDataPrepared function", ANI_INVALID_ARGS,
        "failed to get type of efficient data handler, ani status: %{public}d", static_cast<int>(status));
    CHECK_COND_LOG_THROW_RETURN_RET(env, onDataPrepared != nullptr, OHOS_INVALID_PARAM_CODE,
        "invalid onDataPrepared", ANI_INVALID_ARGS, "onDataPrepared is nullptr");
    context->needsExtraInfo = true;
    return ANI_OK;
}

static ani_status ParseArgsForRequestMovingPhoto(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context,
    ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    FileAssetAni *fileAssetAni = FileAssetAni::Unwrap(env, asset);
    CHECK_COND_WITH_RET_MESSAGE(env, fileAssetAni != nullptr, ANI_INVALID_ARGS, "Failed to parse photo asset");
    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, fileAssetPtr != nullptr, ANI_INVALID_ARGS, "fileAsset is null");
    context->photoUri = fileAssetPtr->GetUri();
    context->fileId = fileAssetPtr->GetId();
    context->returnDataType = ReturnDataType::TYPE_MOVING_PHOTO;
    context->hasReadPermission = HasReadPermission();
    context->subType = PhotoSubType::MOVING_PHOTO;

    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgGetRequestOption(env, requestOptions, context) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to parse request option");
    CHECK_COND_WITH_RET_MESSAGE(env, IsMovingPhoto(fileAssetPtr->GetPhotoSubType(),
        fileAssetPtr->GetMovingPhotoEffectMode(), static_cast<int32_t>(context->sourceMode)),
        ANI_INVALID_ARGS, "Asset is not a moving photo");

    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgGetDataHandler(env, dataHandler, context) == ANI_OK,
        ANI_INVALID_ARGS, "requestMovingPhoto ParseArgGetDataHandler error");
    return ANI_OK;
}

bool MediaAssetManagerAni::InitUserFileClient(ani_env *env, ani_object context)
{
    if (UserFileClient::IsValid()) {
        return true;
    }

    std::unique_lock<std::mutex> helperLock(MediaLibraryAni::sUserFileClientMutex_);
    if (!UserFileClient::IsValid()) {
        UserFileClient::Init(env, context);
    }
    helperLock.unlock();
    return UserFileClient::IsValid();
}

static int32_t GetPhotoSubtype(ani_env *env, ani_object photoAssetArg)
{
    if (photoAssetArg == nullptr) {
        ANI_ERR_LOG(
            "Dfx adaptation to moving photo collector error: failed to get photo subtype, photo asset is null");
        return -1;
    }
    FileAssetAni *obj = FileAssetAni::Unwrap(env, photoAssetArg);
    if (obj == nullptr || obj->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("Dfx adaptation to moving photo collector error: failed to unwrap file asset");
        return -1;
    }
    return obj->GetFileAssetInstance()->GetPhotoSubType();
}

ani_status MediaAssetManagerAni::CreateDataHandlerRef(ani_env *env,
    const unique_ptr<MediaAssetManagerAniContext> &context, ani_ref &dataHandlerRef)
{
    ani_status status = env->GlobalReference_Create(static_cast<ani_ref>(context->dataHandler), &dataHandlerRef);
    if (status != ANI_OK) {
        dataHandlerRef = nullptr;
        ANI_ERR_LOG("GlobalReference_Create failed");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "GlobalReference_Create fail");
    }
    return status;
}

ani_status MediaAssetManagerAni::CreateProgressHandlerRef(ani_env *env,
    const unique_ptr<MediaAssetManagerAniContext> &context, ani_ref &dataHandlerRef)
{
    ani_status status =
        env->GlobalReference_Create(static_cast<ani_ref>(context->mediaAssetProgressHandler), &dataHandlerRef);
    if (status != ANI_OK) {
        dataHandlerRef = nullptr;
        ANI_ERR_LOG("GlobalReference_Create failed");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "GlobalReference_Create fail");
    }
    return status;
}

ani_status MediaAssetManagerAni::CreateOnDataPreparedThreadSafeFunc(ThreadFuncitonOnData &threadSafeFunc)
{
    threadSafeFunc = [](AssetHandler* assetHandler) {
        CHECK_IF_EQUAL(assetHandler != nullptr, "assetHandler is null");
        CHECK_IF_EQUAL(assetHandler->env != nullptr, "assetHandler env is null");

        ani_vm *etsVm {};
        CHECK_IF_EQUAL(assetHandler->env->GetVM(&etsVm) == ANI_OK, "Get etsVm fail");

        ani_env *etsEnv {};
        ani_option interopEnabled {"--interop=disable", nullptr};
        ani_options aniArgs {1, &interopEnabled};
        CHECK_IF_EQUAL(etsVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &etsEnv) == ANI_OK,
            "AttachCurrentThread fail");

        // Do OnDataPrepared in thread
        MediaAssetManagerAni::OnDataPrepared(etsEnv, assetHandler);

        CHECK_IF_EQUAL(etsVm->DetachCurrentThread() == ANI_OK, "DetachCurrentThread fail");
    };
    return ANI_OK;
}

ani_status MediaAssetManagerAni::CreateOnProgressThreadSafeFunc(ThreadFuncitonOnProgress &progressFunc)
{
    progressFunc = [](ProgressHandler* progressHandler) {
        CHECK_IF_EQUAL(progressHandler != nullptr, "progressHandler is null");
        CHECK_IF_EQUAL(progressHandler->env != nullptr, "progressHandler env is null");

        ani_vm *etsVm {};
        CHECK_IF_EQUAL(progressHandler->env->GetVM(&etsVm) == ANI_OK, "Get etsVm fail");

        ani_env *etsEnv {};
        ani_option interopEnabled {"--interop=disable", nullptr};
        ani_options aniArgs {1, &interopEnabled};
        CHECK_IF_EQUAL(etsVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &etsEnv) == ANI_OK,
            "AttachCurrentThread fail");

        // Do OnProgress in thread
        MediaAssetManagerAni::OnProgress(etsEnv, progressHandler);

        CHECK_IF_EQUAL(etsVm->DetachCurrentThread() == ANI_OK, "DetachCurrentThread fail");
    };
    return ANI_OK;
}

static void SavePicture(std::string &fileUri)
{
    std::string uriStr = PATH_SAVE_PICTURE;
    std::string tempStr = fileUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length());
    std::size_t index = tempStr.find("/");
    std::string fileId = tempStr.substr(0, index);
    MediaLibraryAniUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaLibraryAniUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_ID, fileId);
    MediaLibraryAniUtils::UriAppendKeyValue(uriStr, IMAGE_FILE_TYPE, "1");
    MediaLibraryAniUtils::UriAppendKeyValue(uriStr, "uri", fileUri);
    Uri uri(uriStr);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    DataShare::DataSharePredicates predicate;
    UserFileClient::Update(uri, predicate, valuesBucket);
}

static string PhotoQualityToString(MultiStagesCapturePhotoStatus photoQuality)
{
    static const string HIGH_QUALITY_STRING = "high";
    static const string LOW_QUALITY_STRING = "low";
    if (photoQuality != MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS &&
        photoQuality != MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
        ANI_ERR_LOG("Invalid photo quality: %{public}d", static_cast<int>(photoQuality));
        return HIGH_QUALITY_STRING;
    }

    return (photoQuality == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) ? HIGH_QUALITY_STRING :
        LOW_QUALITY_STRING;
}

static ani_object GetInfoMapAniValue(ani_env *env, AssetHandler *assetHandler)
{
    std::map<std::string, std::string> infoMap;
    infoMap.insert({"quality", PhotoQualityToString(assetHandler->photoQuality)});

    ani_object mapAniValue {};
    if (MediaLibraryAniUtils::ToAniMap(env, infoMap, mapAniValue) != ANI_OK) {
        ANI_ERR_LOG("InfoMap ToAniMap fail");
        return nullptr;
    }
    return mapAniValue;
}

void MediaAssetManagerAni::GetImageSourceAniObject(const std::string &fileUri, ani_object &imageSourceAniObj,
    bool isSource, ani_env *env)
{
    if (env == nullptr) {
        ANI_ERR_LOG(" create image source object failed, need to initialize env");
        return;
    }
    std::string tmpUri = fileUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
        ANI_INFO_LOG("request source image's imageSource");
    }
    Uri uri(tmpUri);
    int fd = UserFileClient::OpenFile(uri, "r");
    if (fd < 0) {
        ANI_ERR_LOG("get image fd failed, errno: %{public}d", errno);
        return;
    }

    SourceOptions opts;
    uint32_t errCode = 0;
    auto nativeImageSourcePtr = ImageSource::CreateImageSource(fd, opts, errCode);
    close(fd);
    if (nativeImageSourcePtr == nullptr) {
        ANI_ERR_LOG("get ImageSource::CreateImageSource failed");
        return;
    }

    std::unique_ptr<ImageSourceAni> imageSourceAni = std::make_unique<ImageSourceAni>();
    if (imageSourceAni == nullptr) {
        ANI_ERR_LOG("Create native imageSourceAni failed");
        return;
    }
    imageSourceAni->nativeImageSource_ = std::move(nativeImageSourcePtr);
    ani_object tempImageSourceAni = ImageAniUtils::CreateAniImageSource(env, imageSourceAni);
    if (tempImageSourceAni == nullptr) {
        ANI_ERR_LOG("Create imageSource ani object failed");
        return;
    }
    imageSourceAniObj = tempImageSourceAni;
}

void MediaAssetManagerAni::GetPictureAniObject(const std::string &fileUri, ani_object &pictureAniObj,
    bool isSource, ani_env *env,  bool& isPicture)
{
    if (env == nullptr) {
        ANI_ERR_LOG(" create image source object failed, need to initialize env");
        return;
    }
    ANI_DEBUG_LOG("GetPictureAniObject");

    std::string tempStr = fileUri.substr(PhotoColumn::PHOTO_URI_PREFIX.length());
    std::size_t index = tempStr.find("/");
    std::string fileId = tempStr.substr(0, index);
    auto pic = PictureHandlerClient::RequestPicture(std::atoi(fileId.c_str()));
    if (pic == nullptr) {
        ANI_ERR_LOG("picture is null");
        isPicture = false;
        GetImageSourceAniObject(fileUri, pictureAniObj, isSource, env);
        return;
    }
    ANI_INFO_LOG("picture is not null");
    pictureAniObj = nullptr;
}

void MediaAssetManagerAni::GetByteArrayAniObject(const std::string &requestUri, ani_object &arrayBuffer,
    bool isSource, ani_env *env)
{
    if (env == nullptr) {
        ANI_ERR_LOG("create byte array object failed, need to initialize env");
        return;
    }
    
    std::string tmpUri = requestUri;
    if (isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    Uri uri(tmpUri);
    int imageFd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    if (imageFd < 0) {
        ANI_ERR_LOG("get image fd failed, %{public}d", errno);
        return;
    }
    ssize_t imgLen = lseek(imageFd, 0, SEEK_END);
    void* buffer = nullptr;
    ani_arraybuffer aniArrayBuffer {};
    env->CreateArrayBuffer(static_cast<size_t>(imgLen), &buffer, &aniArrayBuffer);
    lseek(imageFd, 0, SEEK_SET);
    ssize_t readRet = read(imageFd, buffer, imgLen);
    close(imageFd);
    if (readRet != imgLen) {
        ANI_ERR_LOG("read image failed");
        return;
    }
    arrayBuffer = static_cast<ani_object>(aniArrayBuffer);
}

void MediaAssetManagerAni::WriteDataToDestPath(WriteData &writeData, ani_object &resultAniValue,
    std::string requestId)
{
    if (writeData.env == nullptr) {
        ANI_ERR_LOG("create byte array object failed, need to initialize env");
        return;
    }
    if (writeData.requestUri.empty() || writeData.destUri.empty()) {
        MediaLibraryAniUtils::ToAniBooleanObject(writeData.env, false, resultAniValue);
        ANI_ERR_LOG("requestUri or responseUri is nullptr");
        return;
    }
    std::string tmpUri = writeData.requestUri;
    if (writeData.isSource) {
        MediaFileUtils::UriAppendKeyValue(tmpUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    }
    Uri srcUri(tmpUri);
    int srcFd = UserFileClient::OpenFile(srcUri, MEDIA_FILEMODE_READONLY);
    if (srcFd < 0) {
        MediaLibraryAniUtils::ToAniBooleanObject(writeData.env, false, resultAniValue);
        ANI_ERR_LOG("get source file fd failed %{public}d", srcFd);
        return;
    }
    struct stat statSrc;
    if (fstat(srcFd, &statSrc) == -1) {
        close(srcFd);
        MediaLibraryAniUtils::ToAniBooleanObject(writeData.env, false, resultAniValue);
        ANI_ERR_LOG("File get stat failed, %{public}d", errno);
        return;
    }
    int destFd = GetFdFromSandBoxUri(writeData.destUri);
    if (destFd < 0) {
        close(srcFd);
        MediaLibraryAniUtils::ToAniBooleanObject(writeData.env, false, resultAniValue);
        ANI_ERR_LOG("get dest fd failed %{public}d", destFd);
        return;
    }
    ANI_INFO_LOG("WriteDataToDestPath compatibleMode %{public}d", writeData.compatibleMode);
    if (writeData.compatibleMode == CompatibleMode::COMPATIBLE_FORMAT_MODE) {
        isTranscoderMap_.Insert(requestId, true);
        MediaCallTranscode::RegisterCallback(NotifyOnProgress);
        MediaCallTranscode::CallTranscodeHandle(writeData.env, srcFd, destFd, resultAniValue,
            statSrc.st_size, requestId);
    } else {
        SendFile(writeData.env, srcFd, destFd, resultAniValue, statSrc.st_size);
    }
    close(srcFd);
    close(destFd);
    return;
}

void MediaAssetManagerAni::SendFile(ani_env *env, int srcFd, int destFd, ani_object &result, off_t fileSize)
{
    if (srcFd < 0 || destFd < 0) {
        ANI_ERR_LOG("srcFd or destFd is invalid");
        MediaLibraryAniUtils::ToAniBooleanObject(env, false, result);
        return;
    }
    if (sendfile(destFd, srcFd, nullptr, fileSize) == -1) {
        close(srcFd);
        close(destFd);
        MediaLibraryAniUtils::ToAniBooleanObject(env, false, result);
        ANI_ERR_LOG("send file failed, %{public}d", errno);
        return;
    }
    MediaLibraryAniUtils::ToAniBooleanObject(env, true, result);
}

int32_t MediaAssetManagerAni::GetFdFromSandBoxUri(const std::string &sandBoxUri)
{
    AppFileService::ModuleFileUri::FileUri destUri(sandBoxUri);
    string destPath = destUri.GetRealPath();
    if (!MediaFileUtils::IsFileExists(destPath) && !MediaFileUtils::CreateFile(destPath)) {
        ANI_DEBUG_LOG("Create empty dest file in sandbox failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    string absDestPath;
    if (!PathToRealPath(destPath, absDestPath)) {
        ANI_DEBUG_LOG("PathToRealPath failed, path:%{private}s", destPath.c_str());
        return E_ERR;
    }
    return MediaFileUtils::OpenFile(absDestPath, MEDIA_FILEMODE_WRITETRUNCATE);
}

static ani_object GetAniValueOfMedia(ani_env *env, const std::shared_ptr<AniMediaAssetDataHandler>& dataHandler,
    bool& isPicture)
{
    ANI_DEBUG_LOG("GetAniValueOfMedia");
    ani_object aniValueOfMedia {};
    if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_ARRAY_BUFFER) {
        MediaAssetManagerAni::GetByteArrayAniObject(dataHandler->GetRequestUri(), aniValueOfMedia,
            dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE, env);
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_IMAGE_SOURCE) {
        MediaAssetManagerAni::GetImageSourceAniObject(dataHandler->GetRequestUri(), aniValueOfMedia,
            dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE, env);
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_TARGET_PATH) {
        WriteData param;
        param.compatibleMode = dataHandler->GetCompatibleMode();
        param.destUri = dataHandler->GetDestUri();
        param.requestUri = dataHandler->GetRequestUri();
        param.env = env;
        param.isSource = dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE;
        MediaAssetManagerAni::WriteDataToDestPath(param, aniValueOfMedia, dataHandler->GetRequestId());
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_MOVING_PHOTO) {
        aniValueOfMedia = MovingPhotoAni::NewMovingPhotoAni(
            env, dataHandler->GetRequestUri(), dataHandler->GetSourceMode());
    } else if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_PICTURE) {
        MediaAssetManagerAni::GetPictureAniObject(dataHandler->GetRequestUri(), aniValueOfMedia,
            dataHandler->GetSourceMode() == SourceMode::ORIGINAL_MODE, env, isPicture);
    } else {
        ANI_ERR_LOG("source mode type invalid");
    }
    return aniValueOfMedia;
}

static bool IsSaveCallbackInfoByTranscoder(ani_object aniValueOfMedia, ani_env *env, AssetHandler *assetHandler,
    ani_object aniValueOfInfoMap)
{
    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        ANI_ERR_LOG("data handler is nullptr");
        return false;
    }
    if (aniValueOfMedia == nullptr) {
        MediaLibraryAniUtils::GetUndefinedObject(env, aniValueOfInfoMap);
    }
    bool isTranscoder;
    if (!isTranscoderMap_.Find(assetHandler->requestId, isTranscoder)) {
        ANI_INFO_LOG("not find key from map");
        isTranscoder = false;
    }
    ANI_INFO_LOG("IsSaveCallbackInfoByTranscoder isTranscoder_ %{public}d", isTranscoder);
    if (isTranscoder) {
        onPreparedResult_.EnsureInsert(assetHandler->requestId, assetHandler);
        onPreparedResultValue_.EnsureInsert(assetHandler->requestId, aniValueOfMedia);
        return true;
    }
    dataHandler->EtsOnDataPrepared(env, aniValueOfMedia, aniValueOfInfoMap);
    return false;
}

void MediaAssetManagerAni::OnDataPrepared(ani_env *env, AssetHandler *assetHandler)
{
    CHECK_NULL_PTR_RETURN_VOID(assetHandler, "assetHandler is nullptr");
    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        ANI_ERR_LOG("data handler is nullptr");
        DeleteAssetHandlerSafe(assetHandler, env);
        return;
    }

    NotifyMode notifyMode = dataHandler->GetNotifyMode();
    if (notifyMode == NotifyMode::FAST_NOTIFY) {
        AssetHandler *tmp;
        if (!inProcessFastRequests.Find(assetHandler->requestId, tmp)) {
            ANI_ERR_LOG("The request has been canceled");
            DeleteAssetHandlerSafe(assetHandler, env);
            return;
        }
    }

    ani_object aniValueOfInfoMap = nullptr;
    if (assetHandler->needsExtraInfo) {
        aniValueOfInfoMap = GetInfoMapAniValue(env, assetHandler);
        if (aniValueOfInfoMap == nullptr) {
            ANI_ERR_LOG("Failed to get info map");
            MediaLibraryAniUtils::GetUndefinedObject(env, aniValueOfInfoMap);
        }
    }
    bool isPicture = true;
    if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_ARRAY_BUFFER ||
        dataHandler->GetReturnDataType() == ReturnDataType::TYPE_IMAGE_SOURCE) {
        string uri = dataHandler->GetRequestUri();
        SavePicture(uri);
    }
    ani_object aniValueOfMedia = GetAniValueOfMedia(env, dataHandler, isPicture);
    if (dataHandler->GetReturnDataType() == ReturnDataType::TYPE_PICTURE) {
        if (isPicture) {
            dataHandler->EtsOnDataPrepared(env, aniValueOfMedia, nullptr, aniValueOfInfoMap);
        } else {
            if (aniValueOfMedia == nullptr) {
                MediaLibraryAniUtils::GetUndefinedObject(env, aniValueOfMedia);
            }
            dataHandler->EtsOnDataPrepared(env, nullptr, aniValueOfMedia, aniValueOfInfoMap);
        }
    } else if (IsSaveCallbackInfoByTranscoder(aniValueOfMedia, env, assetHandler, aniValueOfInfoMap)) {
        return;
    }
    DeleteDataHandler(notifyMode, assetHandler->requestUri, assetHandler->requestId);
    ANI_INFO_LOG("delete assetHandler: %{public}p", assetHandler);
    DeleteAssetHandlerSafe(assetHandler, env);
}

void CallPreparedCallbackAfterProgress(ani_env *env, ProgressHandler *progressHandler, ani_object aniValueOfMedia)
{
    MediaCallTranscode::CallTranscodeRelease(progressHandler->requestId);
    MediaAssetManagerAni::progressHandlerMap_.Erase(progressHandler->requestId);
    AssetHandler *assetHandler = nullptr;
    if (!onPreparedResult_.Find(progressHandler->requestId, assetHandler)) {
        ANI_ERR_LOG("not find key from map");
        return;
    }
    onPreparedResult_.Erase(progressHandler->requestId);
    auto dataHandler = assetHandler->dataHandler;
    if (dataHandler == nullptr) {
        ANI_ERR_LOG("data handler is nullptr");
        DeleteAssetHandlerSafe(assetHandler, env);
        return;
    }

    NotifyMode notifyMode = dataHandler->GetNotifyMode();
    ani_object aniValueOfInfoMap = nullptr;
    if (assetHandler->needsExtraInfo) {
        aniValueOfInfoMap = GetInfoMapAniValue(env, assetHandler);
        if (aniValueOfInfoMap == nullptr) {
            ANI_ERR_LOG("Failed to get info map");
            MediaLibraryAniUtils::GetUndefinedObject(env, aniValueOfInfoMap);
        }
    }
    dataHandler->EtsOnDataPrepared(env, aniValueOfMedia, aniValueOfInfoMap);
    ANI_INFO_LOG("delete assetHandler: %{public}p", assetHandler);
    DeleteProcessHandlerSafe(progressHandler, env);
    DeleteDataHandler(notifyMode, assetHandler->requestUri, assetHandler->requestId);
    DeleteAssetHandlerSafe(assetHandler, env);
}

void CallProgressCallback(ani_env *env, ProgressHandler &progressHandler, int32_t process)
{
    ani_double processAni = static_cast<ani_double>(process);

    if (progressHandler.progressRef == nullptr) {
        ANI_ERR_LOG("Ets processHandler reference is null");
        DeleteProcessHandlerSafe(&progressHandler, env);
        return;
    }
    ani_object progressHandlerAni = static_cast<ani_object>(progressHandler.progressRef);
    ani_method onProgressRef {};
    ani_status status = MediaLibraryAniUtils::FindClassMethod(env, PAH_ANI_CLASS_MEDIA_PROGRESS_HANDLER,
        std::string(ON_PROGRESS_FUNC), &onProgressRef);
    if (status != ANI_OK) {
        ANI_ERR_LOG("onProgressRef Object_GetPropertyByName_Ref fail, ani status: %{public}d",
            static_cast<int>(status));
        DeleteProcessHandlerSafe(&progressHandler, env);
        return;
    }

    static const char *className = PAH_ANI_CLASS_MEDIA_MANAGER.c_str();
    ani_class cls {};
    status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s, ani status: %{public}d", className, static_cast<int>(status));
        return;
    }
    static const char *methodName = ON_MEDIA_ASSET_PROGRESS_FUNC;
    ani_static_method etsOnProgress {};
    status = env->Class_FindStaticMethod(cls, methodName, nullptr, &etsOnProgress);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find static method: %{public}s, ani status: %{public}d",
            methodName, static_cast<int>(status));
        return;
    }

    status = env->Class_CallStaticMethod_Void(cls, etsOnProgress, processAni, progressHandlerAni);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to execure static method: %{public}s, ani status: %{public}d",
            methodName, static_cast<int>(status));
        AniError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
        return;
    }
}

void MediaAssetManagerAni::OnProgress(ani_env *env, ProgressHandler *progressHandler)
{
    if (progressHandler == nullptr) {
        ANI_ERR_LOG("progressHandler handler is nullptr");
        DeleteProcessHandlerSafe(progressHandler, env);
        return;
    }
    int32_t process = progressHandler->retProgressValue.progress;
    int32_t type = progressHandler->retProgressValue.type;

    if (type == INFO_TYPE_TRANSCODER_COMPLETED || type == INFO_TYPE_ERROR) {
        bool isTranscoder;
        if (isTranscoderMap_.Find(progressHandler->requestId, isTranscoder)) {
            isTranscoderMap_.Erase(progressHandler->requestId);
        }

        ani_object aniValueOfMedia;
        if (onPreparedResultValue_.Find(progressHandler->requestId, aniValueOfMedia)) {
            onPreparedResultValue_.Erase(progressHandler->requestId);
        }
        if (type == INFO_TYPE_ERROR) {
            MediaLibraryAniUtils::ToAniBooleanObject(env, false, aniValueOfMedia);
        }
        ANI_INFO_LOG("CallPreparedCallbackAfterProgress type %{public}d", type);
        CallPreparedCallbackAfterProgress(env, progressHandler, aniValueOfMedia);
        return;
    }
    if (progressHandler->progressRef == nullptr) {
        ANI_INFO_LOG("progressHandler->progressRef == nullptr");
        return;
    }
    CallProgressCallback(env, *progressHandler, process);
}

MultiStagesCapturePhotoStatus MediaAssetManagerAni::QueryPhotoStatus(int fileId,
    const string& photoUri, std::string &photoId, bool hasReadPermission)
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
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        ANI_ERR_LOG("query resultSet is nullptr");
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
        ANI_DEBUG_LOG("query photo status : lowQuality");
        return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
    }
    ANI_DEBUG_LOG("query photo status quality: %{public}d", currentPhotoQuality);
    return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
}

void MediaAssetManagerAni::ProcessImage(const int fileId, const int deliveryMode)
{
    std::string uriStr = PAH_PROCESS_IMAGE;
    MediaLibraryAniUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    std::vector<std::string> columns {std::to_string(fileId), std::to_string(deliveryMode)};
    UserFileClient::Query(uri, predicates, columns, errCode);
}

void MediaAssetManagerAni::NotifyDataPreparedWithoutRegister(ani_env *env,
    unique_ptr<MediaAssetManagerAniContext> &context)
{
    AssetHandler *assetHandler = InsertDataHandler(NotifyMode::FAST_NOTIFY, env, context);
    if (assetHandler == nullptr) {
        ANI_ERR_LOG("assetHandler is nullptr");
        return;
    }
    context->assetHandler = assetHandler;
}

void MediaAssetManagerAni::RegisterTaskObserver(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context)
{
    auto dataObserver = std::make_shared<MultiStagesTaskObserver>(context->fileId);
    auto uriLocal = MediaFileUtils::GetUriWithoutDisplayname(context->photoUri);
    ANI_INFO_LOG("uri: %{public}s, %{public}s", context->photoUri.c_str(), uriLocal.c_str());
    Uri uri(context->photoUri);
    std::unique_lock<std::mutex> registerLock(registerTaskLock);
    if (multiStagesObserverMap.find(uriLocal) == multiStagesObserverMap.end()) {
        UserFileClient::RegisterObserverExt(Uri(uriLocal),
            static_cast<std::shared_ptr<DataShare::DataShareObserver>>(dataObserver), false);
        multiStagesObserverMap.insert(std::make_pair(uriLocal, dataObserver));
    }
    registerLock.unlock();

    InsertDataHandler(NotifyMode::WAIT_FOR_HIGH_QUALITY, env, context);

    MediaAssetManagerAni::ProcessImage(context->fileId, static_cast<int32_t>(context->deliveryMode));
}

void MediaAssetManagerAni::OnHandleRequestImage(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context)
{
    MultiStagesCapturePhotoStatus status = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    switch (context->deliveryMode) {
        case DeliveryMode::FAST:
            if (context->needsExtraInfo) {
                context->photoQuality = MediaAssetManagerAni::QueryPhotoStatus(context->fileId,
                    context->photoUri, context->photoId, context->hasReadPermission);
            }
            MediaAssetManagerAni::NotifyDataPreparedWithoutRegister(env, context);
            break;
        case DeliveryMode::HIGH_QUALITY:
            status = MediaAssetManagerAni::QueryPhotoStatus(context->fileId,
                context->photoUri, context->photoId, context->hasReadPermission);
            context->photoQuality = status;
            if (status == MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
                MediaAssetManagerAni::NotifyDataPreparedWithoutRegister(env, context);
            } else {
                RegisterTaskObserver(env, context);
            }
            break;
        case DeliveryMode::BALANCED_MODE:
            status = MediaAssetManagerAni::QueryPhotoStatus(context->fileId,
                context->photoUri, context->photoId, context->hasReadPermission);
            context->photoQuality = status;
            MediaAssetManagerAni::NotifyDataPreparedWithoutRegister(env, context);
            if (status == MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS) {
                RegisterTaskObserver(env, context);
            }
            break;
        default: {
            ANI_ERR_LOG("invalid delivery mode");
            return;
        }
    }
}

void MediaAssetManagerAni::RequestExecute(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("RequestExecute");

    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    OnHandleRequestImage(env, context);
    if (context->subType == PhotoSubType::MOVING_PHOTO) {
        string uri = LOG_MOVING_PHOTO;
        Uri logMovingPhotoUri(uri);
        DataShare::DataShareValuesBucket valuesBucket;
        string result;
        valuesBucket.Put("adapted", context->returnDataType == ReturnDataType::TYPE_MOVING_PHOTO);
        UserFileClient::InsertExt(logMovingPhotoUri, valuesBucket, result);
    }
}

void MediaAssetManagerAni::NotifyMediaDataPrepared(AssetHandler *assetHandler)
{
    auto t = std::thread(assetHandler->threadSafeFunc, assetHandler);
    t.join();
}

void MultiStagesTaskObserver::OnChange(const ChangeInfo &changeInfo)
{
    if (changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_UPDATE)) {
        ANI_DEBUG_LOG("ignore notify change, type: %{public}d", changeInfo.changeType_);
        return;
    }
    for (auto &uri : changeInfo.uris_) {
        string uriString = uri.ToString();
        ANI_DEBUG_LOG("%{public}s", uriString.c_str());
        std::string photoId = "";
        if (MediaAssetManagerAni::QueryPhotoStatus(fileId_, uriString, photoId, true) !=
            MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS) {
            ANI_ERR_LOG("requested data not prepared");
            continue;
        }

        std::lock_guard<std::mutex> lock(multiStagesCaptureLock);
        if (inProcessUriMap.find(uriString) == inProcessUriMap.end()) {
            ANI_INFO_LOG("current uri does not in process, uri: %{public}s", uriString.c_str());
            return;
        }
        std::map<std::string, AssetHandler*> assetHandlers = inProcessUriMap[uriString];
        for (auto handler : assetHandlers) {
            auto assetHandler = handler.second;
            assetHandler->photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
            MediaAssetManagerAni::NotifyMediaDataPrepared(assetHandler);
        }
    }
}

void MediaAssetManagerAni::NotifyOnProgress(int32_t type, int32_t progress, std::string requestId)
{
    ANI_DEBUG_LOG("NotifyOnProgress start %{public}d,type %{public}d", progress, type);
    ProgressHandler *progressHandler = nullptr;
    if (!MediaAssetManagerAni::progressHandlerMap_.Find(requestId, progressHandler)) {
        ANI_ERR_LOG("not find key from map");
        return;
    }
    if (progressHandler == nullptr) {
        ANI_ERR_LOG("ProgressHandler is nullptr.");
        return;
    }
    progressHandler->retProgressValue.progress = progress;
    progressHandler->retProgressValue.type = type;

    auto t = std::thread(progressHandler->progressFunc, progressHandler);
    t.join();
}

ani_string MediaAssetManagerAni::RequestComplete(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_COND_RET(context, nullptr, "context is null");
    if (context->dataHandlerRef != nullptr) {
        env->GlobalReference_Delete(context->dataHandlerRef);
        context->dataHandlerRef = nullptr;
    }
    if (context->dataHandlerRef2 != nullptr) {
        env->GlobalReference_Delete(context->dataHandlerRef2);
        context->dataHandlerRef2 = nullptr;
    }

    if (context->assetHandler) {
        NotifyMediaDataPrepared(context->assetHandler);
        context->assetHandler = nullptr;
    }
    ani_string result {};
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        CHECK_COND_RET(MediaLibraryAniUtils::ToAniString(env, context->requestId, result) == ANI_OK, nullptr,
            "ToAniString faceTag fail");
    } else {
        context->HandleError(env, errorObj);
    }
    context.reset();
    return result;
}

static std::string GenerateRequestId()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

ani_string MediaAssetManagerAni::RequestMovingPhoto(ani_env *env, [[maybe_unused]] ani_class clazz,
    ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    MediaLibraryTracer tracer;
    tracer.Start("RequestMovingPhoto");

    unique_ptr<MediaAssetManagerAniContext> aniContext = make_unique<MediaAssetManagerAniContext>();
    CHECK_COND_RET(ParseArgsForRequestMovingPhoto(env, aniContext, asset, requestOptions, dataHandler) == ANI_OK,
        nullptr, "ParseArgsForRequestMovingPhoto fail");
    CHECK_COND(env, InitUserFileClient(env, context), JS_INNER_FAIL);
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef2) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr2) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    aniContext->requestId = GenerateRequestId();

    RequestExecute(env, aniContext);
    return RequestComplete(env, aniContext);
}

ani_status MediaAssetManagerAni::ParseRequestMediaArgs(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context,
    ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    if (ParseArgGetPhotoAsset(env, asset, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return ANI_INVALID_ARGS;
    }
    if (ParseArgGetRequestOption(env, requestOptions, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetRequestOption error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOption error");
        return ANI_INVALID_ARGS;
    }
    if (ParseArgGetRequestOptionMore(env, requestOptions, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetRequestOptionMore error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOptionMore error");
        return ANI_INVALID_ARGS;
    }
    if (ParseArgGetDataHandler(env, dataHandler, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetDataHandler error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetDataHandler error");
        return ANI_INVALID_ARGS;
    }
    context->hasReadPermission = HasReadPermission();
    return ANI_OK;
}

ani_string MediaAssetManagerAni::RequestImageData(ani_env *env, [[maybe_unused]] ani_class clazz,
    ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    CHECK_COND(env, InitUserFileClient(env, context), JS_INNER_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("RequestImageData");

    unique_ptr<MediaAssetManagerAniContext> aniContext = make_unique<MediaAssetManagerAniContext>();
    aniContext->returnDataType = ReturnDataType::TYPE_ARRAY_BUFFER;
    CHECK_COND_RET(ParseRequestMediaArgs(env, aniContext, asset, requestOptions, dataHandler) == ANI_OK,
        nullptr, "ParseRequestMediaArgs fail");
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef2) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr2) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    aniContext->requestId = GenerateRequestId();
    aniContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asset));

    RequestExecute(env, aniContext);
    return RequestComplete(env, aniContext);
}

ani_string MediaAssetManagerAni::RequestImage(ani_env *env, [[maybe_unused]] ani_class clazz,
    ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    CHECK_COND(env, InitUserFileClient(env, context), JS_INNER_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("RequestImage");

    unique_ptr<MediaAssetManagerAniContext> aniContext = make_unique<MediaAssetManagerAniContext>();
    aniContext->returnDataType = ReturnDataType::TYPE_IMAGE_SOURCE;
    CHECK_COND_RET(ParseRequestMediaArgs(env, aniContext, asset, requestOptions, dataHandler) == ANI_OK,
        nullptr, "ParseRequestMediaArgs fail");
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef2) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr2) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    aniContext->requestId = GenerateRequestId();
    aniContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asset));

    RequestExecute(env, aniContext);
    return RequestComplete(env, aniContext);
}

ani_status MediaAssetManagerAni::ParseEfficentRequestMediaArgs(ani_env *env,
    unique_ptr<MediaAssetManagerAniContext> &context,
    ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    if (ParseArgGetPhotoAsset(env, asset, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetPhotoAsset error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetPhotoAsset error");
        return ANI_INVALID_ARGS;
    }
    if (ParseArgGetRequestOption(env, requestOptions, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetRequestOption error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetRequestOption error");
        return ANI_INVALID_ARGS;
    }
    if (ParseArgGetEfficientImageDataHandler(env, dataHandler, context) != ANI_OK) {
        ANI_ERR_LOG("requestMedia ParseArgGetEfficientImageDataHandler error");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requestMedia ParseArgGetEfficientImageDataHandler error");
        return ANI_INVALID_ARGS;
    }
    context->hasReadPermission = HasReadPermission();
    return ANI_OK;
}

ani_string MediaAssetManagerAni::RequestEfficientImage(ani_env *env, [[maybe_unused]] ani_class clazz,
    ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler)
{
    CHECK_COND(env, InitUserFileClient(env, context), JS_INNER_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("RequestEfficientImage");

    unique_ptr<MediaAssetManagerAniContext> aniContext = make_unique<MediaAssetManagerAniContext>();
    aniContext->returnDataType = ReturnDataType::TYPE_PICTURE;
    CHECK_COND_RET(ParseEfficentRequestMediaArgs(env, aniContext, asset, requestOptions, dataHandler) == ANI_OK,
        nullptr, "ParseEfficentRequestMediaArgs fail");
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    if (CreateDataHandlerRef(env, aniContext, aniContext->dataHandlerRef2) != ANI_OK ||
        CreateOnDataPreparedThreadSafeFunc(aniContext->onDataPreparedPtr2) != ANI_OK) {
        ANI_ERR_LOG("CreateDataHandlerRef or CreateOnDataPreparedThreadSafeFunc failed");
        return nullptr;
    }
    aniContext->requestId = GenerateRequestId();
    aniContext->subType = static_cast<PhotoSubType>(GetPhotoSubtype(env, asset));

    RequestExecute(env, aniContext);
    return RequestComplete(env, aniContext);
}

static bool IsFastRequestCanceled(const std::string &requestId, std::string &photoId)
{
    AssetHandler *assetHandler = nullptr;
    if (!inProcessFastRequests.Find(requestId, assetHandler)) {
        ANI_ERR_LOG("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        ANI_ERR_LOG("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    inProcessFastRequests.Erase(requestId);
    return true;
}

static bool IsMapRecordCanceled(const std::string &requestId, std::string &photoId, ani_env *env)
{
    AssetHandler *assetHandler = nullptr;
    if (!IsInProcessInMapRecord(requestId, assetHandler)) {
        ANI_ERR_LOG("requestId(%{public}s) not in progress.", requestId.c_str());
        return false;
    }

    if (assetHandler == nullptr) {
        ANI_ERR_LOG("assetHandler is nullptr.");
        return false;
    }
    photoId = assetHandler->photoId;
    DeleteInProcessMapRecord(assetHandler->requestUri, assetHandler->requestId);
    DeleteAssetHandlerSafe(assetHandler, env);
    return true;
}

void MediaAssetManagerAni::CancelProcessImage(const std::string &photoId)
{
    std::string uriStr = PAH_CANCEL_PROCESS_IMAGE;
    MediaLibraryAniUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    std::vector<std::string> columns { photoId };
    UserFileClient::Query(uri, predicates, columns, errCode);
}

void MediaAssetManagerAni::CancelRequestExecute(unique_ptr<MediaAssetManagerAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Ani context is null");
    MediaAssetManagerAni::CancelProcessImage(context->photoId);
}

void MediaAssetManagerAni::CancelRequestComplete(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context)
{
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void MediaAssetManagerAni::CancelRequest(ani_env *env, [[maybe_unused]] ani_class clazz,
    ani_object context, ani_string requestIdAni)
{
    string requestId;
    CHECK_ARGS_RET_VOID(env,
        MediaLibraryAniUtils::GetParamStringWithLength(env, requestIdAni, REQUEST_ID_MAX_LEN, requestId),
        OHOS_INVALID_PARAM_CODE);
    std::string photoId = "";
    bool hasFastRequestInProcess = IsFastRequestCanceled(requestId, photoId);
    bool hasMapRecordInProcess = IsMapRecordCanceled(requestId, photoId, env);
    if (hasFastRequestInProcess || hasMapRecordInProcess) {
        unique_ptr<MediaAssetManagerAniContext> aniContext = make_unique<MediaAssetManagerAniContext>();
        aniContext->photoId = photoId;
        CancelRequestExecute(aniContext);
        CancelRequestComplete(env, aniContext);
    }
}

} // namespace Media