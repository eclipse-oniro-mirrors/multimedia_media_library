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

#define MLOG_TAG "MediaAssetsChangeRequestNapi"

#include "media_assets_change_request_napi.h"

#include <unordered_set>

#include "file_asset_napi.h"
#include "media_column.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_tracer.h"
#include "userfile_client.h"
#include "userfile_manager_types.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "modify_assets_vo.h"
#include "media_change_request_utils.h"

using namespace std;

namespace OHOS::Media {
static const string MEDIA_ASSETS_CHANGE_REQUEST_CLASS = "MediaAssetsChangeRequest";
thread_local napi_ref MediaAssetsChangeRequestNapi::constructor_ = nullptr;

constexpr int32_t YES = 1;
constexpr int32_t NO = 0;
constexpr int32_t USER_COMMENT_MAX_LEN = 420;

napi_value MediaAssetsChangeRequestNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_ASSETS_CHANGE_REQUEST_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_PROPERTY("comment", MediaChangeRequestUtils::CreateComment(env)),
            DECLARE_NAPI_FUNCTION("setFavorite", JSSetFavorite),
            DECLARE_NAPI_FUNCTION("setHidden", JSSetHidden),
            DECLARE_NAPI_FUNCTION("setUserComment", JSSetUserComment),
            DECLARE_NAPI_FUNCTION("setIsRecentShow", JSSetIsRecentShow),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

static napi_value GetAssetArray(napi_env env, napi_value arg, vector<shared_ptr<FileAsset>>& fileAssets)
{
    bool isArray = false;
    uint32_t len = 0;
    CHECK_ARGS(env, napi_is_array(env, arg, &isArray), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, isArray, "Failed to check array type");
    CHECK_ARGS(env, napi_get_array_length(env, arg, &len), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, len > 0, "Failed to check array length");
    for (uint32_t i = 0; i < len; i++) {
        napi_value asset = nullptr;
        CHECK_ARGS(env, napi_get_element(env, arg, i, &asset), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, asset != nullptr, "Failed to get asset element");

        FileAssetNapi* fileAssetNapi = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, asset, reinterpret_cast<void**>(&fileAssetNapi)), JS_INNER_FAIL);
        CHECK_COND_WITH_MESSAGE(env, fileAssetNapi != nullptr, "Failed to get FileAssetNapi object");

        auto fileAssetPtr = fileAssetNapi->GetFileAssetInstance();
        CHECK_COND_WITH_MESSAGE(env, fileAssetPtr != nullptr, "fileAsset is null");
        CHECK_COND_WITH_MESSAGE(env,
            fileAssetPtr->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER &&
                (fileAssetPtr->GetMediaType() == MEDIA_TYPE_IMAGE || fileAssetPtr->GetMediaType() == MEDIA_TYPE_VIDEO),
            "Unsupported type of fileAsset");
        fileAssets.push_back(fileAssetPtr);
    }
    RETURN_NAPI_TRUE(env);
}

napi_value MediaAssetsChangeRequestNapi::Constructor(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "The constructor can be called only by system apps");
        return nullptr;
    }

    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Failed to check new.target");

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");

    vector<shared_ptr<FileAsset>> fileAssets;
    CHECK_COND_WITH_MESSAGE(env, GetAssetArray(env, argv[PARAM0], fileAssets), "Failed to parse args");
    unique_ptr<MediaAssetsChangeRequestNapi> obj = make_unique<MediaAssetsChangeRequestNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    obj->fileAssets_ = fileAssets;
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), MediaAssetsChangeRequestNapi::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void MediaAssetsChangeRequestNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* assetsChangeRequest = reinterpret_cast<MediaAssetsChangeRequestNapi*>(nativeObject);
    if (assetsChangeRequest != nullptr) {
        delete assetsChangeRequest;
        assetsChangeRequest = nullptr;
    }
}

vector<string> MediaAssetsChangeRequestNapi::GetFileAssetUriArray() const
{
    vector<string> uriArray;
    uriArray.reserve(fileAssets_.size());
    for (const auto& fileAsset : fileAssets_) {
        uriArray.push_back(fileAsset->GetUri());
    }
    return uriArray;
}

