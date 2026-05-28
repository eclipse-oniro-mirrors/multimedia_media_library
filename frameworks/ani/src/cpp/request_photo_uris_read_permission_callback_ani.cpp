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
#include "request_photo_uris_read_permission_callback_ani.h"

#include "media_library_ani.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_ani_log.h"
#include "media_file_utils.h"
#include "userfile_client.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t REQUEST_SUCCESS = 0;
static const string REQUEST_PHOTO_URIS_DES_FILE_URIS = "desFileUris";
static const string RESULT_PARAM = "result";
static const string DATA_PARAM = "data";
}

RequestPhotoUrisReadPermissionCallback::RequestPhotoUrisReadPermissionCallback(ani_env *env,
    Ace::UIContent *uiContent)
{
    ani_vm *etsVm {};
    if (env == nullptr || env->GetVM(&etsVm) != ANI_OK) {
        ANI_ERR_LOG("Failed to get ani_vm");
    }
    this->etsVm_ = etsVm;
    this->env_ = env;
    this->uiContent = uiContent;
}

void RequestPhotoUrisReadPermissionCallback::OnRelease(int32_t releaseCode)
{
    ANI_INFO_LOG("ReleaseCode is %{public}d.", releaseCode);
    CloseModalUIExtension();
}

void RequestPhotoUrisReadPermissionCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    ANI_INFO_LOG("ResultCode is %{public}d.", resultCode);
    ani_env *etsEnv {};
    CHECK_IF_EQUAL(this->etsVm_ != nullptr, "etsVm_ is nullptr");
    CHECK_IF_EQUAL(this->etsVm_->GetEnv(ANI_VERSION_1, &etsEnv) == ANI_OK, "GetEnv fail");

    std::vector<std::string> desFileUris;
    if (resultCode == REQUEST_SUCCESS) {
        this->resultCode_ = resultCode;

        // check if the desFileUris exist
        if (!want.HasParameter(REQUEST_PHOTO_URIS_DES_FILE_URIS)) {
            ANI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(this->env_, true, JS_INNER_FAIL);
            return;
        }

        // get desFileUris from want
        desFileUris = want.GetStringArrayParam(REQUEST_PHOTO_URIS_DES_FILE_URIS);
        for (std::string mem : desFileUris) {
            ANI_INFO_LOG("mem %{public}s", mem.c_str());
        }
    } else {
        ANI_INFO_LOG("ResultCode is %{public}d.", resultCode);
        this->resultCode_ = JS_INNER_FAIL;
    }

    SendMessageBack(desFileUris);
}

void RequestPhotoUrisReadPermissionCallback::OnError(int32_t code, const std::string &name,
    const std::string &message)
{
    ANI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());

    this->resultCode_ = JS_INNER_FAIL;
    std::vector<std::string> desFileUris;
    SendMessageBack(desFileUris);
}

void RequestPhotoUrisReadPermissionCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    ANI_INFO_LOG("RequestPhotoUrisReadPermissionCallback Called.");
}

void RequestPhotoUrisReadPermissionCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void RequestPhotoUrisReadPermissionCallback::SetFunc(ani_fn_object func)
{
    ani_ref cbOnRef {};
        if (this->env_ == nullptr) {
        ANI_ERR_LOG("env_ is nullptr");
        AniError::ThrowError(this->env_, JS_INNER_FAIL, __FUNCTION__, __LINE__);
        return;
    }
    this->env_->GlobalReference_Create(static_cast<ani_ref>(func), &cbOnRef);
    this->callbackRef = cbOnRef;
}

void RequestPhotoUrisReadPermissionCallback::SendMessageBack(const std::vector<std::string> &desFileUris)
{
    ANI_INFO_LOG("SendMessageBack enter.");
    CloseModalUIExtension();
    ani_env *etsEnv {};
    CHECK_IF_EQUAL(this->etsVm_ != nullptr, "etsVm_ is nullptr");
    CHECK_IF_EQUAL(this->etsVm_->GetEnv(ANI_VERSION_1, &etsEnv) == ANI_OK, "GetEnv fail");

    if (etsEnv == nullptr || this->callbackRef == nullptr) {
        ANI_ERR_LOG("env_ or callbackRef is nullptr");
        AniError::ThrowError(etsEnv, JS_INNER_FAIL, __FUNCTION__, __LINE__);
        return;
    }
    if (this->resultCode_ != 0) {
        ANI_ERR_LOG("resultCode_ = %{public}d", this->resultCode_);
        AniError::ThrowError(etsEnv, this->resultCode_, __FUNCTION__, __LINE__);
        return;
    }
    ani_object arg = nullptr;
    CHECK_ARGS_RET_VOID(etsEnv, MediaLibraryAniUtils::ToAniStringArray(etsEnv, desFileUris, arg),
        JS_INNER_FAIL);
    std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(arg)};
    ani_fn_object aniCallback = static_cast<ani_fn_object>(this->callbackRef);

    ani_ref returnVal;
    CHECK_ARGS_RET_VOID(etsEnv, etsEnv->FunctionalObject_Call(aniCallback, args.size(), args.data(), &returnVal),
        JS_INNER_FAIL);
}

void RequestPhotoUrisReadPermissionCallback::CloseModalUIExtension()
{
    ANI_INFO_LOG("Called.");

    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
}
}
}