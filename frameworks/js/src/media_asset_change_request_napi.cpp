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

#define MLOG_TAG "MediaAssetChangeRequestNapi"

#include "media_asset_change_request_napi.h"

#include <fcntl.h>
#include <functional>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>

#include "ability_context.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "delete_callback.h"
#include "directory_ex.h"
#include "file_uri.h"
#include "image_packer.h"
#include "ipc_skeleton.h"
#include "js_native_api.h"
#include "js_native_api_types.h"
#include "media_asset_edit_data_napi.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_tracer.h"
#include "modal_ui_extension_config.h"
#ifdef ABILITY_CAMERA_SUPPORT
#include "output/deferred_photo_proxy_napi.h"
#endif
#include "permission_utils.h"
#include "securec.h"
#ifdef HAS_ACE_ENGINE_PART
#include "ui_content.h"
#endif
#include "unique_fd.h"
#include "userfile_client.h"
#include "userfile_manager_types.h"
#include "want.h"

using namespace std;
using namespace OHOS::Security::AccessToken;

namespace OHOS::Media {
static const string MEDIA_ASSET_CHANGE_REQUEST_CLASS = "MediaAssetChangeRequest";
thread_local napi_ref MediaAssetChangeRequestNapi::constructor_ = nullptr;

constexpr int64_t CREATE_ASSET_REQUEST_PENDING = -4;

constexpr int32_t YES = 1;
constexpr int32_t NO = 0;

constexpr int32_t USER_COMMENT_MAX_LEN = 420;
constexpr int32_t MAX_DELETE_NUMBER = 300;

const std::string PAH_SUBTYPE = "subtype";
const std::string CAMERA_SHOT_KEY = "cameraShotKey";
const std::map<std::string, std::string> PHOTO_CREATE_OPTIONS_PARAM = {
    { PAH_SUBTYPE, PhotoColumn::PHOTO_SUBTYPE },
    { CAMERA_SHOT_KEY, PhotoColumn::CAMERA_SHOT_KEY },
};

const std::string TITLE = "title";
const std::map<std::string, std::string> CREATE_OPTIONS_PARAM = {
    { TITLE, PhotoColumn::MEDIA_TITLE },
    { PAH_SUBTYPE, PhotoColumn::PHOTO_SUBTYPE },
};

const std::string DEFAULT_TITLE_TIME_FORMAT = "%Y%m%d_%H%M%S";
const std::string DEFAULT_TITLE_IMG_PREFIX = "IMG_";
const std::string DEFAULT_TITLE_VIDEO_PREFIX = "VID_";
const std::string MOVING_PHOTO_VIDEO_EXTENSION = "mp4";

int32_t MediaDataSource::ReadData(const shared_ptr<AVSharedMemory>& mem, uint32_t length)
{
    if (readPos_ >= size_) {
        NAPI_ERR_LOG("Failed to check read position");
        return SOURCE_ERROR_EOF;
    }

    if (memcpy_s(mem->GetBase(), mem->GetSize(), (char*)buffer_ + readPos_, length) != E_OK) {
        NAPI_ERR_LOG("Failed to copy buffer to mem");
        return SOURCE_ERROR_IO;
    }
    readPos_ += static_cast<int64_t>(length);
    return static_cast<int32_t>(length);
}

int32_t MediaDataSource::ReadAt(const std::shared_ptr<AVSharedMemory>& mem, uint32_t length, int64_t pos)
{
    readPos_ = pos;
    return ReadData(mem, length);
}

int32_t MediaDataSource::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory>& mem)
{
    readPos_ = pos;
    return ReadData(mem, length);
}

int32_t MediaDataSource::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory>& mem)
{
    return ReadData(mem, length);
}

int32_t MediaDataSource::GetSize(int64_t& size)
{
    size = size_;
    return E_OK;
}

napi_value MediaAssetChangeRequestNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_ASSET_CHANGE_REQUEST_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("createAssetRequest", JSCreateAssetRequest),
            DECLARE_NAPI_STATIC_FUNCTION("createImageAssetRequest", JSCreateImageAssetRequest),
            DECLARE_NAPI_STATIC_FUNCTION("createVideoAssetRequest", JSCreateVideoAssetRequest),
            DECLARE_NAPI_STATIC_FUNCTION("deleteAssets", JSDeleteAssets),
            DECLARE_NAPI_FUNCTION("getAsset", JSGetAsset),
            DECLARE_NAPI_FUNCTION("setEditData", JSSetEditData),
            DECLARE_NAPI_FUNCTION("setFavorite", JSSetFavorite),
            DECLARE_NAPI_FUNCTION("setHidden", JSSetHidden),
            DECLARE_NAPI_FUNCTION("setTitle", JSSetTitle),
            DECLARE_NAPI_FUNCTION("setUserComment", JSSetUserComment),
            DECLARE_NAPI_FUNCTION("getWriteCacheHandler", JSGetWriteCacheHandler),
            DECLARE_NAPI_FUNCTION("setLocation", JSSetLocation),
            DECLARE_NAPI_FUNCTION("addResource", JSAddResource),
            DECLARE_NAPI_FUNCTION("setCameraShotKey", JSSetCameraShotKey),
            DECLARE_NAPI_FUNCTION("saveCameraPhoto", JSSaveCameraPhoto),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value MediaAssetChangeRequestNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Failed to check new.target");

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    napi_valuetype valueType;
    FileAssetNapi* fileAssetNapi;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");
    CHECK_ARGS(env, napi_typeof(env, argv[PARAM0], &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");
    CHECK_ARGS(env, napi_unwrap(env, argv[PARAM0], reinterpret_cast<void**>(&fileAssetNapi)), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, fileAssetNapi != nullptr, "Failed to get FileAssetNapi object");

    auto fileAssetPtr = fileAssetNapi->GetFileAssetInstance();
    CHECK_COND_WITH_MESSAGE(env, fileAssetPtr != nullptr, "fileAsset is null");
    CHECK_COND_WITH_MESSAGE(env,
        fileAssetPtr->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER &&
            (fileAssetPtr->GetMediaType() == MEDIA_TYPE_IMAGE || fileAssetPtr->GetMediaType() == MEDIA_TYPE_VIDEO),
        "Unsupported type of fileAsset");

    unique_ptr<MediaAssetChangeRequestNapi> obj = make_unique<MediaAssetChangeRequestNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    obj->fileAsset_ = fileAssetPtr;
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), MediaAssetChangeRequestNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

static void DeleteCache(const string& cacheFileName)
{
    if (cacheFileName.empty()) {
        return;
    }

    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + cacheFileName;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri deleteCacheUri(uri);
    DataShare::DataSharePredicates predicates;
    int32_t ret = UserFileClient::Delete(deleteCacheUri, predicates);
    if (ret < 0) {
        NAPI_WARN_LOG("Failed to delete cache: %{private}s, error: %{public}d", cacheFileName.c_str(), ret);
    }
}

void MediaAssetChangeRequestNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* assetChangeRequest = reinterpret_cast<MediaAssetChangeRequestNapi*>(nativeObject);
    if (assetChangeRequest == nullptr) {
        return;
    }

    DeleteCache(assetChangeRequest->cacheFileName_);
    DeleteCache(assetChangeRequest->cacheMovingPhotoVideoName_);

    delete assetChangeRequest;
    assetChangeRequest = nullptr;
}

shared_ptr<FileAsset> MediaAssetChangeRequestNapi::GetFileAssetInstance() const
{
    return fileAsset_;
}

#ifdef ABILITY_CAMERA_SUPPORT
sptr<CameraStandard::DeferredPhotoProxy> MediaAssetChangeRequestNapi::GetPhotoProxyObj()
{
    return photoProxy_;
}

void MediaAssetChangeRequestNapi::ReleasePhotoProxyObj()
{
    photoProxy_.clear();
    photoProxy_ = nullptr;
}
#endif

void MediaAssetChangeRequestNapi::RecordChangeOperation(AssetChangeOperation changeOperation)
{
    if ((changeOperation == AssetChangeOperation::GET_WRITE_CACHE_HANDLER ||
            changeOperation == AssetChangeOperation::ADD_RESOURCE) &&
        Contains(AssetChangeOperation::CREATE_FROM_SCRATCH)) {
        assetChangeOperations_.insert(assetChangeOperations_.begin() + 1, changeOperation);
        return;
    }
    assetChangeOperations_.push_back(changeOperation);
}

bool MediaAssetChangeRequestNapi::Contains(AssetChangeOperation changeOperation) const
{
    return std::find(assetChangeOperations_.begin(), assetChangeOperations_.end(), changeOperation) !=
           assetChangeOperations_.end();
}

bool MediaAssetChangeRequestNapi::IsMovingPhoto() const
{
    return fileAsset_ != nullptr &&
        fileAsset_->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
}

bool MediaAssetChangeRequestNapi::CheckMovingPhotoResource(ResourceType resourceType) const
{
    if (resourceType == ResourceType::INVALID_RESOURCE) {
        NAPI_ERR_LOG("Invalid resource type");
        return false;
    }

    bool isResourceTypeVaild =
        std::find(addResourceTypes_.begin(), addResourceTypes_.end(), resourceType) == addResourceTypes_.end();
    int addResourceTimes =
        std::count(assetChangeOperations_.begin(), assetChangeOperations_.end(), AssetChangeOperation::ADD_RESOURCE);
    return isResourceTypeVaild && addResourceTimes <= 1; // currently, add resource no more than once
}

