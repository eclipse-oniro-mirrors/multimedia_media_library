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
#define MLOG_TAG "SendablePhotoAlbumNapi"

#include "sendable_photo_album_napi.h"

#include "media_file_utils.h"
#include "photo_album_napi.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_utils_ext.h"
#include "medialibrary_tracer.h"
#include "result_set_utils.h"
#include "sendable_medialibrary_napi_utils.h"
#include "sendable_fetch_file_result_napi.h"
#include "shooting_mode_column.h"
#include "userfile_client.h"
#include "album_operation_uri.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS::Media {
thread_local PhotoAlbum *SendablePhotoAlbumNapi::pAlbumData_ = nullptr;
thread_local napi_ref SendablePhotoAlbumNapi::photoAccessConstructor_ = nullptr;
static const string PHOTO_ALBUM_CLASS = "UserFileMgrPhotoAlbum";
static const string PHOTOACCESS_PHOTO_ALBUM_CLASS = "PhotoAccessPhotoAlbum";
static const string COUNT_GROUP_BY = "count(*) AS count";

using CompleteCallback = napi_async_complete_callback;

SendablePhotoAlbumNapi::SendablePhotoAlbumNapi() : env_(nullptr) {}

SendablePhotoAlbumNapi::~SendablePhotoAlbumNapi() = default;


napi_value SendablePhotoAlbumNapi::PhotoAccessInit(napi_env env, napi_value exports)
{
    napi_value ctorObj;
    napi_property_descriptor props[] = {
        DECLARE_NAPI_GETTER_SETTER("albumName", JSPhotoAccessGetAlbumName, JSPhotoAccessSetAlbumName),
        DECLARE_NAPI_GETTER("albumUri", JSPhotoAccessGetAlbumUri),
        DECLARE_NAPI_GETTER("count", JSPhotoAccessGetAlbumCount),
        DECLARE_NAPI_GETTER("imageCount", JSPhotoAccessGetAlbumImageCount),
        DECLARE_NAPI_GETTER("videoCount", JSPhotoAccessGetAlbumVideoCount),
        DECLARE_NAPI_GETTER("albumType", JSGetPhotoAlbumType),
        DECLARE_NAPI_GETTER("albumSubtype", JSGetPhotoAlbumSubType),
        DECLARE_NAPI_GETTER("coverUri", JSGetCoverUri),
        DECLARE_NAPI_GETTER("lpath", JSGetAlbumLPath),
        DECLARE_NAPI_GETTER("uploadStatus", JSGetUploadStatus),
        DECLARE_NAPI_FUNCTION("commitModify", PhotoAccessHelperCommitModify),
        DECLARE_NAPI_FUNCTION("getAssets", JSPhotoAccessGetPhotoAssets),
        DECLARE_NAPI_FUNCTION("convertToPhotoAlbum", ConvertToPhotoAlbum),
        DECLARE_NAPI_FUNCTION("getSharedPhotoAssets", JSPhotoAccessGetSharedPhotoAssets),
        DECLARE_NAPI_FUNCTION("getFaceId", PhotoAccessHelperGetFaceId),
    };
    napi_define_sendable_class(env, PHOTOACCESS_PHOTO_ALBUM_CLASS.c_str(), NAPI_AUTO_LENGTH,
                               PhotoAlbumNapiConstructor, nullptr, sizeof(props) / sizeof(props[0]), props,
                               nullptr, &ctorObj);
    NAPI_CALL(env, napi_create_reference(env, ctorObj, NAPI_INIT_REF_COUNT, &photoAccessConstructor_));
    NAPI_CALL(env, napi_set_named_property(env, exports, PHOTOACCESS_PHOTO_ALBUM_CLASS.c_str(), ctorObj));
    return exports;
}

