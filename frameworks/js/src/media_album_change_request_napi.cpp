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

#define MLOG_TAG "MediaAlbumChangeRequestNapi"

#include "media_album_change_request_napi.h"

#include <unordered_map>
#include <unordered_set>

#include "file_asset_napi.h"
#include "media_file_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_tracer.h"
#include "photo_album_napi.h"
#include "photo_map_column.h"
#include "result_set_utils.h"
#include "userfile_client.h"
#include "vision_column.h"
#include "vision_face_tag_column.h"
#include "vision_photo_map_column.h"

using namespace std;

namespace OHOS::Media {
static const string MEDIA_ALBUM_CHANGE_REQUEST_CLASS = "MediaAlbumChangeRequest";
thread_local napi_ref MediaAlbumChangeRequestNapi::constructor_ = nullptr;

napi_value MediaAlbumChangeRequestNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_ALBUM_CHANGE_REQUEST_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("createAlbumRequest", JSCreateAlbumRequest),
            DECLARE_NAPI_STATIC_FUNCTION("deleteAlbums", JSDeleteAlbums),
            DECLARE_NAPI_FUNCTION("getAlbum", JSGetAlbum),
            DECLARE_NAPI_FUNCTION("addAssets", JSAddAssets),
            DECLARE_NAPI_FUNCTION("removeAssets", JSRemoveAssets),
            DECLARE_NAPI_FUNCTION("moveAssets", JSMoveAssets),
            DECLARE_NAPI_FUNCTION("recoverAssets", JSRecoverAssets),
            DECLARE_NAPI_FUNCTION("deleteAssets", JSDeleteAssets),
            DECLARE_NAPI_FUNCTION("setAlbumName", JSSetAlbumName),
            DECLARE_NAPI_FUNCTION("setCoverUri", JSSetCoverUri),
            DECLARE_NAPI_FUNCTION("placeBefore", JSPlaceBefore),
            DECLARE_NAPI_FUNCTION("setDisplayLevel", JSSetDisplayLevel),
            DECLARE_NAPI_FUNCTION("mergeAlbum", JSMergeAlbum),
            DECLARE_NAPI_FUNCTION("dismissAssets", JSDismissAssets),
            DECLARE_NAPI_FUNCTION("setIsMe", JSSetIsMe),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

static napi_value ParsePhotoAlbum(napi_env env, napi_value arg, shared_ptr<PhotoAlbum>& photoAlbum)
{
    napi_valuetype valueType;
    PhotoAlbumNapi* photoAlbumNapi;
    CHECK_ARGS(env, napi_typeof(env, arg, &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");
    CHECK_ARGS(env, napi_unwrap(env, arg, reinterpret_cast<void**>(&photoAlbumNapi)), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, photoAlbumNapi != nullptr, "Failed to get PhotoAlbumNapi object");

    auto photoAlbumPtr = photoAlbumNapi->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbumPtr != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        photoAlbumPtr->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER &&
            PhotoAlbum::CheckPhotoAlbumType(photoAlbumPtr->GetPhotoAlbumType()) &&
            PhotoAlbum::CheckPhotoAlbumSubType(photoAlbumPtr->GetPhotoAlbumSubType()),
        "Unsupported type of photoAlbum");
    photoAlbum = photoAlbumPtr;
    RETURN_NAPI_TRUE(env);
}

napi_value MediaAlbumChangeRequestNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Failed to check new.target");

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    shared_ptr<PhotoAlbum> photoAlbum = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");
    CHECK_COND_WITH_MESSAGE(env, ParsePhotoAlbum(env, argv[PARAM0], photoAlbum), "Failed to parse album");

    unique_ptr<MediaAlbumChangeRequestNapi> obj = make_unique<MediaAlbumChangeRequestNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    obj->photoAlbum_ = photoAlbum;
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), MediaAlbumChangeRequestNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void MediaAlbumChangeRequestNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* albumChangeRequest = reinterpret_cast<MediaAlbumChangeRequestNapi*>(nativeObject);
    if (albumChangeRequest != nullptr) {
        delete albumChangeRequest;
        albumChangeRequest = nullptr;
    }
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestNapi::GetPhotoAlbumInstance() const
{
    return photoAlbum_;
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestNapi::GetReferencePhotoAlbumInstance() const
{
    return referencePhotoAlbum_;
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestNapi::GetTargetPhotoAlbumInstance() const
{
    return targetAlbum_;
}

vector<string> MediaAlbumChangeRequestNapi::GetAddAssetArray() const
{
    return assetsToAdd_;
}

vector<string> MediaAlbumChangeRequestNapi::GetRemoveAssetArray() const
{
    return assetsToRemove_;
}

vector<string> MediaAlbumChangeRequestNapi::GetRecoverAssetArray() const
{
    return assetsToRecover_;
}

vector<string> MediaAlbumChangeRequestNapi::GetDeleteAssetArray() const
{
    return assetsToDelete_;
}

vector<string> MediaAlbumChangeRequestNapi::GetDismissAssetArray() const
{
    return dismissAssets_;
}

map<shared_ptr<PhotoAlbum>, vector<string>, PhotoAlbumPtrCompare> MediaAlbumChangeRequestNapi::GetMoveMap() const
{
    return moveMap_;
}

void MediaAlbumChangeRequestNapi::RecordMoveAssets(vector<string>& assetArray, shared_ptr<PhotoAlbum>& targetAlbum)
{
    if (targetAlbum == nullptr || assetArray.empty()) {
        return;
    }

    auto iter = moveMap_.find(targetAlbum);
    if (iter != moveMap_.end()) {
        iter->second.insert(iter->second.end(), assetArray.begin(), assetArray.end());
    } else {
        moveMap_.insert(make_pair(targetAlbum, assetArray));
    }
}

void MediaAlbumChangeRequestNapi::ClearAddAssetArray()
{
    assetsToAdd_.clear();
}

void MediaAlbumChangeRequestNapi::ClearRemoveAssetArray()
{
    assetsToRemove_.clear();
}

void MediaAlbumChangeRequestNapi::ClearRecoverAssetArray()
{
    assetsToRecover_.clear();
}

void MediaAlbumChangeRequestNapi::ClearDeleteAssetArray()
{
    assetsToDelete_.clear();
}

void MediaAlbumChangeRequestNapi::ClearDismissAssetArray()
{
    dismissAssets_.clear();
}

void MediaAlbumChangeRequestNapi::ClearMoveMap()
{
    moveMap_.clear();
}

bool MediaAlbumChangeRequestNapi::CheckChangeOperations(napi_env env)
{
    if (albumChangeOperations_.empty()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "None request to apply");
        return false;
    }

    auto photoAlbum = GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "photoAlbum is null");
        return false;
    }

