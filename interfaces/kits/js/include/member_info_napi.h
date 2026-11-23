/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEMBER_INFO_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEMBER_INFO_NAPI_H

#include <vector>

#include "medialibrary_napi_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_remote_object.h"
#include "napi_error.h"
#include "member_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MemberInfoNapi {
public:
    EXPORT MemberInfoNapi();
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value CreateMemberInfoNapi(napi_env env, shared_ptr<MemberInfo>& memberInfo);
    // NAPI Property declarations
    int32_t GetStatus() const;
    const std::string& GetMember() const;
    EXPORT ~MemberInfoNapi();

private:
    EXPORT void SetMemberInfoNapiProperties();
    EXPORT static napi_value MemberInfoNapiConstructor(napi_env env, napi_callback_info info);
    EXPORT static void MemberInfoNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value JSGetStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetMember(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetMember(napi_env env, napi_callback_info info);

    napi_env env_;
    std::shared_ptr<MemberInfo> memberInfoPtr = nullptr;
    static thread_local napi_ref constructor_;
};
} // namespace Media
} // namespace OHOS
#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEMBER_INFO_NAPI_H