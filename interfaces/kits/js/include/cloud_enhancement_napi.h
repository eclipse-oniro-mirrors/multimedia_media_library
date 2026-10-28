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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CLOUD_ENHANCEMENT_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CLOUD_ENHANCEMENT_NAPI_H

#include <vector>

#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "file_asset_napi.h"
#include "photo_proxy.h"
#include "values_bucket.h"
#include "napi_base_context.h"
#include "napi_error.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "fetch_file_result_napi.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class CE_AVAILABLE : int32_t {
    NOT_SUPPORT,
    SUPPORT,
    PROCESSING,
    FAILED_RETRY,
    FAILED,
    SUCCESS,
    EDIT,
};

enum class PHOTO_CE_STATUS_CODE : int32_t {
    // failed retry
    LIMIT_USAGE = 100,
    LIMIT_REQUEST,
    TASK_CACHE_TIMEOUT,
    NETWORK_UNAVAILABLE,
    TEMPERATURES_GUARD,
    NETWORK_WEAK,
    // failed
    EXECUTE_FAILED,
    // failed retry
    DO_AUTH_FAILED,
    TASK_CANNOT_EXECUTE,
    // failed
    NON_RECOVERABLE = 200
};

class CloudEnhancementNapi {
public:
    EXPORT CloudEnhancementNapi() = default;
    EXPORT ~CloudEnhancementNapi() = default;
    EXPORT static napi_value Init(napi_env env, napi_value exports);

    static bool InitUserFileClient(napi_env env, napi_callback_info info);
    static napi_status ParseArgGetPhotoAsset(napi_env env, napi_value arg, int &fileId, std::string &uri,
        std::string &displayName);
    static napi_status ParseArgGetPhotoAssetForLocalEnhancement(napi_env env, napi_value arg, int &fileId,
        std::string &uri, std::string &displayName);

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSGetCloudEnhancementInstance(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSubmitCloudEnhancementTasks(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSPrioritizeCloudEnhancementTask(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCancelCloudEnhancementTasks(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCancelAllCloudEnhancementTasks(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSQueryCloudEnhancementTaskState(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSQueryLocalEnhancementTaskState(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSyncCloudEnhancementTaskStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetCloudEnhancementPair(napi_env env, napi_callback_info info);

    static thread_local napi_ref constructor_;
};

struct CloudEnhancementAsyncContext : public NapiError {
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;

    const int32_t UNDEFINED = -1;
    CloudEnhancementNapi* objectInfo;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<std::string> uris;
    std::string photoUri;
    int32_t fileId {UNDEFINED};
    std::string displayName;
    bool hasCloudWatermark_;
    int32_t triggerMode_;
    ResultNapiType resultNapiType;
    NapiAssetType assetType;
    std::unique_ptr<FileAsset> fileAsset;
    CloudEnhancementTaskStage cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_EXCEPTION;
    int32_t transferredFileSize_ {UNDEFINED};
    int32_t totalFileSize_ {UNDEFINED};
    int32_t expectedDuration_ {UNDEFINED};
    int32_t statusCode_ {UNDEFINED};
    int32_t localEnhancementProgress_ {UNDEFINED};
    std::unique_ptr<FetchResult<FileAsset>> fetchFileResult;
    bool GetPairAsset();
};
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CLOUD_ENHANCEMENT_NAPI_H
