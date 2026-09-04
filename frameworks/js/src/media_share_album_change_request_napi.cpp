/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version  2.0 (the "License");
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

#define MLOG_TAG "MediaShareAlbumChangeRequestNapi"

#include "media_share_album_change_request_napi.h"

#include <unordered_set>
#include "album_operation_uri.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "media_change_request_napi.h"
#include "media_file_utils.h"
#include "medialibrary_business_code.h"
#include "medialibrary_client_errno.h"
#include "media_library_error_code.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_tracer.h"
#include "napi_error.h"
#include "set_share_album_name_vo.h"
#include "delete_share_album_vo.h"
#include "add_share_member_vo.h"
#include "delete_share_member_vo.h"
#include "delete_member_share_album_vo.h"
#include "permission_utils.h"
#include "photo_album_napi.h"
#include "user_define_ipc_client.h"
#include "userfile_client.h"

using namespace std;
using namespace OHOS::Security::AccessToken;

namespace OHOS::Media {
static const string MEDIA_SHARE_ALBUM_CHANGE_REQUEST_CLASS = "MediaShareAlbumChangeRequest";

thread_local napi_ref MediaShareAlbumChangeRequestNapi::constructor_ = nullptr;

napi_value MediaShareAlbumChangeRequestNapi::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = { .name = MEDIA_SHARE_ALBUM_CHANGE_REQUEST_CLASS,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_FUNCTION("setShareAlbumName", JSSetShareAlbumName),
            DECLARE_NAPI_FUNCTION("addShareMember", JSAddShareMember),
            DECLARE_NAPI_FUNCTION("deleteShareMember", JSDeleteShareMember),
            DECLARE_NAPI_FUNCTION("updateShareMemberStatus", JSUpdateShareMemberStatus),
            DECLARE_NAPI_STATIC_FUNCTION("deleteShareAlbum", JSDeleteShareAlbums),
            DECLARE_NAPI_STATIC_FUNCTION("deleteMemberShareAlbum", JSDeleteMemberShareAlbum),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

static napi_value ParsePhotoAlbum(napi_env env, napi_value arg, shared_ptr<PhotoAlbum> &photoAlbum)
{
    napi_valuetype valueType;
    PhotoAlbumNapi* photoAlbumNapi;
    CHECK_ARGS(env, napi_typeof(env, arg, &valueType), JS_INNER_FAIL);
    CHECK_WITH_INT_ERR_MESSAGE(env, valueType == napi_object, OHOS_INVALID_PARAM_CODE, "Invalid argument type");
    CHECK_ARGS(env, napi_unwrap(env, arg, reinterpret_cast<void**>(&photoAlbumNapi)), JS_INNER_FAIL);
    CHECK_WITH_INT_ERR_MESSAGE(env, photoAlbumNapi != nullptr, OHOS_INVALID_PARAM_CODE,
        "Failed to get PhotoAlbumNapi object");

    auto photoAlbumPtr = photoAlbumNapi->GetPhotoAlbumInstance();
    CHECK_WITH_INT_ERR_MESSAGE(env, photoAlbumPtr != nullptr, OHOS_INVALID_PARAM_CODE, "photoAlbum is null");
    CHECK_WITH_INT_ERR_MESSAGE(env,
        photoAlbumPtr->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER &&
            PhotoAlbum::CheckPhotoAlbumType(photoAlbumPtr->GetPhotoAlbumType()) &&
            PhotoAlbum::CheckPhotoAlbumSubType(photoAlbumPtr->GetPhotoAlbumSubType()),
        OHOS_INVALID_PARAM_CODE, "Unsupported type of photoAlbum");
    photoAlbum = photoAlbumPtr;
    RETURN_NAPI_TRUE(env);
}

static bool ParseSharePhotoAlbum(napi_env env, napi_value value, shared_ptr<PhotoAlbum> &photoAlbum)
{
    if (!ParsePhotoAlbum(env, value, photoAlbum)) {
        return false;
    }
    if (!PhotoAlbum::IsShareAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        NAPI_ERR_LOG("The album is not a share album");
        return false;
    }
    return true;
}

napi_value MediaShareAlbumChangeRequestNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_INNER_FAIL);
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Failed to check new.target");

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    shared_ptr<PhotoAlbum> photoAlbum = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_WITH_INT_ERR_MESSAGE(env, argc == ARGS_ONE, OHOS_INVALID_PARAM_CODE, "Number of args is invalid");
    CHECK_WITH_INT_ERR_MESSAGE(env, ParsePhotoAlbum(env, argv[PARAM0], photoAlbum),
        OHOS_INVALID_PARAM_CODE, "Failed to parse album");

