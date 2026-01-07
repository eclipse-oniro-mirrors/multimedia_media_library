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

#include "media_library_enum_ani.h"

#include "ani_class_name.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media {
static const std::map<MediaType, int32_t> ANI_MEDIATYPE_INDEX_MAP = {
    {MediaType::MEDIA_TYPE_IMAGE, 0},
    {MediaType::MEDIA_TYPE_VIDEO, 1},
};

static const std::map<PhotoSubType, int32_t> ANI_PHOTOSUBTYPE_INDEX_MAP = {
    {PhotoSubType::DEFAULT, 0},
    {PhotoSubType::SCREENSHOT, 1},
    {PhotoSubType::MOVING_PHOTO, 2},
    {PhotoSubType::BURST, 3},
};

static const std::map<DynamicRangeType, int32_t> ANI_DYNAMICRANGETYPE_INDEX_MAP = {
    {DynamicRangeType::SDR, 0},
    {DynamicRangeType::HDR, 1},
};

static const std::map<PhotoPositionType, int32_t> ANI_PHOTOPOSITIONTYPE_INDEX_MAP = {
    {PhotoPositionType::LOCAL, 0},
    {PhotoPositionType::CLOUD, 1},
};

static const std::map<PhotoAlbumType, int32_t> ANI_PHOTOALBUMTYPE_INDEX_MAP = {
    {PhotoAlbumType::USER, 0},
    {PhotoAlbumType::SYSTEM, 1},
    {PhotoAlbumType::SOURCE, 2},
    {PhotoAlbumType::SMART, 3},
};

static const std::map<PhotoAlbumSubType, int32_t> ANI_PHOTOALBUMSUBTYPE_INDEX_MAP = {
    {PhotoAlbumSubType::USER_GENERIC, 0},
    {PhotoAlbumSubType::FAVORITE, 1},
    {PhotoAlbumSubType::VIDEO, 2},
    {PhotoAlbumSubType::HIDDEN, 3},
    {PhotoAlbumSubType::TRASH, 4},
    {PhotoAlbumSubType::SCREENSHOT, 5},
    {PhotoAlbumSubType::CAMERA, 6},
    {PhotoAlbumSubType::IMAGE, 7},
    {PhotoAlbumSubType::CLOUD_ENHANCEMENT, 8},
    {PhotoAlbumSubType::SOURCE_GENERIC, 9},
    {PhotoAlbumSubType::CLASSIFY, 10},
    {PhotoAlbumSubType::GEOGRAPHY_LOCATION, 11},
    {PhotoAlbumSubType::GEOGRAPHY_CITY, 12},
    {PhotoAlbumSubType::SHOOTING_MODE, 13},
    {PhotoAlbumSubType::PORTRAIT, 14},
    {PhotoAlbumSubType::GROUP_PHOTO, 15},
    {PhotoAlbumSubType::HIGHLIGHT, 16},
    {PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS, 17},
    {PhotoAlbumSubType::ANY, 18},
};

static const std::map<NotifyType, int32_t> ANI_NOTIFYTYPE_INDEX_MAP = {
    {NotifyType::NOTIFY_ADD, 0},
    {NotifyType::NOTIFY_UPDATE, 1},
    {NotifyType::NOTIFY_REMOVE, 2},
    {NotifyType::NOTIFY_ALBUM_ADD_ASSET, 3},
    {NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, 4},
};

static const std::map<MovingPhotoEffectMode, int32_t> ANI_MOVINGPHOTOEFFECTMODE_INDEX_MAP = {
    {MovingPhotoEffectMode::DEFAULT, 0},
    {MovingPhotoEffectMode::BOUNCE_PLAY, 1},
    {MovingPhotoEffectMode::LOOP_PLAY, 2},
    {MovingPhotoEffectMode::LONG_EXPOSURE, 3},
    {MovingPhotoEffectMode::MULTI_EXPOSURE, 4},
    {MovingPhotoEffectMode::CINEMA_GRAPH, 5},
    {MovingPhotoEffectMode::IMAGE_ONLY, 6},
};

static const std::map<CloudEnhancementTaskStage, int32_t> ANI_CLOUDENHANCEMENTTASKSTAGE_INDEX_MAP = {
    {CloudEnhancementTaskStage::TASK_STAGE_EXCEPTION, 0},
    {CloudEnhancementTaskStage::TASK_STAGE_PREPARING, 1},
    {CloudEnhancementTaskStage::TASK_STAGE_UPLOADING, 2},
    {CloudEnhancementTaskStage::TASK_STAGE_EXECUTING, 3},
    {CloudEnhancementTaskStage::TASK_STAGE_DOWNLOADING, 4},
    {CloudEnhancementTaskStage::TASK_STAGE_FAILED, 5},
    {CloudEnhancementTaskStage::TASK_STAGE_COMPLETED, 6},
};

