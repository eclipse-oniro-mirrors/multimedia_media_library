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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_LIBRARY_ENUM_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_LIBRARY_ENUM_ANI_H

#include <ani.h>
#include <string>

#include "userfile_manager_types.h"
#include "media_asset_data_handler_ani.h"
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media {
class MediaLibraryEnumAni {
public:
    static ani_status EnumGetValueInt32(ani_env *env, ani_enum_item enumItem, int32_t &value);
    static ani_status EnumGetValueString(ani_env *env, ani_enum_item enumItem, std::string &value);

    static ani_status ToAniEnum(ani_env *env, MediaType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, PhotoSubType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, DynamicRangeType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, PhotoPositionType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, PhotoAlbumType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, PhotoAlbumSubType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, NotifyType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, MovingPhotoEffectMode value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, CloudEnhancementTaskStage value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, CoverUriSource value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, NotifyChangeType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, PhotoType value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, ThumbnailVisibility value, ani_enum_item &aniEnumItem);
    static ani_status ToAniEnum(ani_env *env, StrongAssociationType value, ani_enum_item &aniEnumItem);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_LIBRARY_ENUM_ANI_H