    unique_ptr<MediaShareAlbumChangeRequestNapi> obj = make_unique<MediaShareAlbumChangeRequestNapi>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    obj->photoAlbum_ = photoAlbum;
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), MediaShareAlbumChangeRequestNapi::Destructor,
            nullptr, nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void MediaShareAlbumChangeRequestNapi::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* albumChangeRequest = reinterpret_cast<MediaShareAlbumChangeRequestNapi*>(nativeObject);
    if (albumChangeRequest != nullptr) {
        delete albumChangeRequest;
        albumChangeRequest = nullptr;
    }
}

shared_ptr<PhotoAlbum> MediaShareAlbumChangeRequestNapi::GetPhotoAlbumInstance() const
{
    return photoAlbum_;
}

static bool CheckShareAlbumCallerPermission(const std::string &permission)
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (result != PermissionState::PERMISSION_GRANTED) {
        NAPI_ERR_LOG("Have no media permission: %{public}s", permission.c_str());
        return false;
    }
    return true;
}

static bool SetShareAlbumNameExecute(MediaShareAlbumChangeRequestAsyncContext &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetShareAlbumNameExecute");

    if (!context.hasValidAlbum) {
        NAPI_ERR_LOG("setShareAlbumName: album is invalid");
        return false;
    }

    SetShareAlbumNameReqBody reqBody;
    reqBody.albumId = context.albumId;
    reqBody.owner = context.shareOwnerInfo;
    reqBody.albumName = context.albumName;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_SHARE_ALBUM_NAME);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret != E_OK) {
        context.SaveError(ret);
        NAPI_ERR_LOG("Failed to set share album name, ret: %{public}d", ret);
        return false;
    }
    NAPI_INFO_LOG("SetShareAlbumName done, albumId=%{public}d", context.albumId);
    return true;
}

static void DeleteShareAlbumsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteShareAlbumsExecute");
    auto *context = static_cast<MediaShareAlbumChangeRequestAsyncContext *>(data);
    if (context->owner.empty() || context->deleteIds.empty()) {
        NAPI_ERR_LOG("deleteShareAlbums: owner or deleteIds is empty");
        context->SaveError(MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
        return;
    }
    DeleteShareAlbumReqBody reqBody;
    reqBody.owner = context->owner;
    reqBody.albumIds = context->deleteIds;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_SHARE_PHOTO_ALBUMS);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret != E_OK) {
        context->SaveError(ret);
        NAPI_ERR_LOG("deleteShareAlbums failed, ret=%{public}d", ret);
        return;
    }
}

static bool AddShareMemberExecute(MediaShareAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("AddShareMemberExecute");
 
    CHECK_COND_RET(context.hasValidAlbum, false, "album is invalid");
 
    AddShareMemberReqBody reqBody;
    reqBody.albumId = context.albumId;
    reqBody.owner = context.memberOwner;
    NAPI_INFO_LOG("AddShareMemberExecute: owner=%{public}s", context.memberOwner.c_str());
    reqBody.member = context.member;
    NAPI_INFO_LOG("AddShareMemberExecute: member=%{public}s", context.member.c_str());
    reqBody.status = context.memberStatus;
    NAPI_INFO_LOG("AddShareMemberExecute: status=%{public}d", context.memberStatus);
 
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_ADD_SHARE_MEMBER);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret != E_OK) {
        context.SaveError(ret);
        NAPI_ERR_LOG("AddShareMemberExecute failed, err: %{public}d", ret);
        return false;
    }
    return true;
}
 
