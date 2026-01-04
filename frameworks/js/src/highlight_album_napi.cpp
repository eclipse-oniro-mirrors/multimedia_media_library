/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "HighlightAlbumNapi"

#include "highlight_album_napi.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_tracer.h"
#include "media_album_change_request_napi.h"
#include "result_set_utils.h"
#include "story_album_column.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "vision_photo_map_column.h"
#include "highlight_column.h"
#include "album_operation_uri.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "delete_highlight_albums_vo.h"
#include "set_subtitle_vo.h"
#include "set_highlight_user_action_data_vo.h"
#include "get_order_position_vo.h"
#include "get_highlight_album_info_vo.h"
#include "get_relationship_vo.h"
#include "query_result_vo.h"
#include "photo_album_napi.h"

using namespace std;

namespace OHOS::Media {
static const string HIGHLIGHT_ALBUM_CLASS = "HighlightAlbum";
static const string ANALYSIS_ALBUM_CLASS = "AnalysisAlbum";
thread_local napi_ref HighlightAlbumNapi::constructor_ = nullptr;
thread_local napi_ref HighlightAlbumNapi::analysisAlbumConstructor_ = nullptr;
static const int32_t NAPI_INVALID_PARAMETER_ERROR = 23800151;
static const int32_t MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR = 23800301;

using CompleteCallback = napi_async_complete_callback;

HighlightAlbumNapi::HighlightAlbumNapi() : highlightmEnv_(nullptr) {}

HighlightAlbumNapi::~HighlightAlbumNapi() = default;

napi_value HighlightAlbumNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = HIGHLIGHT_ALBUM_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_FUNCTION("getHighlightAlbumInfo", JSGetHighlightAlbumInfo),
            DECLARE_NAPI_FUNCTION("setHighlightUserActionData", JSSetHighlightUserActionData),
            DECLARE_NAPI_FUNCTION("getHighlightResource", JSGetHighlightResource),
            DECLARE_NAPI_FUNCTION("getOrderPosition", JSGetOrderPosition),
            DECLARE_NAPI_FUNCTION("setSubTitle", JSSetHighlightSubtitle),
            DECLARE_NAPI_STATIC_FUNCTION("deleteHighlightAlbums", JSDeleteHighlightAlbums),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

napi_value HighlightAlbumNapi::AnalysisAlbumInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = ANALYSIS_ALBUM_CLASS,
        .ref = &analysisAlbumConstructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_FUNCTION("getOrderPosition", JSGetOrderPosition),
            DECLARE_NAPI_FUNCTION("getRelationship", JSAnalysisAlbumGetRelationship),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

static napi_value ParseHighlightAlbum(napi_env env, napi_value arg, shared_ptr<PhotoAlbum>& photoAlbum)
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

napi_value HighlightAlbumNapi::Constructor(napi_env env, napi_callback_info info)
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
    CHECK_COND_WITH_MESSAGE(env, ParseHighlightAlbum(env, argv[PARAM0], photoAlbum), "Failed to parse album");

    unique_ptr<HighlightAlbumNapi> obj = make_unique<HighlightAlbumNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    obj->highlightAlbumPtr = photoAlbum;
    obj->highlightmEnv_ = env;
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), HighlightAlbumNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void HighlightAlbumNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* hightlihgtAlbum = reinterpret_cast<HighlightAlbumNapi*>(nativeObject);
    if (hightlihgtAlbum != nullptr) {
        delete hightlihgtAlbum;
        hightlihgtAlbum = nullptr;
    }
}

