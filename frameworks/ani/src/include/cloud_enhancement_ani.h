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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_CLOUD_ENHANCEMENT_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_CLOUD_ENHANCEMENT_ANI_H

#include <memory>
#include <string>
#include <vector>
#include "ani_error.h"
#include "datashare_helper.h"
#include "fetch_result.h"
#include "file_asset_ani.h"
#include "medialibrary_ani_utils.h"

namespace OHOS {
namespace Media {
enum class CE_AVAILABLE : int32_t {
    NOT_SUPPORT,
    SUPPORT,
    PROCESSING,
    FAILED_RETRY,
    FAILED,
    SUCCESS,
    EDIT,
};

class CloudEnhancementAni {
public:
    static ani_status Init(ani_env *env);
    static CloudEnhancementAni* Unwrap(ani_env *env, ani_object aniObject);

    static bool InitUserFileClient(ani_env *env, ani_object aniObject);
    static ani_status ParseArgGetPhotoAsset(ani_env *env, ani_object photoAsset, int &fileId, std::string &uri,
        std::string &displayName);

private:
    static ani_object Constructor(ani_env *env, ani_class clazz, ani_object context);
    static void SubmitCloudEnhancementTasks(ani_env *env, ani_object aniObject, ani_object photoAssets,
        ani_boolean hasCloudWatermark, ani_object triggerMode);
    static void PrioritizeCloudEnhancementTask(ani_env *env, ani_object aniObject, ani_object photoAsset);
    static void CancelCloudEnhancementTasks(ani_env *env, ani_object aniObject, ani_object photoAssets);
    static void CancelAllCloudEnhancementTasks(ani_env *env, ani_object aniObject);
    static ani_object QueryCloudEnhancementTaskState(ani_env *env, ani_object aniObject, ani_object photoAsset);
    static void SyncCloudEnhancementTaskStatus(ani_env *env, ani_object aniObject);
    static ani_object GetCloudEnhancementPair(ani_env *env, ani_object aniObject, ani_object asset);
    static ani_object QueryLocalEnhancementTaskState(ani_env *env, ani_object aniObject, ani_object photoAsset);
};

struct CloudEnhancementAniContext : public AniError {
    const int32_t UNDEFINED = -1;
    CloudEnhancementAni* objectInfo;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<std::string> uris;
    std::string photoUri;
    int32_t fileId {UNDEFINED};
    std::string displayName;
    bool hasCloudWatermark_;
    int32_t triggerMode_;
    ResultNapiType resultNapiType;
    AniAssetType assetType;
    std::unique_ptr<FileAsset> fileAsset;
    CloudEnhancementTaskStage cloudEnhancementTaskStage_ = CloudEnhancementTaskStage::TASK_STAGE_EXCEPTION;
    int32_t transferredFileSize_ {UNDEFINED};
    int32_t totalFileSize_ {UNDEFINED};
    int32_t expectedDuration_ {UNDEFINED};
    int32_t statusCode_ {UNDEFINED};
    std::unique_ptr<FetchResult<FileAsset>> fetchFileResult;
    int32_t localEnhancementProgress_ {0};
    bool GetPairAsset();
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_CLOUD_ENHANCEMENT_ANI_H