static bool UpdateShareMemberStatusExecute(MediaShareAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateShareMemberStatusExecute");
 
    CHECK_COND_RET(context.hasValidAlbum, false, "album is invalid");
 
    UpdateShareMemberStatusReqBody reqBody;
    reqBody.albumId = context.albumId;
    reqBody.owner = context.memberOwner;
    NAPI_INFO_LOG("UpdateShareMemberStatusExecute: owner=%{public}s", context.memberOwner.c_str());
    reqBody.member = context.member;
    NAPI_INFO_LOG("UpdateShareMemberStatusExecute: member=%{public}s", context.member.c_str());
    reqBody.status = context.memberStatus;
    NAPI_INFO_LOG("UpdateShareMemberStatusExecute: status=%{public}d", context.memberStatus);
 
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_UPDATE_SHARE_MEMBER_STATUS);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret != E_OK) {
        context.SaveError(ret);
        NAPI_ERR_LOG("UpdateShareMemberStatusExecute failed, err: %{public}d", ret);
        return false;
    }
    return true;
}
 
static bool DeleteShareMemberExecute(MediaShareAlbumChangeRequestAsyncContext& context)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteShareMemberExecute");
 
    CHECK_COND_RET(context.hasValidAlbum, false, "album is invalid");
 
    DeleteShareMemberReqBody reqBody;
    reqBody.albumId = context.albumId;
    reqBody.owner = context.memberOwner;
    NAPI_INFO_LOG("DeleteShareMemberExecute: owner=%{public}s", context.memberOwner.c_str());
    reqBody.member = context.member;
    NAPI_INFO_LOG("DeleteShareMemberExecute: member=%{public}s", context.member.c_str());
 
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_SHARE_MEMBER);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret != E_OK) {
        context.SaveError(ret);
        NAPI_ERR_LOG("DeleteShareMemberExecute failed, err: %{public}d", ret);
        return false;
    }
    return true;
}

// ===================== ApplyChanges mode =====================
using ShareAlbumExecFunc = bool (*)(MediaShareAlbumChangeRequestAsyncContext&);
static const unordered_map<ShareAlbumChangeOperation, ShareAlbumExecFunc> SHARE_EXEC_MAP = {
    { ShareAlbumChangeOperation::SET_SHARE_ALBUM_NAME, SetShareAlbumNameExecute },
    { ShareAlbumChangeOperation::ADD_SHARE_MEMBER, AddShareMemberExecute },
    { ShareAlbumChangeOperation::UPDATE_SHARE_MEMBER_STATUS, UpdateShareMemberStatusExecute },
    { ShareAlbumChangeOperation::DELETE_SHARE_MEMBER, DeleteShareMemberExecute },
};

static void ApplyShareAlbumChangeRequestExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ApplyShareAlbumChangeRequestExecute");

    auto *context = static_cast<MediaShareAlbumChangeRequestAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");

    unordered_set<ShareAlbumChangeOperation> appliedOperations;
    for (const auto &changeOperation : context->albumChangeOperations) {
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = false;
        auto iter = SHARE_EXEC_MAP.find(changeOperation);
        if (iter != SHARE_EXEC_MAP.end()) {
            valid = iter->second(*context);
        } else {
            NAPI_ERR_LOG("Invalid share album change operation: %{public}d",
                static_cast<int32_t>(changeOperation));
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
        }

        if (!valid) {
            NAPI_ERR_LOG("Failed to apply share album change request, operation: %{public}d",
                static_cast<int32_t>(changeOperation));
            return;
        }
        appliedOperations.insert(changeOperation);
    }
}

static void ApplyShareAlbumChangeRequestCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ApplyShareAlbumChangeRequestCompleteCallback");

    auto *context = static_cast<MediaShareAlbumChangeRequestAsyncContext*>(data);
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
    MediaLibraryNapiUtils::DeleteAsyncContextWithRef(env, context);
    delete context;
}

bool MediaShareAlbumChangeRequestNapi::CheckChangeOperations(napi_env env)
{
    if (albumChangeOperations_.empty()) {
        NAPI_ERR_LOG("No change operations to apply");
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "No change operations to apply");
        return false;
    }
    return true;
}

napi_value MediaShareAlbumChangeRequestNapi::ApplyChanges(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("MediaShareAlbumChangeRequestNapi::ApplyChanges start");
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    auto asyncContext = make_unique<MediaShareAlbumChangeRequestAsyncContext>();
    CHECK_WITH_INT_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextGetArgs(env, info, asyncContext, minArgs, maxArgs) == napi_ok,
        OHOS_INVALID_PARAM_CODE, "Failed to get args");
    asyncContext->shareAlbumObjectInfo = this;
    CHECK_WITH_INT_ERR_MESSAGE(env, napi_create_reference(env, asyncContext->argv[PARAM0], NAPI_INIT_REF_COUNT,
        &asyncContext->objectInfoRef) == napi_ok, OHOS_INVALID_PARAM_CODE, "Failed to create objectInfo reference");
    CHECK_WITH_INT_ERR_MESSAGE(env, CheckChangeOperations(env), OHOS_INVALID_PARAM_CODE,
        "Failed to check share album change request operations");
    asyncContext->albumChangeOperations = albumChangeOperations_;
    albumChangeOperations_.clear();
    auto photoAlbum = GetPhotoAlbumInstance();
    if (photoAlbum != nullptr) {
        asyncContext->albumId = photoAlbum->GetAlbumId();
        asyncContext->hasValidAlbum = true;
    }
    asyncContext->shareOwnerInfo = shareOwnerInfo_;
    asyncContext->albumName = albumName_;
    asyncContext->memberOwner = shareMemberOwner_;
    asyncContext->member = shareMemberMember_;
    asyncContext->memberStatus = shareMemberStatus_;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ApplyShareAlbumChangeRequest",
        ApplyShareAlbumChangeRequestExecute, ApplyShareAlbumChangeRequestCompleteCallback);
}

static napi_value ParseArgsSetShareAlbumName(napi_env env, napi_value argv[],
    SetShareAlbumNameParam &param, MediaShareAlbumChangeRequestNapi *changeRequest)
{
    NAPI_INFO_LOG("enter ParseArgsSetShareAlbumName");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowErrorWithIntCode(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can only be called by system apps");
        return nullptr;
    }
    if (changeRequest == nullptr) {
        NAPI_ERR_LOG("setShareAlbumName: changeRequest is null");
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "changeRequest is null");
        return nullptr;
    }
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    if (!PhotoAlbum::IsShareAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        NAPI_ERR_LOG("setShareAlbumName: the album is not a share album");
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "the album is not a share album");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM0], param.owner) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get owner");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], param.albumName) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get album name");
        return nullptr;
    }
    if (MediaFileUtils::CheckAlbumName(param.albumName) != E_OK) {
        NAPI_ERR_LOG("setShareAlbumName: invalid album name");
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "invalid album name");
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}

napi_value MediaShareAlbumChangeRequestNapi::JSSetShareAlbumName(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetShareAlbumName");
    NAPI_INFO_LOG("enter JSSetShareAlbumName");
    napi_value argv[ARGS_TWO] = { nullptr };
    size_t argc = ARGS_TWO;
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    if (argc != ARGS_TWO) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "The number of parameters is incorrect");
        return nullptr;
    }
    MediaShareAlbumChangeRequestNapi *changeRequest = nullptr;
    napi_unwrap(env, thisVar, reinterpret_cast<void **>(&changeRequest));
    SetShareAlbumNameParam param;
    CHECK_PARAMETER_WITH_MESSAGE(env, ParseArgsSetShareAlbumName(env, argv, param, changeRequest) != nullptr,
        "Failed to parse args");
    changeRequest->shareOwnerInfo_ = param.owner;
    changeRequest->albumName_ = param.albumName;
    changeRequest->albumChangeOperations_.push_back(ShareAlbumChangeOperation::SET_SHARE_ALBUM_NAME);
    RETURN_NAPI_UNDEFINED(env);
}

