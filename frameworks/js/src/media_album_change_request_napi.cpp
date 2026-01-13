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
#include <sstream>

#include "file_asset_napi.h"
#include "media_file_uri.h"
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
#include "album_operation_uri.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "delete_albums_vo.h"
#include "change_request_set_album_name_vo.h"
#include "change_request_set_cover_uri_vo.h"
#include "change_request_dismiss_vo.h"
#include "change_request_set_display_level_vo.h"
#include "change_request_set_is_me_vo.h"
#include "change_request_add_assets_vo.h"
#include "change_request_remove_assets_vo.h"
#include "change_request_move_assets_vo.h"
#include "change_request_recover_assets_vo.h"
#include "change_request_delete_assets_vo.h"
#include "change_request_dismiss_assets_vo.h"
#include "change_request_merge_album_vo.h"
#include "change_request_place_before_vo.h"
#include "change_request_set_order_position_vo.h"
#include "change_request_set_upload_status_vo.h"
#include "change_request_set_relationship_vo.h"
#include "change_request_set_highlight_attribute_vo.h"
#include "create_analysis_album_vo.h"
#include "media_change_request_utils.h"

using namespace std;

namespace OHOS::Media {
static const string MEDIA_ALBUM_CHANGE_REQUEST_CLASS = "MediaAlbumChangeRequest";
static const string MEDIA_ANALYSIS_ALBUM_CHANGE_REQUEST_CLASS = "MediaAnalysisAlbumChangeRequest";
static const string MEDIA_HIGHLIGHT_ALBUM_CHANGE_REQUEST_CLASS = "MediaHighlightAlbumChangeRequest";
thread_local napi_ref MediaAlbumChangeRequestNapi::constructor_ = nullptr;
thread_local napi_ref MediaAlbumChangeRequestNapi::mediaAnalysisAlbumChangeRequestConstructor_ = nullptr;
thread_local napi_ref MediaAlbumChangeRequestNapi::mediaHighlightAlbumChangeRequestConstructor_ = nullptr;
static const int32_t VALUE_IS_ME = 1;
static const int32_t VALUE_IS_REMOVED = 1;
static const int32_t QUERY_IS_ME_VALUE = 2;
static const int32_t NAPI_INVALID_PARAMETER_ERROR = 23800151;
static const int32_t MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR = 23800301;
static const int32_t MEDIA_LIBRARY_PARAM_ERROR = 23800151;
static const int32_t MAX_HIGHLIGHT_ATTRIBUTE_VALUE_LEN = 20;

napi_value MediaAlbumChangeRequestNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_ALBUM_CHANGE_REQUEST_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_PROPERTY("comment", MediaChangeRequestUtils::CreateComment(env)),
            DECLARE_NAPI_STATIC_FUNCTION("createAlbumRequest", JSCreateAlbumRequest),
            DECLARE_NAPI_STATIC_FUNCTION("deleteAlbums", JSDeleteAlbums),
            DECLARE_NAPI_STATIC_FUNCTION("deleteAlbumsWithUri", JSDeleteAlbumsWithUri),
            DECLARE_NAPI_FUNCTION("getAlbum", JSGetAlbum),
            DECLARE_NAPI_FUNCTION("addAssets", JSAddAssets),
            DECLARE_NAPI_FUNCTION("removeAssets", JSRemoveAssets),
            DECLARE_NAPI_FUNCTION("moveAssets", JSMoveAssets),
            DECLARE_NAPI_FUNCTION("moveAssetsWithUri", JSMoveAssetsWithUri),
            DECLARE_NAPI_FUNCTION("recoverAssets", JSRecoverAssets),
            DECLARE_NAPI_FUNCTION("recoverAssetsWithUri", JSRecoverAssetsWithUri),
            DECLARE_NAPI_FUNCTION("deleteAssets", JSDeleteAssets),
            DECLARE_NAPI_FUNCTION("deleteAssetsWithUri", JSDeleteAssetsWithUri),
            DECLARE_NAPI_FUNCTION("setAlbumName", JSSetAlbumName),
            DECLARE_NAPI_FUNCTION("setCoverUri", JSSetCoverUri),
            DECLARE_NAPI_FUNCTION("resetCoverUri", JSResetCoverUri),
            DECLARE_NAPI_FUNCTION("placeBefore", JSPlaceBefore),
            DECLARE_NAPI_FUNCTION("setDisplayLevel", JSSetDisplayLevel),
            DECLARE_NAPI_FUNCTION("mergeAlbum", JSMergeAlbum),
            DECLARE_NAPI_FUNCTION("dismissAssets", JSDismissAssets),
            DECLARE_NAPI_FUNCTION("setIsMe", JSSetIsMe),
            DECLARE_NAPI_FUNCTION("dismiss", JSDismiss),
            DECLARE_NAPI_STATIC_FUNCTION("setUploadStatus", JSSetUploadStatus),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value MediaAlbumChangeRequestNapi::MediaAnalysisAlbumChangeRequestInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_ANALYSIS_ALBUM_CHANGE_REQUEST_CLASS,
        .ref = &mediaAnalysisAlbumChangeRequestConstructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_PROPERTY("comment",
                MediaChangeRequestUtils::CreateComment(env)),
            DECLARE_NAPI_STATIC_FUNCTION("createAlbumRequest", JSCreateAlbumRequest),
            DECLARE_NAPI_STATIC_FUNCTION("deleteAlbums", JSDeleteAlbums),
            DECLARE_NAPI_STATIC_FUNCTION("createAnalysisAlbumRequest", JSCreateAnalysisAlbumRequest),
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
            DECLARE_NAPI_FUNCTION("dismiss", JSDismiss),
            DECLARE_NAPI_FUNCTION("setOrderPosition", JSSetOrderPosition),
            DECLARE_NAPI_FUNCTION("setRelationship", JSSetRelationship),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value MediaAlbumChangeRequestNapi::MediaHighlightAlbumChangeRequestInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_HIGHLIGHT_ALBUM_CHANGE_REQUEST_CLASS,
        .ref = &mediaHighlightAlbumChangeRequestConstructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_PROPERTY("comment",
                MediaChangeRequestUtils::CreateComment(env)),
            DECLARE_NAPI_FUNCTION("setHighlightAttribute", JSSetHighlightAttribute),
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

std::string MediaAlbumChangeRequestNapi::GetRelationship() const
{
    return relationship_;
}

int32_t MediaAlbumChangeRequestNapi::GetIsMe() const
{
    return isMe_;
}

std::vector<std::pair<std::string, int32_t>> MediaAlbumChangeRequestNapi::GetIdOrderPositionPairs() const
{
    return idOrderPositionPairs_;
}

map<shared_ptr<PhotoAlbum>, vector<string>, PhotoAlbumPtrCompare> MediaAlbumChangeRequestNapi::GetMoveMap() const
{
    return moveMap_;
}

int32_t MediaAlbumChangeRequestNapi::GetUserId() const
{
    return userId_;
}

std::pair<int32_t, std::string> MediaAlbumChangeRequestNapi::GetHighlightAlbumChangeAttributePair() const
{
    return highlightAlbumChangeAttributePair_;
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

const std::unordered_set<AlbumChangeOperation> VALID_CREATE_OPERATIONS = {
    AlbumChangeOperation::CREATE_ALBUM,
    AlbumChangeOperation::CREATE_ANALYSIS_ALBUM
};

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

    if (VALID_CREATE_OPERATIONS.find(albumChangeOperations_.front()) == VALID_CREATE_OPERATIONS.end() &&
        photoAlbum->GetAlbumId() <= 0) {
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

static bool CheckAssetsUri(const string& uri)
{
    if (uri.empty()) {
        NAPI_ERR_LOG("uri is empty, can not get fileId");
        return false;
    }
    MediaFileUri fileUri(uri);
    if (!fileUri.IsApi10()) {
        fileUri = MediaFileUri(MediaFileUtils::GetRealUriFromVirtualUri(uri));
    }
    string fileId = fileUri.GetFileId();
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit) || atoi(fileId.c_str()) <= 0) {
        return false;
    }
    return true;
}

static napi_value GetUriArray(napi_env env, vector<napi_value> &napiValues, vector<string> &values)
{
    napi_valuetype valueType = napi_undefined;
    unique_ptr<char[]> buffer = make_unique<char[]>(PATH_MAX);
    for (const auto &napiValue : napiValues) {
        CHECK_ARGS(env, napi_typeof(env, napiValue, &valueType), JS_ERR_PARAMETER_INVALID);
        CHECK_COND(env, valueType == napi_string, JS_ERR_PARAMETER_INVALID);

        size_t res = 0;
        CHECK_ARGS(
            env, napi_get_value_string_utf8(env, napiValue, buffer.get(), PATH_MAX, &res), JS_ERR_PARAMETER_INVALID);
        string uri = buffer.get();
        if (!CheckAssetsUri(uri)) {
            NAPI_ERR_LOG("fileId is invalid, uri is %{public}s", uri.c_str());
            continue;
        }
        values.emplace_back(uri);
    }
    napi_value ret = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &ret), JS_INNER_FAIL);
    return ret;
}