void MediaAssetsChangeRequestNapi::GetFileAssetIds(std::vector<int32_t> &fileIds) const
{
    for (const auto& fileAsset : fileAssets_) {
        fileIds.push_back(fileAsset->GetId());
    }
}

bool MediaAssetsChangeRequestNapi::GetFavoriteStatus() const
{
    return isFavorite_;
}

bool MediaAssetsChangeRequestNapi::GetHiddenStatus() const
{
    return isHidden_;
}

string MediaAssetsChangeRequestNapi::GetUpdatedUserComment() const
{
    return userComment_;
}

bool MediaAssetsChangeRequestNapi::GetRecentShowStatus() const
{
    return isRecentShow_;
}

bool MediaAssetsChangeRequestNapi::CheckChangeOperations(napi_env env)
{
    if (assetsChangeOperations_.empty()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "None request to apply");
        return false;
    }

    if (fileAssets_.empty()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "fileAssets is empty");
        return false;
    }

    for (const auto& fileAsset : fileAssets_) {
        if (fileAsset == nullptr || fileAsset->GetId() <= 0 || fileAsset->GetUri().empty()) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid fileAsset to apply");
            return false;
        }
    }

    return true;
}

napi_value MediaAssetsChangeRequestNapi::JSSetFavorite(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetsChangeRequestAsyncContext>();
    bool isFavorite;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, isFavorite) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto changeRequest = asyncContext->objectInfo;
    changeRequest->isFavorite_ = isFavorite;
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        fileAsset->SetFavorite(isFavorite);
    }
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_FAVORITE);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetsChangeRequestNapi::JSSetHidden(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetsChangeRequestAsyncContext>();
    bool isHidden;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, isHidden) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto changeRequest = asyncContext->objectInfo;
    changeRequest->isHidden_ = isHidden;
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        fileAsset->SetHidden(isHidden);
    }
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_HIDDEN);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetsChangeRequestNapi::JSSetUserComment(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetsChangeRequestAsyncContext>();
    string userComment;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, userComment) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");
    CHECK_COND_WITH_MESSAGE(env, userComment.length() <= USER_COMMENT_MAX_LEN, "user comment too long");

    auto changeRequest = asyncContext->objectInfo;
    changeRequest->userComment_ = userComment;
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        fileAsset->SetUserComment(userComment);
    }
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_USER_COMMENT);
    RETURN_NAPI_UNDEFINED(env);
}

napi_value MediaAssetsChangeRequestNapi::JSSetIsRecentShow(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<MediaAssetsChangeRequestAsyncContext>();
    bool isRecentShow;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, isRecentShow) == napi_ok,
        "Failed to parse args");
    CHECK_COND_WITH_MESSAGE(env, asyncContext->argc == ARGS_ONE, "Number of args is invalid");

    auto changeRequest = asyncContext->objectInfo;
    changeRequest->isRecentShow_ = isRecentShow;
    for (const auto& fileAsset : changeRequest->fileAssets_) {
        fileAsset->SetRecentShow(isRecentShow);
    }
    changeRequest->assetsChangeOperations_.push_back(AssetsChangeOperation::BATCH_SET_RECENT_SHOW);
    RETURN_NAPI_UNDEFINED(env);
}

static bool CallSetAssetProperty(MediaAssetsChangeRequestAsyncContext& context,
    const AssetsChangeOperation& changeOperation)
{
    uint32_t businessCode = 0;
    ModifyAssetsReqBody reqBody;
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    switch (changeOperation) {
        case AssetsChangeOperation::BATCH_SET_FAVORITE:
            reqBody.favorite = changeRequest->GetFavoriteStatus() ? YES : NO;
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE);
            break;
        case AssetsChangeOperation::BATCH_SET_HIDDEN:
            reqBody.hiddenStatus = changeRequest->GetHiddenStatus() ? YES : NO;
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN);
            break;
        case AssetsChangeOperation::BATCH_SET_USER_COMMENT:
            reqBody.userComment = changeRequest->GetUpdatedUserComment();
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT);
            break;
        case AssetsChangeOperation::BATCH_SET_RECENT_SHOW:
            reqBody.recentShowStatus = changeRequest->GetRecentShowStatus() ? YES : NO;
            businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW);
            break;
        default:
            context.SaveError(E_FAIL);
            NAPI_ERR_LOG("Unsupported assets change operation: %{public}d", changeOperation);
            return false;
    }

    changeRequest->GetFileAssetIds(reqBody.fileIds);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", result);
        context.SaveError(result);
        return false;
    }
    return true;
}