    if (albumChangeOperations_.front() != AlbumChangeOperation::CREATE_ALBUM && photoAlbum->GetAlbumId() <= 0) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid album change request");
        return false;
    }

    return true;
}

static napi_value ParseAssetArray(napi_env env, napi_value arg, vector<string>& uriArray)
{
    vector<napi_value> napiValues;
    napi_valuetype valueType = napi_undefined;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, arg, napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetUriArrayFromAssets(env, napiValues, uriArray));
    RETURN_NAPI_TRUE(env);
}

static bool CheckDuplicatedAssetArray(const vector<string>& arrayToCheck, const vector<string>& currentArray)
{
    if (currentArray.empty()) {
        return true;
    }

    for (const auto& element : arrayToCheck) {
        if (std::find(currentArray.begin(), currentArray.end(), element) != currentArray.end()) {
            return false;
        }
    }
    return true;
}

napi_value MediaAlbumChangeRequestNapi::JSGetAlbum(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_ARGS_THROW_INVALID_PARAM(
        env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO));

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND(env, photoAlbum != nullptr, JS_INNER_FAIL);
    if (photoAlbum->GetAlbumId() > 0) {
        return PhotoAlbumNapi::CreatePhotoAlbumNapi(env, photoAlbum);
    }

    // PhotoAlbum object has not been actually created, return null.
    napi_value nullValue;
    napi_get_null(env, &nullValue);
    return nullValue;
}

static napi_value ParseArgsCreateAlbum(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext>& context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_TWO, ARGS_TWO) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, MediaAlbumChangeRequestNapi::InitUserFileClient(env, info), JS_INNER_FAIL);

    string albumName;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM1], albumName) == napi_ok,
        "Failed to get album name");
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckAlbumName(albumName) == E_OK, "Invalid album name");
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    RETURN_NAPI_TRUE(env);
}