static napi_value ParseUriArray(napi_env env, napi_value arg, vector<string>& uriArray)
{
    vector<napi_value> napiValues;
    napi_valuetype valueType = napi_undefined;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, arg, napiValues));
    CHECK_ARGS_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_string, "Invalid argument type");
    CHECK_NULLPTR_RET(GetUriArray(env, napiValues, uriArray));
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

static napi_value DealWithDeletedAlbumsDefault(napi_env env, vector<napi_value>& napiValues,
    unique_ptr<MediaAlbumChangeRequestAsyncContext>& context)
{
    vector<string> deleteIds;
    for (const auto& napiValue : napiValues) {
        PhotoAlbumNapi* obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void**>(&obj)), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, obj != nullptr, "Failed to get album napi object");
        CHECK_COND_WITH_MESSAGE(env,
            PhotoAlbum::IsUserPhotoAlbumByType(obj->GetPhotoAlbumType()) ||
            PhotoAlbum::IsHighlightAlbum(obj->GetPhotoAlbumType(), obj->GetPhotoAlbumSubType()) ||
            PhotoAlbum::IsSourceAlbum(obj->GetPhotoAlbumType(), obj->GetPhotoAlbumSubType()),
            "Only user, source and highlight album can be deleted");
        deleteIds.push_back(to_string(obj->GetAlbumId()));
    }
    context->deleteIds = deleteIds;
    RETURN_NAPI_TRUE(env);
}

int32_t GetAlbumIdFromUri(const string &uri, string &albumId)
{
    auto startIndex = uri.find(PhotoAlbumColumns::ALBUM_URI_PREFIX);
    if (startIndex != std::string::npos) {
        albumId.clear();
        albumId = uri.substr(startIndex + PhotoAlbumColumns::ALBUM_URI_PREFIX.length());
        if (!all_of(albumId.begin(), albumId.end(), ::isdigit)) {
            NAPI_ERR_LOG("albumId is not digit, albumId is %{private}s", albumId.c_str());
            return E_URI_INVALID;
        }
    } else {
        NAPI_ERR_LOG("Photo album uri format error");
        return E_URI_INVALID;
    }
    return E_OK;
}

napi_value DealWithDeletedAlbumsByUri(napi_env env, vector<napi_value> &napiValues,
    unique_ptr<MediaAlbumChangeRequestAsyncContext>& context)
{
    vector<string> deleteIds;
    vector<string> albumUris;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetStringArray(env, napiValues, albumUris));
    for (const auto& albumUri : albumUris) {
        string albumId = "";
        CHECK_ARGS_WITH_MESSAGE(env, GetAlbumIdFromUri(albumUri, albumId) == E_OK, "Failed to get albumId");
        deleteIds.push_back(albumId);
    }

    context->deleteIds = deleteIds;
    RETURN_NAPI_TRUE(env);
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

    if (valueType == napi_object) {
        CHECK_NULLPTR_RET(DealWithDeletedAlbumsDefault(env, napiValues, context));
    } else if (valueType == napi_string) {
        CHECK_NULLPTR_RET(DealWithDeletedAlbumsByUri(env, napiValues, context));
    } else {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid argument type");
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}

static void DeleteAlbumsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteAlbumsExecute");

    auto *context = static_cast<MediaAlbumChangeRequestAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_IF_EQUAL(!context->deleteIds.empty(), "albumIds is empty");

    DeleteAlbumsReqBody reqBody;
    reqBody.albumIds = context->deleteIds;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
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

napi_value MediaAlbumChangeRequestNapi::JSDeleteAlbumsWithUri(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsDeleteAlbums(env, info, asyncContext), "Failed to parse args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env, asyncContext, "ChangeRequestDeleteAlbums", DeleteAlbumsExecute, DeleteAlbumsCompleteCallback);
}

