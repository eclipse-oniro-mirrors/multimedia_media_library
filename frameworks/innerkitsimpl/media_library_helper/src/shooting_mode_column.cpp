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

#include "shooting_mode_column.h"

#include <string>

#include "abs_rdb_predicates.h"
#include "datashare_predicates.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "photo_query_filter.h"
#include "value_object.h"
#include "vision_column.h"

namespace OHOS::Media {
using namespace std;
using namespace NativeRdb;

const std::vector<std::pair<ShootingModeValue, ShootingModeAlbumType>>
    ShootingModeAlbum::SHOOTING_MODE_TO_ALBUM_TYPE = {
    { ShootingModeValue::PORTRAIT_SHOOTING_MODE,       ShootingModeAlbumType::PORTRAIT },
    { ShootingModeValue::WIDE_APERTURE_SHOOTING_MODE,  ShootingModeAlbumType::WIDE_APERTURE },
    { ShootingModeValue::NIGHT_SHOT_SHOOTING_MODE,     ShootingModeAlbumType::NIGHT_SHOT },
    { ShootingModeValue::PRO_PHOTO_SHOOTING_MODE,      ShootingModeAlbumType::PRO_PHOTO },
    { ShootingModeValue::SLOW_MOTION_SHOOTING_MODE,    ShootingModeAlbumType::SLOW_MOTION },
    { ShootingModeValue::LIGHT_PAINTING_SHOOTING_MODE, ShootingModeAlbumType::LIGHT_PAINTING },
    { ShootingModeValue::HIGH_PIXEL_SHOOTING_MODE,     ShootingModeAlbumType::HIGH_PIXEL },
    { ShootingModeValue::SUPER_MACRO_SHOOTING_MODE,    ShootingModeAlbumType::SUPER_MACRO },
    { ShootingModeValue::PANORAMA_SHOOTING_MODE,       ShootingModeAlbumType::PANORAMA_MODE },
    { ShootingModeValue::TIME_LAPSE,                   ShootingModeAlbumType::TIME_LAPSE },
    { ShootingModeValue::QUICK_CAPTURE_ALBUM,          ShootingModeAlbumType::QUICK_CAPTURE_ALBUM }
};

const std::unordered_set<std::string> ShootingModeAlbum::VALID_TAGS = {
    PORTRAIT_ALBUM_TAG,
    WIDE_APERTURE_ALBUM_TAG,
    NIGHT_SHOT_ALBUM_TAG,
    REAR_CAMERA_NIGHT_SHOT_TAG,
    MOVING_PICTURE_ALBUM_TAG,
    PRO_PHOTO_ALBUM_TAG,
    TAIL_LIGHT_ALBUM_TAG,
    LIGHT_GRAFFITI_TAG,
    SILKY_WATER_TAG,
    STAR_TRACK_TAG,
    HIGH_PIXEL_ALBUM_TAG,
    AI_HIGH_PIXEL_TAG,
    SUPER_MACRO_ALBUM_TAG,
    SLOW_MOTION_ALBUM_TAG,
    SUPER_SLOW_MOTION_ALBUM_TAG,
    CAMERA_CUSTOM_SM_PANORAMA,
    CAMERA_CUSTOM_SM_PHOTO_STITCHING,
    TIME_LAPSE_TAG,
    QUICK_CAPTURE_TAG
};

template <class T>
void ShootingModeAlbum::GetMovingPhotoAlbumPredicates(T& predicates, const bool hiddenState)
{
    PhotoQueryFilter::Config config {};
    config.hiddenConfig = hiddenState ? PhotoQueryFilter::ConfigType::INCLUDE : PhotoQueryFilter::ConfigType::EXCLUDE;
    PhotoQueryFilter::ModifyPredicate(config, predicates);
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    predicates.Or();
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    predicates.EndWrap();
    predicates.EndWrap();
}

template <class T>
void ShootingModeAlbum::Get3DGSAlbumPredicates(T& predicates, const bool hiddenState)
{
    PhotoQueryFilter::Config config {};
    config.hiddenConfig = hiddenState ? PhotoQueryFilter::ConfigType::INCLUDE : PhotoQueryFilter::ConfigType::EXCLUDE;
    PhotoQueryFilter::ModifyPredicate(config, predicates);
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS));
}