napi_value SendablePhotoAlbumNapi::CreatePhotoAlbumNapi(napi_env env, unique_ptr<PhotoAlbum> &albumData)
{
    if (albumData == nullptr) {
        return nullptr;
    }

    if (photoAccessConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendablePhotoAlbumNapi::PhotoAccessInit(env, exports);
    }

    napi_value constructor;
    napi_ref constructorRef = photoAccessConstructor_;

    CHECK_ARGS(env, napi_get_reference_value(env, constructorRef, &constructor), JS_INNER_FAIL);

    napi_value result = nullptr;
    pAlbumData_ = albumData.release();
    CHECK_ARGS(env, napi_new_instance(env, constructor, 0, nullptr, &result), JS_INNER_FAIL);
    pAlbumData_ = nullptr;
    return result;
}

napi_value SendablePhotoAlbumNapi::CreatePhotoAlbumNapi(napi_env env, shared_ptr<PhotoAlbum>& albumData)
{
    if (albumData == nullptr || albumData->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        NAPI_ERR_LOG("Unsupported photo album data");
        return nullptr;
    }

    if (photoAccessConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendablePhotoAlbumNapi::PhotoAccessInit(env, exports);
    }

    napi_value constructor = nullptr;
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_reference_value(env, photoAccessConstructor_, &constructor), JS_INNER_FAIL);
    CHECK_ARGS(env, napi_new_instance(env, constructor, 0, nullptr, &result), JS_INNER_FAIL);
    CHECK_COND(env, result != nullptr, JS_INNER_FAIL);

    SendablePhotoAlbumNapi* photoAlbumNapi = nullptr;
    CHECK_ARGS(env, napi_unwrap_sendable(env, result, reinterpret_cast<void**>(&photoAlbumNapi)), JS_INNER_FAIL);
    CHECK_COND(env, photoAlbumNapi != nullptr, JS_INNER_FAIL);
    photoAlbumNapi->photoAlbumPtr = albumData;
    return result;
}

int32_t SendablePhotoAlbumNapi::GetAlbumId() const
{
    return photoAlbumPtr->GetAlbumId();
}

int32_t SendablePhotoAlbumNapi::GetCount() const
{
    return photoAlbumPtr->GetCount();
}

void SendablePhotoAlbumNapi::SetCount(int32_t count)
{
    return photoAlbumPtr->SetCount(count);
}

int32_t SendablePhotoAlbumNapi::GetImageCount() const
{
    return photoAlbumPtr->GetImageCount();
}

void SendablePhotoAlbumNapi::SetImageCount(int32_t count)
{
    return photoAlbumPtr->SetImageCount(count);
}

int32_t SendablePhotoAlbumNapi::GetVideoCount() const
{
    return photoAlbumPtr->GetVideoCount();
}

void SendablePhotoAlbumNapi::SetVideoCount(int32_t count)
{
    return photoAlbumPtr->SetVideoCount(count);
}

const string& SendablePhotoAlbumNapi::GetAlbumUri() const
{
    return photoAlbumPtr->GetAlbumUri();
}

const string& SendablePhotoAlbumNapi::GetCoverUri() const
{
    return photoAlbumPtr->GetCoverUri();
}

int64_t SendablePhotoAlbumNapi::GetDateModified() const
{
    return photoAlbumPtr->GetDateModified();
}

const string& SendablePhotoAlbumNapi::GetAlbumName() const
{
    return photoAlbumPtr->GetAlbumName();
}

PhotoAlbumType SendablePhotoAlbumNapi::GetPhotoAlbumType() const
{
    return photoAlbumPtr->GetPhotoAlbumType();
}

PhotoAlbumSubType SendablePhotoAlbumNapi::GetPhotoAlbumSubType() const
{
    return photoAlbumPtr->GetPhotoAlbumSubType();
}

double SendablePhotoAlbumNapi::GetLatitude() const
{
    return photoAlbumPtr->GetLatitude();
}

double SendablePhotoAlbumNapi::GetLongitude() const
{
    return photoAlbumPtr->GetLongitude();
}

const string& SendablePhotoAlbumNapi::GetLPath() const
{
    return photoAlbumPtr->GetLPath();
}

