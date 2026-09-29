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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CUSTOM_RECORD_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CUSTOM_RECORD_NAPI_H_

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
static const std::string PHOTO_ASSET_CUSTOM_RECORDS_NAPI_CLASS_NAME = "PhotoAssetCustomRecord";
class PhotoAssetCustomRecordNapi {
public:
    EXPORT PhotoAssetCustomRecordNapi() = default;
    EXPORT ~PhotoAssetCustomRecordNapi() = default;
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);
    EXPORT static napi_value CreateCustomRecordNapi(napi_env env, std::unique_ptr<PhotoAssetCustomRecord> &cRecord);

    int32_t GetFileId() const;
    int32_t GetShareCount() const;
    int32_t GetLcdJumpCount() const;
private:
    EXPORT void SetCustomRecordNapiProperties();

    EXPORT static napi_value JSGetFileId(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetShareCount(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetLcdJumpCount(napi_env env, napi_callback_info info);

    static thread_local PhotoAssetCustomRecord* cRecordData_;
    std::shared_ptr<PhotoAssetCustomRecord> customRecordPtr = nullptr;
    static thread_local napi_ref constructor_;
    napi_env env_;
    static std::mutex mutex_;
};

struct PhotoAssetCustomRecordNapiContext : public NapiError {
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;

    PhotoAssetCustomRecordNapi *objectInfo;
    std::shared_ptr<PhotoAssetCustomRecord> objectPtr;
    bool status;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CUSTOM_RECORD_NAPI_H_