bool MediaAssetChangeRequestNapi::CheckMovingPhotoWriteOperation()
{
    bool containsAddResource = Contains(AssetChangeOperation::ADD_RESOURCE);
    if (!containsAddResource) {
        return true;
    }

    bool isCreation = Contains(AssetChangeOperation::CREATE_FROM_SCRATCH);
    if (!isCreation) {
        NAPI_ERR_LOG("Moving photo is not supported to edit now");
        return false;
    }

    int addResourceTimes =
        std::count(assetChangeOperations_.begin(), assetChangeOperations_.end(), AssetChangeOperation::ADD_RESOURCE);
    bool isImageExist = std::find(addResourceTypes_.begin(), addResourceTypes_.end(), ResourceType::IMAGE_RESOURCE) !=
                        addResourceTypes_.end();
    bool isVideoExist = std::find(addResourceTypes_.begin(), addResourceTypes_.end(), ResourceType::VIDEO_RESOURCE) !=
                        addResourceTypes_.end();
    return addResourceTimes == 2 && isImageExist && isVideoExist; // must add resource 2 times with image and video
}

bool MediaAssetChangeRequestNapi::CheckChangeOperations(napi_env env)
{
    if (assetChangeOperations_.empty()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "None request to apply");
        return false;
    }

    bool isCreateFromScratch = Contains(AssetChangeOperation::CREATE_FROM_SCRATCH);
    bool isCreateFromUri = Contains(AssetChangeOperation::CREATE_FROM_URI);
    bool containsEdit = Contains(AssetChangeOperation::SET_EDIT_DATA);
    bool containsGetHandler = Contains(AssetChangeOperation::GET_WRITE_CACHE_HANDLER);
    bool containsAddResource = Contains(AssetChangeOperation::ADD_RESOURCE);
    if ((isCreateFromScratch || containsEdit) && !containsGetHandler && !containsAddResource) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Cannot create or edit asset without data to write");
        return false;
    }

    if (containsEdit && (isCreateFromScratch || isCreateFromUri)) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Cannot create together with edit");
        return false;
    }

    auto fileAsset = GetFileAssetInstance();
    if (fileAsset == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "fileAsset is null");
        return false;
    }

    AssetChangeOperation firstOperation = assetChangeOperations_.front();
    if (fileAsset->GetId() <= 0 && firstOperation != AssetChangeOperation::CREATE_FROM_SCRATCH &&
        firstOperation != AssetChangeOperation::CREATE_FROM_URI) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid asset change request");
        return false;
    }

    bool isMovingPhoto = IsMovingPhoto();
    if (isMovingPhoto && !CheckMovingPhotoWriteOperation()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid write operation for moving photo");
        return false;
    }

    return true;
}

void MediaAssetChangeRequestNapi::SetCacheFileName(string& fileName)
{
    cacheFileName_ = fileName;
}

void MediaAssetChangeRequestNapi::SetCacheMovingPhotoVideoName(string& fileName)
{
    cacheMovingPhotoVideoName_ = fileName;
}

string MediaAssetChangeRequestNapi::GetFileRealPath() const
{
    return realPath_;
}

AddResourceMode MediaAssetChangeRequestNapi::GetAddResourceMode() const
{
    return addResourceMode_;
}

void* MediaAssetChangeRequestNapi::GetDataBuffer() const
{
    return dataBuffer_;
}

size_t MediaAssetChangeRequestNapi::GetDataBufferSize() const
{
    return dataBufferSize_;
}

string MediaAssetChangeRequestNapi::GetMovingPhotoVideoPath() const
{
    return movingPhotoVideoRealPath_;
}

AddResourceMode MediaAssetChangeRequestNapi::GetMovingPhotoVideoMode() const
{
    return movingPhotoVideoResourceMode_;
}

void* MediaAssetChangeRequestNapi::GetMovingPhotoVideoBuffer() const
{
    return movingPhotoVideoDataBuffer_;
}

size_t MediaAssetChangeRequestNapi::GetMovingPhotoVideoSize() const
{
    return movingPhotoVideoBufferSize_;
}

string MediaAssetChangeRequestNapi::GetCacheMovingPhotoVideoName() const
{
    return cacheMovingPhotoVideoName_;
}

napi_value MediaAssetChangeRequestNapi::JSGetAsset(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_ARGS_THROW_INVALID_PARAM(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO));

    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);
    if (fileAsset->GetId() > 0) {
        return FileAssetNapi::CreatePhotoAsset(env, fileAsset);
    }

    // FileAsset object has not been actually created, return null.
    napi_value nullValue;
    napi_get_null(env, &nullValue);
    return nullValue;
}

static bool HasWritePermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_WRITE_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

static bool CheckMovingPhotoCreationArgs(MediaAssetChangeRequestAsyncContext& context)
{
    bool isValid = false;
    int32_t mediaType = context.valuesBucket.Get(MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    if (!isValid) {
        NAPI_ERR_LOG("Failed to get media type");
        return false;
    }

    if (mediaType != static_cast<int32_t>(MEDIA_TYPE_IMAGE)) {
        NAPI_ERR_LOG("Failed to cehck media type (%{public}d) for moving photo", mediaType);
        return false;
    }

    string extension = context.valuesBucket.Get(ASSET_EXTENTION, isValid);
    if (isValid) {
        return MediaFileUtils::CheckMovingPhotoExtension(extension);
    }

    string displayName = context.valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
    return isValid && MediaFileUtils::CheckMovingPhotoExtension(MediaFileUtils::GetExtensionFromPath(displayName));
}

static napi_status CheckCreateOption(MediaAssetChangeRequestAsyncContext& context, bool isSystemApi)
{
    bool isValid = false;
    int32_t subtype = context.valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);
    if (isValid) {
        if (subtype < static_cast<int32_t>(PhotoSubType::DEFAULT) ||
            subtype >= static_cast<int32_t>(PhotoSubType::SUBTYPE_END)) {
            NAPI_ERR_LOG("Failed to check subtype: %{public}d", subtype);
            return napi_invalid_arg;
        }

        // check media type and extension for moving photo
        if (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) &&
            !CheckMovingPhotoCreationArgs(context)) {
            NAPI_ERR_LOG("Failed to check creation args for moving photo");
            return napi_invalid_arg;
        }

        // check subtype for public api
        if (!isSystemApi && subtype != static_cast<int32_t>(PhotoSubType::DEFAULT) &&
            subtype != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            NAPI_ERR_LOG("Failed to check subtype: %{public}d", subtype);
            return napi_invalid_arg;
        }
    }

    string cameraShotKey = context.valuesBucket.Get(PhotoColumn::CAMERA_SHOT_KEY, isValid);
    if (isValid) {
        if (cameraShotKey.size() < CAMERA_SHOT_KEY_SIZE) {
            NAPI_ERR_LOG("cameraShotKey is not null but is less than CAMERA_SHOT_KEY_SIZE");
            return napi_invalid_arg;
        }
        if (subtype == static_cast<int32_t>(PhotoSubType::SCREENSHOT)) {
            NAPI_ERR_LOG("cameraShotKey is not null with subtype is SCREENSHOT");
            return napi_invalid_arg;
        } else {
            context.valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CAMERA));
        }
    }
    return napi_ok;
}

static napi_status ParseAssetCreateOptions(napi_env env, napi_value arg, MediaAssetChangeRequestAsyncContext& context,
    const map<string, string>& createOptionsMap, bool isSystemApi)
{
    for (const auto& iter : createOptionsMap) {
        string param = iter.first;
        bool present = false;
        napi_status result = napi_has_named_property(env, arg, param.c_str(), &present);
        CHECK_COND_RET(result == napi_ok, result, "Failed to check named property");
        if (!present) {
            continue;
        }
        napi_value value;
        result = napi_get_named_property(env, arg, param.c_str(), &value);
        CHECK_COND_RET(result == napi_ok, result, "Failed to get named property");
        napi_valuetype valueType = napi_undefined;
        result = napi_typeof(env, value, &valueType);
        CHECK_COND_RET(result == napi_ok, result, "Failed to get value type");
        if (valueType == napi_number) {
            int32_t number = 0;
            result = napi_get_value_int32(env, value, &number);
            CHECK_COND_RET(result == napi_ok, result, "Failed to get int32_t");
            context.valuesBucket.Put(iter.second, number);
        } else if (valueType == napi_boolean) {
            bool isTrue = false;
            result = napi_get_value_bool(env, value, &isTrue);
            CHECK_COND_RET(result == napi_ok, result, "Failed to get bool");
            context.valuesBucket.Put(iter.second, isTrue);
        } else if (valueType == napi_string) {
            char buffer[ARG_BUF_SIZE];
            size_t res = 0;
            result = napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res);
            CHECK_COND_RET(result == napi_ok, result, "Failed to get string");
            context.valuesBucket.Put(iter.second, string(buffer));
        } else if (valueType == napi_undefined || valueType == napi_null) {
            continue;
        } else {
            NAPI_ERR_LOG("valueType %{public}d is unaccepted", static_cast<int>(valueType));
            return napi_invalid_arg;
        }
    }
    return CheckCreateOption(context, isSystemApi);
}

