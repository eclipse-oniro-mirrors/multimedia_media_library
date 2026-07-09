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
#ifndef DELETE_CALLBACK_H
#define DELETE_CALLBACK_H

#include <string>

#include "want.h"
#ifdef HAS_ACE_ENGINE_PART
#include "ui_content.h"
#endif
#include "medialibrary_client_errno.h"

namespace OHOS {
namespace Media {
const std::string DELETE_UI_PACKAGE_NAME = "com.ohos.photos";
const std::string DELETE_UI_EXT_ABILITY_NAME = "DeleteUIExtensionAbility";
const std::string DELETE_UI_EXTENSION_TYPE = "ability.want.params.uiExtensionType";
const std::string DELETE_UI_REQUEST_TYPE = "sysDialog/common";
const std::string DELETE_UI_APPNAME = "appName";
const std::string DELETE_UI_URIS = "uris";
const std::string RESULT = "result";
const int32_t DELETE_CODE_SUCCESS = 0;
const int32_t DEFAULT_SESSION_ID = 0;

class DeleteCallback {
public:
#ifdef HAS_ACE_ENGINE_PART
    explicit DeleteCallback(Ace::UIContent* uiContent);
#else
    explicit DeleteCallback();
#endif
    void OnRelease(int32_t releaseCode);
    void OnResult(int32_t resultCode, const OHOS::AAFwk::Want& result);
    void OnReceive(const OHOS::AAFwk::WantParams& request);
    void OnError(int32_t code, const std::string& name, const std::string& message);
    void SetSessionId(int32_t sessionId);
    void SetUris(std::vector<std::string> uris);
    void SetFunc();

private:
    int32_t sessionId_ = DEFAULT_SESSION_ID;
    int32_t resultCode_ = JS_ERR_PERMISSION_DENIED;
    std::vector<std::string> uris_;
#ifdef HAS_ACE_ENGINE_PART
    Ace::UIContent* uiContent = nullptr;
#endif
    void SendMessageBack();
    void CloseModalUIExtension();
};
} // namespace Media
} // namespace OHOS

#endif