napi_value MediaAlbumChangeRequestNapi::JSAddAssets(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("enter");
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
            PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user and highlight album can add assets");

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

napi_value CheckPhotoAlbumType(napi_env env, shared_ptr<PhotoAlbum> photoAlbum,
    shared_ptr<PhotoAlbum> targetAlbum)
{
    CHECK_NULLPTR_RET(photoAlbum);
    CHECK_NULLPTR_RET(targetAlbum);

    bool isUserOrSourceAlbum = PhotoAlbum::IsUserPhotoAlbum(targetAlbum->GetPhotoAlbumType(),
        targetAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSourceAlbum(targetAlbum->GetPhotoAlbumType(),
        targetAlbum->GetPhotoAlbumSubType());

    bool isSmartPortrait = PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(),
        photoAlbum->GetPhotoAlbumSubType()) &&
        PhotoAlbum::IsSmartPortraitPhotoAlbum(targetAlbum->GetPhotoAlbumType(),
        targetAlbum->GetPhotoAlbumSubType());
    if (!isUserOrSourceAlbum && !isSmartPortrait) {
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}

napi_value MediaAlbumChangeRequestNapi::JSMoveAssetsImplement(napi_env env, napi_callback_info info,
    ParameterType parameterType)
{
    MEDIA_INFO_LOG("enter");
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

    shared_ptr<PhotoAlbum> targetAlbum = nullptr;
    CHECK_COND_WITH_MESSAGE(
        env, ParsePhotoAlbum(env, asyncContext->argv[PARAM1], targetAlbum), "Failed to parse targetAlbum");
    CHECK_COND_WITH_MESSAGE(env, targetAlbum->GetAlbumId() != photoAlbum->GetAlbumId(), "targetAlbum cannot be self");

    vector<string> assetUriArray;
    if (parameterType == ParameterType::ASSET_URI) {
        CHECK_ARGS_WITH_MESSAGE(env,
            CheckPhotoAlbumType(env, photoAlbum, targetAlbum) != nullptr, "checkAlbumType error");
        CHECK_ARGS_WITH_MESSAGE(env, ParseUriArray(env, asyncContext->argv[PARAM0], assetUriArray), "Fail get uri");
    } else {
        CHECK_COND_WITH_MESSAGE(env,
            CheckPhotoAlbumType(env, photoAlbum, targetAlbum) != nullptr, "checkAlbumType error");
        CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray), "Fail parse");
    }
    auto moveMap = changeRequest->GetMoveMap();
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        if (!CheckDuplicatedAssetArray(assetUriArray, iter->second)) {
            NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
                "The previous moveAssets operation has contained the same asset");
            return nullptr;
        }
    }
    changeRequest->RecordMoveAssets(assetUriArray, targetAlbum);

    if (PhotoAlbum::IsSmartPortraitPhotoAlbum(targetAlbum->GetPhotoAlbumType(),
        targetAlbum->GetPhotoAlbumSubType())) {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::SMART_MOVE_ASSETS);
        RETURN_NAPI_UNDEFINED(env);
    }

    if (parameterType == ParameterType::ASSET_URI) {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::MOVE_ASSETS_WITH_URI);
    } else {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::MOVE_ASSETS);
    }
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSMoveAssets(napi_env env, napi_callback_info info)
{
    return MediaAlbumChangeRequestNapi::JSMoveAssetsImplement(env, info, ParameterType::PHOTO_ASSET);
}

napi_value MediaAlbumChangeRequestNapi::JSMoveAssetsWithUri(napi_env env, napi_callback_info info)
{
    return MediaAlbumChangeRequestNapi::JSMoveAssetsImplement(env, info, ParameterType::ASSET_URI);
}

napi_value MediaAlbumChangeRequestNapi::JSRecoverAssetsImplement(napi_env env, napi_callback_info info,
    ParameterType parameterType)
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
    if (parameterType == ParameterType::ASSET_URI) {
        CHECK_ARGS_WITH_MESSAGE(env, ParseUriArray(env, asyncContext->argv[PARAM0], assetUriArray),
            "Failed to parse assets");
    } else {
        CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
            "Failed to parse assets");
    }
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToRecover_)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous recoverAssets operation has contained the same asset");
        return nullptr;
    }
    changeRequest->assetsToRecover_.insert(
        changeRequest->assetsToRecover_.end(), assetUriArray.begin(), assetUriArray.end());
    if (parameterType == ParameterType::ASSET_URI) {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::RECOVER_ASSETS_WITH_URI);
    } else {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::RECOVER_ASSETS);
    }
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSRecoverAssets(napi_env env, napi_callback_info info)
{
    return MediaAlbumChangeRequestNapi::JSRecoverAssetsImplement(env, info, ParameterType::PHOTO_ASSET);
}

napi_value MediaAlbumChangeRequestNapi::JSRecoverAssetsWithUri(napi_env env, napi_callback_info info)
{
    return MediaAlbumChangeRequestNapi::JSRecoverAssetsImplement(env, info, ParameterType::ASSET_URI);
}

napi_value MediaAlbumChangeRequestNapi::JSDeleteAssetsImplement(napi_env env, napi_callback_info info,
    ParameterType parameterType)
{
    NAPI_INFO_LOG("enter");
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
    if (parameterType == ParameterType::ASSET_URI) {
        CHECK_ARGS_WITH_MESSAGE(env, ParseUriArray(env, asyncContext->argv[PARAM0], assetUriArray),
            "Failed to parse assets");
    } else {
        CHECK_COND_WITH_MESSAGE(env, ParseAssetArray(env, asyncContext->argv[PARAM0], assetUriArray),
            "Failed to parse assets");
    }
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToDelete_)) {
        NapiError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous deleteAssets operation has contained the same asset");
        return nullptr;
    }
    changeRequest->assetsToDelete_.insert(
        changeRequest->assetsToDelete_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->userId_ = photoAlbum->GetUserId();
    if (parameterType == ParameterType::ASSET_URI) {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::DELETE_ASSETS_WITH_URI);
    } else {
        changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::DELETE_ASSETS);
    }
    RETURN_NAPI_UNDEFINED(env);
    NAPI_INFO_LOG("end");
}

napi_value MediaAlbumChangeRequestNapi::JSDeleteAssets(napi_env env, napi_callback_info info)
{
    return MediaAlbumChangeRequestNapi::JSDeleteAssetsImplement(env, info, ParameterType::PHOTO_ASSET);
}

napi_value MediaAlbumChangeRequestNapi::JSDeleteAssetsWithUri(napi_env env, napi_callback_info info)
{
    return MediaAlbumChangeRequestNapi::JSDeleteAssetsImplement(env, info, ParameterType::ASSET_URI);
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
    auto type = photoAlbum->GetPhotoAlbumType();
    auto subtype = photoAlbum->GetPhotoAlbumSubType();
    CHECK_COND_WITH_MESSAGE(env, PhotoAlbum::IsSmartPortraitPhotoAlbum(type, subtype) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(type, subtype) || PhotoAlbum::IsSmartClassifyAlbum(type, subtype) ||
        PhotoAlbum::IsHighlightAlbum(type, subtype) ||
        PhotoAlbum::IsPetAlbum(type, subtype),
        "Only portrait, highlight, pet, group photo and classify album can dismiss asset");

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
        ((PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(),
            photoAlbum->GetPhotoAlbumSubType())) &&
        (PhotoAlbum::IsSmartPortraitPhotoAlbum(targetAlbum->GetPhotoAlbumType(),
            targetAlbum->GetPhotoAlbumSubType()))) ||
        ((PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) &&
        (PhotoAlbum::IsPetAlbum(targetAlbum->GetPhotoAlbumType(), targetAlbum->GetPhotoAlbumSubType()))),
        "Only portrait or only pet album can merge");
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
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only portrait album can set album display level");
    photoAlbum->SetDisplayLevel(displayLevel);
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_DISPLAY_LEVEL);
    NAPI_INFO_LOG("Set display level change request, album id: %{public}d, display level: %{public}d",
        photoAlbum->GetAlbumId(), displayLevel);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL);
    return result;
}