static napi_value ParseArgsCreateAssetSystem(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext>& context)
{
    // Parse displayName.
    string displayName;
    MediaType mediaType;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM1], displayName) == napi_ok,
        "Failed to get displayName");
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayName(displayName) == E_OK, "Failed to check displayName");
    mediaType = MediaFileUtils::GetMediaType(displayName);
    CHECK_COND_WITH_MESSAGE(env, mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO, "Invalid file type");
    context->valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));

    // Parse options if exists.
    if (context->argc == ARGS_THREE) {
        napi_valuetype valueType;
        napi_value createOptionsNapi = context->argv[PARAM2];
        CHECK_COND_WITH_MESSAGE(
            env, napi_typeof(env, createOptionsNapi, &valueType) == napi_ok, "Failed to get napi type");
        if (valueType != napi_object) {
            NAPI_ERR_LOG("Napi type is wrong in PhotoCreateOptions");
            return nullptr;
        }

        CHECK_COND_WITH_MESSAGE(env,
            ParseAssetCreateOptions(env, createOptionsNapi, *context, PHOTO_CREATE_OPTIONS_PARAM, true) == napi_ok,
            "Parse PhotoCreateOptions failed");
    }
    RETURN_NAPI_TRUE(env);
}

static napi_value ParseArgsCreateAssetCommon(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext>& context)
{
    // Parse photoType.
    MediaType mediaType;
    int32_t type = 0;
    CHECK_COND_WITH_MESSAGE(
        env, napi_get_value_int32(env, context->argv[PARAM1], &type) == napi_ok, "Failed to get photoType");
    mediaType = static_cast<MediaType>(type);
    CHECK_COND_WITH_MESSAGE(env, mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO, "Invalid photoType");

    // Parse extension.
    string extension;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM2], extension) == napi_ok,
        "Failed to get extension");
    CHECK_COND_WITH_MESSAGE(
        env, mediaType == MediaFileUtils::GetMediaType("." + extension), "Failed to check extension");
    context->valuesBucket.Put(ASSET_EXTENTION, extension);
    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));

    // Parse options if exists.
    if (context->argc == ARGS_FOUR) {
        napi_valuetype valueType;
        napi_value createOptionsNapi = context->argv[PARAM3];
        CHECK_COND_WITH_MESSAGE(
            env, napi_typeof(env, createOptionsNapi, &valueType) == napi_ok, "Failed to get napi type");
        if (valueType != napi_object) {
            NAPI_ERR_LOG("Napi type is wrong in CreateOptions");
            return nullptr;
        }

        CHECK_COND_WITH_MESSAGE(env,
            ParseAssetCreateOptions(env, createOptionsNapi, *context, CREATE_OPTIONS_PARAM, false) == napi_ok,
            "Parse CreateOptions failed");
    }

    bool isValid = false;
    string title = context->valuesBucket.Get(PhotoColumn::MEDIA_TITLE, isValid);
    if (!isValid) {
        title = mediaType == MEDIA_TYPE_IMAGE ? DEFAULT_TITLE_IMG_PREFIX : DEFAULT_TITLE_VIDEO_PREFIX;
        title += MediaFileUtils::StrCreateTime(DEFAULT_TITLE_TIME_FORMAT, MediaFileUtils::UTCTimeSeconds());
        context->valuesBucket.Put(PhotoColumn::MEDIA_TITLE, title);
    }

    string displayName = title + "." + extension;
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayName(displayName) == E_OK, "Failed to check displayName");
    context->valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    RETURN_NAPI_TRUE(env);
}

static napi_value ParseArgsCreateAsset(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext>& context)
{
    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_FOUR;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, maxArgs) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, MediaAssetChangeRequestNapi::InitUserFileClient(env, info), JS_INNER_FAIL);

    napi_valuetype valueType;
    CHECK_COND_WITH_MESSAGE(
        env, napi_typeof(env, context->argv[PARAM1], &valueType) == napi_ok, "Failed to get napi type");
    if (valueType == napi_string) {
        if (!MediaLibraryNapiUtils::IsSystemApp()) {
            NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
            return nullptr;
        }
        CHECK_COND_WITH_MESSAGE(env, context->argc <= ARGS_THREE, "Number of args is invalid");
        return ParseArgsCreateAssetSystem(env, info, context);
    } else if (valueType == napi_number) {
        return ParseArgsCreateAssetCommon(env, info, context);
    } else {
        NAPI_ERR_LOG("param type %{public}d is invalid", static_cast<int32_t>(valueType));
        return nullptr;
    }
}

napi_value MediaAssetChangeRequestNapi::JSCreateAssetRequest(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsCreateAsset(env, info, asyncContext), "Failed to parse args");

    bool isValid = false;
    string displayName = asyncContext->valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
    int32_t subtype = asyncContext->valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid); // default is 0
    auto emptyFileAsset = make_unique<FileAsset>();
    emptyFileAsset->SetDisplayName(displayName);
    emptyFileAsset->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
    emptyFileAsset->SetMediaType(MediaFileUtils::GetMediaType(displayName));
    emptyFileAsset->SetPhotoSubType(subtype);
    emptyFileAsset->SetTimePending(CREATE_ASSET_REQUEST_PENDING);
    emptyFileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    napi_value fileAssetNapi = FileAssetNapi::CreateFileAsset(env, emptyFileAsset);
    CHECK_COND(env, fileAssetNapi != nullptr, JS_INNER_FAIL);

    napi_value constructor = nullptr;
    napi_value instance = nullptr;
    CHECK_ARGS(env, napi_get_reference_value(env, constructor_, &constructor), JS_INNER_FAIL);
    CHECK_ARGS(env, napi_new_instance(env, constructor, 1, &fileAssetNapi, &instance), JS_INNER_FAIL);
    CHECK_COND(env, instance != nullptr, JS_INNER_FAIL);

    MediaAssetChangeRequestNapi* changeRequest = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, instance, reinterpret_cast<void**>(&changeRequest)), JS_INNER_FAIL);
    CHECK_COND(env, changeRequest != nullptr, JS_INNER_FAIL);
    changeRequest->creationValuesBucket_ = std::move(asyncContext->valuesBucket);
    changeRequest->RecordChangeOperation(AssetChangeOperation::CREATE_FROM_SCRATCH);
    return instance;
}

static napi_value ParseFileUri(napi_env env, napi_value arg, MediaType mediaType,
    unique_ptr<MediaAssetChangeRequestAsyncContext>& context)
{
    string fileUriStr;
    CHECK_COND_WITH_MESSAGE(
        env, MediaLibraryNapiUtils::GetParamStringPathMax(env, arg, fileUriStr) == napi_ok, "Failed to get fileUri");
    AppFileService::ModuleFileUri::FileUri fileUri(fileUriStr);
    string path = fileUri.GetRealPath();
    CHECK_COND(env, PathToRealPath(path, context->realPath), JS_ERR_NO_SUCH_FILE);

    CHECK_COND_WITH_MESSAGE(env, mediaType == MediaFileUtils::GetMediaType(context->realPath), "Invalid file type");
    RETURN_NAPI_TRUE(env);
}

static napi_value ParseArgsCreateAssetFromFileUri(napi_env env, napi_callback_info info, MediaType mediaType,
    unique_ptr<MediaAssetChangeRequestAsyncContext>& context)
{
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_TWO, ARGS_TWO) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, MediaAssetChangeRequestNapi::InitUserFileClient(env, info), JS_INNER_FAIL);
    return ParseFileUri(env, context->argv[PARAM1], mediaType, context);
}

napi_value MediaAssetChangeRequestNapi::CreateAssetRequestFromRealPath(napi_env env, const string& realPath)
{
    string displayName = MediaFileUtils::GetFileName(realPath);
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayName(displayName) == E_OK, "Invalid fileName");
    string title = MediaFileUtils::GetTitleFromDisplayName(displayName);
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    auto emptyFileAsset = make_unique<FileAsset>();
    emptyFileAsset->SetDisplayName(displayName);
    emptyFileAsset->SetTitle(title);
    emptyFileAsset->SetMediaType(mediaType);
    emptyFileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::DEFAULT));
    emptyFileAsset->SetTimePending(CREATE_ASSET_REQUEST_PENDING);
    emptyFileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    napi_value fileAssetNapi = FileAssetNapi::CreateFileAsset(env, emptyFileAsset);
    CHECK_COND(env, fileAssetNapi != nullptr, JS_INNER_FAIL);

    napi_value constructor = nullptr;
    napi_value instance = nullptr;
    CHECK_ARGS(env, napi_get_reference_value(env, constructor_, &constructor), JS_INNER_FAIL);
    CHECK_ARGS(env, napi_new_instance(env, constructor, 1, &fileAssetNapi, &instance), JS_INNER_FAIL);
    CHECK_COND(env, instance != nullptr, JS_INNER_FAIL);

    MediaAssetChangeRequestNapi* changeRequest = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, instance, reinterpret_cast<void**>(&changeRequest)), JS_INNER_FAIL);
    CHECK_COND(env, changeRequest != nullptr, JS_INNER_FAIL);
    changeRequest->realPath_ = realPath;
    changeRequest->creationValuesBucket_.Put(MEDIA_DATA_DB_NAME, displayName);
    changeRequest->creationValuesBucket_.Put(ASSET_EXTENTION, MediaFileUtils::GetExtensionFromPath(displayName));
    changeRequest->creationValuesBucket_.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    changeRequest->creationValuesBucket_.Put(PhotoColumn::MEDIA_TITLE, title);
    changeRequest->RecordChangeOperation(AssetChangeOperation::CREATE_FROM_URI);
    return instance;
}

