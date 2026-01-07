/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_SHOOTING_MODE_COLUMN_H
#define INTERFACES_INNERKITS_SHOOTING_MODE_COLUMN_H

#include <string>
#include <unordered_set>
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))
/* ShootingMode album const values */
constexpr int32_t SHOOTING_MODE_TYPE = PhotoAlbumType::SMART;
constexpr int32_t SHOOTING_MODE_SUB_TYPE = PhotoAlbumSubType::SHOOTING_MODE;

/* ShootingMode tag value wrote by camera */
const std::string PORTRAIT_ALBUM_TAG = "23";
const std::string WIDE_APERTURE_ALBUM_TAG = "19";
const std::string NIGHT_SHOT_ALBUM_TAG = "7";
const std::string REAR_CAMERA_NIGHT_SHOT_TAG = "42";
const std::string MOVING_PICTURE_ALBUM_TAG = "20";
const std::string PRO_PHOTO_ALBUM_TAG = "2";
const std::string TAIL_LIGHT_ALBUM_TAG = "9";
const std::string LIGHT_GRAFFITI_TAG = "10";
const std::string SILKY_WATER_TAG = "11";
const std::string STAR_TRACK_TAG = "12";
const std::string HIGH_PIXEL_ALBUM_TAG = "53";
const std::string AI_HIGH_PIXEL_TAG = "52";
const std::string SUPER_MACRO_ALBUM_TAG = "47";
const std::string SLOW_MOTION_ALBUM_TAG = "TypeSlowMotion";
const std::string SUPER_SLOW_MOTION_ALBUM_TAG = "TypeSSlowMotion";
const std::string CAMERA_CUSTOM_SM_PANORAMA = "8";
const std::string CAMERA_CUSTOM_SM_PHOTO_STITCHING = "77";
const std::string TIME_LAPSE_TAG = "TypeTimeLapse";
const std::string QUICK_CAPTURE_TAG = "62";

/* ShootingModeAlbumName in medialibrary */
enum class ShootingModeAlbumType : int32_t {
    START = 1,
    PORTRAIT = 1,
    WIDE_APERTURE,
    NIGHT_SHOT,
    MOVING_PICTURE,
    PRO_PHOTO,
    SLOW_MOTION,
    LIGHT_PAINTING,
    HIGH_PIXEL,
    SUPER_MACRO,
    PANORAMA_MODE,
    BURST_MODE_ALBUM,
    FRONT_CAMERA_ALBUM,
    RAW_IMAGE_ALBUM,
    MP4_3DGS_ALBUM,
    TIME_LAPSE,
    QUICK_CAPTURE_ALBUM,
    END = QUICK_CAPTURE_ALBUM
};

/* ShootingMode value in medialibrary */
enum class ShootingModeValue : int32_t {
    START = 1,
    PORTRAIT_SHOOTING_MODE = 1,
    WIDE_APERTURE_SHOOTING_MODE,
    NIGHT_SHOT_SHOOTING_MODE,
    MOVING_PICTURE_SHOOTING_MODE,
    PRO_PHOTO_SHOOTING_MODE,
    SLOW_MOTION_SHOOTING_MODE,
    LIGHT_PAINTING_SHOOTING_MODE,
    HIGH_PIXEL_SHOOTING_MODE,
    SUPER_MACRO_SHOOTING_MODE,
    PANORAMA_SHOOTING_MODE,
    TIME_LAPSE,
    QUICK_CAPTURE_ALBUM,
    END = QUICK_CAPTURE_ALBUM
};

class ShootingModeAlbum {
public:
    template <class T>
    EXPORT static void GetMovingPhotoAlbumPredicates(T& predicates,
        const bool hiddenState);

    template <class T>
    EXPORT static void Get3DGSAlbumPredicates(T& predicates, const bool hiddenState);

    template <class T>
    EXPORT static void GetBurstModeAlbumPredicates(T& predicates, const bool hiddenState);

    template <class T>
    EXPORT static void GetFrontCameraAlbumPredicates(T& predicates, const bool hiddenState);

    template <class T>
    EXPORT static void GetRAWImageAlbumPredicates(T& predicates, const bool hiddenState);

    template <class T>
    EXPORT static void GetGeneralShootingModeAlbumPredicates(const ShootingModeAlbumType type, T& predicates,
        const bool hiddenState);

    template <class T>
    EXPORT static void GetShootingModeAlbumPredicates(const ShootingModeAlbumType type,
        T& predicates, const bool hiddenState);

    EXPORT static bool AlbumNameToShootingModeAlbumType(const std::string& albumName,
        ShootingModeAlbumType& parseResult);
    EXPORT static std::string GetQueryAssetsIndex(const ShootingModeAlbumType type);
    EXPORT static bool IsAssetInMovingPhotoAlbum(int32_t photoSubType, int32_t movingPhotoEffectMode);
    EXPORT static std::string MapShootingModeTagToShootingMode(const std::string& tag);

    // Get all shooting mode albums that the asset is in.
    EXPORT static std::vector<ShootingModeAlbumType> GetShootingModeAlbumOfAsset(int32_t photoSubType,
        const std::string& mimetype, int32_t movingPhotoEffectMode, const std::string& frontCamera,
        const std::string& shootingMode);

    EXPORT static std::string LookUpShootingModeAlbumType(const ShootingModeValue value);
    EXPORT static std::string LookUpShootingModeValues(const std::string albumType);

    static const std::vector<std::pair<ShootingModeValue, ShootingModeAlbumType>> SHOOTING_MODE_TO_ALBUM_TYPE;
    static const std::unordered_set<std::string> VALID_TAGS;
};
} // namespace Media
} // namespace OHOS
#endif // INTERFACES_INNERKITS_SHOOTING_MODE_COLUMN_H