const std::unordered_set<std::string> ALLOWED_RELATIONSHIPS = {
    "me", "son", "daughter", "wife", "husband", "father", "mother",
    "colleague", "friend", "classmate", "best_friend_female",
    "boyfriend", "girlfriend", "family", "maternal_grandfather", "maternal_grandmother",
    "paternal_grandfather", "paternal_grandmother", "older_brother", "relative",
    "older_sister", "younger_brother", "younger_sister", "other", ""
};

static void GetIsMeInfo(int32_t &isMe, bool &isMeExist, const int32_t albumId)
{
    Uri queryUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    std::vector<std::string> fetchColumn{IS_ME};
    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(ALBUM_ID, albumId);
    int errCode = 0;
    auto isMeResultSet = UserFileClient::Query(queryUri, queryPredicates, fetchColumn, errCode);
    if (errCode == E_SUCCESS && isMeResultSet != nullptr && isMeResultSet->GoToFirstRow() == NativeRdb::E_OK) {
        isMe = get<int32_t>(ResultSetUtils::GetValFromColumn(IS_ME, isMeResultSet, TYPE_INT32));
    }

    if (isMe == 0) {
        // check has is me
        std::vector<std::string> fetchColumnNew{IS_ME, ALBUM_ID};
        DataShare::DataSharePredicates queryPredicatesNew;
        queryPredicatesNew.EqualTo(IS_ME, QUERY_IS_ME_VALUE);
        int errCodeNew = 0;
        auto isMeExistRet = UserFileClient::Query(queryUri, queryPredicatesNew, fetchColumnNew, errCodeNew);
        if (errCodeNew == E_SUCCESS && isMeExistRet != nullptr && isMeExistRet->GoToFirstRow() == NativeRdb::E_OK) {
            NAPI_INFO_LOG("Has is me album on portrait album list");
            isMeExist = true;
        }
    }
}

napi_value MediaAlbumChangeRequestNapi::JSSetRelationship(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("JSSetRelationship called");
    MediaLibraryTracer tracer;
    tracer.Start("JSSetRelationship");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    // Create undefined return value in case of failure
    napi_value undefinedObject = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedObject));

    // Create async context
    std::unique_ptr<MediaAlbumChangeRequestAsyncContext> asyncContext =
        std::make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, undefinedObject, "Failed to create asyncContext");
    CHECK_COND_WITH_ERR_MESSAGE(env,
        (MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_TWO) == napi_ok),
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "Failed to get object info");
    // Retrieve the album instance
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_ERR_MESSAGE(env, photoAlbum != nullptr,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "Failed to get photo album instance");
    CHECK_COND_WITH_ERR_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        NAPI_INVALID_PARAMETER_ERROR, "Only portrait album can set relationship");
    
    // Check is me
    int32_t isMe = 0;
    bool isMeExist = false;
    GetIsMeInfo(isMe, isMeExist, photoAlbum->GetAlbumId());

    // Parse the relationship string from the first argument
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, asyncContext->argv[PARAM0], &valueType));
    CHECK_COND_WITH_ERR_MESSAGE(env, (valueType == napi_string),
        NAPI_INVALID_PARAMETER_ERROR, "Relationship must be a string");
    char relationshipCStr[256] = {0};
    size_t strLen = 0;
    CHECK_ARGS(env, napi_get_value_string_utf8(env, asyncContext->argv[PARAM0],
        relationshipCStr, sizeof(relationshipCStr), &strLen), JS_ERR_PARAMETER_INVALID);
    std::string relationship(relationshipCStr, strLen);
    CHECK_COND_WITH_ERR_MESSAGE(env, ALLOWED_RELATIONSHIPS.find(relationship) != ALLOWED_RELATIONSHIPS.end(),
        NAPI_INVALID_PARAMETER_ERROR, "Invalid relationship value provided");
    NAPI_INFO_LOG("Set relationship is: %{public}s, albumId: %{public}d",
        relationship.c_str(), photoAlbum->GetAlbumId());

    // 清空relationship和更改is_me场景：
    // 1、存在“我”相册且“我”相册不是当前相册，将其它相册人物关系设置为“我”
    // 2、当前相册为“我”相册并将已设置的“我”设置为其他关系
    if ((isMeExist && relationship == "me" && isMe == 0) || (isMe == 1 && relationship != "me")) {
        NAPI_INFO_LOG("Clear relationship");
        bool result = MediaLibraryNapiUtils::ClearAllRelationship();
        if (!result) {
            NAPI_WARN_LOG("Failed to clear all relationship after set is me.");
        }
    }
    // Store relationship in context via objectInfo
    asyncContext->objectInfo->relationship_ = relationship;
    asyncContext->objectInfo->isMe_ = isMe;
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_RELATIONSHIP);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSSetOrderPosition(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetOrderPosition");

    // make undefined
    napi_value undefinedObject = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedObject));

    // make async context, if error then return undefined
    unique_ptr<MediaAlbumChangeRequestAsyncContext> asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, undefinedObject, "Failed to create asyncContext");
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_TWO, ARGS_TWO) == napi_ok,
        "Failed to get object info");

    // get this album, check it is an analysis album
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "Failed to get photo album instance");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsAnalysisAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only analysis album can set asset order positions");

    // get assets, check duplicated
    vector<string> assetIdArray;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseAssetIdArray(env, asyncContext->argv[PARAM0], assetIdArray),
        "Failed to parse assets");
    CHECK_COND_WITH_MESSAGE(
        env, assetIdArray.size() > 0, "The setOrderPosition operation needs at least one asset id");
    NAPI_INFO_LOG("setOrderPosition id length: %{public}d", static_cast<int>(assetIdArray.size()));
    std::set<std::string> idSet(assetIdArray.begin(), assetIdArray.end());
    CHECK_COND_WITH_MESSAGE(
        env, assetIdArray.size() == idSet.size(), "The setOrderPosition operation has same assets");

    // get order positions, check duplicated
    vector<int32_t> orderPositionArray;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseIntegerArray(env, asyncContext->argv[PARAM1], orderPositionArray),
        "Failed to parse order positions");
    NAPI_INFO_LOG("setOrderPosition order length: %{public}d", static_cast<int>(orderPositionArray.size()));
    std::set<int32_t> positionSet(orderPositionArray.begin(), orderPositionArray.end());
    CHECK_COND_WITH_MESSAGE(env,
        orderPositionArray.size() == positionSet.size(),
        "The setOrderPosition operation has same order positions");
    CHECK_COND_WITH_MESSAGE(env,
        assetIdArray.size() == orderPositionArray.size(),
        "The setOrderPosition operation needs same assets and order positions size");

    // store pairs
    auto &pairs = asyncContext->objectInfo->idOrderPositionPairs_;
    for (size_t i = 0; i < assetIdArray.size(); i++) {
        pairs.emplace_back(assetIdArray[i], orderPositionArray[i]);
    }

    // add task to queue
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_ORDER_POSITION);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSDismiss(napi_env env, napi_callback_info info)
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
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only group photo can be dismissed");
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::DISMISS);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
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
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only user source, highlight, pet, smart portrait album and group photo can set album name");
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
    auto subtype = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        (PhotoAlbum::IsSystemAlbum(photoAlbum->GetPhotoAlbumType()) &&
        !PhotoAlbum::IsHiddenAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) ||
        PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "can't set album cover of album subtype:" + to_string(subtype));
    photoAlbum->SetCoverUri(coverUri);
    photoAlbum->SetCoverUriSource(static_cast<int32_t>(CoverUriSource::MANUAL_CLOUD_COVER));
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_COVER_URI);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAlbumChangeRequestNapi::JSResetCoverUri(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO) == napi_ok,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "Failed to get object info");

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_ERR_MESSAGE(env, photoAlbum != nullptr,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "photoAlbum is null");
    auto subtype = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    CHECK_COND_WITH_ERR_MESSAGE(env,
        !(photoAlbum->GetHiddenOnly()) &&
        (PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSystemAlbum(photoAlbum->GetPhotoAlbumType()) ||
        PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())),
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
        "can't reset album cover of album subtype:" + to_string(subtype));
    photoAlbum->SetCoverUriSource(static_cast<int32_t>(CoverUriSource::DEFAULT_COVER));
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::RESET_COVER_URI);
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