napi_value MediaAssetChangeRequestNapi::JSCreateImageAssetRequest(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsCreateAssetFromFileUri(env, info, MediaType::MEDIA_TYPE_IMAGE, asyncContext),
        "Failed to parse args");
    return CreateAssetRequestFromRealPath(env, asyncContext->realPath);
}

napi_value MediaAssetChangeRequestNapi::JSCreateVideoAssetRequest(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsCreateAssetFromFileUri(env, info, MediaType::MEDIA_TYPE_VIDEO, asyncContext),
        "Failed to parse args");
    return CreateAssetRequestFromRealPath(env, asyncContext->realPath);
}

static napi_value initDeleteRequest(napi_env env, MediaAssetChangeRequestAsyncContext& context,
    OHOS::AAFwk::Want& request, shared_ptr<DeleteCallback>& callback)
{
    request.SetElementName(DELETE_UI_PACKAGE_NAME, DELETE_UI_EXT_ABILITY_NAME);
    request.SetParam(DELETE_UI_EXTENSION_TYPE, DELETE_UI_REQUEST_TYPE);

    CHECK_COND(env, !context.appName.empty(), JS_INNER_FAIL);
    request.SetParam(DELETE_UI_APPNAME, context.appName);

    request.SetParam(DELETE_UI_URIS, context.uris);
    callback->SetUris(context.uris);

    napi_valuetype valueType = napi_undefined;
    CHECK_COND_WITH_MESSAGE(env, context.argc >= ARGS_THREE && context.argc <= ARGS_FOUR, "Failed to check args");
    napi_value func = context.argv[PARAM1];
    CHECK_ARGS(env, napi_typeof(env, func, &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_function, "Failed to check args");
    callback->SetFunc(func);
    RETURN_NAPI_TRUE(env);
}

static napi_value ParseArgsDeleteAssets(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext>& context)
{
    constexpr size_t minArgs = ARGS_THREE;
    constexpr size_t maxArgs = ARGS_FOUR;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, maxArgs) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, MediaAssetChangeRequestNapi::InitUserFileClient(env, info), JS_INNER_FAIL);

    napi_valuetype valueType = napi_undefined;
    CHECK_ARGS(env, napi_typeof(env, context->argv[PARAM1], &valueType), JS_INNER_FAIL);
    CHECK_COND(env, valueType == napi_function, JS_INNER_FAIL);

    vector<string> uris;
    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[PARAM2], napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    if (valueType == napi_string) { // array of asset uri
        CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetStringArray(env, napiValues, uris));
    } else if (valueType == napi_object) { // array of asset object
        CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetUriArrayFromAssets(env, napiValues, uris));
    } else {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid type");
        return nullptr;
    }

    CHECK_COND_WITH_MESSAGE(env, !uris.empty(), "Failed to check empty array");
    for (const auto& uri : uris) {
        CHECK_COND(env, uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos, JS_E_URI);
    }

    context->predicates.In(PhotoColumn::MEDIA_ID, uris);
    context->valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeSeconds());
    context->uris.assign(uris.begin(), uris.end());
    RETURN_NAPI_TRUE(env);
}

static void DeleteAssetsExecute(napi_env env, void* data)
{
    auto* context = static_cast<MediaAssetChangeRequestAsyncContext*>(data);
    string trashUri = PAH_TRASH_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(trashUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(trashUri);
    int32_t changedRows = UserFileClient::Update(updateAssetUri, context->predicates, context->valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Failed to delete assets, err: %{public}d", changedRows);
    }
}

static void DeleteAssetsCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<MediaAssetChangeRequestAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value MediaAssetChangeRequestNapi::JSDeleteAssets(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsDeleteAssets(env, info, asyncContext), "Failed to parse args");
    if (MediaLibraryNapiUtils::IsSystemApp()) {
        return MediaLibraryNapiUtils::NapiCreateAsyncWork(
            env, asyncContext, "ChangeRequestDeleteAssets", DeleteAssetsExecute, DeleteAssetsCompleteCallback);
    }

#ifdef HAS_ACE_ENGINE_PART
    // Deletion control by ui extension
    CHECK_COND(env, HasWritePermission(), OHOS_PERMISSION_DENIED_CODE);
    CHECK_COND_WITH_MESSAGE(
        env, asyncContext->uris.size() <= MAX_DELETE_NUMBER, "No more than 300 assets can be deleted at one time");
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, asyncContext->argv[PARAM0]);
    CHECK_COND_WITH_MESSAGE(env, context != nullptr, "Failed to get stage mode context");
    auto abilityContext = OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    CHECK_COND(env, abilityContext != nullptr, JS_INNER_FAIL);
    auto abilityInfo = abilityContext->GetAbilityInfo();
    abilityContext->GetResourceManager()->GetStringById(abilityInfo->labelId, asyncContext->appName);
    auto uiContent = abilityContext->GetUIContent();
    CHECK_COND(env, uiContent != nullptr, JS_INNER_FAIL);

    auto callback = std::make_shared<DeleteCallback>(env, uiContent);
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        std::bind(&DeleteCallback::OnRelease, callback, std::placeholders::_1),
        std::bind(&DeleteCallback::OnResult, callback, std::placeholders::_1, std::placeholders::_2),
        std::bind(&DeleteCallback::OnReceive, callback, std::placeholders::_1),
        std::bind(
            &DeleteCallback::OnError, callback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    OHOS::AAFwk::Want request;
    CHECK_COND(env, initDeleteRequest(env, *asyncContext, request, callback), JS_INNER_FAIL);

    int32_t sessionId = uiContent->CreateModalUIExtension(request, extensionCallback, config);
    CHECK_COND(env, sessionId != 0, JS_INNER_FAIL);
    callback->SetSessionId(sessionId);
    RETURN_NAPI_UNDEFINED(env);
#else
    NapiError::ThrowError(env, JS_INNER_FAIL, "ace_engine is not support");
    return nullptr;
#endif
}

napi_value MediaAssetChangeRequestNapi::JSSetEditData(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    napi_value arg = asyncContext->argv[PARAM0];
    napi_valuetype valueType;
    MediaAssetEditDataNapi* editDataNapi;
    CHECK_ARGS(env, napi_typeof(env, arg, &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");
    CHECK_ARGS(env, napi_unwrap(env, arg, reinterpret_cast<void**>(&editDataNapi)), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, editDataNapi != nullptr, "Failed to get MediaAssetEditDataNapi object");

    shared_ptr<MediaAssetEditData> editData = editDataNapi->GetMediaAssetEditData();
    CHECK_COND_WITH_MESSAGE(env, editData != nullptr, "editData is null");
    CHECK_COND_WITH_MESSAGE(env, !editData->GetCompatibleFormat().empty(), "Invalid compatibleFormat");
    CHECK_COND_WITH_MESSAGE(env, !editData->GetFormatVersion().empty(), "Invalid formatVersion");
    CHECK_COND_WITH_MESSAGE(env, !editData->GetData().empty(), "Invalid data");
    changeRequest->editData_ = editData;
    changeRequest->RecordChangeOperation(AssetChangeOperation::SET_EDIT_DATA);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSSetFavorite(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    bool isFavorite;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, isFavorite) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND(env, changeRequest->GetFileAssetInstance() != nullptr, JS_INNER_FAIL);
    changeRequest->GetFileAssetInstance()->SetFavorite(isFavorite);
    changeRequest->RecordChangeOperation(AssetChangeOperation::SET_FAVORITE);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSSetHidden(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    bool isHidden;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, isHidden) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND(env, changeRequest->GetFileAssetInstance() != nullptr, JS_INNER_FAIL);
    changeRequest->GetFileAssetInstance()->SetHidden(isHidden);
    changeRequest->RecordChangeOperation(AssetChangeOperation::SET_HIDDEN);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSSetTitle(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    string title;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, title) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);
    string extension = MediaFileUtils::SplitByChar(fileAsset->GetDisplayName(), '.');
    string displayName = title + "." + extension;
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayName(displayName) == E_OK, "Invalid title");

    fileAsset->SetTitle(title);
    fileAsset->SetDisplayName(displayName);
    changeRequest->RecordChangeOperation(AssetChangeOperation::SET_TITLE);

    // Merge the creation and SET_TITLE operations.
    if (changeRequest->Contains(AssetChangeOperation::CREATE_FROM_SCRATCH) ||
        changeRequest->Contains(AssetChangeOperation::CREATE_FROM_URI)) {
        changeRequest->creationValuesBucket_.valuesMap[MEDIA_DATA_DB_NAME] = displayName;
        changeRequest->creationValuesBucket_.valuesMap[PhotoColumn::MEDIA_TITLE] = title;
    }
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSSetLocation(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    double latitude;
    double longitude;
    MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_TWO, ARGS_TWO);
    MediaLibraryNapiUtils::GetDouble(env, asyncContext->argv[0], latitude);
    MediaLibraryNapiUtils::GetDouble(env, asyncContext->argv[1], longitude);
    asyncContext->objectInfo->fileAsset_->SetLongitude(longitude);
    asyncContext->objectInfo->fileAsset_->SetLatitude(latitude);
    asyncContext->objectInfo->assetChangeOperations_.push_back(AssetChangeOperation::SET_LOCATION);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);
    return result;
}

#ifdef ABILITY_CAMERA_SUPPORT
static int SaveImage(const string &fileName, void *output, size_t writeSize)
{
    Uri fileUri(fileName);
    int fd = UserFileClient::OpenFile(fileUri, "rw");
    if (fd < 0) {
        NAPI_ERR_LOG("fd.Get() < 0 fd %{public}d status %{public}d", fd, errno);
        return E_ERR;
    }

    int ret = write(fd, output, writeSize);
    close(fd);
    if (ret < 0) {
        NAPI_ERR_LOG("write err %{public}d", errno);
        return ret;
    }
    return E_OK;
}

