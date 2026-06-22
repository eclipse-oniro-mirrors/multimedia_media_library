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
#include "short_term_callback_ani.h"

#include "media_library_ani.h"
#include "medialibrary_ani_log.h"
#include "media_file_utils.h"
#include "userfile_client.h"
#include "medialibrary_ani_utils.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace {
static constexpr int32_t SHORT_TERM_CODE_SUCCESS = 0;
static const string SHORT_TERM_DES_FILE_URIS = "desFileUris";
static const string RESULT_PARAM = "result";
static const string DATA_PARAM = "data";
}

ShortTermCallback::ShortTermCallback(ani_env *env, Ace::UIContent *uiContent)
{
    ani_vm *etsVm {};
    if (env == nullptr || env->GetVM(&etsVm) != ANI_OK) {
        ANI_ERR_LOG("Failed to get ani_vm");
    }
    this->etsVm_ = etsVm;
    this->env_ = env;
    this->uiContent = uiContent;
}

void ShortTermCallback::OnRelease(int32_t releaseCode)
{
    CloseModalUIExtension();
}

void ShortTermCallback::OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want)
{
    ani_env *etsEnv {};
    CHECK_IF_EQUAL(this->etsVm_ != nullptr, "etsVm_ is nullptr");
    CHECK_IF_EQUAL(this->etsVm_->GetEnv(ANI_VERSION_1, &etsEnv) == ANI_OK, "GetEnv fail");
    vector<string> desFileUris;
    if (resultCode == SHORT_TERM_CODE_SUCCESS) {
        this->resultCode_ = resultCode;
        if (!want.HasParameter(SHORT_TERM_DES_FILE_URIS)) {
            ANI_ERR_LOG("Can't get string array from want.");
            CHECK_ARGS_RET_VOID(etsEnv, true, JS_INNER_FAIL);
            return;
        }
        desFileUris = want.GetStringArrayParam(SHORT_TERM_DES_FILE_URIS);
    } else {
        this->resultCode_ = JS_INNER_FAIL;
    }
    size_t len  = desFileUris.size();
    if (len > 0) {
        SendMessageBack(desFileUris[0]);
    } else {
        string desFileUri;
        SendMessageBack(desFileUri);
    }
}

void ShortTermCallback::OnError(int32_t code, const std::string &name, const std::string &message)
{
    ANI_INFO_LOG("Code is %{public}d, name is %{public}s, message is %{public}s.", code, name.c_str(),
        message.c_str());
    this->resultCode_ = JS_INNER_FAIL;
    string desFileUri;
    SendMessageBack(desFileUri);
}

void ShortTermCallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    ANI_INFO_LOG("OnReceive enter.");
}

void ShortTermCallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_ = sessionId;
}

void ShortTermCallback::SetFunc(ani_fn_object func)
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

void ShortTermCallback::SendMessageBack(const string &desFileUri)
{
    CloseModalUIExtension();

    ani_env *etsEnv {};
    CHECK_IF_EQUAL(this->etsVm_ != nullptr, "etsVm_ is nullptr");
    CHECK_IF_EQUAL(this->etsVm_->GetEnv(ANI_VERSION_1, &etsEnv) == ANI_OK, "GetEnv fail");

    if (this->resultCode_ != 0) {
        AniError::ThrowError(etsEnv, JS_INNER_FAIL, __FUNCTION__, __LINE__);
        return;
    }
    ani_string arg = {};
    CHECK_ARGS_RET_VOID(etsEnv, etsEnv->String_NewUTF8(desFileUri.c_str(), desFileUri.size(), &arg),
        JS_INNER_FAIL);
    std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(arg)};
    ani_fn_object aniCallback = static_cast<ani_fn_object>(this->callbackRef);
    ani_ref returnVal;
    CHECK_ARGS_RET_VOID(etsEnv, etsEnv->FunctionalObject_Call(aniCallback, args.size(),
        args.data(), &returnVal), JS_INNER_FAIL);
}

void ShortTermCallback::CloseModalUIExtension()
{
    if (this->uiContent != nullptr) {
        uiContent->CloseModalUIExtension(this->sessionId_);
    }
}
}
}