shared_ptr<PhotoAlbum> SendablePhotoAlbumNapi::GetPhotoAlbumInstance() const
{
    return photoAlbumPtr;
}

bool SendablePhotoAlbumNapi::GetHiddenOnly() const
{
    return photoAlbumPtr->GetHiddenOnly();
}

void SendablePhotoAlbumNapi::SetHiddenOnly(const bool hiddenOnly_)
{
    return photoAlbumPtr->SetHiddenOnly(hiddenOnly_);
}

void SendablePhotoAlbumNapi::SetPhotoAlbumNapiProperties()
{
    photoAlbumPtr = shared_ptr<PhotoAlbum>(pAlbumData_);
}

int32_t SendablePhotoAlbumNapi::GetUploadStatus() const
{
    return photoAlbumPtr->GetUploadStatus();
}

// Constructor callback
napi_value SendablePhotoAlbumNapi::PhotoAlbumNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);

    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    unique_ptr<SendablePhotoAlbumNapi> obj = make_unique<SendablePhotoAlbumNapi>();
    obj->env_ = env;
    if (pAlbumData_ != nullptr) {
        obj->SetPhotoAlbumNapiProperties();
    }
    CHECK_ARGS(env, napi_wrap_sendable(env, thisVar, reinterpret_cast<void *>(obj.get()),
        SendablePhotoAlbumNapi::PhotoAlbumNapiDestructor, nullptr), JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void SendablePhotoAlbumNapi::PhotoAlbumNapiDestructor(napi_env env, void *nativeObject, void *finalizeHint)
{
    auto *album = reinterpret_cast<SendablePhotoAlbumNapi*>(nativeObject);
    if (album != nullptr) {
        delete album;
        album = nullptr;
    }
}

napi_value UnwrapPhotoAlbumObject(napi_env env, napi_callback_info info, SendablePhotoAlbumNapi** obj)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);

    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), JS_INNER_FAIL);
    if (thisVar == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    CHECK_ARGS(env, napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(obj)), JS_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetAlbumName(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetAlbumName().c_str(), NAPI_AUTO_LENGTH, &jsResult),
        JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetAlbumUri(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetAlbumUri().c_str(), NAPI_AUTO_LENGTH, &jsResult),
        JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetAlbumCount(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_int32(env, obj->GetCount(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetAlbumImageCount(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_int32(env, obj->GetImageCount(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetAlbumVideoCount(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_int32(env, obj->GetVideoCount(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSGetPhotoAlbumType(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_int32(env, obj->GetPhotoAlbumType(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSGetPhotoAlbumSubType(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_int32(env, obj->GetPhotoAlbumSubType(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSGetCoverUri(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetCoverUri().c_str(), NAPI_AUTO_LENGTH, &jsResult),
        JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSGetAlbumLPath(napi_env env, napi_callback_info info)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, SendableMediaLibraryNapiUtils::IsSystemApp(), JS_ERR_PERMISSION_DENIED,
        "Get lpath permission denied: not a system app", nullptr, "Get album lpath failed: not a system app");
    CHECK_COND(env, SendableMediaLibraryNapiUtils::IsSystemApp(), JS_ERR_PERMISSION_DENIED);
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetLPath().c_str(), NAPI_AUTO_LENGTH, &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value SendablePhotoAlbumNapi::JSGetUploadStatus(napi_env env, napi_callback_info info)
{
    CHECK_COND_LOG_THROW_RETURN_RET(env, SendableMediaLibraryNapiUtils::IsSystemApp(), JS_ERR_PERMISSION_DENIED,
        "GetUploadStatus permission denied: not a system app", nullptr, "GetUploadStatus failed: not a system app");
    SendablePhotoAlbumNapi *obj = nullptr;
    CHECK_NULLPTR_RET(UnwrapPhotoAlbumObject(env, info, &obj));

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_int32(env, obj->GetUploadStatus(), &jsResult), JS_INNER_FAIL);
    return jsResult;
}

napi_value GetStringArg(napi_env env, napi_callback_info info, SendablePhotoAlbumNapi **obj, string &output)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE];
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND(env, argc == ARGS_ONE, JS_ERR_PARAMETER_INVALID);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);
    napi_valuetype valueType = napi_undefined;
    if ((thisVar == nullptr) || (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok) || (valueType != napi_string)) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    size_t res = 0;
    char buffer[FILENAME_MAX];
    CHECK_ARGS(env, napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res), JS_INNER_FAIL);
    output = string(buffer);

    CHECK_ARGS(env, napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(obj)), JS_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }

    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static int32_t NapiGetAnalysisAlbumPredicates(const shared_ptr<PhotoAlbum>& photoAlbum,
    DataSharePredicates& predicates)
{
    PhotoAlbumSubType subType = photoAlbum->GetPhotoAlbumSubType();
    if (subType == PhotoAlbumSubType::PORTRAIT || subType == PhotoAlbumSubType::GROUP_PHOTO) {
        SendableMediaLibraryNapiUtils::GetPortraitAlbumPredicates(photoAlbum->GetAlbumId(), predicates);
        return E_SUCCESS;
    }
    if (subType == PhotoAlbumSubType::GEOGRAPHY_LOCATION) {
        return SendableMediaLibraryNapiUtils::GetAllLocationPredicates(predicates);
    }
    if (subType == PhotoAlbumSubType::SHOOTING_MODE) {
        ShootingModeAlbumType type {};
        if (!ShootingModeAlbum::AlbumNameToShootingModeAlbumType(photoAlbum->GetAlbumName(), type)) {
            NAPI_ERR_LOG("Invalid shooting mode album name: %{public}s", photoAlbum->GetAlbumName().c_str());
            return E_INVALID_ARGUMENTS;
        }
        ShootingModeAlbum::GetShootingModeAlbumPredicates(type, predicates, photoAlbum->GetHiddenOnly());
        return E_SUCCESS;
    }
    string albumName = photoAlbum->GetAlbumName();
    if (SendableMediaLibraryNapiUtils::IsFeaturedSinglePortraitAlbum(albumName, predicates)) {
        return SendableMediaLibraryNapiUtils::GetFeaturedSinglePortraitAlbumPredicates(
            photoAlbum->GetAlbumId(), predicates);
    }
    SendableMediaLibraryNapiUtils::GetAnalysisPhotoMapPredicates(photoAlbum->GetAlbumId(), predicates);
    return E_SUCCESS;
}

static int32_t GetPredicatesByAlbumTypes(const shared_ptr<PhotoAlbum> &photoAlbum,
    DataSharePredicates &predicates, const bool hiddenOnly)
{
    int32_t albumId = photoAlbum->GetAlbumId();
    PhotoAlbumSubType subType = photoAlbum->GetPhotoAlbumSubType();
    bool isLocationAlbum = subType == PhotoAlbumSubType::GEOGRAPHY_LOCATION;
    if (albumId <= 0 && !isLocationAlbum) {
        NAPI_ERR_LOG("Invalid album ID");
        return E_INVALID_ARGUMENTS;
    }
    PhotoAlbumType type = photoAlbum->GetPhotoAlbumType();
    if ((!PhotoAlbum::CheckPhotoAlbumType(type)) || (!PhotoAlbum::CheckPhotoAlbumSubType(subType))) {
        NAPI_ERR_LOG("Invalid album type or subtype: type=%{public}d, subType=%{public}d", static_cast<int>(type),
            static_cast<int>(subType));
        return E_INVALID_ARGUMENTS;
    }

    if (PhotoAlbum::IsUserPhotoAlbum(type, subType)) {
        return SendableMediaLibraryNapiUtils::GetUserAlbumPredicates(photoAlbum->GetAlbumId(), predicates, hiddenOnly);
    }

    if (PhotoAlbum::IsSourceAlbum(type, subType)) {
        return SendableMediaLibraryNapiUtils::GetSourceAlbumPredicates(photoAlbum->GetAlbumId(),
            predicates, hiddenOnly);
    }

    if (type == PhotoAlbumType::SMART) {
        return NapiGetAnalysisAlbumPredicates(photoAlbum, predicates);
    }

    if (type == PhotoAlbumType::SYSTEM) {
        return SendableMediaLibraryNapiUtils::GetSystemAlbumPredicates(subType, predicates, hiddenOnly);
    }

    NAPI_ERR_LOG("Invalid album type and subtype: type=%{public}d, subType=%{public}d", static_cast<int>(type),
        static_cast<int>(subType));
    return E_INVALID_ARGUMENTS;
}

static napi_value ParseArgsGetPhotoAssets(napi_env env, napi_callback_info info,
    unique_ptr<SendablePhotoAlbumNapiAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    /* Parse the first argument */
    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::GetFetchOption(env, context->argv[PARAM0], ASSET_FETCH_OPT, context),
        JS_INNER_FAIL);

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    auto ret = GetPredicatesByAlbumTypes(photoAlbum, context->predicates, photoAlbum->GetHiddenOnly());
    if (ret != E_SUCCESS) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    CHECK_NULLPTR_RET(SendableMediaLibraryNapiUtils::AddDefaultAssetColumns(env, context->fetchColumn,
        PhotoColumn::IsPhotoColumn, NapiAssetType::TYPE_PHOTO));
    if (photoAlbum->GetHiddenOnly() || photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIDDEN) {
        if (!SendableMediaLibraryNapiUtils::IsSystemApp()) {
            NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
            return nullptr;
        }
        // sort by hidden time desc if is hidden asset
        context->predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

void ConvertColumnsForPortrait(SendablePhotoAlbumNapiAsyncContext *context)
{
    if (context == nullptr || context->objectInfo == nullptr) {
        NAPI_ERR_LOG("context is null or PhotoAlbumNapi is null");
        return;
    }
    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum != nullptr && (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::PORTRAIT ||
        photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::GROUP_PHOTO)) {
        for (size_t i = 0; i < context->fetchColumn.size(); i++) {
            context->fetchColumn[i] = PhotoColumn::PHOTOS_TABLE + "." + context->fetchColumn[i];
        }
    }
}

void ConvertColumnsForFeaturedSinglePortrait(SendablePhotoAlbumNapiAsyncContext *context)
{
    if (context == nullptr || context->objectInfo == nullptr) {
        return;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    int portraitAlbumId = 0;
    if ((photoAlbum != nullptr && photoAlbum->GetPhotoAlbumSubType() != PhotoAlbumSubType::CLASSIFY)||
        photoAlbum->GetAlbumName().compare(to_string(portraitAlbumId)) != 0) {
        return;
    }

    for (size_t i = 0; i < context->fetchColumn.size(); i++) {
        context->fetchColumn[i] = PhotoColumn::PHOTOS_TABLE + "." + context->fetchColumn[i];
    }
}

static void JSPhotoAccessGetPhotoAssetsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSPhotoAccessGetPhotoAssetsExecute");

    auto *context = static_cast<SendablePhotoAlbumNapiAsyncContext *>(data);
    Uri uri(CONST_PAH_QUERY_PHOTO_MAP);
    ConvertColumnsForPortrait(context);
    ConvertColumnsForFeaturedSinglePortrait(context);
    int32_t errCode = 0;
    int32_t userId = -1;
    if (context->objectInfo != nullptr) {
        shared_ptr<PhotoAlbum> photoAlbum =  context->objectInfo->GetPhotoAlbumInstance();
        if (photoAlbum != nullptr) {
            userId = photoAlbum->GetUserId();
        }
    }
    auto resultSet = UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, userId);
    if (resultSet == nullptr) {
        context->SaveError(errCode);
        return;
    }
    context->fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    context->fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    context->fetchResult->SetUserId(userId);
}

static void GetPhotoMapQueryResult(napi_env env, SendablePhotoAlbumNapiAsyncContext *context,
    unique_ptr<SendableJSAsyncContextOutput> &jsContext)
{
    napi_value fetchRes = SendableFetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchResult));
    if (fetchRes == nullptr) {
        SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
            "Failed to create js object for FetchFileResult");
        return;
    }
    jsContext->data = fetchRes;
    jsContext->status = true;
}

static void JSGetPhotoAssetsCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAssetsCallbackComplete");

    auto *context = static_cast<SendablePhotoAlbumNapiAsyncContext *>(data);

    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->fetchResult != nullptr) {
        GetPhotoMapQueryResult(env, context, jsContext);
    } else {
        NAPI_ERR_LOG("No fetch file result found!");
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetPhotoAssets(napi_env env, napi_callback_info info)
{
    unique_ptr<SendablePhotoAlbumNapiAsyncContext> asyncContext = make_unique<SendablePhotoAlbumNapiAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAssets(env, info, asyncContext));

    return SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAssets",
        JSPhotoAccessGetPhotoAssetsExecute, JSGetPhotoAssetsCallbackComplete);
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessSetAlbumName(napi_env env, napi_callback_info info)
{
    SendablePhotoAlbumNapi *obj = nullptr;
    string albumName;
    CHECK_NULLPTR_RET(GetStringArg(env, info, &obj, albumName));
    obj->photoAlbumPtr->SetAlbumName(albumName);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);
    return result;
}

static napi_value ParseArgsCommitModify(napi_env env, napi_callback_info info,
    unique_ptr<SendablePhotoAlbumNapiAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ZERO;
    constexpr size_t maxArgs = ARGS_ONE;
    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    if (!PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (MediaFileUtils::CheckAlbumName(photoAlbum->GetAlbumName()) < 0) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(photoAlbum->GetAlbumId()));
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, photoAlbum->GetAlbumName());
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_COVER_URI, photoAlbum->GetCoverUri());

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void JSCommitModifyExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyExecute");

    auto *context = static_cast<SendablePhotoAlbumNapiAsyncContext*>(data);
    string commitModifyUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
        CONST_UFM_UPDATE_PHOTO_ALBUM : CONST_PAH_UPDATE_PHOTO_ALBUM;
    Uri uri(commitModifyUri);
    int changedRows = UserFileClient::Update(uri, context->predicates, context->valuesBucket);
    context->SaveError(changedRows);
    context->changedRows = changedRows;
}

static void JSCommitModifyCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyCompleteCallback");

    auto *context = static_cast<SendablePhotoAlbumNapiAsyncContext*>(data);
    auto jsContext = make_unique<SendableJSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value SendablePhotoAlbumNapi::PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<SendablePhotoAlbumNapiAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsCommitModify(env, info, asyncContext));
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    return SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSCommitModify",
        JSCommitModifyExecute, JSCommitModifyCompleteCallback);
}