static napi_value ParseArgsDeleteShareAlbums(napi_env env, napi_value argv[], DeleteShareAlbumParam &param)
{
    NAPI_INFO_LOG("enter ParseArgsDeleteShareAlbums");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowErrorWithIntCode(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return nullptr;
    }

    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], param.owner) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
            "fail to get owner");
        return nullptr;
    }
    vector<napi_value> albumArray;
    if (MediaLibraryNapiUtils::GetNapiValueArray(env, argv[PARAM2], albumArray)) {
        for (auto &album : albumArray) {
            shared_ptr<PhotoAlbum> photoAlbum = nullptr;
            if (!ParseSharePhotoAlbum(env, album, photoAlbum)) {
                NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "Invalid share album");
                return nullptr;
            }
            if (!PhotoAlbum::IsShareAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
                NAPI_ERR_LOG("deleteShareAlbum: album is not a share album");
                NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
                    "album is not a share album");
                return nullptr;
            }
            param.deleteIds.push_back(photoAlbum->GetAlbumId());
        }
    }
    RETURN_NAPI_TRUE(env);
}

static void DeleteShareAlbumsCompleteCallback(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<MediaShareAlbumChangeRequestAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error, true);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value MediaShareAlbumChangeRequestNapi::JSDeleteShareAlbums(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteShareAlbums");
    NAPI_INFO_LOG("enter JSDeleteShareAlbums");
    napi_value argv[ARGS_THREE] = { nullptr };
    size_t argc = ARGS_THREE;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), JS_INNER_FAIL);
    if (argc != ARGS_THREE) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "The number of parameters is incorrect");
        return nullptr;
    }
 
    napi_valuetype ctxType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &ctxType);
    CHECK_WITH_INT_ERR_MESSAGE(env, ctxType == napi_object,
        MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "context is null");

    DeleteShareAlbumParam param;
    CHECK_PARAMETER_WITH_MESSAGE(env, ParseArgsDeleteShareAlbums(env, argv, param) != nullptr,
        "Failed to parse args");
    auto asyncContext = make_unique<MediaShareAlbumChangeRequestAsyncContext>();
    CHECK_PARAMETER_WITH_MESSAGE(env, asyncContext != nullptr, "Async context is null");
    asyncContext->owner = param.owner;
    asyncContext->deleteIds = std::move(param.deleteIds);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "deleteShareAlbum",
        DeleteShareAlbumsExecute, DeleteShareAlbumsCompleteCallback);
}

static napi_value ParseArgsAddShareMember(napi_env env, napi_value argv[],
    ShareMemberParam &param, MediaShareAlbumChangeRequestNapi *request)
{
    NAPI_INFO_LOG("enter ParseArgsAddShareMember");
    auto photoAlbum = request->GetPhotoAlbumInstance();
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM0], param.owner) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get owner");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], param.member) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get member");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetInt32(env, argv[PARAM2], param.status) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get status");
        return nullptr;
    }
    if (param.status < static_cast<int32_t>(ShareMemberStatus::INVITING) ||
        param.status > static_cast<int32_t>(ShareMemberStatus::REQUESTING)) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "invalid status");
        return nullptr;
    }
    if (!PhotoAlbum::IsShareAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        NAPI_ERR_LOG("AddShareMember: the album is not a share album");
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
            "AddShareMember: the album is not a share album");
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}
 
