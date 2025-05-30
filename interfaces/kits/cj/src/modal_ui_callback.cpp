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

#include "modal_ui_callback.h"

#include "ui_content.h"


using namespace OHOS::Ace;
using namespace std;

namespace OHOS {
namespace Media {
ModalUICallback::ModalUICallback(Ace::UIContent* uiContent, PickerCallBack* pickerCallBack)
{
    this->uiContent = uiContent;
    this->pickerCallBack_ = pickerCallBack;
}

void ModalUICallback::SetSessionId(int32_t sessionId)
{
    this->sessionId_=sessionId;
}

void ModalUICallback::OnRelease(int32_t releaseCode)
{
    LOGI("OnRelease enter. release code is %{public}d", releaseCode);
    this->uiContent->CloseModalUIExtension(this->sessionId_);
    pickerCallBack_->ready = true;
}

void ModalUICallback::OnError(int32_t code, const std::string& name, const std::string& message)
{
    LOGE("OnError enter. errorCode=%{public}d, name=%{public}s, message=%{public}s",
        code, name.c_str(), message.c_str());
    if (!pickerCallBack_->ready) {
        this->uiContent->CloseModalUIExtension(this->sessionId_);
        pickerCallBack_->ready = true;
    }
}

void ModalUICallback::OnResultForModal(int32_t resultCode, const OHOS::AAFwk::Want &result)
{
    LOGI("OnResultForModal enter. resultCode is %{public}d", resultCode);
    pickerCallBack_->uris = result.GetStringArrayParam("select-item-list");
    pickerCallBack_->isOrigin = result.GetBoolParam("isOriginal", false);
    pickerCallBack_->resultCode = resultCode;
}

void ModalUICallback::OnReceive(const OHOS::AAFwk::WantParams &request)
{
    LOGI("OnReceive enter.");
}

void ModalUICallback::OnDestroy()
{
    LOGI("OnDestroy enter.");
}
} // namespace Media
} // namespace OHOS