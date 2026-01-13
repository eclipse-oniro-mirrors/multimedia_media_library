
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

#ifndef FRAMEWORKS_JS_NAPI_COMMON_MEDIA_NAPI_UTILS_H_
#define FRAMEWORKS_JS_NAPI_COMMON_MEDIA_NAPI_UTILS_H_

#include "napi/native_api.h"
#include <iremote_object.h>

namespace OHOS {
namespace Media {
class MediaNapiUtils {
public:
    static sptr<IRemoteObject> InitNapiToken(napi_env env, napi_callback_info info);
    static napi_status CheckIsStage(napi_env env, napi_callback_info info, bool &result);
    static sptr<IRemoteObject> ParseTokenInStagemode(napi_env env, napi_callback_info info);
    static sptr<IRemoteObject> ParseTokenInAbility(napi_env env, napi_callback_info info);
private:
    MediaNapiUtils() = default;
};

} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_JS_NAPI_COMMON_MEDIA_NAPI_UTILS_H_