static const std::map<CoverUriSource, int32_t> ANI_COVERURISOURCE_INDEX_MAP = {
    {CoverUriSource::DEFAULT_COVER, 0},
    {CoverUriSource::MANUAL_CLOUD_COVER, 1},
    {CoverUriSource::MANUAL_LOCAL_COVER, 2},
    {CoverUriSource::MANUAL_WAIT_PULL_COVER, 3},
};

static const std::map<NotifyChangeType, int32_t> ANI_NOTIFYCHANGETYPE_INDEX_MAP = {
    {NotifyChangeType::NOTIFY_CHANGE_ADD, 0},
    {NotifyChangeType::NOTIFY_CHANGE_UPDATE, 1},
    {NotifyChangeType::NOTIFY_CHANGE_REMOVE, 2},
    {NotifyChangeType::NOTIFY_CHANGE_YUV_READY, 3},
};

static const std::map<PhotoType, int32_t> ANI_PHOTOTYPE_INDEX_MAP = {
    {PhotoType::PHOTOTYPE_IMAGE, 0},
    {PhotoType::PHOTOTYPE_VIDEO, 1},
};

static const std::map<StrongAssociationType, int32_t> ANI_STRONGASSOCIATIONTYPE_INDEX_MAP = {
    {StrongAssociationType::NORMAL, 0},
    {StrongAssociationType::CLOUD_ENHANCEMENT, 1},
};

static const std::map<ThumbnailVisibility, int32_t> ANI_THUMBNAILVISIBILITY_INDEX_MAP = {
    {ThumbnailVisibility::INVISIBLE, 0},
    {ThumbnailVisibility::VISIBLE, 1},
};

ani_status MediaLibraryEnumAni::EnumGetValueInt32(ani_env *env, ani_enum_item enumItem, int32_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    ani_int aniInt {};
    CHECK_STATUS_RET(env->EnumItem_GetValue_Int(enumItem, &aniInt), "EnumItem_GetValue_Int failed");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, aniInt, value), "GetInt32 failed");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::EnumGetValueString(ani_env *env, ani_enum_item enumItem, std::string &value)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    ani_string aniString {};
    CHECK_STATUS_RET(env->EnumItem_GetValue_String(enumItem, &aniString), "EnumItem_GetValue_String failed");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, aniString, value), "GetString failed");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, MediaType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_MEDIATYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_MEDIATYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_PHOTO_TYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, PhotoSubType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_PHOTOSUBTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_PHOTOSUBTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_PHOTO_SUBTYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, DynamicRangeType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_DYNAMICRANGETYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_DYNAMICRANGETYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_DYNAMIC_RANGE_TYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, PhotoPositionType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_PHOTOPOSITIONTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_PHOTOPOSITIONTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_POSITION_TYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, PhotoAlbumType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_PHOTOALBUMTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_PHOTOALBUMTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_ALBUM_TYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, PhotoAlbumSubType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_PHOTOALBUMSUBTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_PHOTOALBUMSUBTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_ALBUM_SUBTYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, NotifyType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_NOTIFYTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_NOTIFYTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS, "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_NOTIFY_TYPE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, MovingPhotoEffectMode value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_MOVINGPHOTOEFFECTMODE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_MOVINGPHOTOEFFECTMODE_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_MOVING_PHOTO_EFFECT_MODE.c_str(), &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, CloudEnhancementTaskStage value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_CLOUDENHANCEMENTTASKSTAGE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_CLOUDENHANCEMENTTASKSTAGE_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_CLOUD_ENHANCEMENT_TASK_STAGE.c_str(),
        &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, CoverUriSource value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_COVERURISOURCE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_COVERURISOURCE_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_COVERURI_SOURCE.c_str(),
        &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, NotifyChangeType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_NOTIFYCHANGETYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_NOTIFYCHANGETYPE_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_NOTIFYCHANGE_TYPE.c_str(),
        &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, PhotoType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_PHOTOTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_PHOTOTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_PHOTO_TYPE.c_str(),
        &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, ThumbnailVisibility value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_THUMBNAILVISIBILITY_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_THUMBNAILVISIBILITY_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_THUMBNAIL_VISIBILITY_TYPE.c_str(),
        &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

ani_status MediaLibraryEnumAni::ToAniEnum(ani_env *env, StrongAssociationType value, ani_enum_item &aniEnumItem)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "Invalid env");

    auto it = ANI_STRONGASSOCIATIONTYPE_INDEX_MAP.find(value);
    CHECK_COND_RET(it != ANI_STRONGASSOCIATIONTYPE_INDEX_MAP.end(), ANI_INVALID_ARGS,
        "Unsupport enum: %{public}d", value);
    ani_int enumIndex = static_cast<ani_int>(it->second);

    ani_enum aniEnum {};
    CHECK_STATUS_RET(env->FindEnum(PAH_ANI_CLASS_ENUM_STRONG_ASSOCIATION_TYPE.c_str(),
        &aniEnum), "Find Enum Fail");
    CHECK_STATUS_RET(env->Enum_GetEnumItemByIndex(aniEnum, enumIndex, &aniEnumItem), "Find Enum item Fail");
    return ANI_OK;
}

} // namespace Media
} // namespace OHOS