static int SavePhotoProxyImage(const string &fileUri, sptr<CameraStandard::DeferredPhotoProxy> photoProxyPtr)
{
    void* imageAddr = photoProxyPtr->GetFileDataAddr();
    size_t imageSize = photoProxyPtr->GetFileSize();
    if (imageAddr == nullptr || imageSize == 0) {
        NAPI_ERR_LOG("imageAddr is nullptr or imageSize(%{public}zu)==0", imageSize);
        return E_ERR;
    }

    NAPI_INFO_LOG("start pack PixelMap");
    Media::InitializationOptions opts;
    opts.pixelFormat = Media::PixelFormat::RGBA_8888;
    opts.size = {
        .width = photoProxyPtr->GetWidth(),
        .height = photoProxyPtr->GetHeight()
    };
    auto pixelMap = Media::PixelMap::Create(opts);
    if (pixelMap == nullptr) {
        NAPI_ERR_LOG("Create pixelMap failed.");
        return E_ERR;
    }
    pixelMap->SetPixelsAddr(imageAddr, nullptr, imageSize, Media::AllocatorType::SHARE_MEM_ALLOC, nullptr);
    auto pixelSize = static_cast<uint32_t>(pixelMap->GetByteCount());

    // encode rgba to jpeg
    auto buffer = new (std::nothrow) uint8_t[pixelSize];
    int64_t packedSize = 0L;
    Media::ImagePacker imagePacker;
    Media::PackOption packOption;
    packOption.format = "image/jpeg";
    imagePacker.StartPacking(buffer, pixelSize, packOption);
    imagePacker.AddImage(*pixelMap);
    imagePacker.FinalizePacking(packedSize);
    if (buffer == nullptr) {
        NAPI_ERR_LOG("packet pixelMap failed");
        return E_ERR;
    }
    NAPI_INFO_LOG("pack pixelMap success, packedSize: %{public}" PRId64, packedSize);

    auto ret = SaveImage(fileUri, buffer, packedSize);
    delete[] buffer;
    return ret;
}
#endif

napi_value MediaAssetChangeRequestNapi::JSSetUserComment(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    string userComment;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, userComment) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");
    CHECK_COND_WITH_MESSAGE(env, userComment.length() <= USER_COMMENT_MAX_LEN, "user comment too long");

    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND(env, changeRequest->GetFileAssetInstance() != nullptr, JS_INNER_FAIL);
    changeRequest->GetFileAssetInstance()->SetUserComment(userComment);
    changeRequest->RecordChangeOperation(AssetChangeOperation::SET_USER_COMMENT);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSSetCameraShotKey(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    string cameraShotKey;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, cameraShotKey) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");
    CHECK_COND_WITH_MESSAGE(env, cameraShotKey.length() >= CAMERA_SHOT_KEY_SIZE, "Failed to check cameraShotKey");

    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND(env, changeRequest->GetFileAssetInstance() != nullptr, JS_INNER_FAIL);
    changeRequest->GetFileAssetInstance()->SetCameraShotKey(cameraShotKey);
    changeRequest->RecordChangeOperation(AssetChangeOperation::SET_CAMERA_SHOT_KEY);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSSaveCameraPhoto(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);
    changeRequest->RecordChangeOperation(AssetChangeOperation::SAVE_CAMERA_PHOTO);
    RETURN_NAPI_UNDEFINED(env);
}

static int32_t OpenWriteCacheHandler(MediaAssetChangeRequestAsyncContext& context, bool isMovingPhotoVideo = false)
{
    auto changeRequest = context.objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        context.SaveError(E_FAIL);
        NAPI_ERR_LOG("fileAsset is null");
        return E_FAIL;
    }

    // specify mp4 extension for cache file of moving photo video
    string extension = isMovingPhotoVideo ? MOVING_PHOTO_VIDEO_EXTENSION
                                          : MediaFileUtils::GetExtensionFromPath(fileAsset->GetDisplayName());
    string cacheFileName = to_string(MediaFileUtils::UTCTimeNanoSeconds()) + "." + extension;
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + cacheFileName;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);
    int32_t ret = UserFileClient::OpenFile(openCacheUri, MEDIA_FILEMODE_WRITEONLY);
    if (ret == E_PERMISSION_DENIED) {
        context.error = OHOS_PERMISSION_DENIED_CODE;
        NAPI_ERR_LOG("Open cache file failed, permission denied");
        return ret;
    }
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Open cache file failed, ret: %{public}d", ret);
    }

    if (isMovingPhotoVideo) {
        changeRequest->SetCacheMovingPhotoVideoName(cacheFileName);
    } else {
        changeRequest->SetCacheFileName(cacheFileName);
    }
    return ret;
}

static void GetWriteCacheHandlerExecute(napi_env env, void* data)
{
    auto* context = static_cast<MediaAssetChangeRequestAsyncContext*>(data);
    int32_t ret = OpenWriteCacheHandler(*context);
    if (ret < 0) {
        NAPI_ERR_LOG("Failed to open write cache handler, ret: %{public}d", ret);
        return;
    }
    context->fd = ret;
    context->objectInfo->RecordChangeOperation(AssetChangeOperation::GET_WRITE_CACHE_HANDLER);
}

static void GetWriteCacheHandlerCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<MediaAssetChangeRequestAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->fd, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
        jsContext->status = false;
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static ResourceType GetResourceType(int32_t value)
{
    ResourceType result = ResourceType::INVALID_RESOURCE;
    switch (value) {
        case static_cast<int32_t>(ResourceType::IMAGE_RESOURCE):
        case static_cast<int32_t>(ResourceType::VIDEO_RESOURCE):
        case static_cast<int32_t>(ResourceType::PHOTO_PROXY):
            result = static_cast<ResourceType>(value);
            break;
        default:
            break;
    }
    return result;
}

static napi_value CheckWriteOperation(napi_env env, MediaAssetChangeRequestNapi* changeRequest,
    ResourceType resourceType = ResourceType::INVALID_RESOURCE)
{
    if (changeRequest == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "changeRequest is null");
        return nullptr;
    }

    if (changeRequest->IsMovingPhoto()) {
        if (!changeRequest->CheckMovingPhotoResource(resourceType)) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check resource to add for moving photo");
            return nullptr;
        }
        RETURN_NAPI_TRUE(env);
    }

    if (changeRequest->Contains(AssetChangeOperation::CREATE_FROM_URI) ||
        changeRequest->Contains(AssetChangeOperation::GET_WRITE_CACHE_HANDLER) ||
        changeRequest->Contains(AssetChangeOperation::ADD_RESOURCE)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous asset creation/modification request has not been applied");
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}

napi_value MediaAssetChangeRequestNapi::JSGetWriteCacheHandler(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);
    CHECK_COND(env, !changeRequest->IsMovingPhoto(), JS_E_OPERATION_NOT_SUPPORT);
    CHECK_COND(env, CheckWriteOperation(env, changeRequest), JS_E_OPERATION_NOT_SUPPORT);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ChangeRequestGetWriteCacheHandler",
        GetWriteCacheHandlerExecute, GetWriteCacheHandlerCompleteCallback);
}

static bool CheckMovingPhotoVideo(void* dataBuffer, size_t size)
{
    auto dataSource = make_shared<MediaDataSource>(dataBuffer, static_cast<int64_t>(size));
    auto avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper == nullptr) {
        NAPI_WARN_LOG("Failed to create AVMetadataHelper, ignore checking duration of moving photo video");
        return true;
    }

    int32_t err = avMetadataHelper->SetSource(dataSource);
    if (err != E_OK) {
        NAPI_ERR_LOG("SetSource failed for dataSource, err = %{public}d", err);
        return false;
    }

    unordered_map<int32_t, string> resultMap = avMetadataHelper->ResolveMetadata();
    if (resultMap.find(AV_KEY_DURATION) == resultMap.end()) {
        NAPI_ERR_LOG("AV_KEY_DURATION does not exist");
        return false;
    }

    string durationStr = resultMap.at(AV_KEY_DURATION);
    int32_t duration = std::atoi(durationStr.c_str());
    if (!MediaFileUtils::CheckMovingPhotoVideoDuration(duration)) {
        NAPI_ERR_LOG("Failed to check duration of moving photo video: %{public}d ms", duration);
        return false;
    }
    return true;
}