napi_value MediaAlbumChangeRequestNapi::JSCreateAlbumRequest(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsCreateAlbum(env, info, asyncContext), "Failed to parse args");

    bool isValid = false;
    string albumName = asyncContext->valuesBucket.Get(PhotoAlbumColumns::ALBUM_NAME, isValid);
    auto photoAlbum = make_unique<PhotoAlbum>();
    photoAlbum->SetAlbumName(albumName);
    photoAlbum->SetPhotoAlbumType(USER);
    photoAlbum->SetPhotoAlbumSubType(USER_GENERIC);
    photoAlbum->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    napi_value photoAlbumNapi = PhotoAlbumNapi::CreatePhotoAlbumNapi(env, photoAlbum);
    CHECK_COND(env, photoAlbumNapi != nullptr, JS_INNER_FAIL);

    napi_value constructor = nullptr;
    napi_value instance = nullptr;
    CHECK_ARGS(env, napi_get_reference_value(env, constructor_, &constructor), JS_INNER_FAIL);
    CHECK_ARGS(env, napi_new_instance(env, constructor, 1, &photoAlbumNapi, &instance), JS_INNER_FAIL);
    CHECK_COND(env, instance != nullptr, JS_INNER_FAIL);

    MediaAlbumChangeRequestNapi* changeRequest = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, instance, reinterpret_cast<void**>(&changeRequest)), JS_INNER_FAIL);
    CHECK_COND(env, changeRequest != nullptr, JS_INNER_FAIL);
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::CREATE_ALBUM);
    return instance;
}

static napi_value ParseArgsDeleteAlbums(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext>& context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_THREE;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, minArgs, maxArgs) == napi_ok,
        "Failed to get args");
    CHECK_COND(env, MediaAlbumChangeRequestNapi::InitUserFileClient(env, info), JS_INNER_FAIL);

    vector<napi_value> napiValues;
    napi_valuetype valueType = napi_undefined;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[PARAM1], napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");

    vector<string> deleteIds;
    for (const auto& napiValue : napiValues) {
        PhotoAlbumNapi* obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void**>(&obj)), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, obj != nullptr, "Failed to get album napi object");
        CHECK_COND_WITH_MESSAGE(env,
            PhotoAlbum::IsUserPhotoAlbum(obj->GetPhotoAlbumType(), obj->GetPhotoAlbumSubType()),
            "Only user album can be deleted");
        deleteIds.push_back(to_string(obj->GetAlbumId()));
    }
    context->predicates.In(PhotoAlbumColumns::ALBUM_ID, deleteIds);
    RETURN_NAPI_TRUE(env);
}

static void DeleteAlbumsExecute(napi_env env, void* data)
{
    auto* context = static_cast<MediaAlbumChangeRequestAsyncContext*>(data);
    Uri deleteAlbumUri(PAH_DELETE_PHOTO_ALBUM);
    int ret = UserFileClient::Delete(deleteAlbumUri, context->predicates);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Failed to delete albums, err: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("Delete %{public}d album(s)", ret);
}

static void DeleteAlbumsCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<MediaAlbumChangeRequestAsyncContext*>(data);
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

napi_value MediaAlbumChangeRequestNapi::JSDeleteAlbums(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsDeleteAlbums(env, info, asyncContext), "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env, asyncContext, "ChangeRequestDeleteAlbums", DeleteAlbumsExecute, DeleteAlbumsCompleteCallback);
}

napi_value MediaAlbumChangeRequestNapi::JSAddAssets(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user album can add assets");

    vector<string> assetUriArray;
    CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
        "Failed to parse assets");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToAdd_)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous addAssets operation has contained the same asset");
        return nullptr;
    }
    changeRequest->assetsToAdd_.insert(changeRequest->assetsToAdd_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::ADD_ASSETS);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSRemoveAssets(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user album can remove assets");

    vector<string> assetUriArray;
    CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
        "Failed to parse assets");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToRemove_)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous removeAssets operation has contained the same asset");
        return nullptr;
    }
    changeRequest->assetsToRemove_.insert(
        changeRequest->assetsToRemove_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::REMOVE_ASSETS);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSMoveAssets(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_TWO, ARGS_TWO) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user album can move assets");

    shared_ptr<PhotoAlbum> targetAlbum = nullptr;
    CHECK_COND_WITH_MESSAGE(
        env, ParsePhotoAlbum(env, asyncContext->argv[PARAM1], targetAlbum), "Failed to parse targetAlbum");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(targetAlbum->GetPhotoAlbumType(), targetAlbum->GetPhotoAlbumSubType()),
        "targetAlbum is not user album");
    CHECK_COND_WITH_MESSAGE(env, targetAlbum->GetAlbumId() != photoAlbum->GetAlbumId(), "targetAlbum cannot be self");

    vector<string> assetUriArray;
    CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
        "Failed to parse assets");
    auto moveMap = changeRequest->GetMoveMap();
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        if (!CheckDuplicatedAssetArray(assetUriArray, iter->second)) {
            NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
                "The previous moveAssets operation has contained the same asset");
            return nullptr;
        }
    }
    changeRequest->RecordMoveAssets(assetUriArray, targetAlbum);
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::MOVE_ASSETS);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSRecoverAssets(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only trash album can recover assets");

    vector<string> assetUriArray;
    CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
        "Failed to parse assets");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToRecover_)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous recoverAssets operation has contained the same asset");
        return nullptr;
    }
    changeRequest->assetsToRecover_.insert(
        changeRequest->assetsToRecover_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::RECOVER_ASSETS);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSDeleteAssets(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only trash album can delete assets permanently");

    vector<string> assetUriArray;
    CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
        "Failed to parse assets");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToDelete_)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous deleteAssets operation has contained the same asset");
        return nullptr;
    }
    changeRequest->assetsToDelete_.insert(
        changeRequest->assetsToDelete_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::DELETE_ASSETS);
    RETURN_NAPI_UNDEFINED(env);
}

