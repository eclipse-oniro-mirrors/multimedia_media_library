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
#ifndef CONFIRM_CALLBACK_H
#define CONFIRM_CALLBACK_H

#include <string>
#include <vector>

#include "cj_ffi/cj_common_ffi.h"
#include "want.h"
#ifdef HAS_ACE_ENGINE_PART
#include "ui_content.h"
#endif
#include "medialibrary_client_errno.h"
#include "photo_accesshelper_impl.h"

namespace OHOS {
namespace Media {
class ConfirmCallback {
public:
#ifdef HAS_ACE_ENGINE_PART
    explicit ConfirmCallback(Ace::UIContent *uiContent, int64_t funcId);
#else
    explicit ConfirmCallback(int64_t funcId);
#endif
    virtual ~ConfirmCallback() = default;
    void OnRelease(int32_t releaseCode);
    void OnResult(int32_t resultCode, const OHOS::AAFwk::Want &want);
    void OnReceive(const OHOS::AAFwk::WantParams &request);
    void OnError(int32_t code, const std::string &name, const std::string &message);
    void SetSessionId(int32_t sessionId);

private:
    int32_t sessionId_ = 0;
    int32_t resultCode_ = JS_ERR_PERMISSION_DENIED;
    int64_t funcId;
    RetDataCArrString retDataCArrString;
#ifdef HAS_ACE_ENGINE_PART
    Ace::UIContent *uiContent = nullptr;
#endif
    void SendMessageBack(const std::vector<std::string> &desFileUris);
    void CloseModalUIExtension();
};
}
}
#endif