const map<int32_t, struct HighlightAlbumInfo> GetHighlightAlbumInfoMap()
{
    static const map<int32_t, struct HighlightAlbumInfo> HIGHLIGHT_ALBUM_INFO_MAP = {
        { COVER_INFO, { PAH_QUERY_HIGHLIGHT_COVER, { ID, HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
            AI_ALBUM_ID, SUB_TITLE, CLUSTER_TYPE, CLUSTER_SUB_TYPE,
            CLUSTER_CONDITION, MIN_DATE_ADDED, MAX_DATE_ADDED, GENERATE_TIME, HIGHLIGHT_VERSION,
            REMARKS, HIGHLIGHT_STATUS, RATIO, BACKGROUND, FOREGROUND, WORDART, IS_COVERED, COLOR,
            RADIUS, SATURATION, BRIGHTNESS, BACKGROUND_COLOR_TYPE, SHADOW_LEVEL, TITLE_SCALE_X,
            TITLE_SCALE_Y, TITLE_RECT_WIDTH, TITLE_RECT_HEIGHT, BACKGROUND_SCALE_X, BACKGROUND_SCALE_Y,
            BACKGROUND_RECT_WIDTH, BACKGROUND_RECT_HEIGHT, LAYOUT_INDEX, COVER_ALGO_VERSION, COVER_KEY, COVER_STATUS,
            HIGHLIGHT_IS_MUTED, HIGHLIGHT_IS_FAVORITE, HIGHLIGHT_THEME,
            HIGHLIGHT_PIN_TIME, HIGHLIGHT_USE_SUBTITLE } } },
        { PLAY_INFO, { PAH_QUERY_HIGHLIGHT_PLAY, { ID, HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
            MUSIC, FILTER, HIGHLIGHT_PLAY_INFO, IS_CHOSEN, PLAY_INFO_VERSION, PLAY_INFO_ID } } },
        { ALBUM_INFO, { PAH_QUERY_HIGHLIGHT_ALBUM, { ID, HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
            HIGHLIGHT_STATUS, HIGHLIGHT_IS_VIEWED, HIGHLIGHT_NOTIFICATION_TIME } } },
    };
    return HIGHLIGHT_ALBUM_INFO_MAP;
}

static const map<int32_t, std::string> HIGHLIGHT_USER_ACTION_MAP = {
    { INSERTED_PIC_COUNT, HIGHLIGHT_INSERT_PIC_COUNT },
    { REMOVED_PIC_COUNT, HIGHLIGHT_REMOVE_PIC_COUNT },
    { SHARED_SCREENSHOT_COUNT, HIGHLIGHT_SHARE_SCREENSHOT_COUNT },
    { SHARED_COVER_COUNT, HIGHLIGHT_SHARE_COVER_COUNT },
    { RENAMED_COUNT, HIGHLIGHT_RENAME_COUNT },
    { CHANGED_COVER_COUNT, HIGHLIGHT_CHANGE_COVER_COUNT },
    { RENDER_VIEWED_TIMES, HIGHLIGHT_RENDER_VIEWED_TIMES },
    { RENDER_VIEWED_DURATION, HIGHLIGHT_RENDER_VIEWED_DURATION },
    { ART_LAYOUT_VIEWED_TIMES, HIGHLIGHT_ART_LAYOUT_VIEWED_TIMES },
    { ART_LAYOUT_VIEWED_DURATION, HIGHLIGHT_ART_LAYOUT_VIEWED_DURATION },
};

static void JSGetHighlightAlbumInfoExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetHighlightAlbumInfoExecute");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    std::vector<std::string> fetchColumn;
    DataShare::DataSharePredicates predicates;
    auto infoMap = GetHighlightAlbumInfoMap();
    if (infoMap.find(context->highlightAlbumInfoType) != infoMap.end()) {
        fetchColumn = infoMap.at(context->highlightAlbumInfoType).fetchColumn;
    } else {
        NAPI_ERR_LOG("Invalid highlightAlbumInfoType");
        return;
    }
    int errCode = 0;
    GetHighlightAlbumReqBody reqBody;
    reqBody.highlightAlbumInfoType = context->highlightAlbumInfoType;
    reqBody.albumId = context->albumId;
    reqBody.subType = static_cast<int32_t>(context->subType);
    QueryResultRespBody respBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_HIGHLIGHT_ALBUM_INFO), reqBody, respBody);
    shared_ptr<DataShare::DataShareResultSet> resultSet = respBody.resultSet;
    if (resultSet != nullptr) {
        context->highlightAlbumInfo = MediaLibraryNapiUtils::ParseResultSet2JsonStr(resultSet, fetchColumn);
    }
}

static void JSGetHighlightAlbumInfoCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetHighlightAlbumInfoCompleteCallback");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, context->highlightAlbumInfo.c_str(),
            NAPI_AUTO_LENGTH, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static void JSSetHighlightUserActionDataExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightUserActionDataExecute");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    int32_t albumId = context->objectInfo->GetPhotoAlbumInstance()->GetAlbumId();
    SetHighlightUserActionDataReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.userActionType = context->highlightUserActionType;
    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    reqBody.albumType = static_cast<int32_t>(photoAlbum->GetPhotoAlbumType());
    reqBody.albumSubType = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HIGH_LIGHT_USER_ACTION_DATA);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Submit cloud enhancement tasks failed, err: %{public}d", ret);
        return;
    }
}

static void JSSetHighlightUserActionDataCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightUserActionDataCompleteCallback");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
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
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void JSSetHighlightSubtitleExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightSubtitleExecute");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    int albumId = context->objectInfo->GetPhotoAlbumInstance()->GetAlbumId();
    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    SetSubtitleReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.subtitle = context->subtitle;
    reqBody.albumType = static_cast<int32_t>(photoAlbum->GetPhotoAlbumType());
    reqBody.albumSubType = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUBTITLE);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Failed to set highlight subtitle, err: %{public}d", ret);
        return;
    }
}

static void JSSetHighlightSubtitleCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightSubtitleCompleteCallback");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
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
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static int32_t GetFdForArrayBuffer(std::string uriStr)
{
    int32_t fd = 0;
    Uri uri(uriStr);
    fd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_READONLY);
    if (fd == E_ERR) {
        NAPI_ERR_LOG("Open highlight cover file failed, error: %{public}d", errno);
        return E_HAS_FS_ERROR;
    } else if (fd < 0) {
        NAPI_ERR_LOG("Open highlight cover file failed due to OpenFile failure");
        return fd;
    }
    return fd;
}

static void JSGetHighlightResourceExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetHighlightResourceExecute");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    if (context->resourceUri.find(MEDIA_DATA_DB_HIGHLIGHT) == string::npos) {
        NAPI_ERR_LOG("Invalid highlight resource uri");
        return;
    }
    
    int32_t fd = GetFdForArrayBuffer(context->resourceUri);
    if (fd < 0) {
        return;
    }
    UniqueFd uniqueFd(fd);
    off_t fileLen = lseek(uniqueFd.Get(), 0, SEEK_END);
    if (fileLen < 0) {
        NAPI_ERR_LOG("Failed to get highlight cover file length, error: %{public}d", errno);
        return;
    }
    off_t ret = lseek(uniqueFd.Get(), 0, SEEK_SET);
    if (ret < 0) {
        NAPI_ERR_LOG("Failed to reset highlight cover file offset, error: %{public}d", errno);
        return;
    }
    void* arrayBufferData = nullptr;
    napi_value arrayBuffer;
    if (napi_create_arraybuffer(env, fileLen, &arrayBufferData, &arrayBuffer) != napi_ok) {
        NAPI_ERR_LOG("failed to create napi arraybuffer");
        return;
    }

    ssize_t readBytes = read(uniqueFd.Get(), arrayBufferData, fileLen);
    if (readBytes != fileLen) {
        NAPI_ERR_LOG("read file failed, read bytes is %{public}zu,""actual length is %{public}" PRId64
            ",error: %{public}d", readBytes, fileLen, errno);
        return;
    }
    context->napiArrayBuffer = arrayBuffer;
}

static void JSGetHighlightResourceCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetHighlightResourceCompleteCallback");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        jsContext->data = context->napiArrayBuffer;
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void JSGetOrderPositionExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetOrderPositionExecute");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();

    GetOrderPositionRespBody respBody;
    GetOrderPositionReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    reqBody.assetIdArray = context->assetIdArray;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ORDER_POSITION);

    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret == E_OK) {
        context->orderPositionArray.clear();
        context->orderPositionArray = respBody.orderPositionArray;
    }
    NAPI_INFO_LOG("GetOrderPosition: orderPositionArray size: %{public}d",
                  static_cast<int>(context->orderPositionArray.size()));
}