napi_value SendablePhotoAlbumNapi::ConvertToPhotoAlbum(napi_env env, napi_callback_info info)
{
    if (photoAccessConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendablePhotoAlbumNapi::PhotoAccessInit(env, exports);
        PhotoAlbumNapi::PhotoAccessInit(env, exports);
    }

    napi_value result = nullptr;
    napi_status status;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("ConvertToPhotoAlbum Invalid arguments! status: %{public}d", status);
        return result;
    }

    SendablePhotoAlbumNapi *obj = nullptr;
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) && (obj == nullptr)) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "ConvertToPhotoAlbum napi unwrap sendable failed");
        return nullptr;
    }

    auto photoAlbum = obj->GetPhotoAlbumInstance();
    CHECK_COND(env, photoAlbum != nullptr, JS_INNER_FAIL);
    if (photoAlbum->GetAlbumId() > 0) {
        return PhotoAlbumNapi::CreatePhotoAlbumNapi(env, photoAlbum);
    }

    // PhotoAlbum object has not been actually created, return null.
    napi_value nullValue;
    napi_get_null(env, &nullValue);
    return nullValue;
}

napi_value SendablePhotoAlbumNapi::JSPhotoAccessGetSharedPhotoAssets(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSPhotoAccessGetSharedPhotoAssets");
    unique_ptr<SendablePhotoAlbumNapiAsyncContext> asyncContext =
        make_unique<SendablePhotoAlbumNapiAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAssets(env, info, asyncContext));

    SendablePhotoAlbumNapiAsyncContext* context =
        static_cast<SendablePhotoAlbumNapiAsyncContext*>((asyncContext.get()));

    Uri uri(CONST_PAH_QUERY_PHOTO_MAP);
    ConvertColumnsForPortrait(context);
    ConvertColumnsForFeaturedSinglePortrait(context);
    shared_ptr<NativeRdb::ResultSet> resultSet = UserFileClient::QueryRdb(uri,
        context->predicates, context->fetchColumn);
    CHECK_NULLPTR_RET(resultSet);

    napi_value jsFileArray = 0;
    napi_create_array(env, &jsFileArray);

    int count = 0;
    int err = resultSet->GoToFirstRow();
    if (err != napi_ok) {
        NAPI_ERR_LOG("Failed GoToFirstRow %{public}d", err);
        return jsFileArray;
    }
    do {
        napi_value item = SendableMediaLibraryNapiUtils::GetNextRowObject(env, resultSet);
        napi_set_element(env, jsFileArray, count++, item);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    return jsFileArray;
}