napi_value MediaAlbumChangeRequestNapi::JSSetHighlightAttribute(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightAttribute");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_value undefinedObject = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedObject));
    unique_ptr<MediaAlbumChangeRequestAsyncContext> asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, undefinedObject, "Failed to create asyncContext");

    int32_t highlightAlbumChangeAttribute = HighlightAlbumChangeAttribute::INVALID_CHANGE_ATTRIBUTE;
    string value;
    CHECK_ARGS_WITH_MEG(env, MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext,
        highlightAlbumChangeAttribute) == napi_ok, MEDIA_LIBRARY_PARAM_ERROR,
        "Failed to parse highlightAlbumChangeAttribute");
    
    CHECK_ARGS_WITH_MEG(env, MediaLibraryNapiUtils::GetParamStringWithLength(env, asyncContext->argv[1],
        MAX_HIGHLIGHT_ATTRIBUTE_VALUE_LEN, value) == napi_ok, MEDIA_LIBRARY_PARAM_ERROR, "Failed to parse value arg");
    
    CHECK_ARGS_WITH_MEG(env, asyncContext->argc == ARGS_TWO, MEDIA_LIBRARY_PARAM_ERROR,
        "Number of args is invalid");
    
    CHECK_ARGS_WITH_MEG(env, !value.empty(), MEDIA_LIBRARY_PARAM_ERROR, "value is empty");

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_ARGS_WITH_MEG(env, photoAlbum != nullptr, MEDIA_LIBRARY_PARAM_ERROR, "photoAlbum is null");
    CHECK_ARGS_WITH_MEG(env,
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        MEDIA_LIBRARY_PARAM_ERROR, "Only highlight album can SetHighlightAttribute");
    
    asyncContext->objectInfo->highlightAlbumChangeAttributePair_ = make_pair(highlightAlbumChangeAttribute, value);
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_HIGHLIGHT_ATTRIBUTE);
    RETURN_NAPI_UNDEFINED(env);
}

static napi_value ParseArgsSetUploadStatus(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext>& context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    constexpr size_t jsArgs = ARGS_THREE;
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, jsArgs, jsArgs) == napi_ok, JS_E_PARAM_INVALID,
        "Failed to get args");
    CHECK_COND(env, MediaAlbumChangeRequestNapi::InitUserFileClient(env, info), JS_E_INNER_FAIL);

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, context->argv[PARAM0], &valueType) != napi_ok || valueType != napi_object) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "invalid context");
        return nullptr;
    }

    vector<napi_value> napiValues;
    valueType = napi_undefined;
    CHECK_COND_WITH_ERR_MESSAGE(env, MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[PARAM1], napiValues),
        JS_E_PARAM_INVALID, "Failed to get array");
    CHECK_COND_WITH_ERR_MESSAGE(env, !napiValues.empty(), JS_E_PARAM_INVALID, "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_E_PARAM_INVALID);
    CHECK_COND_WITH_ERR_MESSAGE(env, valueType == napi_object, JS_E_PARAM_INVALID, "Argument must be array of album");
 
    constexpr size_t sizeOfArray = 0;
    constexpr size_t kBatchSetMaxNumber = 500;
    if (napiValues.size() > kBatchSetMaxNumber || napiValues.size() == sizeOfArray) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "size of albums is 0 or over 500.");
        return nullptr;
    }
    for (const auto& napiValue : napiValues) {
        PhotoAlbumNapi* obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void**>(&obj)), JS_E_PARAM_INVALID);
        CHECK_COND_WITH_ERR_MESSAGE(env, obj != nullptr, JS_E_PARAM_INVALID, "Failed to get album napi object");

        if (PhotoAlbum::IsUserPhotoAlbum(obj->GetPhotoAlbumType(), obj->GetPhotoAlbumSubType()) ||
            PhotoAlbum::IsSourceAlbum(obj->GetPhotoAlbumType(), obj->GetPhotoAlbumSubType())) {
            context->deleteIds.push_back(to_string(obj->GetAlbumId()));
            context->photoAlbumTypes.push_back(static_cast<int32_t>(obj->GetPhotoAlbumType()));
            context->photoAlbumSubtypes.push_back(static_cast<int32_t>(obj->GetPhotoAlbumSubType()));
        }
    }

    CHECK_COND_WITH_ERR_MESSAGE(env, MediaLibraryNapiUtils::GetParamBool(env, context->argv[PARAM2],
        context->allowUpload) == napi_ok, JS_E_PARAM_INVALID, "Failed to get allowUpload");
    RETURN_NAPI_TRUE(env);
}

static void SetUploadStatusExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetUploadStatusExecute");

    auto *context = static_cast<MediaAlbumChangeRequestAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_IF_EQUAL(!context->deleteIds.empty(), "deleteIds is empty");

    NAPI_INFO_LOG("enter SetUploadStatusExecute, allowUpload: %{public}d.", context->allowUpload);
    ChangeRequestSetUploadStatusReqBody reqBody;
    reqBody.allowUpload = static_cast<int>(context->allowUpload);
    reqBody.albumIds = context->deleteIds;
    reqBody.photoAlbumTypes = context->photoAlbumTypes;
    reqBody.photoAlbumSubtypes = context->photoAlbumSubtypes;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_UPLOAD_STATUS);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED || ret == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(ret);
        } else {
            context->error = JS_E_INNER_FAIL;
        }
        NAPI_ERR_LOG("Failed to set uploadStatus, err: %{public}d", ret);
        return;
    }
}

static void SetUploadStatusCallback(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetUploadStatusCallback");
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

napi_value MediaAlbumChangeRequestNapi::JSSetUploadStatus(napi_env env, napi_callback_info info)
{
    NAPI_DEBUG_LOG("enter JSSetUploadStatus.");
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    if (!ParseArgsSetUploadStatus(env, info, asyncContext)) {
        NAPI_ERR_LOG("Failed to parse args");
        return nullptr;
    }
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ChangeRequestSetUploadStatus",
        SetUploadStatusExecute, SetUploadStatusCallback);
}

