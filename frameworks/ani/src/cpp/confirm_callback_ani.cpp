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
#include "confirm_callback_ani.h"

#include "media_library_ani.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "media_file_utils.h"
#include "userfile_client.h"

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t CONFIRM_CODE_SUCCESS = 0;
static constexpr int32_t CONFIRM_CODE_USER_DENY = -1;
static const std::string CONFIRM_BOX_DES_FILE_URIS = "desFileUris";
static const std::string RESULT_PARAM = "result";
static const std::string DATA_PARAM = "data";
}
#ifdef HAS_ACE_ENGINE_PART
ConfirmCallback::ConfirmCallback(ani_env *env, Ace::UIContent *uiContent)
{
    ani_vm *etsVm {};
    if (env == nullptr || env->GetVM(&etsVm) != ANI_OK) {
        ANI_ERR_LOG("Failed to get ani_vm");
    }
    this->etsVm_ = etsVm;
    this->env_ = env;
    this->uiContent = uiContent;
}
#else
ConfirmCallback::ConfirmCallback(ani_env *env_)
{
    ani_vm *etsVm {};
    if (env == nullptr || env->GetVM(&etsVm) != ANI_OK) {
        ANI_ERR_LOG("Failed to get ani_vm");
    }
    this->etsVm_ = etsVm;
    this->env_ = env_;
}
#endif

void ConfirmCallback::OnRelease(int32_t releaseCode)
{
    ANI_INFO_LOG("ReleaseCode is %{public}d.", releaseCode);

    CloseModalUIExtension();
}

void ConfirmCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    ANI_INFO_LOG("ResultCode is %{public}d.", resultCode);
    ani_env *etsEnv {};
    CHECK_IF_EQUAL(this->etsVm_ != nullptr, "etsVm_ is nullptr");
    CHECK_IF_EQUAL(this->etsVm_->GetEnv(ANI_VERSION_1, &etsEnv) == ANI_OK, "GetEnv fail");

    this->resultCode_ = resultCode;

    std::vector<std::string> desFileUris;
    if (resultCode == CONFIRM_CODE_SUCCESS) {
        // check if the desFileUris exsit
        if (!want.HasParameter(CONFIRM_BOX_DES_FILE_URIS)) {
            ANI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(etsEnv, true, JS_INNER_FAIL);
            return;
        }

        // get desFileUris from want
        desFileUris = want.GetStringArrayParam(CONFIRM_BOX_DES_FILE_URIS);
    } else if (resultCode == CONFIRM_CODE_USER_DENY) {
        this->resultCode_ = CONFIRM_CODE_SUCCESS; // user deny return success with empty uris
    }

    SendMessageBack(desFileUris);
}

void ConfirmCallback::OnError(int32_t code, const std::string &name, const std::string &message)
{
    ANI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());

    this->resultCode_ = JS_INNER_FAIL;
    std::vector<std::string> desFileUris;
    SendMessageBack(desFileUris);
}

void ConfirmCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    ANI_INFO_LOG("Called.");
}

void ConfirmCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void ConfirmCallback::SetFunc(ani_fn_object func)
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

void ConfirmCallback::SendMessageBack(const std::vector<std::string> &desFileUris)
{
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

void ConfirmCallback::CloseModalUIExtension()
{
    ANI_INFO_LOG("Called.");

#ifdef HAS_ACE_ENGINE_PART
    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
#endif
}
}
}