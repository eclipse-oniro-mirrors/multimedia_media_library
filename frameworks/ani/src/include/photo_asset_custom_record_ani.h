/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ASSET_CUSTOM_RECORD_ANI_H_
#define FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ASSET_CUSTOM_RECORD_ANI_H_

#include <algorithm>
#include <vector>

#include "ability.h"
#include "abs_shared_result_set.h"
#include "data_ability_helper.h"
#include "data_ability_predicates.h"
#include "fetch_result_ani.h"
#include "fetch_result.h"
#include "smart_album_asset.h"
#include "medialibrary_ani_utils.h"

#include "result_set.h"
#include "uri.h"
#include "values_bucket.h"
#include "datashare_helper.h"

namespace OHOS {
namespace Media {

struct AniPhotoAssetCustomRecordOperator {
    std::string clsName;
    ani_class cls { nullptr };
    ani_method ctor { nullptr };
    ani_method setFileId { nullptr };
    ani_method setShareCount { nullptr };
    ani_method setLcdJumpCount { nullptr };
};

class PhotoAssetCustomRecordAni {
public:
    EXPORT PhotoAssetCustomRecordAni() = default;
    EXPORT ~PhotoAssetCustomRecordAni() = default;
    EXPORT static ani_status CustomRecordInit(ani_env *env);
    EXPORT static ani_object CreatePhotoAssetCustomRecordAni(ani_env *env,
        std::unique_ptr<PhotoAssetCustomRecord> recordData,
        const AniPhotoAssetCustomRecordOperator &photoAssetCustomRecordOperator);
    EXPORT static PhotoAssetCustomRecordAni* UnwrapPhotoAssetCustomRecordObject(ani_env *env, ani_object object);
    EXPORT static ani_object CreatePhotoAssetCustomRecordAni(ani_env *env,
        std::unique_ptr<PhotoAssetCustomRecord> recordData);
    EXPORT static ani_object CreatePhotoAssetCustomRecordAni(ani_env *env,
        std::shared_ptr<PhotoAssetCustomRecord> &recordData);
    EXPORT static ani_status InitAniPhotoAssetCustomRecordOperator(ani_env *env,
        AniPhotoAssetCustomRecordOperator &albumOrderOperator);

    std::shared_ptr<PhotoAssetCustomRecord> GetPhotoAssetCustomRecordInstance() const;
    private:
    EXPORT void SetCustomRecordAniProperties();
    EXPORT static ani_object PhotoAssetCustomRecordAniConstructor(ani_env *env,
        const AniPhotoAssetCustomRecordOperator &opt);
    EXPORT static void PhotoAssetCustomRecordAniDestructor(ani_env *env,
        ani_object object);
    EXPORT static ani_int GetFileId(ani_env *env, ani_object object);
    EXPORT static ani_int GetShareCount(ani_env *env, ani_object object);
    EXPORT static ani_int GetLcdJumpCount(ani_env *env, ani_object object);

    static thread_local PhotoAssetCustomRecord* cRecordData_;
    std::shared_ptr<PhotoAssetCustomRecord> customRecordPtr = nullptr;
    ani_env *env_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ASSET_CUSTOM_RECORD_ANI_H_