static void PhotoAccessHelperGetFaceIdExec(napi_env env, void *data)
{
    auto *context = static_cast<SendablePhotoAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    auto *objectInfo = context->objectInfo;
    CHECK_NULL_PTR_RETURN_VOID(objectInfo, "objectInfo is null");

    auto photoAlbumInstance = objectInfo->GetPhotoAlbumInstance();
    CHECK_NULL_PTR_RETURN_VOID(photoAlbumInstance, "photoAlbumInstance is null");

    PhotoAlbumSubType albumSubType = photoAlbumInstance->GetPhotoAlbumSubType();
    if (albumSubType != PhotoAlbumSubType::PORTRAIT && albumSubType != PhotoAlbumSubType::GROUP_PHOTO) {
        NAPI_WARN_LOG("albumSubType: %{public}d, not support getFaceId", albumSubType);
        return;
    }

    Uri uri(CONST_PAH_QUERY_ANA_PHOTO_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, objectInfo->GetAlbumId());
    vector<string> fetchColumn = { GROUP_TAG };
    int errCode = 0;

    int32_t userId = -1;
    if (context->objectInfo != nullptr) {
        shared_ptr<PhotoAlbum> photoAlbum =  objectInfo->GetPhotoAlbumInstance();
        if (photoAlbum != nullptr) {
            userId = photoAlbum->GetUserId();
        }
    }
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode, userId);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != 0) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(E_FAIL);
        }
        NAPI_ERR_LOG("get face id failed, errCode is %{public}d", errCode);
        return;
    }

    context->faceTag = GetStringVal(GROUP_TAG, resultSet);
}

static void GetFaceIdCompleteCallback(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<SendablePhotoAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<SendableJSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        CHECK_ARGS_RET_VOID(env,
            napi_create_string_utf8(env, context->faceTag.c_str(), NAPI_AUTO_LENGTH, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    }

    if (context->work != nullptr) {
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef, context->work,
            *jsContext);
    }
    delete context;
}

napi_value SendablePhotoAlbumNapi::PhotoAccessHelperGetFaceId(napi_env env, napi_callback_info info)
{
    if (!SendableMediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "Only system apps can get the Face ID of the album");
        return nullptr;
    }

    auto asyncContext = make_unique<SendablePhotoAlbumNapiAsyncContext>();

    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, 0, 0),
        JS_ERR_PARAMETER_INVALID);

    return SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSAnalysisAlbumGetFaceId",
        PhotoAccessHelperGetFaceIdExec, GetFaceIdCompleteCallback);
}
} // namespace OHOS::Media