template <class T>
void ShootingModeAlbum::GetBurstModeAlbumPredicates(T& predicates, const bool hiddenState)
{
    PhotoQueryFilter::Config config {};
    config.hiddenConfig = hiddenState ? PhotoQueryFilter::ConfigType::INCLUDE : PhotoQueryFilter::ConfigType::EXCLUDE;
    PhotoQueryFilter::ModifyPredicate(config, predicates);
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::BURST)));
    predicates.IsNotNull(PhotoColumn::PHOTO_BURST_KEY);
}

template <class T>
void ShootingModeAlbum::GetFrontCameraAlbumPredicates(T& predicates, const bool hiddenState)
{
    PhotoQueryFilter::Config config {};
    config.hiddenConfig = hiddenState ? PhotoQueryFilter::ConfigType::INCLUDE : PhotoQueryFilter::ConfigType::EXCLUDE;
    PhotoQueryFilter::ModifyPredicate(config, predicates);
    predicates.EqualTo(PhotoColumn::PHOTO_FRONT_CAMERA, to_string(1));
}

template <class T>
void ShootingModeAlbum::GetRAWImageAlbumPredicates(T& predicates, const bool hiddenState)
{
    const string rawImageMimeType = "image/x-adobe-dng";
    PhotoQueryFilter::Config config {};
    config.hiddenConfig = hiddenState ? PhotoQueryFilter::ConfigType::INCLUDE : PhotoQueryFilter::ConfigType::EXCLUDE;
    PhotoQueryFilter::ModifyPredicate(config, predicates);
    predicates.EqualTo(MediaColumn::MEDIA_MIME_TYPE, rawImageMimeType);
}

template <class T>
void ShootingModeAlbum::GetGeneralShootingModeAlbumPredicates(const ShootingModeAlbumType type,
    T& predicates, const bool hiddenState)
{
    PhotoQueryFilter::Config config {};
    config.hiddenConfig = hiddenState ? PhotoQueryFilter::ConfigType::INCLUDE : PhotoQueryFilter::ConfigType::EXCLUDE;
    PhotoQueryFilter::ModifyPredicate(config, predicates);
    std::string shootingModeValue = LookUpShootingModeValues(std::to_string(static_cast<int32_t>(type)));
    predicates.EqualTo(PhotoColumn::PHOTO_SHOOTING_MODE, shootingModeValue);
}

template <class T>
void ShootingModeAlbum::GetShootingModeAlbumPredicates(const ShootingModeAlbumType type,
    T& predicates, const bool hiddenState)
{
    switch (type) {
        case ShootingModeAlbumType::MOVING_PICTURE: {
            GetMovingPhotoAlbumPredicates(predicates, hiddenState);
            return;
        }
        case ShootingModeAlbumType::BURST_MODE_ALBUM: {
            GetBurstModeAlbumPredicates(predicates, hiddenState);
            return;
        }
        case ShootingModeAlbumType::FRONT_CAMERA_ALBUM: {
            GetFrontCameraAlbumPredicates(predicates, hiddenState);
            return;
        }
        case ShootingModeAlbumType::RAW_IMAGE_ALBUM: {
            GetRAWImageAlbumPredicates(predicates, hiddenState);
            return;
        }
        case ShootingModeAlbumType::MP4_3DGS_ALBUM: {
            Get3DGSAlbumPredicates(predicates, hiddenState);
            return;
        }
        default: {
            GetGeneralShootingModeAlbumPredicates(type, predicates, hiddenState);
            return;
        }
    }
}

bool ShootingModeAlbum::AlbumNameToShootingModeAlbumType(const std::string& albumName,
    ShootingModeAlbumType& parseResult)
{
    int32_t albumValue = atoi(albumName.c_str());
    if (albumValue < static_cast<int32_t>(ShootingModeAlbumType::START) ||
        albumValue > static_cast<int32_t>(ShootingModeAlbumType::END)) {
        MEDIA_ERR_LOG("album name can not be converted to shooting mode album type, input: %{public}s",
            albumName.c_str());
        return false;
    }
    parseResult = static_cast<ShootingModeAlbumType>(albumValue);
    return true;
}