static bool CreateAlbumExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateAlbumExecute");

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
    if (album == nullptr) {
        NAPI_ERR_LOG("Album is null");
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
    MediaLibraryTracer tracer;
    tracer.Start("AddAssetsExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    int32_t albumId = photoAlbum->GetAlbumId();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS);
    ChangeRequestAddAssetsReqBody reqBody;
    ChangeRequestAddAssetsRespBody respBody;
    reqBody.albumId = albumId;
    int32_t ret = 0;

    for (const auto& asset : changeRequest->GetAddAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    reqBody.isHiddenOnly = photoAlbum->GetHiddenOnly();
    if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIGHLIGHT ||
        photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        NAPI_INFO_LOG("Add Assets on highlight album");
        reqBody.isHighlight = true;
        ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    } else {
        reqBody.isHighlight = false;
        NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
        ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    }
    changeRequest->ClearAddAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to add assets into album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }
    if (reqBody.isHighlight) {
        NAPI_INFO_LOG("Add %{public}d asset(s) into highlight album %{public}d", ret, albumId);
        return true;
    }

    NAPI_INFO_LOG("Add %{public}d asset(s) into album %{public}d", ret, albumId);
    photoAlbum->SetVideoCount(respBody.videoCount);
    photoAlbum->SetImageCount(respBody.imageCount);
    photoAlbum->SetCount(respBody.albumCount);
    return true;
}

static bool RemoveAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("RemoveAssetsExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    int32_t albumId = photoAlbum->GetAlbumId();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS);
    ChangeRequestRemoveAssetsReqBody reqBody;
    ChangeRequestRemoveAssetsRespBody respBody;
    reqBody.albumId = albumId;
    reqBody.isHiddenOnly = photoAlbum->GetHiddenOnly();
    for (const auto& asset : changeRequest->GetRemoveAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    changeRequest->ClearRemoveAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to remove assets from album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }

    NAPI_INFO_LOG("Remove %{public}d asset(s) from album %{public}d", ret, albumId);
    photoAlbum->SetVideoCount(respBody.videoCount);
    photoAlbum->SetImageCount(respBody.imageCount);
    photoAlbum->SetCount(respBody.albumCount);
    return true;
}

static bool MoveAssetsExecuteWithUri(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MoveAssetsExecuteWithUri");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    int32_t albumId = photoAlbum->GetAlbumId();
    auto moveMap = changeRequest->GetMoveMap();
    changeRequest->ClearMoveMap();

    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        auto targetPhotoAlbum = iter->first;
        int32_t targetAlbumId = targetPhotoAlbum->GetAlbumId();
        vector<string> moveAssetArray = iter->second;
        // Move into target album.
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
        predicates.And()->In(PhotoColumn::MEDIA_ID, moveAssetArray);

        DataShare::DataShareValuesBucket valuesBuckets;
        valuesBuckets.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, targetAlbumId);
        string uri = PAH_BATCH_UPDATE_OWNER_ALBUM_ID;
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri moveAssetsUri(uri);
        int ret = UserFileClient::Update(moveAssetsUri, predicates, valuesBuckets);
        if (ret < 0) {
            context.SaveError(ret);
            NAPI_ERR_LOG("Failed to move assets into album %{public}d, err: %{public}d", targetAlbumId, ret);
            return false;
        }
        NAPI_INFO_LOG("Move %{public}d asset(s) into album %{public}d", ret, targetAlbumId);
        FetchNewCount(context, targetPhotoAlbum);
    }
    FetchNewCount(context, photoAlbum);
    return true;
}

static bool MoveAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MoveAssetsExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    int32_t albumId = photoAlbum->GetAlbumId();
    auto moveMap = changeRequest->GetMoveMap();
    changeRequest->ClearMoveMap();
    ChangeRequestMoveAssetsReqBody reqBody;
    ChangeRequestMoveAssetsRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS);
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        auto targetPhotoAlbum = iter->first;
        bool isTargetHiddenOnly = targetPhotoAlbum->GetHiddenOnly();
        int32_t targetAlbumId = targetPhotoAlbum->GetAlbumId();
        vector<string> moveAssetArray = iter->second;
        // Move into target album.
        reqBody.albumId = albumId;
        reqBody.targetAlbumId = targetAlbumId;
        for (const auto& asset : moveAssetArray) {
            reqBody.assets.push_back(asset);
        }
        int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
        if (ret < 0) {
            context.SaveError(ret);
            NAPI_ERR_LOG("Failed to move assets into album %{public}d, err: %{public}d", targetAlbumId, ret);
            return false;
        }
        NAPI_INFO_LOG("Move %{public}d asset(s) into album %{public}d", ret, targetAlbumId);
        targetPhotoAlbum->SetVideoCount(isTargetHiddenOnly ? -1 : respBody.targetAlbumVideoCount);
        targetPhotoAlbum->SetImageCount(isTargetHiddenOnly ? -1 : respBody.targetAlbumImageCount);
        targetPhotoAlbum->SetCount(respBody.targetAlbumCount);
    }
    bool isHiddenOnly = photoAlbum->GetHiddenOnly();
    photoAlbum->SetVideoCount(isHiddenOnly ? -1 : respBody.albumVideoCount);
    photoAlbum->SetImageCount(isHiddenOnly ? -1 : respBody.albumImageCount);
    photoAlbum->SetCount(respBody.albumCount);
    NAPI_INFO_LOG("origin album video count: %{public}d, image count: %{public}d, count: %{public}d",
        respBody.albumVideoCount, respBody.albumImageCount, respBody.albumCount);
    return true;
}

static bool SmartMoveAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    NAPI_INFO_LOG("MediaAlbumChangeRequestNapi::SmartMoveAssetsExecute start ");
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is null");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    int32_t albumId = photoAlbum->GetAlbumId();
    auto moveMap = changeRequest->GetMoveMap();
    changeRequest->ClearMoveMap();
    ChangeRequestMoveAssetsReqBody reqBody;
    ChangeRequestMoveAssetsRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SMART_MOVE_ASSETS);
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        auto targetPhotoAlbum = iter->first;
        CHECK_COND_RET(targetPhotoAlbum != nullptr, false, "targetPhotoAlbum is nullptr");
        bool isTargetHiddenOnly = targetPhotoAlbum->GetHiddenOnly();
        int32_t targetAlbumId = targetPhotoAlbum->GetAlbumId();
        vector<string> moveAssetArray = iter->second;
        reqBody.albumId = albumId;
        reqBody.targetAlbumId = targetAlbumId;
        for (const auto& asset : moveAssetArray) {
            reqBody.assets.push_back(asset);
        }
        int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
        if (ret < 0) {
            context.SaveError(ret);
            NAPI_ERR_LOG("Failed to move assets into album %{public}d, err: %{public}d", targetAlbumId, ret);
            return false;
        }
        NAPI_ERR_LOG("Move %{public}d asset(s) into album %{public}d", ret, targetAlbumId);
        targetPhotoAlbum->SetVideoCount(isTargetHiddenOnly ? -1 : respBody.targetAlbumVideoCount);
        targetPhotoAlbum->SetImageCount(isTargetHiddenOnly ? -1 : respBody.targetAlbumImageCount);
        targetPhotoAlbum->SetCount(respBody.targetAlbumCount);
    }
    bool isHiddenOnly = photoAlbum->GetHiddenOnly();
    photoAlbum->SetVideoCount(isHiddenOnly ? -1 : respBody.albumVideoCount);
    photoAlbum->SetImageCount(isHiddenOnly ? -1 : respBody.albumImageCount);
    photoAlbum->SetCount(respBody.albumCount);
    NAPI_ERR_LOG("origin album video count: %{public}d, image count: %{public}d, count: %{public}d",
        respBody.albumVideoCount, respBody.albumImageCount, respBody.albumCount);
    return true;
}


