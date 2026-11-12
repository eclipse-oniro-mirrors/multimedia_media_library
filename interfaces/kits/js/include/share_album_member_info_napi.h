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

#ifndef INTERFACES_KITS_JS_RDB_INCLUDE_SHARE_ALBUM_MEMBER_INFO_NAPI_H_
#define INTERFACES_KITS_JS_RDB_INCLUDE_SHARE_ALBUM_MEMBER_INFO_NAPI_H_

#include <vector>

#include "medialibrary_napi_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_remote_object.h"
#include "napi_error.h"
#include "member_info.h"
#include "share_album_member_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using namespace OHOS::NativeRdb;

static const std::string SHARE_ALBUM_MEMBER_INFO_NAPI_CLASS_NAME = "ShareAlbumMemberInfo";

class ShareAlbumMemberInfoNapi {
public:
    EXPORT ShareAlbumMemberInfoNapi();
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value CreatShareAlbumMemberInfoNapi(napi_env env,
        std::vector<std::shared_ptr<MemberInfo>>& memberInfos, const std::string& shareAlbumOwner);
    EXPORT ~ShareAlbumMemberInfoNapi();

private:
    EXPORT static napi_value ShareAlbumMemberInfoNapiConstructor(napi_env env, napi_callback_info info);
    EXPORT static void ShareAlbumMemberInfoNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    // NAPI Property declarations
    EXPORT static napi_value JSGetMemberInfos(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetMemberInfos(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetShareAlbumOwner(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetShareAlbumOwner(napi_env env, napi_callback_info info);
    // NAPI Method declarations
    std::vector<std::shared_ptr<MemberInfo>> GetMemberInfos() const;
    const std::string& GetShareAlbumOwner() const;

    napi_env env_;
    std::shared_ptr<ShareAlbumMemberInfo> shareAlbumMemberInfoPtr_;
    napi_ref memberInfosArrayRef_ = nullptr;
    static thread_local napi_ref constructor_;
};
}
}
#endif  // INTERFACES_KITS_JS_RDB_INCLUDE_SHARE_ALBUM_MEMBER_INFO_NAPI_H_