static napi_value GetAssetsIdArray(napi_env env, napi_value arg, vector<string> &assetsArray)
{
    bool isArray = false;
    CHECK_ARGS(env, napi_is_array(env, arg, &isArray), JS_INNER_FAIL);
    if (!isArray) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check array type");
        return nullptr;
    }

    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, arg, &len), JS_INNER_FAIL);
    if (len <= 0) {
        NAPI_ERR_LOG("Failed to check array length: %{public}u", len);
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check array length");
        return nullptr;
    }

    for (uint32_t i = 0; i < len; i++) {
        napi_value asset = nullptr;
        CHECK_ARGS(env, napi_get_element(env, arg, i, &asset), JS_INNER_FAIL);
        if (asset == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get asset element");
            return nullptr;
        }

        FileAssetNapi *obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, asset, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
        if (obj == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get asset napi object");
            return nullptr;
        }
        if ((obj->GetMediaType() != MEDIA_TYPE_IMAGE && obj->GetMediaType() != MEDIA_TYPE_VIDEO)) {
            NAPI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", obj->GetMediaType());
            continue;
        }
        assetsArray.push_back(obj->GetFileUri());
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaAlbumChangeRequestNapi::JSSetIsMe(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(
        env, info, asyncContext, ARGS_ZERO, ARGS_ZERO) == napi_ok, "Failed to get object info");

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only portrait album can set is me");
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_IS_ME);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

bool MediaAlbumChangeRequestNapi::CheckDismissAssetVaild(std::vector<std::string> &dismissAssets,
    std::vector<std::string> &newAssetArray)
{
    if (newAssetArray.empty()) {
        return false;
    }
    unordered_set<string> assetSet(dismissAssets.begin(), dismissAssets.end());
    unordered_set<string> tempSet;
    for (const auto& newAsset : newAssetArray) {
        if (assetSet.find(newAsset) != assetSet.end()) {
            return false;
        }
        tempSet.insert(newAsset);
    }
    for (const auto& tmp : tempSet) {
        dismissAssets.push_back(tmp);
    }
    return true;
}