string ShootingModeAlbum::GetQueryAssetsIndex(const ShootingModeAlbumType type)
{
    static const std::unordered_map<ShootingModeAlbumType, std::string> SHOOTING_MODE_INDEX_MAP = {
        {ShootingModeAlbumType::PORTRAIT, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::WIDE_APERTURE, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::NIGHT_SHOT, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::MOVING_PICTURE, PhotoColumn::PHOTO_MOVING_PHOTO_ALBUM_INDEX},
        {ShootingModeAlbumType::PRO_PHOTO, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::SLOW_MOTION, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::LIGHT_PAINTING, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::HIGH_PIXEL, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::SUPER_MACRO, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::PANORAMA_MODE, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::BURST_MODE_ALBUM, PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX},
        {ShootingModeAlbumType::FRONT_CAMERA_ALBUM, PhotoColumn::PHOTO_FRONT_CAMERA_ALBUM_INDEX},
        {ShootingModeAlbumType::RAW_IMAGE_ALBUM, PhotoColumn::PHOTO_RAW_IMAGE_ALBUM_INDEX},
        {ShootingModeAlbumType::MP4_3DGS_ALBUM, PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX},
        {ShootingModeAlbumType::TIME_LAPSE, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
        {ShootingModeAlbumType::QUICK_CAPTURE_ALBUM, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX},
    };
    if (SHOOTING_MODE_INDEX_MAP.find(type) == SHOOTING_MODE_INDEX_MAP.end()) {
        MEDIA_ERR_LOG("Shooting mode type %{public}d is not in the map", static_cast<int32_t>(type));
        return "";
    }
    return SHOOTING_MODE_INDEX_MAP.at(type);
}

bool ShootingModeAlbum::IsAssetInMovingPhotoAlbum(int32_t photoSubType, int32_t movingPhotoEffectMode)
{
    return photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        (photoSubType == static_cast<int32_t>(PhotoSubType::DEFAULT) &&
           movingPhotoEffectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}

vector<ShootingModeAlbumType> ShootingModeAlbum::GetShootingModeAlbumOfAsset(int32_t photoSubType,
    const string& mimetype, int32_t movingPhotoEffectMode, const string& frontCamera, const string& shootingMode)
{
    vector<ShootingModeAlbumType> result;
    if (photoSubType == static_cast<int32_t>(PhotoSubType::BURST)) {
        result.push_back(ShootingModeAlbumType::BURST_MODE_ALBUM);
    }
    if (photoSubType == static_cast<int32_t>(PhotoSubType::SPATIAL_3DGS)) {
        result.push_back(ShootingModeAlbumType::MP4_3DGS_ALBUM);
    }
    if (mimetype == "image/x-adobe-dng") {
        result.push_back(ShootingModeAlbumType::RAW_IMAGE_ALBUM);
    }
    if (IsAssetInMovingPhotoAlbum(photoSubType, movingPhotoEffectMode)) {
        result.push_back(ShootingModeAlbumType::MOVING_PICTURE);
    }
    if (frontCamera == "1") { // "1" means photo is taken using front camera
        result.push_back(ShootingModeAlbumType::FRONT_CAMERA_ALBUM);
    }

    auto shootingModeAlbumType =
        LookUpShootingModeAlbumType(static_cast<ShootingModeValue>(std::atoi(shootingMode.c_str())));
    if (!shootingModeAlbumType.empty()) {
        ShootingModeAlbumType type = static_cast<ShootingModeAlbumType>(std::atoi(shootingModeAlbumType.c_str()));
        result.push_back(type);
    }
    return result;
}

string ShootingModeAlbum::MapShootingModeTagToShootingMode(const string& tag)
{
    static const std::unordered_map<std::string, std::string> SHOOTING_MODE_CAST_MAP = {
        {PORTRAIT_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::PORTRAIT_SHOOTING_MODE))},
        {WIDE_APERTURE_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::WIDE_APERTURE_SHOOTING_MODE))},
        {NIGHT_SHOT_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::NIGHT_SHOT_SHOOTING_MODE))},
        {REAR_CAMERA_NIGHT_SHOT_TAG, to_string(static_cast<int>(ShootingModeValue::NIGHT_SHOT_SHOOTING_MODE))},
        {MOVING_PICTURE_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::MOVING_PICTURE_SHOOTING_MODE))},
        {PRO_PHOTO_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::PRO_PHOTO_SHOOTING_MODE))},
        {TAIL_LIGHT_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::LIGHT_PAINTING_SHOOTING_MODE))},
        {LIGHT_GRAFFITI_TAG, to_string(static_cast<int>(ShootingModeValue::LIGHT_PAINTING_SHOOTING_MODE))},
        {SILKY_WATER_TAG, to_string(static_cast<int>(ShootingModeValue::LIGHT_PAINTING_SHOOTING_MODE))},
        {STAR_TRACK_TAG, to_string(static_cast<int>(ShootingModeValue::LIGHT_PAINTING_SHOOTING_MODE))},
        {AI_HIGH_PIXEL_TAG, to_string(static_cast<int>(ShootingModeValue::HIGH_PIXEL_SHOOTING_MODE))},
        {HIGH_PIXEL_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::HIGH_PIXEL_SHOOTING_MODE))},
        {SUPER_MACRO_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::SUPER_MACRO_SHOOTING_MODE))},
        {SLOW_MOTION_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::SLOW_MOTION_SHOOTING_MODE))},
        {SUPER_SLOW_MOTION_ALBUM_TAG, to_string(static_cast<int>(ShootingModeValue::SLOW_MOTION_SHOOTING_MODE))},
        {CAMERA_CUSTOM_SM_PANORAMA, to_string(static_cast<int>(ShootingModeValue::PANORAMA_SHOOTING_MODE))},
        {CAMERA_CUSTOM_SM_PHOTO_STITCHING, to_string(static_cast<int>(ShootingModeValue::PANORAMA_SHOOTING_MODE))},
        {TIME_LAPSE_TAG, to_string(static_cast<int>(ShootingModeValue::TIME_LAPSE))},
        {QUICK_CAPTURE_TAG, to_string(static_cast<int>(ShootingModeValue::QUICK_CAPTURE_ALBUM))},
    };

    auto it = SHOOTING_MODE_CAST_MAP.find(tag);
    if (it != SHOOTING_MODE_CAST_MAP.end()) {
        return it->second;
    }
    return "";
}

