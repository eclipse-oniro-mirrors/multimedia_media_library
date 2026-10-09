/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_SHARE_ALBUM_CHANGE_REQUEST_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_SHARE_ALBUM_CHANGE_REQUEST_NAPI_H_

#include <string>
#include <vector>
#include "datashare_predicates.h"
#include "media_album_change_request_napi.h"
#include "media_change_request_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_error.h"
#include "photo_album_napi.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {

enum class ShareAlbumChangeOperation {
    SET_SHARE_ALBUM_NAME,
    ADD_SHARE_MEMBER,
    UPDATE_SHARE_MEMBER_STATUS,
    DELETE_SHARE_MEMBER,
};

class MediaShareAlbumChangeRequestNapi : public MediaChangeRequestNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);

    std::shared_ptr<PhotoAlbum> GetPhotoAlbumInstance() const;
    napi_value ApplyChanges(napi_env env, napi_callback_info info) override;
    bool CheckChangeOperations(napi_env env);
    std::string shareMemberOwner_;
    std::string shareMemberMember_;
    int32_t shareMemberStatus_ = 0;

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);
    EXPORT static napi_value JSSetShareAlbumName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteShareAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAddShareMember(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSUpdateShareMemberStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteShareMember(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteMemberShareAlbum(napi_env env, napi_callback_info info);

    static thread_local napi_ref constructor_;
    std::shared_ptr<PhotoAlbum> photoAlbum_ = nullptr;
    std::vector<ShareAlbumChangeOperation> albumChangeOperations_;

public:
    std::string shareOwnerInfo_;
    std::vector<int32_t> deleteIds_;
    std::string albumName_;
};

struct SetShareAlbumNameParam {
    std::string owner;
    std::string albumName;
};

struct ShareMemberParam {
    std::string owner;
    std::string member;
    int32_t status = 0;
};

struct ShareMemberTargetParam {
    std::string owner;
    std::string member;
};

struct DeleteShareAlbumParam {
    std::string owner;
    std::vector<int32_t> deleteIds;
};

struct DeleteMemberShareAlbumParam {
    std::string owner;
    std::vector<int32_t> albumIdsToDelete;
};

struct MediaShareAlbumChangeRequestAsyncContext : public NapiError {
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callbackRef = nullptr;
    napi_ref objectInfoRef = nullptr;
    size_t argc = 0;
    napi_value argv[NAPI_ARGC_MAX] = { nullptr };

    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    int32_t userId = -1;
    MediaShareAlbumChangeRequestNapi* shareAlbumObjectInfo = nullptr;
    std::vector<ShareAlbumChangeOperation> albumChangeOperations;
    int32_t albumId = 0;
    bool hasValidAlbum = false;
    std::string shareOwnerInfo;
    std::string albumName;
    std::string memberOwner;
    std::string member;
    int32_t memberStatus = 0;
    std::string owner;
    std::vector<int32_t> deleteIds;
    std::vector<int32_t> albumIdsToDelete;
    std::string deleteMemberAlbumOwner;
};

} // namespace Media
} // namespace OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_SHARE_ALBUM_CHANGE_REQUEST_NAPI_H_