napi_value MediaAssetChangeRequestNapi::AddMovingPhotoVideoResource(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_TWO, ARGS_TWO) == napi_ok,
        "Failed to get object info");
    auto changeRequest = asyncContext->objectInfo;

    napi_valuetype valueType;
    napi_value value = asyncContext->argv[PARAM1];
    CHECK_COND_WITH_MESSAGE(env, napi_typeof(env, value, &valueType) == napi_ok, "Failed to get napi type");
    if (valueType == napi_string) { // addResource by file uri
        CHECK_COND(env, ParseFileUri(env, value, MediaType::MEDIA_TYPE_VIDEO, asyncContext), OHOS_INVALID_PARAM_CODE);
        CHECK_COND(env, MediaFileUtils::CheckMovingPhotoVideo(asyncContext->realPath), OHOS_INVALID_PARAM_CODE);
        changeRequest->movingPhotoVideoRealPath_ = asyncContext->realPath;
        changeRequest->movingPhotoVideoResourceMode_ = AddResourceMode::FILE_URI;
    } else { // addResource by ArrayBuffer
        bool isArrayBuffer = false;
        CHECK_COND_WITH_MESSAGE(env, napi_is_arraybuffer(env, value, &isArrayBuffer) == napi_ok && isArrayBuffer,
            "Failed to check data type");
        CHECK_COND_WITH_MESSAGE(env,
            napi_get_arraybuffer_info(env, value, &(changeRequest->movingPhotoVideoDataBuffer_),
                &(changeRequest->movingPhotoVideoBufferSize_)) == napi_ok,
            "Failed to get data buffer");
        CHECK_COND_WITH_MESSAGE(env, changeRequest->movingPhotoVideoBufferSize_ > 0,
            "Failed to check size of data buffer");
        CHECK_COND(env, CheckMovingPhotoVideo(changeRequest->movingPhotoVideoDataBuffer_,
            changeRequest->movingPhotoVideoBufferSize_), OHOS_INVALID_PARAM_CODE);
        changeRequest->movingPhotoVideoResourceMode_ = AddResourceMode::DATA_BUFFER;
    }

    changeRequest->RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    changeRequest->addResourceTypes_.push_back(ResourceType::VIDEO_RESOURCE);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetChangeRequestNapi::JSAddResource(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext,
        ARGS_TWO, ARGS_TWO) == napi_ok, "Failed to get object info");
    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);

    int32_t resourceType = static_cast<int32_t>(ResourceType::INVALID_RESOURCE);
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetInt32(env, asyncContext->argv[PARAM0],
        resourceType) == napi_ok, "Failed to get resourceType");
    CHECK_COND(env, CheckWriteOperation(env, changeRequest, GetResourceType(resourceType)), JS_E_OPERATION_NOT_SUPPORT);
    if (changeRequest->IsMovingPhoto() && resourceType == static_cast<int32_t>(ResourceType::VIDEO_RESOURCE)) {
        return AddMovingPhotoVideoResource(env, info);
    }
    CHECK_COND_WITH_MESSAGE(env, resourceType == static_cast<int32_t>(fileAsset->GetMediaType()) ||
        resourceType == static_cast<int32_t>(ResourceType::PHOTO_PROXY), "Failed to check resourceType");

    napi_valuetype valueType;
    napi_value value = asyncContext->argv[PARAM1];
    CHECK_COND_WITH_MESSAGE(env, napi_typeof(env, value, &valueType) == napi_ok, "Failed to get napi type");
    if (valueType == napi_string) {
        // addResource by file uri
        CHECK_COND(env, ParseFileUri(env, value, fileAsset->GetMediaType(), asyncContext), OHOS_INVALID_PARAM_CODE);
        changeRequest->realPath_ = asyncContext->realPath;
        changeRequest->addResourceMode_ = AddResourceMode::FILE_URI;
    } else {
        // addResource by data buffer
        bool isArrayBuffer = false;
        CHECK_COND_WITH_MESSAGE(env, napi_is_arraybuffer(env, value, &isArrayBuffer) == napi_ok,
            "Failed to check data type");
        if (isArrayBuffer) {
            CHECK_COND_WITH_MESSAGE(env, napi_get_arraybuffer_info(env, value, &(changeRequest->dataBuffer_),
                &(changeRequest->dataBufferSize_)) == napi_ok, "Failed to get data buffer");
            CHECK_COND_WITH_MESSAGE(env, changeRequest->dataBufferSize_ > 0, "Failed to check size of data buffer");
            changeRequest->addResourceMode_ = AddResourceMode::DATA_BUFFER;
        } else {
            // addResource by photoProxy
            if (!MediaLibraryNapiUtils::IsSystemApp()) {
                NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
                RETURN_NAPI_UNDEFINED(env);
            }
            #ifdef ABILITY_CAMERA_SUPPORTED
            CameraStandard::DeferredPhotoProxyNapi* napiPhotoProxyPtr = nullptr;
            CHECK_ARGS(env, napi_unwrap(env, asyncContext->argv[PARAM1], reinterpret_cast<void**>(&napiPhotoProxyPtr)),
                JS_INNER_FAIL);
            changeRequest->photoProxy_ = napiPhotoProxyPtr->deferredPhotoProxy_;
            changeRequest->addResourceMode_ = AddResourceMode::PHOTO_PROXY;
            #endif
        }
    }

    changeRequest->RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    changeRequest->addResourceTypes_.push_back(GetResourceType(resourceType));
    RETURN_NAPI_UNDEFINED(env);
}

void MediaAssetChangeRequestNapi::SetNewFileAsset(int32_t id, const string& uri)
{
    if (fileAsset_ == nullptr) {
        NAPI_ERR_LOG("fileAsset_ is nullptr");
        return;
    }

    if (id <= 0 || uri.empty()) {
        NAPI_ERR_LOG("Failed to check file_id: %{public}d and uri: %{public}s", id, uri.c_str());
        return;
    }
    fileAsset_->SetId(id);
    fileAsset_->SetUri(uri);
    fileAsset_->SetTimePending(0);
}

static bool IsCreation(MediaAssetChangeRequestAsyncContext& context)
{
    auto assetChangeOperations = context.assetChangeOperations;
    bool isCreateFromScratch = std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
                                         AssetChangeOperation::CREATE_FROM_SCRATCH) != assetChangeOperations.end();
    bool isCreateFromUri = std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
                                     AssetChangeOperation::CREATE_FROM_URI) != assetChangeOperations.end();
    return isCreateFromScratch || isCreateFromUri;
}

static int32_t SendFile(const UniqueFd& srcFd, const UniqueFd& destFd)
{
    if (srcFd.Get() < 0 || destFd.Get() < 0) {
        NAPI_ERR_LOG("Failed to check srcFd: %{public}d and destFd: %{public}d", srcFd.Get(), destFd.Get());
        return E_ERR;
    }

    struct stat statSrc {};
    int32_t status = fstat(srcFd.Get(), &statSrc);
    if (status != 0) {
        NAPI_ERR_LOG("Failed to get file stat, errno=%{public}d", errno);
        return status;
    }

    off_t offset = 0;
    off_t fileSize = statSrc.st_size;
    while (offset < fileSize) {
        ssize_t sent = sendfile(destFd.Get(), srcFd.Get(), &offset, fileSize - offset);
        if (sent < 0) {
            NAPI_ERR_LOG("Failed to sendfile with errno=%{public}d, srcFd=%{private}d, destFd=%{private}d", errno,
                srcFd.Get(), destFd.Get());
            return sent;
        }
    }

    return E_OK;
}

int32_t MediaAssetChangeRequestNapi::CopyFileToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo)
{
    string srcRealPath = isMovingPhotoVideo ? movingPhotoVideoRealPath_ : realPath_;
    CHECK_COND_RET(!srcRealPath.empty(), E_FAIL, "Failed to check real path of source");
    UniqueFd srcFd(open(srcRealPath.c_str(), O_RDONLY));
    if (srcFd.Get() < 0) {
        NAPI_ERR_LOG("Failed to open %{private}s, errno=%{public}d", srcRealPath.c_str(), errno);
        return srcFd.Get();
    }

    int32_t err = SendFile(srcFd, destFd);
    if (err != E_OK) {
        NAPI_ERR_LOG("Failed to send file from %{public}d to %{public}d", srcFd.Get(), destFd.Get());
    }
    return err;
}

int32_t MediaAssetChangeRequestNapi::CopyDataBufferToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo)
{
    size_t offset = 0;
    size_t length = isMovingPhotoVideo ? movingPhotoVideoBufferSize_ : dataBufferSize_;
    void* dataBuffer = isMovingPhotoVideo ? movingPhotoVideoDataBuffer_ : dataBuffer_;
    while (offset < length) {
        ssize_t written = write(destFd.Get(), (char*)dataBuffer + offset, length - offset);
        if (written < 0) {
            NAPI_ERR_LOG("Failed to copy data buffer, return %{public}d", static_cast<int>(written));
            return written;
        }
        offset += static_cast<size_t>(written);
    }
    return E_OK;
}

int32_t MediaAssetChangeRequestNapi::CopyMovingPhotoVideo(const string& assetUri)
{
    if (assetUri.empty()) {
        NAPI_ERR_LOG("Failed to check empty asset uri");
        return E_INVALID_URI;
    }

    string videoUri = assetUri;
    MediaFileUtils::UriAppendKeyValue(videoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OPEN_MOVING_PHOTO_VIDEO);
    Uri uri(videoUri);
    int videoFd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY);
    if (videoFd < 0) {
        NAPI_ERR_LOG("Failed to open video of moving photo with write-only mode");
        return videoFd;
    }

    int32_t ret = E_ERR;
    UniqueFd uniqueFd(videoFd);
    if (movingPhotoVideoResourceMode_ == AddResourceMode::FILE_URI) {
        ret = CopyFileToMediaLibrary(uniqueFd, true);
    } else if (movingPhotoVideoResourceMode_ == AddResourceMode::DATA_BUFFER) {
        ret = CopyDataBufferToMediaLibrary(uniqueFd, true);
    } else {
        NAPI_ERR_LOG("Invalid mode: %{public}d", movingPhotoVideoResourceMode_);
        return E_INVALID_VALUES;
    }
    return ret;
}