string ShootingModeAlbum::LookUpShootingModeAlbumType(const ShootingModeValue value)
{
    for (const auto& pair : SHOOTING_MODE_TO_ALBUM_TYPE) {
        if (pair.first == value) {
            return std::to_string(static_cast<int32_t>(pair.second));
        }
    }
    return "";
}

string ShootingModeAlbum::LookUpShootingModeValues(const std::string albumType)
{
    ShootingModeAlbumType type = static_cast<ShootingModeAlbumType>(std::atoi(albumType.c_str()));
    for (const auto& pair : SHOOTING_MODE_TO_ALBUM_TYPE) {
        if (pair.second == type) {
            return std::to_string(static_cast<int32_t>(pair.first));
        }
    }
    return "";
}

template void ShootingModeAlbum::GetMovingPhotoAlbumPredicates<DataShare::DataSharePredicates>(
    DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetMovingPhotoAlbumPredicates<NativeRdb::RdbPredicates>(
    NativeRdb::RdbPredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::Get3DGSAlbumPredicates<DataShare::DataSharePredicates>(
    DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::Get3DGSAlbumPredicates<NativeRdb::RdbPredicates>(
    NativeRdb::RdbPredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetBurstModeAlbumPredicates<DataShare::DataSharePredicates>(
    DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetBurstModeAlbumPredicates<NativeRdb::RdbPredicates>(
    NativeRdb::RdbPredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetFrontCameraAlbumPredicates<DataShare::DataSharePredicates>(
    DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetFrontCameraAlbumPredicates<NativeRdb::RdbPredicates>(
    NativeRdb::RdbPredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetRAWImageAlbumPredicates<DataShare::DataSharePredicates>(
    DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetRAWImageAlbumPredicates<NativeRdb::RdbPredicates>(
    NativeRdb::RdbPredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetGeneralShootingModeAlbumPredicates<DataShare::DataSharePredicates>(
    const ShootingModeAlbumType type, DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetGeneralShootingModeAlbumPredicates<NativeRdb::RdbPredicates>(
    const ShootingModeAlbumType type, NativeRdb::RdbPredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetShootingModeAlbumPredicates<DataShare::DataSharePredicates>(
    const ShootingModeAlbumType type, DataShare::DataSharePredicates& predicates, const bool hiddenState);

template void ShootingModeAlbum::GetShootingModeAlbumPredicates<NativeRdb::RdbPredicates>(
    const ShootingModeAlbumType type, NativeRdb::RdbPredicates& predicates, const bool hiddenState);

} // namespace OHOS::Media