static bool SetAssetsPropertyExecute(MediaAssetsChangeRequestAsyncContext& context,
    const AssetsChangeOperation& changeOperation, bool useCall)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetAssetsPropertyExecute");

    if (useCall) {
        return CallSetAssetProperty(context, changeOperation);
    }

    string uri;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto changeRequest = context.objectInfo;
    predicates.In(PhotoColumn::MEDIA_ID, changeRequest->GetFileAssetUriArray());
    switch (changeOperation) {
        case AssetsChangeOperation::BATCH_SET_FAVORITE:
            uri = PAH_BATCH_UPDATE_FAVORITE;
            valuesBucket.Put(PhotoColumn::MEDIA_IS_FAV, changeRequest->GetFavoriteStatus() ? YES : NO);
            NAPI_INFO_LOG("Batch set favorite: %{public}d", changeRequest->GetFavoriteStatus() ? YES : NO);
            break;
        case AssetsChangeOperation::BATCH_SET_HIDDEN:
            uri = PAH_HIDE_PHOTOS;
            valuesBucket.Put(PhotoColumn::MEDIA_HIDDEN, changeRequest->GetHiddenStatus() ? YES : NO);
            break;
        case AssetsChangeOperation::BATCH_SET_USER_COMMENT:
            uri = PAH_BATCH_UPDATE_USER_COMMENT;
            valuesBucket.Put(PhotoColumn::PHOTO_USER_COMMENT, changeRequest->GetUpdatedUserComment());
            break;
        case AssetsChangeOperation::BATCH_SET_RECENT_SHOW:
            uri = PAH_BATCH_UPDATE_RECENT_SHOW;
            valuesBucket.Put(PhotoColumn::PHOTO_IS_RECENT_SHOW, changeRequest->GetRecentShowStatus() ? YES : NO);
            break;
        default:
            context.SaveError(E_FAIL);
            NAPI_ERR_LOG("Unsupported assets change operation: %{public}d", changeOperation);
            return false;
    }

    NAPI_INFO_LOG("changeOperation:%{public}d, size:%{public}zu",
        changeOperation, changeRequest->GetFileAssetUriArray().size());
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetsUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAssetsUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context.SaveError(changedRows);
        NAPI_ERR_LOG("Failed to set property, operation: %{public}d, err: %{public}d", changeOperation, changedRows);
        return false;
    }
    return true;
}

static void ApplyAssetsChangeRequestExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ApplyAssetsChangeRequestExecute");

    auto* context = static_cast<MediaAssetsChangeRequestAsyncContext*>(data);
    unordered_set<AssetsChangeOperation> appliedOperations;
    for (const auto& changeOperation : context->assetsChangeOperations) {
        // Keep the final result(s) of each operation, and commit only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = SetAssetsPropertyExecute(*context, changeOperation, true);
        if (!valid) {
            NAPI_ERR_LOG("Failed to apply assets change request, operation: %{public}d", changeOperation);
            return;
        }
        appliedOperations.insert(changeOperation);
    }
}

static void ApplyAssetsChangeRequestCompleteCallback(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ApplyAssetsChangeRequestCompleteCallback");

    auto* context = static_cast<MediaAssetsChangeRequestAsyncContext*>(data);
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

napi_value MediaAssetsChangeRequestNapi::ApplyChanges(napi_env env, napi_callback_info info)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    auto asyncContext = make_unique<MediaAssetsChangeRequestAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, asyncContext, minArgs, maxArgs) == napi_ok,
        "Failed to get args");
    asyncContext->objectInfo = this;

    CHECK_COND_WITH_MESSAGE(env, CheckChangeOperations(env), "Failed to check assets change request operations");
    asyncContext->assetsChangeOperations = assetsChangeOperations_;
    assetsChangeOperations_.clear();
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ApplyMediaAssetsChangeRequest",
        ApplyAssetsChangeRequestExecute, ApplyAssetsChangeRequestCompleteCallback);
}
} // namespace OHOS::Media