int32_t MediaAssetChangeRequestNapi::CreateAssetBySecurityComponent(string& assetUri)
{
    bool isValid = false;
    string title = creationValuesBucket_.Get(PhotoColumn::MEDIA_TITLE, isValid);
    CHECK_COND_RET(isValid, E_FAIL, "Failed to get title");
    string extension = creationValuesBucket_.Get(ASSET_EXTENTION, isValid);
    CHECK_COND_RET(isValid && MediaFileUtils::CheckDisplayName(title + "." + extension) == E_OK, E_FAIL,
        "Failed to check displayName");
    creationValuesBucket_.valuesMap.erase(MEDIA_DATA_DB_NAME);

    string uri = PAH_CREATE_PHOTO_COMPONENT; // create asset by security component
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri createAssetUri(uri);
    return UserFileClient::InsertExt(createAssetUri, creationValuesBucket_, assetUri);
}

int32_t MediaAssetChangeRequestNapi::CopyToMediaLibrary(bool isCreation, AddResourceMode mode)
{
    CHECK_COND_RET(fileAsset_ != nullptr, E_FAIL, "Failed to check fileAsset_");
    int32_t ret = E_ERR;
    int32_t id = 0;
    string assetUri;
    if (isCreation) {
        ret = CreateAssetBySecurityComponent(assetUri);
        CHECK_COND_RET(ret > 0, (ret == 0 ? E_ERR : ret), "Failed to create asset by security component");
        id = ret;
    } else {
        assetUri = fileAsset_->GetUri();
    }
    CHECK_COND_RET(!assetUri.empty(), E_ERR, "Failed to check empty asset uri");

    if (IsMovingPhoto()) {
        ret = CopyMovingPhotoVideo(assetUri);
        if (ret != E_OK) {
            NAPI_ERR_LOG("Failed to copy data to moving photo video with error: %{public}d", ret);
            return ret;
        }
    }

    Uri uri(assetUri);
    UniqueFd destFd(UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY));
    if (destFd.Get() < 0) {
        NAPI_ERR_LOG("Failed to open %{private}s with error: %{public}d", assetUri.c_str(), destFd.Get());
        return destFd.Get();
    }

    if (mode == AddResourceMode::FILE_URI) {
        ret = CopyFileToMediaLibrary(destFd);
    } else if (mode == AddResourceMode::DATA_BUFFER) {
        ret = CopyDataBufferToMediaLibrary(destFd);
    } else {
        NAPI_ERR_LOG("Invalid mode: %{public}d", mode);
        return E_INVALID_VALUES;
    }

    if (ret == E_OK && isCreation) {
        SetNewFileAsset(id, assetUri);
    }
    return ret;
}

static bool WriteBySecurityComponent(MediaAssetChangeRequestAsyncContext& context)
{
    bool isCreation = IsCreation(context);
    int32_t ret = E_FAIL;
    auto assetChangeOperations = context.assetChangeOperations;
    bool isCreateFromUri = std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
                                     AssetChangeOperation::CREATE_FROM_URI) != assetChangeOperations.end();
    auto changeRequest = context.objectInfo;
    if (isCreateFromUri) {
        ret = changeRequest->CopyToMediaLibrary(isCreation, AddResourceMode::FILE_URI);
    } else {
        ret = changeRequest->CopyToMediaLibrary(isCreation, changeRequest->GetAddResourceMode());
    }

    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to write by security component, ret: %{public}d", ret);
        return false;
    }
    return true;
}

int32_t MediaAssetChangeRequestNapi::PutMediaAssetEditData(DataShare::DataShareValuesBucket& valuesBucket)
{
    if (editData_ == nullptr) {
        return E_OK;
    }

    string compatibleFormat = editData_->GetCompatibleFormat();
    CHECK_COND_RET(!compatibleFormat.empty(), E_FAIL, "Failed to check compatibleFormat");
    string formatVersion = editData_->GetFormatVersion();
    CHECK_COND_RET(!formatVersion.empty(), E_FAIL, "Failed to check formatVersion");
    string data = editData_->GetData();
    CHECK_COND_RET(!data.empty(), E_FAIL, "Failed to check data");

    valuesBucket.Put(COMPATIBLE_FORMAT, compatibleFormat);
    valuesBucket.Put(FORMAT_VERSION, formatVersion);
    valuesBucket.Put(EDIT_DATA, data);
    return E_OK;
}

int32_t MediaAssetChangeRequestNapi::SubmitCache(bool isCreation)
{
    CHECK_COND_RET(fileAsset_ != nullptr, E_FAIL, "Failed to check fileAsset_");
    CHECK_COND_RET(MediaFileUtils::CheckDisplayName(cacheFileName_) == E_OK, E_FAIL, "Failed to check cacheFileName_");

    string uri = PAH_SUBMIT_CACHE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri submitCacheUri(uri);

    string assetUri;
    int32_t ret;
    if (isCreation) {
        bool isValid = false;
        string displayName = creationValuesBucket_.Get(MEDIA_DATA_DB_NAME, isValid);
        CHECK_COND_RET(
            isValid && MediaFileUtils::CheckDisplayName(displayName) == E_OK, E_FAIL, "Failed to check displayName");
        creationValuesBucket_.Put(CACHE_FILE_NAME, cacheFileName_);
        if (IsMovingPhoto()) {
            creationValuesBucket_.Put(CACHE_MOVING_PHOTO_VIDEO_NAME, cacheMovingPhotoVideoName_);
        }
        ret = UserFileClient::InsertExt(submitCacheUri, creationValuesBucket_, assetUri);
    } else {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(PhotoColumn::MEDIA_ID, fileAsset_->GetId());
        valuesBucket.Put(CACHE_FILE_NAME, cacheFileName_);
        ret = PutMediaAssetEditData(valuesBucket);
        CHECK_COND_RET(ret == E_OK, ret, "Failed to put editData");
        ret = UserFileClient::Insert(submitCacheUri, valuesBucket);
    }

    if (ret > 0 && isCreation) {
        SetNewFileAsset(ret, assetUri);
    }
    cacheFileName_.clear();
    cacheMovingPhotoVideoName_.clear();
    return ret;
}

static bool SubmitCacheExecute(MediaAssetChangeRequestAsyncContext& context)
{
    bool isCreation = IsCreation(context);
    auto changeRequest = context.objectInfo;
    int32_t ret = changeRequest->SubmitCache(isCreation);
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to write cache, ret: %{public}d", ret);
        return false;
    }
    return true;
}

static bool WriteCacheByArrayBuffer(MediaAssetChangeRequestAsyncContext& context,
    const UniqueFd& destFd, bool isMovingPhotoVideo = false)
{
    auto changeRequest = context.objectInfo;
    size_t offset = 0;
    size_t length = isMovingPhotoVideo ? changeRequest->GetMovingPhotoVideoSize() : changeRequest->GetDataBufferSize();
    void* dataBuffer = isMovingPhotoVideo ? changeRequest->GetMovingPhotoVideoBuffer() : changeRequest->GetDataBuffer();
    while (offset < length) {
        ssize_t written = write(destFd.Get(), (char*)dataBuffer + offset, length - offset);
        if (written < 0) {
            context.SaveError(written);
            NAPI_ERR_LOG("Failed to write data buffer to cache file, return %{public}d", static_cast<int>(written));
            return false;
        }
        offset += static_cast<size_t>(written);
    }
    return true;
}

static bool SendToCacheFile(MediaAssetChangeRequestAsyncContext& context,
    const UniqueFd& destFd, bool isMovingPhotoVideo = false)
{
    auto changeRequest = context.objectInfo;
    string realPath = isMovingPhotoVideo ? changeRequest->GetMovingPhotoVideoPath() : changeRequest->GetFileRealPath();
    UniqueFd srcFd(open(realPath.c_str(), O_RDONLY));
    if (srcFd.Get() < 0) {
        context.SaveError(srcFd.Get());
        NAPI_ERR_LOG("Failed to open file, errno=%{public}d", errno);
        return false;
    }

    int32_t err = SendFile(srcFd, destFd);
    if (err != E_OK) {
        context.SaveError(err);
        NAPI_ERR_LOG("Failed to send file from %{public}d to %{public}d", srcFd.Get(), destFd.Get());
        return false;
    }
    return true;
}

static bool CreateFromFileUriExecute(MediaAssetChangeRequestAsyncContext& context)
{
    if (!HasWritePermission()) {
        return WriteBySecurityComponent(context);
    }

    int32_t cacheFd = OpenWriteCacheHandler(context);
    if (cacheFd < 0) {
        NAPI_ERR_LOG("Failed to open write cache handler, err: %{public}d", cacheFd);
        return false;
    }

    UniqueFd uniqueFd(cacheFd);
    if (!SendToCacheFile(context, uniqueFd)) {
        NAPI_ERR_LOG("Faild to write cache file");
        return false;
    }
    return SubmitCacheExecute(context);
}

static bool AddPhotoProxyResourceExecute(MediaAssetChangeRequestAsyncContext& context)
{
    #ifdef ABILITY_CAMERA_SUPPORT
    string uri = PAH_ADD_IMAGE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);

    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    std::string fileUri = fileAsset->GetUri();
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(PhotoColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ to_string(fileAsset->GetId()) });

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_ID, context.objectInfo->GetPhotoProxyObj()->GetPhotoId());
    NAPI_INFO_LOG("photoId: %{public}s", context.objectInfo->GetPhotoProxyObj()->GetPhotoId().c_str());
    valuesBucket.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE,
        context.objectInfo->GetPhotoProxyObj()->GetDeferredProcType());
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileAsset->GetId());
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context.SaveError(changedRows);
        NAPI_ERR_LOG("Failed to set, err: %{public}d", changedRows);
        return false;
    }

    int err = SavePhotoProxyImage(fileUri, context.objectInfo->GetPhotoProxyObj());
    context.objectInfo->ReleasePhotoProxyObj();
    if (err < 0) {
        context.SaveError(err);
        NAPI_ERR_LOG("Failed to saveImage , err: %{public}d", err);
        return false;
    }
    #endif
    return true;
}