napi_value MediaShareAlbumChangeRequestNapi::JSAddShareMember(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowErrorWithIntCode(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return nullptr;
    }
 
    napi_value thisVar = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = { nullptr };
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    if (argc != ARGS_THREE) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "The number of parameters is incorrect");
        return nullptr;
    }
    MediaShareAlbumChangeRequestNapi* request = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&request)), JS_INNER_FAIL);
 
    ShareMemberParam param;
    CHECK_PARAMETER_WITH_MESSAGE(env, ParseArgsAddShareMember(env, argv, param, request) != nullptr,
        "Failed to parse args");
    request->shareMemberOwner_ = param.owner;
    request->shareMemberMember_ = param.member;
    request->shareMemberStatus_ = param.status;
 
    request->albumChangeOperations_.push_back(ShareAlbumChangeOperation::ADD_SHARE_MEMBER);
    RETURN_NAPI_TRUE(env);
}
 
static napi_value ParseArgsUpdateShareMemberStatus(napi_env env, napi_value argv[],
    ShareMemberParam &param, MediaShareAlbumChangeRequestNapi *request)
{
    NAPI_INFO_LOG("enter ParseArgsUpdateShareMemberStatus");
    auto photoAlbum = request->GetPhotoAlbumInstance();
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM0], param.owner) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get owner");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], param.member) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get member");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetInt32(env, argv[PARAM2], param.status) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get status");
        return nullptr;
    }
    if (param.status < static_cast<int32_t>(ShareMemberStatus::INVITING) ||
        param.status > static_cast<int32_t>(ShareMemberStatus::REQUESTING)) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "invalid status");
        return nullptr;
    }
    if (!PhotoAlbum::IsShareAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        NAPI_ERR_LOG("UpdateShareMemberStatus: the album is not a share album");
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
            "UpdateShareMemberStatus: the album is not a share album");
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}
 
napi_value MediaShareAlbumChangeRequestNapi::JSUpdateShareMemberStatus(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowErrorWithIntCode(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return nullptr;
    }
 
    napi_value thisVar = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = { nullptr };
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    if (argc != ARGS_THREE) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "The number of parameters is incorrect");
        return nullptr;
    }
    MediaShareAlbumChangeRequestNapi* request = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&request)), JS_INNER_FAIL);
 
    ShareMemberParam param;
    CHECK_PARAMETER_WITH_MESSAGE(env, ParseArgsUpdateShareMemberStatus(env, argv, param, request) != nullptr,
        "Failed to parse args");
    request->shareMemberOwner_ = param.owner;
    request->shareMemberMember_ = param.member;
    request->shareMemberStatus_ = param.status;
 
    request->albumChangeOperations_.push_back(ShareAlbumChangeOperation::UPDATE_SHARE_MEMBER_STATUS);
    RETURN_NAPI_TRUE(env);
}
 
static napi_value ParseArgsDeleteShareMember(napi_env env, napi_value argv[],
    ShareMemberTargetParam &param, MediaShareAlbumChangeRequestNapi *request)
{
    NAPI_INFO_LOG("enter ParseArgsDeleteShareMember");
    auto photoAlbum = request->GetPhotoAlbumInstance();
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM0], param.owner) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get owner");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], param.member) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get member");
        return nullptr;
    }
    if (!PhotoAlbum::IsShareAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        NAPI_ERR_LOG("DeleteShareMember: the album is not a share album");
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR,
            "DeleteShareMember: the album is not a share album");
        return nullptr;
    }
    RETURN_NAPI_TRUE(env);
}
 
napi_value MediaShareAlbumChangeRequestNapi::JSDeleteShareMember(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowErrorWithIntCode(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return nullptr;
    }
 
    napi_value thisVar = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = { nullptr };
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    if (argc != ARGS_TWO) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "The number of parameters is incorrect");
        return nullptr;
    }
    MediaShareAlbumChangeRequestNapi* request = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&request)), JS_INNER_FAIL);
 
    ShareMemberTargetParam param;
    CHECK_PARAMETER_WITH_MESSAGE(env, ParseArgsDeleteShareMember(env, argv, param, request) != nullptr,
        "Failed to parse args");
    request->shareMemberOwner_ = param.owner;
    request->shareMemberMember_ = param.member;
    request->albumChangeOperations_.push_back(ShareAlbumChangeOperation::DELETE_SHARE_MEMBER);
    RETURN_NAPI_TRUE(env);
}
 
static void DeleteMemberShareAlbumExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteMemberShareAlbumExecute");
 
    auto *context = static_cast<MediaShareAlbumChangeRequestAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
 
    DeleteMemberShareAlbumReqBody reqBody;
    reqBody.owner = context->deleteMemberAlbumOwner;
    reqBody.albumIds = context->albumIdsToDelete;
 
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_MEMBER_SHARE_ALBUM);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret != E_OK) {
        context->SaveError(ret);
        NAPI_ERR_LOG("DeleteMemberShareAlbumExecute failed, err: %{public}d", ret);
    }
}
 
static void DeleteMemberShareAlbumComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<MediaShareAlbumChangeRequestAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error, true);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    MediaLibraryNapiUtils::DeleteAsyncContextWithRef(env, context);
}
 
static napi_value ParseArgsDeleteMemberShareAlbum(napi_env env, napi_value argv[],
    DeleteMemberShareAlbumParam &param)
{
    NAPI_INFO_LOG("enter ParseArgsDeleteMemberShareAlbum");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowErrorWithIntCode(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return nullptr;
    }
    if (MediaLibraryNapiUtils::GetParamStringPathMax(env, argv[PARAM1], param.owner) != napi_ok) {
        NapiError::ThrowErrorWithIntCode(env, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "failed to get owner");
        return nullptr;
    }
    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, argv[PARAM2], napiValues));
    CHECK_WITH_INT_ERR_MESSAGE(env, !napiValues.empty(),
        MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "albums array is empty");
 
    for (const auto& napiValue : napiValues) {
        PhotoAlbumNapi *albumNapi = nullptr;
        CHECK_WITH_INT_ERR_MESSAGE(env,
            napi_unwrap(env, napiValue, reinterpret_cast<void **>(&albumNapi)) == napi_ok,
            MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "Failed to unwrap album object");
        CHECK_WITH_INT_ERR_MESSAGE(env, albumNapi != nullptr && albumNapi->GetAlbumId() > 0,
            MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "Invalid album object");
        param.albumIdsToDelete.push_back(albumNapi->GetAlbumId());
    }
    RETURN_NAPI_TRUE(env);
}
 
napi_value MediaShareAlbumChangeRequestNapi::JSDeleteMemberShareAlbum(napi_env env, napi_callback_info info)
{
    napi_value argv[ARGS_THREE] = { nullptr };
    size_t argc = ARGS_THREE;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), JS_INNER_FAIL);
    if (argc != ARGS_THREE) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "The number of parameters is incorrect");
        return nullptr;
    }
 
    napi_valuetype ctxType = napi_undefined;
    napi_typeof(env, argv[PARAM0], &ctxType);
    CHECK_WITH_INT_ERR_MESSAGE(env, ctxType == napi_object,
        MEDIA_LIBRARY_INVALID_PARAMETER_ERROR, "context is null");

    DeleteMemberShareAlbumParam param;
    CHECK_PARAMETER_WITH_MESSAGE(env, ParseArgsDeleteMemberShareAlbum(env, argv, param) != nullptr,
        "Failed to parse args");
 
    auto asyncContext = make_unique<MediaShareAlbumChangeRequestAsyncContext>();
    CHECK_PARAMETER_WITH_MESSAGE(env, asyncContext != nullptr, "Async context is null");
    asyncContext->deleteMemberAlbumOwner = param.owner;
    asyncContext->albumIdsToDelete = std::move(param.albumIdsToDelete);
 
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSDeleteMemberShareAlbum",
        DeleteMemberShareAlbumExecute, DeleteMemberShareAlbumComplete);
}
} // namespace OHOS::Media