napi_value MediaAlbumChangeRequestNapi::JSDismissAssets(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(
        env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok, "Failed to get object info");

    vector<std::string> newAssetArray;
    CHECK_NULLPTR_RET(GetAssetsIdArray(env, asyncContext->argv[PARAM0], newAssetArray));
    if (!CheckDismissAssetVaild(asyncContext->objectInfo->dismissAssets_, newAssetArray)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT, "This dismissAssets is not support");
        return nullptr;
    }
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
            PhotoAlbum::IsSmartClassifyAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only portrait and classify album can dismiss asset");

    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::DISMISS_ASSET);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaAlbumChangeRequestNapi::JSMergeAlbum(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_valuetype valueType;
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();

    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(
        env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok, "Failed to get object info");
    CHECK_ARGS(env, napi_typeof(env, asyncContext->argv[PARAM0], &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");
    if (valueType == napi_object) {
        PhotoAlbumNapi* photoAlbumNapi;
        CHECK_ARGS(env, napi_unwrap(env, asyncContext->argv[PARAM0],
            reinterpret_cast<void**>(&photoAlbumNapi)), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, photoAlbumNapi != nullptr, "Failed to get PhotoAlbumNapi object");
        asyncContext->objectInfo->targetAlbum_ = photoAlbumNapi->GetPhotoAlbumInstance();
    }
    auto photoAlbum = asyncContext->objectInfo->photoAlbum_;
    auto targetAlbum = asyncContext->objectInfo->targetAlbum_;
    CHECK_COND_WITH_MESSAGE(env,
        (photoAlbum != nullptr) && (targetAlbum != nullptr), "PhotoAlbum  Or TargetAlbum is nullptr");
    CHECK_COND_WITH_MESSAGE(env,
        (PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) &&
        (PhotoAlbum::IsSmartPortraitPhotoAlbum(targetAlbum->GetPhotoAlbumType(), targetAlbum->GetPhotoAlbumSubType())),
        "Only portrait album can merge");
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::MERGE_ALBUM);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaAlbumChangeRequestNapi::JSSetDisplayLevel(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    int32_t displayLevel;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext, displayLevel) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayLevel(displayLevel), "Invalid display level");

    auto photoAlbum = asyncContext->objectInfo->photoAlbum_;
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "PhotoAlbum is nullptr");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only portrait album can set album display level");
    photoAlbum->SetDisplayLevel(displayLevel);
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_DISPLAY_LEVEL);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaAlbumChangeRequestNapi::JSSetAlbumName(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    string albumName;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, albumName) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckAlbumName(albumName) == E_OK, "Invalid album name");

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user album and smart portrait album can set album name");
    photoAlbum->SetAlbumName(albumName);
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_ALBUM_NAME);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSSetCoverUri(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    string coverUri;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, coverUri) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user album and smart portrait album can set album name");
    photoAlbum->SetCoverUri(coverUri);
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_COVER_URI);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSPlaceBefore(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_valuetype valueType;
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();

    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(
        env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok, "Failed to get object info");
    CHECK_ARGS(env, napi_typeof(env, asyncContext->argv[PARAM0], &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object || valueType == napi_null, "Invalid argument type");
    if (valueType == napi_object) {
        PhotoAlbumNapi* photoAlbumNapi;
        CHECK_ARGS(env, napi_unwrap(env, asyncContext->argv[PARAM0],
            reinterpret_cast<void**>(&photoAlbumNapi)), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, photoAlbumNapi != nullptr, "Failed to get PhotoAlbumNapi object");
        asyncContext->objectInfo->referencePhotoAlbum_ = photoAlbumNapi->GetPhotoAlbumInstance();
    }
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::ORDER_ALBUM);
    RETURN_NAPI_UNDEFINED(env);
}

static bool CreateAlbumExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    auto changeRequest = context.objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();

    Uri createAlbumUri(PAH_CREATE_PHOTO_ALBUM);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, photoAlbum->GetAlbumName());
    int32_t ret = UserFileClient::Insert(createAlbumUri, valuesBucket);
    if (ret == -1) {
        context.SaveError(-EEXIST);
        NAPI_ERR_LOG("Album exists");
        return false;
    }
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to create album, ret: %{public}d", ret);
        return false;
    }

    photoAlbum->SetAlbumId(ret);
    photoAlbum->SetAlbumUri(PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(ret));
    return true;
}

static bool FetchNewCount(MediaAlbumChangeRequestAsyncContext& context, shared_ptr<PhotoAlbum>& album)
{
    if (album == nullptr || !PhotoAlbum::IsUserPhotoAlbum(album->GetPhotoAlbumType(), album->GetPhotoAlbumSubType())) {
        NAPI_ERR_LOG("Only user album can add or remove assets");
        context.SaveError(E_FAIL);
        return false;
    }

    Uri queryUri(PAH_QUERY_PHOTO_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, album->GetAlbumId());
    vector<string> fetchColumns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT, PhotoAlbumColumns::ALBUM_VIDEO_COUNT };
    int errCode = 0;
    auto resultSet = UserFileClient::Query(queryUri, predicates, fetchColumns, errCode);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        context.SaveError(E_HAS_DB_ERROR);
        return false;
    }
    if (resultSet->GoToFirstRow() != 0) {
        NAPI_ERR_LOG("go to first row failed when fetch new count");
        context.SaveError(E_HAS_DB_ERROR);
        return false;
    }

    bool hiddenOnly = album->GetHiddenOnly();
    int imageCount = hiddenOnly ? -1 :
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet, TYPE_INT32));
    int videoCount = hiddenOnly ? -1 :
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet, TYPE_INT32));
    album->SetCount(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COUNT, resultSet, TYPE_INT32)));
    album->SetImageCount(imageCount);
    album->SetVideoCount(videoCount);
    return true;
}