static void JSGetOrderPositionCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetOrderPositionCompleteCallback");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    size_t positionSize = context->orderPositionArray.size();
    size_t assetSize = context->assetIdArray.size();
    if (positionSize != assetSize) {
        NAPI_ERR_LOG("GetOrderPosition failed, position size: %{public}d, asset size: %{public}d",
                     static_cast<int>(positionSize), static_cast<int>(assetSize));
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    } else {
        napi_value jsArray = nullptr;
        napi_create_array_with_length(env, positionSize, &jsArray);
        for (size_t i = 0; i < positionSize; i++) {
            napi_value element;
            napi_create_int32(env, context->orderPositionArray[i], &element);
            napi_set_element(env, jsArray, i, element);
        }
        jsContext->data = jsArray;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred,
                                                   context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value HighlightAlbumNapi::JSGetOrderPosition(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetOrderPosition");

    // make undefined
    napi_value undefinedObject = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedObject));

    // make async context, if error then return undefined
    unique_ptr<HighlightAlbumNapiAsyncContext> asyncContext = make_unique<HighlightAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, undefinedObject, "asyncContext context is null");
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    // get this album, check it is an analysis album
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "Failed to get photo album instance");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsAnalysisAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only analysis album can get asset order positions");

    // get assets, check duplicated
    vector<string> assetIdArray;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseAssetIdArray(env, asyncContext->argv[PARAM0], assetIdArray),
        "Failed to parse assets");
    NAPI_INFO_LOG("GetOrderPosition: get assets id size: %{public}d", static_cast<int>(assetIdArray.size()));
    CHECK_COND_WITH_MESSAGE(
        env, assetIdArray.size() > 0, "The getOrderPosition operation needs at least one asset id");
    std::set<std::string> idSet(assetIdArray.begin(), assetIdArray.end());
    CHECK_COND_WITH_MESSAGE(
        env, assetIdArray.size() == idSet.size(), "The getOrderPosition operation has same assets");
    asyncContext->assetIdArray = std::move(assetIdArray);

    // make async task
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env, asyncContext, "JSGetOrderPosition", JSGetOrderPositionExecute, JSGetOrderPositionCompleteCallback);
}

napi_value HighlightAlbumNapi::JSGetHighlightAlbumInfo(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetHighlightAlbumInfo");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<HighlightAlbumNapiAsyncContext> asyncContext = make_unique<HighlightAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext,
        asyncContext->highlightAlbumInfoType), JS_ERR_PARAMETER_INVALID);

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only and smart highlight album can get highlight album info");

    asyncContext->albumId = photoAlbum->GetAlbumId();
    asyncContext->subType = photoAlbum->GetPhotoAlbumSubType();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetHighlightAlbumInfo",
        JSGetHighlightAlbumInfoExecute, JSGetHighlightAlbumInfoCompleteCallback);
}

napi_value HighlightAlbumNapi::JSSetHighlightUserActionData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightUserActionData");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<HighlightAlbumNapiAsyncContext> asyncContext = make_unique<HighlightAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");

    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext,
        asyncContext->highlightUserActionType), JS_ERR_PARAMETER_INVALID);
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, asyncContext->argv[1], asyncContext->actionData));

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only and smart highlight album can set user action info");

    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSSetHighlightUserActionData",
        JSSetHighlightUserActionDataExecute, JSSetHighlightUserActionDataCompleteCallback);
}

napi_value HighlightAlbumNapi::JSGetHighlightResource(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetHighlightResource");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<HighlightAlbumNapiAsyncContext> asyncContext = make_unique<HighlightAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");

    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->resourceUri),
        JS_ERR_PARAMETER_INVALID);

    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only and smart highlight album can set user action info");

    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetHighlightResource",
        JSGetHighlightResourceExecute, JSGetHighlightResourceCompleteCallback);
}

napi_value HighlightAlbumNapi::JSSetHighlightSubtitle(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetHighlightSubtitle");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<HighlightAlbumNapiAsyncContext> asyncContext = make_unique<HighlightAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->subtitle),
        OHOS_INVALID_PARAM_CODE);

    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckHighlightSubtitle(asyncContext->subtitle) == E_OK,
        "Invalid highlight subtitle");
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_MESSAGE(env, photoAlbum != nullptr, "photoAlbum is null");
    CHECK_COND_WITH_MESSAGE(env,
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        "Only highlight album can set highlight sub title");
    
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSSetHighlightSubtitle",
        JSSetHighlightSubtitleExecute, JSSetHighlightSubtitleCompleteCallback);
}

