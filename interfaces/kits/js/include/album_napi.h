/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_ALBUM_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_ALBUM_NAPI_H_

#include <algorithm>
#include <vector>

#include "ability.h"
#include "abs_shared_result_set.h"
#include "album_asset.h"
#include "data_ability_helper.h"
#include "data_ability_predicates.h"
#include "fetch_file_result_napi.h"
#include "fetch_result.h"
#include "medialibrary_db_const.h"
#include "medialibrary_napi_utils.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "result_set.h"
#include "uri.h"
#include "values_bucket.h"
#include "napi_remote_object.h"
#include "datashare_helper.h"
#include "napi_error.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const std::string ALBUM_NAPI_CLASS_NAME = "Album";
static const std::string USERFILEMGR_ALBUM_NAPI_CLASS_NAME = "UserFileMgrAlbum";
static const std::string PHOTOACCESSHELPER_ALBUM_NAPI_CLASS_NAME = "PhotoAccessHelperAlbum";

class AlbumNapi {
public:
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value UserFileMgrInit(napi_env env, napi_value exports);
    EXPORT static napi_value PhotoAccessHelperInit(napi_env env, napi_value exports);
    EXPORT static napi_value CreateAlbumNapi(napi_env env, std::unique_ptr<AlbumAsset> &albumData);
    int32_t GetAlbumId() const;
    std::string GetAlbumName() const;
    std::string GetAlbumPath() const;
    std::string GetAlbumUri() const;
    std::string GetNetworkId() const;
    EXPORT AlbumNapi();
    EXPORT ~AlbumNapi();

#ifdef MEDIALIBRARY_COMPATIBILITY
    PhotoAlbumType GetAlbumType() const;
    PhotoAlbumSubType GetAlbumSubType() const;
#endif

private:
    EXPORT static void AlbumNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value AlbumNapiConstructor(napi_env env, napi_callback_info info);
    EXPORT void SetAlbumNapiProperties();

    EXPORT static napi_value JSGetAlbumId(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumDateModified(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetCount(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumRelativePath(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetCoverUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCommitModify(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumFileAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAlbumNameSetter(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSGetAlbumPath(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumVirtual(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetAlbumPath(napi_env env, napi_callback_info info);

    EXPORT static napi_value UserFileMgrGetAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrCommitModify(napi_env env, napi_callback_info info);

    EXPORT static napi_value PhotoAccessHelperGetAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info);

    napi_env env_;

    static thread_local napi_ref sConstructor_;
    static thread_local napi_ref userFileMgrConstructor_;
    static thread_local napi_ref photoAccessHelperConstructor_;
    static thread_local AlbumAsset *sAlbumData_;
    std::shared_ptr<AlbumAsset> albumAssetPtr = nullptr;
};

struct AlbumNapiAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    AlbumNapi *objectInfo;
    std::shared_ptr<AlbumAsset> objectPtr;
    bool status;
    int32_t changedRows;
    std::string selection;
    std::vector<std::string> selectionArgs;
    std::string order;
    std::unique_ptr<FetchResult<FileAsset>> fetchResult;
    std::string networkId;
    std::string uri;
    std::map<std::string, std::string> uriMap;

    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    ResultNapiType resultNapiType;
    std::vector<uint32_t> mediaTypes;
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<std::string> fetchColumn;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_ALBUM_NAPI_H_