static bool AddAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    auto changeRequest = context.objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    int32_t albumId = photoAlbum->GetAlbumId();
    vector<DataShare::DataShareValuesBucket> valuesBuckets;
    for (const auto& asset : changeRequest->GetAddAssetArray()) {
        DataShare::DataShareValuesBucket pair;
        pair.Put(PhotoMap::ALBUM_ID, albumId);
        pair.Put(PhotoMap::ASSET_ID, asset);
        valuesBuckets.push_back(pair);
    }

    Uri addAssetsUri(PAH_PHOTO_ALBUM_ADD_ASSET);
    int ret = UserFileClient::BatchInsert(addAssetsUri, valuesBuckets);
    changeRequest->ClearAddAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to add assets into album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }

    NAPI_INFO_LOG("Add %{public}d asset(s) into album %{public}d", ret, albumId);
    FetchNewCount(context, photoAlbum);
    return true;
}

static bool RemoveAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    auto changeRequest = context.objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    int32_t albumId = photoAlbum->GetAlbumId();
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
    predicates.And()->In(PhotoMap::ASSET_ID, changeRequest->GetRemoveAssetArray());

    Uri removeAssetsUri(PAH_PHOTO_ALBUM_REMOVE_ASSET);
    int ret = UserFileClient::Delete(removeAssetsUri, predicates);
    changeRequest->ClearRemoveAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to remove assets from album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }

    NAPI_INFO_LOG("Remove %{public}d asset(s) from album %{public}d", ret, albumId);
    FetchNewCount(context, photoAlbum);
    return true;
}

static bool MoveAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    auto changeRequest = context.objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    int32_t albumId = photoAlbum->GetAlbumId();
    auto moveMap = changeRequest->GetMoveMap();
    changeRequest->ClearMoveMap();

    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        auto targetPhotoAlbum = iter->first;
        int32_t targetAlbumId = targetPhotoAlbum->GetAlbumId();
        vector<string> moveAssetArray = iter->second;
        // Add into target album.
        vector<DataShare::DataShareValuesBucket> valuesBuckets;
        for (const auto& asset : moveAssetArray) {
            DataShare::DataShareValuesBucket pair;
            pair.Put(PhotoMap::ALBUM_ID, targetAlbumId);
            pair.Put(PhotoMap::ASSET_ID, asset);
            valuesBuckets.push_back(pair);
        }

        Uri addAssetsUri(PAH_PHOTO_ALBUM_ADD_ASSET);
        int ret = UserFileClient::BatchInsert(addAssetsUri, valuesBuckets);
        if (ret < 0) {
            context.SaveError(ret);
            NAPI_ERR_LOG("Failed to move assets into album %{public}d, err: %{public}d", targetAlbumId, ret);
            return false;
        }
        NAPI_INFO_LOG("Move %{public}d asset(s) into album %{public}d", ret, targetAlbumId);

        // Remove from current album.
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
        predicates.And()->In(PhotoMap::ASSET_ID, moveAssetArray);

        Uri removeAssetsUri(PAH_PHOTO_ALBUM_REMOVE_ASSET);
        ret = UserFileClient::Delete(removeAssetsUri, predicates);
        if (ret < 0) {
            context.SaveError(ret);
            NAPI_ERR_LOG("Failed to move assets from album %{public}d, err: %{public}d", albumId, ret);
            return false;
        }
        NAPI_INFO_LOG("Move %{public}d asset(s) from album %{public}d", ret, albumId);
        FetchNewCount(context, targetPhotoAlbum);
    }
    FetchNewCount(context, photoAlbum);
    return true;
}

static bool RecoverAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    predicates.In(PhotoColumn::MEDIA_ID, context.objectInfo->GetRecoverAssetArray());
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, 0);

    Uri recoverAssetsUri(PAH_RECOVER_PHOTOS);
    int ret = UserFileClient::Update(recoverAssetsUri, predicates, valuesBucket);
    context.objectInfo->ClearRecoverAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to recover assets, err: %{public}d", ret);
        return false;
    }

    NAPI_INFO_LOG("Recover %{public}d assets from trash album", ret);
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    int32_t currentCount = photoAlbum->GetCount() - ret;
    photoAlbum->SetCount(currentCount > 0 ? currentCount : 0);
    return true;
}