static bool RecoverAssetsExecuteWithUri(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("RecoverAssetsExecute");

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

static bool RecoverAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("RecoverAssetsExecute");
    CHECK_COND_RET(context.objectInfo != nullptr, false, "context.objectInfo is nullptr");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS);
    ChangeRequestRecoverAssetsReqBody reqBody;

    for (const auto& asset : context.objectInfo->GetRecoverAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
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

static bool DeleteAssetsExecuteWithUri(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteAssetsExecuteWithUri");

    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, context.objectInfo->GetDeleteAssetArray());
    predicates.GreaterThan(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, 0);

    Uri deleteAssetsUri(PAH_DELETE_PHOTOS);
    int ret = UserFileClient::Update(deleteAssetsUri, predicates, valuesBucket, context.objectInfo->GetUserId());
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

static bool DeleteAssetsExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteAssetsExecute");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS);
    ChangeRequestDeleteAssetsReqBody reqBody;
    CHECK_COND_RET(context.objectInfo != nullptr, false, "context.objectInfo is nullptr");
    for (const auto& asset : context.objectInfo->GetDeleteAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().SetUserId(context.objectInfo->GetUserId()).Call(businessCode, reqBody);

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
    MediaLibraryTracer tracer;
    tracer.Start("OrderAlbumExecute");

    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    auto referenceAlum = context.objectInfo->GetReferencePhotoAlbumInstance();
    int32_t referenceAlbumId = -1;
    if (referenceAlum != nullptr) {
        referenceAlbumId = referenceAlum->GetAlbumId();
    }

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE);
    ChangeRequestPlaceBeforeReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.referenceAlbumId = referenceAlbumId;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        NAPI_ERR_LOG("Failed to order albums err: %{public}d", result);
        return false;
    }
    return true;
}

static bool SetRelationshipExecute(MediaAlbumChangeRequestAsyncContext &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetRelationshipExecute");
    NAPI_INFO_LOG("SetRelationshipExecute start");
    // Retrieve the photo album instance
    const auto &photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        NAPI_ERR_LOG("Photo album is null in SetRelationshipExecute");
        context.SaveError(JS_INNER_FAIL);
        return false;
    }

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_RELATIONSHIP);
    ChangeRequestSetRelationshipReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.relationship = context.objectInfo->GetRelationship();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    reqBody.isMe = context.objectInfo->GetIsMe();
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        NAPI_ERR_LOG("Failed to set relationship, error: %{public}d", result);
        context.SaveError(result);
        return false;
    }
    return true;
}

static bool SetOrderPositionExecute(MediaAlbumChangeRequestAsyncContext &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetOrderPositionExecute");

    const auto &photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    const auto &pairs = context.objectInfo->GetIdOrderPositionPairs();
    ChangeRequestSetOrderPositionReqBody reqBody;
    vector<string> ids;
    ids.reserve(pairs.size());
    stringstream orderString;
    const string mapTable = ANALYSIS_PHOTO_MAP_TABLE;
    orderString << "CASE " << mapTable << "." << MAP_ASSET << " ";
    for (const auto &[assetId, orderPosition] : pairs) {
        orderString << "WHEN " << assetId << " THEN " << orderPosition << " ";
        ids.push_back(assetId);
    }
    orderString << "END";

    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.orderString  = orderString.str();
    reqBody.assetIds = ids;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        NAPI_ERR_LOG("Failed to set order position err: %{public}d", result);
        return false;
    }
    return true;
}

static bool DismissAssetExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("DismissAssetExecute");

    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS);
    ChangeRequestDismissAssetsReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.photoAlbumSubType = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    for (const auto& asset : context.objectInfo->GetDismissAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to dismiss asset err: %{public}d", ret);
        return false;
    }
    context.objectInfo->ClearDismissAssetArray();
    return true;
}

static bool MergeAlbumExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MergeAlbumExecute");

    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    auto targetAlum = context.objectInfo->GetTargetPhotoAlbumInstance();
    int32_t targetAlbumId = -1;
    if (targetAlum != nullptr) {
        targetAlbumId = targetAlum->GetAlbumId();
    }
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM);
    ChangeRequestMergeAlbumReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.targetAlbumId = targetAlbumId;
    int result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        NAPI_ERR_LOG("Failed to merge albums err: %{public}d", result);
        return false;
    }
    return true;
}

static bool SetAlbumNameExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetAlbumNameExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetAlbumNameReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.albumName = photoAlbum->GetAlbumName();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        NAPI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetCoverUriExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetCoverUriExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.coverUri = photoAlbum->GetCoverUri();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        NAPI_ERR_LOG("Failed to set cover uri, err: %{public}d", changedRows);
        context.error = JS_INNER_FAIL;
        return false;
    }
    return true;
}

static bool SetDisplayLevelExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetDisplayLevelExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetDisplayLevelReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.displayLevel = photoAlbum->GetDisplayLevel();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_DISPLAY_LEVEL);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        NAPI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetIsMeExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetIsMeExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    int32_t isMe = 0;
    bool isMeExist = false;
    GetIsMeInfo(isMe, isMeExist, photoAlbum->GetAlbumId());
    // 存在“我”相册且“我”相册不是当前相册
    if (isMeExist && isMe == 0) {
        bool result = MediaLibraryNapiUtils::ClearAllRelationship();
        if (!result) {
            NAPI_WARN_LOG("Failed to clear all relationship after set is me");
        }
    }
    ChangeRequestSetIsMeReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.isMe = VALUE_IS_ME;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_IS_ME);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        NAPI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool DismissExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("DismissExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequesDismissReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.isRemoved = VALUE_IS_REMOVED;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        NAPI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool ResetCoverUriExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("ResetCoverUriExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequesDismissReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        NAPI_ERR_LOG("Failed to reset cover uri, err: %{public}d", changedRows);
        context.error = JS_E_INNER_FAIL;
        return false;
    }
    return true;
}