static void DeleteHighlightAlbumsCompleteCallback(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteHighlightAlbumsCompleteCallback");
    int32_t deleteAlbumFailed = 1;
    auto* context = static_cast<MediaAlbumChangeRequestAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, E_SUCCESS, &jsContext->data);
        jsContext->status = true;
    } else {
        napi_create_int32(env, deleteAlbumFailed, &jsContext->data);
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static void DeleteHighlightAlbumsExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteHighlightAlbumsExecute");
    NAPI_INFO_LOG("Start delete highlight album(s)");

    auto* context = static_cast<MediaAlbumChangeRequestAsyncContext*>(data);
    DeleteHighLightAlbumsReqBody reqBody;
    reqBody.albumIds = context->deleteIds;
    reqBody.photoAlbumTypes = context->photoAlbumTypes;
    reqBody.photoAlbumSubtypes = context->photoAlbumSubtypes;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_HIGH_LIGHT_ALBUMS);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Failed to delete highlight albums, err: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("Delete highlight album(s): %{public}d", ret);
}

static napi_value ParseArgsDeleteHighlightAlbums(
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
 
    for (const auto& napiValue : napiValues) {
        PhotoAlbumNapi* obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void**>(&obj)), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, obj != nullptr, "Failed to get album napi object");
        CHECK_COND_WITH_MESSAGE(env,
            PhotoAlbum::IsHighlightAlbum(obj->GetPhotoAlbumType(), obj->GetPhotoAlbumSubType()),
            "Only highlight album can be deleted");
        context->deleteIds.push_back(to_string(obj->GetAlbumId()));
        context->photoAlbumTypes.push_back(static_cast<int32_t>(obj->GetPhotoAlbumType()));
        context->photoAlbumSubtypes.push_back(static_cast<int32_t>(obj->GetPhotoAlbumSubType()));
    }
    RETURN_NAPI_TRUE(env);
}

napi_value HighlightAlbumNapi::JSDeleteHighlightAlbums(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, ParseArgsDeleteHighlightAlbums(env, info, asyncContext),
        "Failed to parse highlight args");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env, asyncContext, "ChangeRequestDeleteHighlightAlbums",
        DeleteHighlightAlbumsExecute, DeleteHighlightAlbumsCompleteCallback);
}

shared_ptr<PhotoAlbum> HighlightAlbumNapi::GetPhotoAlbumInstance() const
{
    return highlightAlbumPtr;
}

static void JSGetRelationshipExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetRelationshipExecute");
    NAPI_INFO_LOG("JSGetRelationshipExecute start");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectInfo, "objectInfo is null");
    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    CHECK_NULL_PTR_RETURN_VOID(photoAlbum, "photoAlbum is null");

    GetRelationshipReqBody reqBody;
    GetRelationshipRespBody respBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_RELATIONSHIP);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (result < 0) {
        NAPI_ERR_LOG("Failed to get relationship, error: %{public}d", result);
        context->error = JS_INNER_FAIL;
        return;
    }
    context->relationship = respBody.relationship;

    NAPI_INFO_LOG("GetRelationship: albumId: %{public}d, relationship is: %{public}s",
        reqBody.albumId, context->relationship.c_str());
}

static void JSGetRelationshipCompleteCallback(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetRelationshipCompleteCallback");

    auto *context = static_cast<HighlightAlbumNapiAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env,
            napi_create_string_utf8(env, context->relationship.c_str(), NAPI_AUTO_LENGTH, &jsContext->data),
            JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value HighlightAlbumNapi::JSAnalysisAlbumGetRelationship(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSAnalysisAlbumGetRelationship");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    // make undefined
    napi_value undefinedObject = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedObject));

    // Create async context
    unique_ptr<HighlightAlbumNapiAsyncContext> asyncContext = make_unique<HighlightAlbumNapiAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, undefinedObject, "asyncContext context is null");

    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ZERO) == napi_ok,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "Failed to get object info");
    // Check album instance
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_ERR_MESSAGE(env, photoAlbum != nullptr,
        MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "photoAlbum is null");
    CHECK_COND_WITH_ERR_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        NAPI_INVALID_PARAMETER_ERROR, "Only portrait album can set relationship");
    
    // make assync task
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env, asyncContext, "JSGetRelationship", JSGetRelationshipExecute, JSGetRelationshipCompleteCallback);
}
} // namespace OHOS::Media