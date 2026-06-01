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
#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_SHORT_TERM_CALLBACK_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_SHORT_TERM_CALLBACK_ANI_H

#include <js_native_api.h>
#include <napi/native_api.h>
#include <node_api.h>
#include <string>
#include <uv.h>
#include "ani.h"
#include "want.h"
#include "ui_content.h"
#include "medialibrary_client_errno.h"

namespace OHOS {
namespace Media {

class ShortTermCallback {
public:
    explicit ShortTermCallback(ani_env *env, Ace::UIContent *uiContent);
    virtual ~ShortTermCallback() = default;
    void OnRelease(int32_t releaseCode);
    void OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want);
    void OnReceive(const OHOS::AAFwk::WantParams &request);
    void OnError(int32_t code, const std::string &name, const std::string &message);
    void SetSessionId(int32_t sessionId);
    void SetFunc(ani_fn_object func);

private:
    int32_t sessionId_ = 0;
    int32_t resultCode_ = JS_ERR_PERMISSION_DENIED;
    ani_vm *etsVm_ = nullptr;
    ani_env *env_ = nullptr;
    ani_ref callbackRef = nullptr;
    Ace::UIContent *uiContent = nullptr;
    void SendMessageBack(const std::string &desFileUri);
    void CloseModalUIExtension();
};
}
}
#endif