static bool DeleteAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, context.objectInfo->GetDeleteAssetArray());
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, 0);

    Uri deleteAssetsUri(PAH_DELETE_PHOTOS);
    int ret = UserFileClient::Update(deleteAssetsUri, predicates, valuesBucket);
    context.objectInfo->ClearDeleteAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to delete assets from trash album permanently, err: %{public}d", ret);
        return false;
    }

    NAPI_INFO_LOG("Delete %{public}d assets permanently from trash album", ret);
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    int32_t currentCount = photoAlbum->GetCount() - ret;
    photoAlbum->SetCount(currentCount > 0 ? currentCount : 0);
    return true;
}

static bool OrderAlbumExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    auto referenceAlum = context.objectInfo->GetReferencePhotoAlbumInstance();
    Uri updateAlbumUri(PAH_ORDER_ALBUM);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, photoAlbum->GetAlbumId());
    int32_t referenceAlbumId = -1;
    if (referenceAlum != nullptr) {
        referenceAlbumId = referenceAlum->GetAlbumId();
    }
    valuesBucket.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, referenceAlbumId);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_TYPE, photoAlbum->GetPhotoAlbumType());
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, photoAlbum->GetPhotoAlbumSubType());
    int32_t result = UserFileClient::Update(updateAlbumUri, predicates, valuesBucket);
    if (result < 0) {
        context.SaveError(result);
        NAPI_ERR_LOG("Failed to order albums err: %{public}d", result);
        return false;
    }
    return true;
}

static bool DismissAssetExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    string disMissAssetAssetsUri = PAH_DISMISS_ASSET;
    Uri uri(disMissAssetAssetsUri);

    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MAP_ALBUM, to_string(photoAlbum->GetAlbumId()));
    predicates.And()->In(MAP_ASSET, context.objectInfo->GetDismissAssetArray());
    predicates.And()->EqualTo(ALBUM_SUBTYPE, to_string(photoAlbum->GetPhotoAlbumSubType()));

    auto deletedRows = UserFileClient::Delete(uri, predicates);
    context.objectInfo->ClearDismissAssetArray();
    if (deletedRows < 0) {
        context.SaveError(deletedRows);
        NAPI_ERR_LOG("Failed to dismiss asset err: %{public}d", deletedRows);
        return false;
    }
    return true;
}

static bool MergeAlbumExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    auto targetAlum = context.objectInfo->GetTargetPhotoAlbumInstance();
    Uri updateAlbumUri(PAH_PORTRAIT_MERGE_ALBUM);
    valuesBucket.Put(ALBUM_ID, photoAlbum->GetAlbumId());
    int32_t targetAlbumId = -1;
    if (targetAlum != nullptr) {
        targetAlbumId = targetAlum->GetAlbumId();
    }
    valuesBucket.Put(TARGET_ALBUM_ID, targetAlbumId);
    int32_t result = UserFileClient::Update(updateAlbumUri, predicates, valuesBucket);
    if (result < 0) {
        context.SaveError(result);
        NAPI_ERR_LOG("Failed to merge albums err: %{public}d", result);
        return false;
    }
    return true;
}

static bool GetAlbumUpdateValue(shared_ptr<PhotoAlbum>& photoAlbum, const AlbumChangeOperation changeOperation,
    string& uri, DataShare::DataShareValuesBucket& valuesBucket, string& property)
{
    if (photoAlbum == nullptr) {
        NAPI_ERR_LOG("photoAlbum is null");
        return false;
    }

    switch (changeOperation) {
        case AlbumChangeOperation::SET_ALBUM_NAME:
            if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::PORTRAIT) {
                uri = PAH_PORTRAIT_ANAALBUM_ALBUM_NAME;
            } else {
                uri = PAH_UPDATE_PHOTO_ALBUM;
            }
            property = PhotoAlbumColumns::ALBUM_NAME;
            valuesBucket.Put(property, photoAlbum->GetAlbumName());
            break;
        case AlbumChangeOperation::SET_COVER_URI:
            if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::PORTRAIT) {
                uri = PAH_PORTRAIT_ANAALBUM_COVER_URI;
            } else {
                uri = PAH_UPDATE_PHOTO_ALBUM;
            }
            property = PhotoAlbumColumns::ALBUM_COVER_URI;
            valuesBucket.Put(property, photoAlbum->GetCoverUri());
            break;
        case AlbumChangeOperation::SET_DISPLAY_LEVEL:
            uri = PAH_PORTRAIT_DISPLAY_LEVLE;
            property = USER_DISPLAY_LEVEL;
            valuesBucket.Put(property, photoAlbum->GetDisplayLevel());
            break;
        case AlbumChangeOperation::SET_IS_ME:
            uri = PAH_PORTRAIT_IS_ME;
            property = IS_ME;
            valuesBucket.Put(property, 1);
            break;
        default:
            return false;
    }
    return true;
}

