/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_SMART_ALBUM_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_SMART_ALBUM_NAPI_H_

#include <algorithm>
#include <vector>

#include "ability.h"
#include "abs_shared_result_set.h"
#include "data_ability_helper.h"
#include "data_ability_predicates.h"
#include "fetch_file_result_napi.h"
#include "fetch_result.h"
#include "smart_album_asset.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_db_const.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_error.h"
#include "result_set.h"
#include "uri.h"
#include "values_bucket.h"
#include "napi_remote_object.h"
#include "datashare_helper.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const std::string SMART_ALBUM_NAPI_CLASS_NAME = "SmartAlbum";
static const std::string USERFILEMGR_SMART_ALBUM_NAPI_CLASS_NAME = "UserFileMgrSmartAlbum";
class SmartAlbumNapi {
public:
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value UserFileMgrInit(napi_env env, napi_value exports);
    EXPORT static napi_value CreateSmartAlbumNapi(napi_env env, std::unique_ptr<SmartAlbumAsset> &albumData);
    int32_t GetSmartAlbumId() const;
    std::string GetSmartAlbumName() const;
    std::string GetSmartAlbumUri() const;
    int32_t GetAlbumPrivateType() const;
    std::string GetDescription() const;
    std::string GetCoverUri() const;
    int32_t GetExpiredTime() const;
    void SetAlbumCapacity(int32_t albumCapacity);
    std::string GetNetworkId() const;
    void SetExpiredTime(int32_t expiredTime);
    void SetDescription(std::string &description);
    void SetCoverUri(std::string &coverUri);
    EXPORT SmartAlbumNapi();
    EXPORT ~SmartAlbumNapi();

private:
    EXPORT static void SmartAlbumNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value SmartAlbumNapiConstructor(napi_env env, napi_callback_info info);
    void SetSmartAlbumNapiProperties();

    EXPORT static napi_value JSGetSmartAlbumId(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumTag(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumCapacity(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumCategoryId(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumCategoryName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumCoverUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumDateModified(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumType(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumDescription(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumExpiredTime(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSSmartAlbumNameSetter(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCommitModify(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAddFileAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRemoveFileAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSmartAlbumFileAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrGetAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrDeleteAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrRecoverAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSmartAlbumDescriptionSetter(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSmartAlbumCoverUriSetter(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSmartAlbumExpiredTimeSetter(napi_env env, napi_callback_info info);

    static thread_local SmartAlbumAsset *sAlbumData_;
    std::shared_ptr<SmartAlbumAsset> smartAlbumAssetPtr = nullptr;
    napi_env env_;

    static thread_local napi_ref sConstructor_;
    static thread_local napi_ref userFileMgrConstructor_;
};
constexpr int32_t DEFAULT_CHANGEDROWS = -1;
struct SmartAlbumNapiAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    SmartAlbumNapi *objectInfo;
    std::shared_ptr<SmartAlbumAsset> objectPtr;
    bool status;
    int32_t changedRows = DEFAULT_CHANGEDROWS;
    std::string selection;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    std::vector<std::string> selectionArgs;
    std::string order;
    std::string uri;
    std::string networkId;
    std::unique_ptr<FetchResult<FileAsset>> fetchResult;
    std::vector<int32_t> assetIds;

    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    ResultNapiType resultNapiType;
    std::vector<uint32_t> mediaTypes;
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<std::string> fetchColumn;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_SMART_ALBUM_NAPI_H_
