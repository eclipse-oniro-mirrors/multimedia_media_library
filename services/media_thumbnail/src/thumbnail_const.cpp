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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_const.h"

#include "media_log.h"

namespace OHOS {
namespace Media {

const std::unordered_map<ThumbnailType, std::string> TYPE_NAME_MAP = {
    { ThumbnailType::LCD, "LCD" },
    { ThumbnailType::THUMB, "THUMB" },
    { ThumbnailType::MTH, "MTH" },
    { ThumbnailType::YEAR, "YEAR" },
    { ThumbnailType::THUMB_ASTC, "THUMB_ASTC" },
    { ThumbnailType::MTH_ASTC, "MTH_ASTC" },
    { ThumbnailType::YEAR_ASTC, "YEAR_ASTC" },
};

const std::unordered_set<ThumbnailQuality> THUMBNAIL_QUALITY_SET = {
    ThumbnailQuality::POOR,
    ThumbnailQuality::NOT_BAD,
    ThumbnailQuality::MID,
    ThumbnailQuality::GOOD,
    ThumbnailQuality::DEFAULT,
    ThumbnailQuality::HIGH,
};

const std::unordered_map<GenThumbScene, std::string> GEN_THUMB_SCENE_NAME = {
    { GenThumbScene::UNDEFINED_SCENE, "UNDEFINED_SCENE" },
    { GenThumbScene::NO_AVAILABLE_THUMB, "NO_AVAILABLE_THUMB" },
    { GenThumbScene::NO_AVAILABLE_HIGHLIGHT_THUMB, "NO_AVAILABLE_HIGHLIGHT_THUMB" },
    { GenThumbScene::NO_THUMB_AND_GEN_IT_BACKGROUND, "NO_THUMB_AND_GEN_IT_BACKGROUND" },
    { GenThumbScene::NO_LCD_AND_GEN_IT_BACKGROUND, "NO_LCD_AND_GEN_IT_BACKGROUND" },
    { GenThumbScene::NO_HIGHLIGHT_THUMB_AND_GEN_IT_BACKGROUND, "NO_HIGHLIGHT_THUMB_AND_GEN_IT_BACKGROUND" },
    { GenThumbScene::CLONE_OR_DUAL_FRAME_UPGRADE, "CLONE_OR_DUAL_FRAME_UPGRADE" },
    { GenThumbScene::ADD_OR_UPDATE_MEDIA, "ADD_OR_UPDATE_MEDIA" },
    { GenThumbScene::FILM_MEDIA_GEN_THUMB_BY_PICTURE, "FILM_MEDIA_GEN_THUMB_BY_PICTURE" },
    { GenThumbScene::UPLOAD_TO_CLOUD_NEED_THUMB, "UPLOAD_TO_CLOUD_NEED_THUMB" },
    { GenThumbScene::UPLOAD_TO_CLOUD_NEED_LCD, "UPLOAD_TO_CLOUD_NEED_LCD" },
    { GenThumbScene::CLOUD_HAS_NO_ANY_THUMB, "CLOUD_HAS_NO_ANY_THUMB" },
    { GenThumbScene::CLOUD_DOWNLOAD_THUMB, "CLOUD_DOWNLOAD_THUMB" },
    { GenThumbScene::CLOUD_DOWNLOAD_ORIGINAL_MEDIA_FIX_EXIF_ROTATE, "CLOUD_DOWNLOAD_ORIGINAL_MEDIA_FIX_EXIF_ROTATE" },
    { GenThumbScene::NEED_MORE_THUMB_READY, "NEED_MORE_THUMB_READY" },
    { GenThumbScene::NO_AVAILABLE_MTH_AND_YEAR_THUMB, "NO_AVAILABLE_MTH_AND_YEAR_THUMB" },
    { GenThumbScene::REPAIR_EXIFROTATE, "REPAIR_EXIFROTATE" },
    { GenThumbScene::THUMB_IS_OBSOLETE, "THUMB_IS_OBSOLETE" },
};

std::string GetGenThumbSceneName(const GenThumbScene &scene)
{
    CHECK_AND_RETURN_RET_LOG(GEN_THUMB_SCENE_NAME.find(scene) != GEN_THUMB_SCENE_NAME.end(),
        "UNDEFINED_SCENE", "The scene: %{public}d has no corresponding name!", scene);
    return GEN_THUMB_SCENE_NAME.at(scene);
}

} // namespace Media
} // namespace OHOS