static bool SetAlbumPropertyExecute(
    MediaAlbumChangeRequestAsyncContext& context, const AlbumChangeOperation changeOperation)
{
    // In the scenario of creation, the new name will be applied when the album is created.
    if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME &&
        context.albumChangeOperations.front() == AlbumChangeOperation::CREATE_ALBUM) {
        return true;
    }

    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(photoAlbum->GetAlbumId()));
    string uri;
    string property;
    if (!GetAlbumUpdateValue(photoAlbum, changeOperation, uri, valuesBucket, property)) {
        context.SaveError(E_FAIL);
        NAPI_ERR_LOG("Failed to parse album change operation: %{public}d", changeOperation);
        return false;
    }
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, photoAlbum->GetPhotoAlbumSubType());
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAlbumUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAlbumUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context.SaveError(changedRows);
        NAPI_ERR_LOG("Failed to set %{public}s, err: %{public}d", property.c_str(), changedRows);
        return false;
    }
    return true;
}

static const unordered_map<AlbumChangeOperation, bool (*)(MediaAlbumChangeRequestAsyncContext&)> EXECUTE_MAP = {
    { AlbumChangeOperation::CREATE_ALBUM, CreateAlbumExecute },
    { AlbumChangeOperation::ADD_ASSETS, AddAssetsExecute },
    { AlbumChangeOperation::REMOVE_ASSETS, RemoveAssetsExecute },
    { AlbumChangeOperation::MOVE_ASSETS, MoveAssetsExecute },
    { AlbumChangeOperation::RECOVER_ASSETS, RecoverAssetsExecute },
    { AlbumChangeOperation::DELETE_ASSETS, DeleteAssetsExecute },
    { AlbumChangeOperation::ORDER_ALBUM, OrderAlbumExecute },
    { AlbumChangeOperation::MERGE_ALBUM, MergeAlbumExecute },
    { AlbumChangeOperation::DISMISS_ASSET, DismissAssetExecute },
};

static void ApplyAlbumChangeRequestExecute(napi_env env, void* data)
{
    auto* context = static_cast<MediaAlbumChangeRequestAsyncContext*>(data);
    unordered_set<AlbumChangeOperation> appliedOperations;
    for (const auto& changeOperation : context->albumChangeOperations) {
        // Keep the final result(s) of each operation, and commit only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = false;
        auto iter = EXECUTE_MAP.find(changeOperation);
        if (iter != EXECUTE_MAP.end()) {
            valid = iter->second(*context);
        } else if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME ||
                   changeOperation == AlbumChangeOperation::SET_COVER_URI ||
                   changeOperation == AlbumChangeOperation::SET_IS_ME ||
                   changeOperation == AlbumChangeOperation::SET_DISPLAY_LEVEL) {
            valid = SetAlbumPropertyExecute(*context, changeOperation);
        } else {
            NAPI_ERR_LOG("Invalid album change operation: %{public}d", changeOperation);
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
        }

        if (!valid) {
            NAPI_ERR_LOG("Failed to apply album change request, operation: %{public}d", changeOperation);
            return;
        }
        appliedOperations.insert(changeOperation);
    }
}

static void ApplyAlbumChangeRequestCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<MediaAlbumChangeRequestAsyncContext*>(data);
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

bool MediaAlbumChangeRequestNapi::CheckPortraitMergeAlbum()
{
    bool hasMergeAlbum = false;
    bool hasAlbumName = false;
    for (auto operation : albumChangeOperations_) {
        if (operation == AlbumChangeOperation::MERGE_ALBUM) {
            hasMergeAlbum = true;
        }
        if (operation == AlbumChangeOperation::SET_ALBUM_NAME) {
            hasAlbumName = true;
        }
    }
    return (hasAlbumName && hasMergeAlbum) || (hasMergeAlbum == false);
}

napi_value MediaAlbumChangeRequestNapi::ApplyChanges(napi_env env, napi_callback_info info)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, asyncContext, minArgs, maxArgs) == napi_ok,
        "Failed to get args");
    asyncContext->objectInfo = this;
    CHECK_COND_WITH_MESSAGE(env, CheckChangeOperations(env), "Failed to check album change request operations");
    asyncContext->albumChangeOperations = albumChangeOperations_;
    albumChangeOperations_.clear();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ApplyMediaAlbumChangeRequest",
        ApplyAlbumChangeRequestExecute, ApplyAlbumChangeRequestCompleteCallback);
}
} // namespace OHOS::Media