static bool AddResourceByMode(MediaAssetChangeRequestAsyncContext& context,
    const UniqueFd& uniqueFd, AddResourceMode mode, bool isMovingPhotoVideo = false)
{
    bool isWriteSuccess = false;
    if (mode == AddResourceMode::DATA_BUFFER) {
        isWriteSuccess = WriteCacheByArrayBuffer(context, uniqueFd, isMovingPhotoVideo);
    } else if (mode == AddResourceMode::FILE_URI) {
        isWriteSuccess = SendToCacheFile(context, uniqueFd, isMovingPhotoVideo);
    } else {
        context.SaveError(E_FAIL);
        NAPI_ERR_LOG("Unsupported addResource mode");
    }
    return isWriteSuccess;
}

static bool AddMovingPhotoVideoExecute(MediaAssetChangeRequestAsyncContext& context)
{
    int32_t cacheVideoFd = OpenWriteCacheHandler(context, true);
    if (cacheVideoFd < 0) {
        NAPI_ERR_LOG("Failed to open cache moving photo video, err: %{public}d", cacheVideoFd);
        return false;
    }

    UniqueFd uniqueFd(cacheVideoFd);
    AddResourceMode mode = context.objectInfo->GetMovingPhotoVideoMode();
    if (!AddResourceByMode(context, uniqueFd, mode, true)) {
        NAPI_ERR_LOG("Faild to write cache file");
        return false;
    }
    return true;
}

static bool AddResourceExecute(MediaAssetChangeRequestAsyncContext& context)
{
    if (!HasWritePermission()) {
        return WriteBySecurityComponent(context);
    }

    AddResourceMode mode = context.objectInfo->GetAddResourceMode();
    if (mode == AddResourceMode::PHOTO_PROXY) {
        return AddPhotoProxyResourceExecute(context);
    }

    if (context.objectInfo->IsMovingPhoto() && !AddMovingPhotoVideoExecute(context)) {
        NAPI_ERR_LOG("Faild to write cache file for video of moving photo");
        return false;
    }

    int32_t cacheFd = OpenWriteCacheHandler(context);
    if (cacheFd < 0) {
        NAPI_ERR_LOG("Failed to open write cache handler, err: %{public}d", cacheFd);
        return false;
    }

    UniqueFd uniqueFd(cacheFd);
    if (!AddResourceByMode(context, uniqueFd, mode)) {
        NAPI_ERR_LOG("Faild to write cache file");
        return false;
    }
    return SubmitCacheExecute(context);
}

static bool UpdateAssetProperty(MediaAssetChangeRequestAsyncContext& context, string uri,
    DataShare::DataSharePredicates& predicates, DataShare::DataShareValuesBucket& valuesBucket)
{
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context.SaveError(changedRows);
        NAPI_ERR_LOG("Failed to update property of asset, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetFavoriteExecute(MediaAssetChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::MEDIA_IS_FAV, fileAsset->IsFavorite() ? YES : NO);
    return UpdateAssetProperty(context, PAH_UPDATE_PHOTO, predicates, valuesBucket);
}

static bool SetHiddenExecute(MediaAssetChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    vector<string> assetUriArray(1, fileAsset->GetUri());
    predicates.In(PhotoColumn::MEDIA_ID, assetUriArray);
    valuesBucket.Put(PhotoColumn::MEDIA_HIDDEN, fileAsset->IsHidden() ? YES : NO);
    return UpdateAssetProperty(context, PAH_HIDE_PHOTOS, predicates, valuesBucket);
}

static bool SetTitleExecute(MediaAssetChangeRequestAsyncContext& context)
{
    // In the scenario of creation, the new title will be applied when the asset is created.
    AssetChangeOperation firstOperation = context.assetChangeOperations.front();
    if (firstOperation == AssetChangeOperation::CREATE_FROM_SCRATCH ||
        firstOperation == AssetChangeOperation::CREATE_FROM_URI) {
        return true;
    }

    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::MEDIA_TITLE, fileAsset->GetTitle());
    return UpdateAssetProperty(context, PAH_UPDATE_PHOTO, predicates, valuesBucket);
}

static bool SetUserCommentExecute(MediaAssetChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::PHOTO_USER_COMMENT, fileAsset->GetUserComment());
    return UpdateAssetProperty(context, PAH_EDIT_USER_COMMENT_PHOTO, predicates, valuesBucket);
}

static bool SetPhotoQualityExecute(MediaAssetChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    std::pair<std::string, int> photoQuality = fileAsset->GetPhotoIdAndQuality();
    valuesBucket.Put(PhotoColumn::PHOTO_ID, photoQuality.first);
    valuesBucket.Put(PhotoColumn::PHOTO_QUALITY, photoQuality.second);
    return UpdateAssetProperty(context, PAH_SET_PHOTO_QUALITY, predicates, valuesBucket);
}

static bool SetLocationExecute(MediaAssetChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::PHOTO_LATITUDE, fileAsset->GetLatitude());
    valuesBucket.Put(PhotoColumn::PHOTO_LONGITUDE, fileAsset->GetLongitude());
    return UpdateAssetProperty(context, PAH_SET_LOCATION, predicates, valuesBucket);
}

static bool SetCameraShotKeyExecute(MediaAssetChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = context.objectInfo->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::CAMERA_SHOT_KEY, fileAsset->GetCameraShotKey());
    return UpdateAssetProperty(context, PAH_UPDATE_PHOTO, predicates, valuesBucket);
}

static bool SaveCameraPhotoExecute(MediaAssetChangeRequestAsyncContext& context)
{
    return true;
}

static const unordered_map<AssetChangeOperation, bool (*)(MediaAssetChangeRequestAsyncContext&)> EXECUTE_MAP = {
    { AssetChangeOperation::CREATE_FROM_URI, CreateFromFileUriExecute },
    { AssetChangeOperation::GET_WRITE_CACHE_HANDLER, SubmitCacheExecute },
    { AssetChangeOperation::ADD_RESOURCE, AddResourceExecute },
    { AssetChangeOperation::SET_FAVORITE, SetFavoriteExecute },
    { AssetChangeOperation::SET_HIDDEN, SetHiddenExecute },
    { AssetChangeOperation::SET_TITLE, SetTitleExecute },
    { AssetChangeOperation::SET_USER_COMMENT, SetUserCommentExecute },
    { AssetChangeOperation::SET_PHOTO_QUALITY_AND_PHOTOID, SetPhotoQualityExecute },
    { AssetChangeOperation::SET_LOCATION, SetLocationExecute },
    { AssetChangeOperation::SET_CAMERA_SHOT_KEY, SetCameraShotKeyExecute },
    { AssetChangeOperation::SAVE_CAMERA_PHOTO, SaveCameraPhotoExecute },
};

static void ApplyAssetChangeRequestExecute(napi_env env, void* data)
{
    auto* context = static_cast<MediaAssetChangeRequestAsyncContext*>(data);
    if (context == nullptr || context->objectInfo == nullptr ||
        context->objectInfo->GetFileAssetInstance() == nullptr) {
        context->SaveError(E_FAIL);
        NAPI_ERR_LOG("Failed to check async context of MediaAssetChangeRequest object");
        return;
    }

    unordered_set<AssetChangeOperation> appliedOperations;
    for (const auto& changeOperation : context->assetChangeOperations) {
        // Keep the final result of each operation, and commit it only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = false;
        auto iter = EXECUTE_MAP.find(changeOperation);
        if (iter != EXECUTE_MAP.end()) {
            valid = iter->second(*context);
        } else if (changeOperation == AssetChangeOperation::CREATE_FROM_SCRATCH ||
                   changeOperation == AssetChangeOperation::SET_EDIT_DATA) {
            // Perform CREATE_FROM_SCRATCH and SET_EDIT_DATA during GET_WRITE_CACHE_HANDLER or ADD_RESOURCE.
            valid = true;
        } else {
            NAPI_ERR_LOG("Invalid asset change operation: %{public}d", changeOperation);
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
        }

        if (!valid) {
            NAPI_ERR_LOG("Failed to apply asset change request, operation: %{public}d", changeOperation);
            return;
        }
        appliedOperations.insert(changeOperation);
    }
}

static void ApplyAssetChangeRequestCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<MediaAssetChangeRequestAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value MediaAssetChangeRequestNapi::ApplyChanges(napi_env env, napi_callback_info info)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    auto asyncContext = make_unique<MediaAssetChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, asyncContext, minArgs, maxArgs) == napi_ok,
        "Failed to get args");
    asyncContext->objectInfo = this;

    CHECK_COND_WITH_MESSAGE(env, CheckChangeOperations(env), "Failed to check asset change request operations");
    asyncContext->assetChangeOperations = assetChangeOperations_;
    assetChangeOperations_.clear();
    addResourceTypes_.clear();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ApplyMediaAssetChangeRequest",
        ApplyAssetChangeRequestExecute, ApplyAssetChangeRequestCompleteCallback);
}
} // namespace OHOS::Media