static bool SetHighlightAttributeExecute(MediaAlbumChangeRequestAsyncContext &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetHighlightAttributeExecute");

    const auto &photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    const auto pair = context.objectInfo->GetHighlightAlbumChangeAttributePair();
    CHECK_COND_RET(!pair.second.empty(), false, "value is empty");
    ChangeRequestSetHighlightAttributeReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.highlightAlbumChangeAttribute = pair.first;
    reqBody.highlightAlbumChangeAttributeValue = pair.second;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_HIGHLIGHT_ATTRIBUTE);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        NAPI_ERR_LOG("Failed to SetHighlightAttributeExecute err: %{public}d", result);
        return false;
    }
    return true;
}

static const unordered_map<AlbumChangeOperation,
    bool (*)(MediaAlbumChangeRequestAsyncContext&)> PROPERTY_EXECUTE_MAP = {
    { AlbumChangeOperation::SET_ALBUM_NAME, SetAlbumNameExecute },
    { AlbumChangeOperation::SET_COVER_URI, SetCoverUriExecute },
    { AlbumChangeOperation::SET_DISPLAY_LEVEL, SetDisplayLevelExecute },
    { AlbumChangeOperation::SET_IS_ME, SetIsMeExecute },
    { AlbumChangeOperation::DISMISS, DismissExecute },
    { AlbumChangeOperation::RESET_COVER_URI, ResetCoverUriExecute },
};

static bool SetAlbumPropertyExecute(
    MediaAlbumChangeRequestAsyncContext& context, const AlbumChangeOperation changeOperation)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetAlbumPropertyExecute");

    // In the scenario of creation, the new name will be applied when the album is created.
    if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME &&
        context.albumChangeOperations.front() == AlbumChangeOperation::CREATE_ALBUM) {
        return true;
    }

    bool valid = false;
    auto iter = PROPERTY_EXECUTE_MAP.find(changeOperation);
    if (iter != PROPERTY_EXECUTE_MAP.end()) {
        valid = iter->second(context);
    }
    if (!valid) {
        return valid;
    }
    return true;
}

static bool CreateAnalysisAlbumExecute(MediaAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateAnalysisAlbumExecute");

    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");

    CreateAnalysisAlbumReqBody reqBody;
    CreateAnalysisAlbumRespBody respBody;
    reqBody.albumName = photoAlbum->GetAlbumName();
    reqBody.subType = photoAlbum->GetPhotoAlbumSubType();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CREATE_ANALYSIS_ALBUM);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to create album, ret: %{public}d", ret);
        return false;
    }

    if (respBody.albumId == -1) {
        context.SaveError(-EEXIST);
        NAPI_ERR_LOG("Album exists");
        return false;
    }

    if (respBody.albumId < 0) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to create album, ret: %{public}d", ret);
        return false;
    }
    photoAlbum->SetAlbumId(respBody.albumId);
    photoAlbum->SetAlbumUri(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX + to_string(respBody.albumId));

    return true;
}

static const unordered_map<AlbumChangeOperation, bool (*)(MediaAlbumChangeRequestAsyncContext&)> EXECUTE_MAP = {
    { AlbumChangeOperation::CREATE_ALBUM, CreateAlbumExecute },
    { AlbumChangeOperation::ADD_ASSETS, AddAssetsExecute },
    { AlbumChangeOperation::REMOVE_ASSETS, RemoveAssetsExecute },
    { AlbumChangeOperation::MOVE_ASSETS, MoveAssetsExecute },
    { AlbumChangeOperation::MOVE_ASSETS_WITH_URI, MoveAssetsExecuteWithUri },
    { AlbumChangeOperation::RECOVER_ASSETS, RecoverAssetsExecute },
    { AlbumChangeOperation::RECOVER_ASSETS_WITH_URI, RecoverAssetsExecuteWithUri },
    { AlbumChangeOperation::DELETE_ASSETS, DeleteAssetsExecute },
    { AlbumChangeOperation::DELETE_ASSETS_WITH_URI, DeleteAssetsExecuteWithUri },
    { AlbumChangeOperation::ORDER_ALBUM, OrderAlbumExecute },
    { AlbumChangeOperation::MERGE_ALBUM, MergeAlbumExecute },
    { AlbumChangeOperation::DISMISS_ASSET, DismissAssetExecute },
    { AlbumChangeOperation::SET_ORDER_POSITION, SetOrderPositionExecute },
    { AlbumChangeOperation::SET_RELATIONSHIP, SetRelationshipExecute },
    { AlbumChangeOperation::SET_HIGHLIGHT_ATTRIBUTE, SetHighlightAttributeExecute },
    { AlbumChangeOperation::SMART_MOVE_ASSETS, SmartMoveAssetsExecute },
    { AlbumChangeOperation::CREATE_ANALYSIS_ALBUM, CreateAnalysisAlbumExecute },
};

static void ApplyAlbumChangeRequestExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ApplyAlbumChangeRequestExecute");

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
                   changeOperation == AlbumChangeOperation::SET_DISPLAY_LEVEL ||
                   changeOperation == AlbumChangeOperation::DISMISS ||
                   changeOperation == AlbumChangeOperation::RESET_COVER_URI) {
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
    MediaLibraryTracer tracer;
    tracer.Start("ApplyAlbumChangeRequestCompleteCallback");

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

static napi_value ParseArgsCreateAnalysisAlbum(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext>& context)
{
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, context, ARGS_THREE, ARGS_THREE) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get args");
    CHECK_COND(env, MediaAlbumChangeRequestNapi::InitUserFileClient(env, info), JS_E_PARAM_INVALID);

    string albumName;
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM1], albumName) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get album name");
    CHECK_COND_WITH_ERR_MESSAGE(env, MediaFileUtils::CheckAlbumName(albumName) == E_OK,
        JS_E_PARAM_INVALID, "Invalid album name");
    CHECK_COND_WITH_ERR_MESSAGE(env, MediaLibraryNapiUtils::GetInt32(env, context->argv[PARAM2],
        context->photoAlbumSubType) == napi_ok, JS_E_PARAM_INVALID, "Failed to get subtype");
    CHECK_COND_WITH_ERR_MESSAGE(env, context->photoAlbumSubType == static_cast<int32_t>(PORTRAIT), JS_E_PARAM_INVALID,
        "Invalid subtype");

    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    RETURN_NAPI_TRUE(env);
}

napi_value MediaAlbumChangeRequestNapi::JSCreateAnalysisAlbumRequest(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND(env, asyncContext != nullptr, JS_E_PARAM_INVALID);
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsCreateAnalysisAlbum(env, info, asyncContext),
        JS_E_PARAM_INVALID, "Failed to parse args");

    bool isValid = false;
    string albumName = asyncContext->valuesBucket.Get(PhotoAlbumColumns::ALBUM_NAME, isValid);
    auto photoAlbum = make_unique<PhotoAlbum>();
    CHECK_COND_WITH_ERR_MESSAGE(env, photoAlbum != nullptr, JS_E_PARAM_INVALID, "The photoAlbum is nullptr");
    photoAlbum->SetAlbumName(albumName);
    photoAlbum->SetPhotoAlbumType(SMART);
    photoAlbum->SetPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(asyncContext->photoAlbumSubType));
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
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::CREATE_ANALYSIS_ALBUM);
    return instance;
}
} // namespace OHOS::Media