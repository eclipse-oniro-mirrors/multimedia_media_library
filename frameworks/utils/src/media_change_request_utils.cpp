/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "media_change_request_utils.h"

namespace OHOS {
namespace Media {

napi_value MediaChangeRequestUtils::CreateComment(napi_env env)
{
    napi_value commentValue = nullptr;
    napi_status status = napi_create_string_utf8(env, "Add, delete and update asset or album",
        NAPI_AUTO_LENGTH, &commentValue);
    if (status == napi_ok) {
        return commentValue;
    }
    return nullptr;
}
} // namespace Media
} // namespace OHOS
