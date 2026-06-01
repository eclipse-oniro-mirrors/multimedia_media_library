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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CLOUD_MEDIA_ASSET_MANAGER_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CLOUD_MEDIA_ASSET_MANAGER_NAPI_H

#include <vector>
#include <map>
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
#include "cloud_media_asset_types.h"
#include "medialibrary_notify_asset_manager_observer.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaOnNotifyAssetManagerObserver;
class AssetManagerChangeListenerNapi {
public:
    explicit AssetManagerChangeListenerNapi(napi_env env) : env_(env) {}

    AssetManagerChangeListenerNapi(const AssetManagerChangeListenerNapi &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
    }

    AssetManagerChangeListenerNapi& operator=(const AssetManagerChangeListenerNapi &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
        return *this;
    }

    ~AssetManagerChangeListenerNapi() {};
    napi_ref cbOnRef_ = nullptr;
    napi_ref cbOffRef_ = nullptr;
    std::vector<std::shared_ptr<MediaOnNotifyAssetManagerObserver>> observers_;
private:
    napi_env env_ = nullptr;
};


class CloudMediaAssetManagerNapi {
public:
    EXPORT CloudMediaAssetManagerNapi() = default;
    EXPORT ~CloudMediaAssetManagerNapi() = default;
    EXPORT static napi_value Init(napi_env env, napi_value exports);

    static bool InitUserFileClient(napi_env env, napi_callback_info info);
private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSGetCloudMediaAssetManagerInstance(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSStartDownloadCloudMedia(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSPauseDownloadCloudMedia(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCancelDownloadCloudMedia(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSRetainCloudMediaAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetCloudMediaAssetStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSStartBatchDownloadCloudResources(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSResumeBatchDownloadCloudResources(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSPauseDownloadCloudResources(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCancelDownloadCloudResources(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetBatchDownloadCloudResourcesStatus(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetBatchDownloadSpecificTaskCount(napi_env env, napi_callback_info info);
    EXPORT static napi_value JsBatchDownloadRegisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value JsBatchDownloadUnRegisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateDownloadCloudAssetCodeEnum(napi_env env);
    EXPORT static napi_value CreateDownloadAssetsNotifyTypeEnum(napi_env env);

    static int32_t RegisterObserverExecute(napi_env env, napi_ref ref, AssetManagerChangeListenerNapi &listObj,
        const Notification::NotifyUriType uriType);
    static int32_t UnregisterObserverExecute(napi_env env,
        const Notification::NotifyUriType uriType, napi_ref ref, AssetManagerChangeListenerNapi &listObj);
    static int32_t AddClientObserver(napi_env env, napi_ref ref,
        std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserver>>> &clientObservers,
        const Notification::NotifyUriType uriType);
    static int32_t RemoveClientObserver(napi_env env, napi_ref ref,
        std::map<Notification::NotifyUriType, std::vector<shared_ptr<ClientObserver>>> &clientObservers,
        const Notification::NotifyUriType uriType);

    static thread_local napi_ref constructor_;
    static thread_local napi_ref sdownloadCloudAssetCodeeEnumRef_;
    static thread_local napi_ref sdownloadAssetsNotifyTypeEnumRef_;
};

struct CloudMediaAssetAsyncContext : public NapiError {
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;

    CloudMediaAssetManagerNapi* objectInfo;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    int32_t cloudMediaDownloadType;
    int32_t cloudMediaRetainType;
    CloudMediaAssetTaskStatus cloudMediaAssetTaskStatus_;
    CloudMediaTaskPauseCause cloudMediaTaskPauseCause_;
    string taskInfo_;
    vector<string> startBatchDownloadUris;
    std::map<std::string, int32_t> startBatchDownloadResp;
    vector<string> resumeBatchDownloadUris;
    vector<string> pauseBatchDownloadUris;
    vector<string> cancelBatchDownloadUris;
    DataShare::DataSharePredicates getStatusBatchDownloadPredicates;
    std::vector<std::string> allBatchDownloadStatus;
    DataShare::DataSharePredicates getCountBatchDownloadPredicates;
    int32_t allBatchDownloadCount;
};
}
}
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_CLOUD_MEDIA_ASSET_MANAGER_NAPI_H