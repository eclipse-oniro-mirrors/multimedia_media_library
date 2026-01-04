/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_FILEMANAGEMENT_USERFILEMGR_TYPES_H
#define OHOS_FILEMANAGEMENT_USERFILEMGR_TYPES_H

#include <limits>
#include <string>

namespace OHOS {
namespace Media {
enum class ResultNapiType {
    TYPE_MEDIALIBRARY,
    TYPE_USERFILE_MGR,
    TYPE_PHOTOACCESS_HELPER,
    TYPE_NAPI_MAX
};

enum MediaType {
    MEDIA_TYPE_FILE,
    MEDIA_TYPE_IMAGE,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_MEDIA,
    MEDIA_TYPE_ALBUM_LIST,
    MEDIA_TYPE_ALBUM_LIST_INFO,
    MEDIA_TYPE_ALBUM,
    MEDIA_TYPE_SMARTALBUM,
    MEDIA_TYPE_DEVICE,
    MEDIA_TYPE_REMOTEFILE,
    MEDIA_TYPE_NOFILE,
    MEDIA_TYPE_PHOTO,
    MEDIA_TYPE_ALL,
    MEDIA_TYPE_DEFAULT,
};

enum class ResourceType {
    INVALID_RESOURCE = -1,
    IMAGE_RESOURCE = 1, // corresponds to MEDIA_TYPE_IMAGE
    VIDEO_RESOURCE,     // corresponds to MEDIA_TYPE_VIDEO
    PHOTO_PROXY,
    PRIVATE_MOVING_PHOTO_RESOURCE,
    PRIVATE_MOVING_PHOTO_METADATA,
};

enum AnalysisType : int32_t {
    ANALYSIS_INVALID = -1,
    ANALYSIS_AESTHETICS_SCORE,
    ANALYSIS_LABEL,
    ANALYSIS_OCR,
    ANALYSIS_FACE,
    ANALYSIS_OBJECT,
    ANALYSIS_RECOMMENDATION,
    ANALYSIS_SEGMENTATION,
    ANALYSIS_COMPOSITION,
    ANALYSIS_SALIENCY,
    ANALYSIS_DETAIL_ADDRESS,
    ANALYSIS_HUMAN_FACE_TAG,
    ANALYSIS_HEAD_POSITION,
    ANALYSIS_BONE_POSE,
    ANALYSIS_VIDEO_LABEL,
    ANALYSIS_HIGHLIGHT,
    ANALYSIS_MULTI_CROP,
    ANALYSIS_SEARCH_INDEX,
    ANALYSIS_VIDEO_AESTHETICS,
    ANALYSIS_PET_TAG,
    ANALYSIS_PET_FACE
};

enum HighlightAlbumInfoType : int32_t {
    INVALID_INFO = -1,
    COVER_INFO,
    PLAY_INFO,
    ALBUM_INFO
};

enum HighlightUserActionType : int32_t {
    INVALID_USER_ACTION = -1,
    INSERTED_PIC_COUNT,
    REMOVED_PIC_COUNT,
    SHARED_SCREENSHOT_COUNT,
    SHARED_COVER_COUNT,
    RENAMED_COUNT,
    CHANGED_COVER_COUNT,
    RENDER_VIEWED_TIMES = 100,
    RENDER_VIEWED_DURATION,
    ART_LAYOUT_VIEWED_TIMES,
    ART_LAYOUT_VIEWED_DURATION
};

enum HighlightAlbumChangeAttribute : int32_t {
    INVALID_CHANGE_ATTRIBUTE = -1,
    IS_VIEWED = 0,
    NOTIFICATION_TIME = 1,
    IS_FAVORITE = 2
};

enum PhotoAlbumType : int32_t {
    INVALID = -1,
    USER = 0,
    SYSTEM = 1024,
    SOURCE = 2048,
    SMART = 4096
};

enum PhotoAlbumSubType : int32_t {
    USER_GENERIC = 1,

    SYSTEM_START = 1025,
    FAVORITE = SYSTEM_START,
    VIDEO,
    HIDDEN,
    TRASH,
    SCREENSHOT,
    CAMERA,
    IMAGE,
    CLOUD_ENHANCEMENT,
    SYSTEM_END = CLOUD_ENHANCEMENT,
    SOURCE_GENERIC = 2049,
    ANALYSIS_START = 4097,
    CLASSIFY = ANALYSIS_START,
    GEOGRAPHY_LOCATION = 4099,
    GEOGRAPHY_CITY,
    SHOOTING_MODE = 4101,
    PORTRAIT = 4102,
    GROUP_PHOTO = 4103,
    HIGHLIGHT = 4104,
    HIGHLIGHT_SUGGESTIONS,
    PET = 4106,
    ANALYSIS_END = PET,
    ANY = std::numeric_limits<int32_t>::max()
};

enum class PhotoAlbumId : int32_t {
    DEFAULT = 0,
    TRASH = -3,
    HIDDEN = -4
};

enum class PhotoSubType : int32_t {
    DEFAULT,
    SCREENSHOT,
    CAMERA,
    MOVING_PHOTO,
    BURST,
    CINEMATIC_VIDEO,
    SLOW_MOTION_VIDEO,
    SPATIAL_3DGS = 7,
    SUBTYPE_END
};

enum class DynamicRangeType : int32_t {
    SDR,
    HDR
};

enum class HdrMode : int32_t {
    DEFAULT = 0,
    HDR_ISO_SINGLE = 1,
    HDR_ISO_DUAL = 2,
    HDR_CUVA = 3,
    HDR_VIVID_SINGLE = 4,
    HDR_VIVID_DUAL = 5,
};

enum class MovingPhotoEffectMode : int32_t {
    EFFECT_MODE_START = 0,
    DEFAULT = EFFECT_MODE_START,
    BOUNCE_PLAY,
    LOOP_PLAY,
    LONG_EXPOSURE,
    MULTI_EXPOSURE,
    CINEMA_GRAPH,
    EFFECT_MODE_END = CINEMA_GRAPH,
    IMAGE_ONLY = 10
};

enum class PhotoPositionType : int32_t {
    INVALID = -1,
    LOCAL = 1,
    CLOUD = 2,
    LOCAL_AND_CLOUD = 3
};

enum class VideoMode : int32_t {
    DEFAULT = -1,
    NOT_LOG_VIDEO = 0,
    LOG_VIDEO = 1
};

enum class PhotoThumbStatusType : int32_t {
    DOWNLOADED = 0,
    ONLY_THM_DOWNLOADED = 1,
    ONLY_LCD_DOWNLOADED = 2,
    NOT_DOWNLOADED = 3
};

enum class CloudEnhancementTaskStage : int32_t {
    TASK_STAGE_EXCEPTION = -1,
    TASK_STAGE_PREPARING,
    TASK_STAGE_UPLOADING,
    TASK_STAGE_EXECUTING,
    TASK_STAGE_DOWNLOADING,
    TASK_STAGE_FAILED,
    TASK_STAGE_COMPLETED
};

enum class CloudEnhancementState : int32_t {
    UNAVAILABLE = 0,
    AVAILABLE,
    EXECUTING,
    COMPLETED
};

enum class VideoEnhancementType : int32_t {
    QUALITY_ENHANCEMENT_LOCAL,
    QUALITY_ENHANCEMENT_CLOUD,
    QUALITY_ENHANCEMENT_LOCAL_AND_CLOUD,
};

const std::string URI_PARAM_API_VERSION = "api_version";

enum class MediaLibraryApi : uint32_t {
    API_START = 8,
    API_OLD = 9,
    API_10,
    API_END
};

enum NotifyType {
    NOTIFY_ADD,
    NOTIFY_UPDATE,
    NOTIFY_REMOVE,
    NOTIFY_ALBUM_ADD_ASSET,
    NOTIFY_ALBUM_REMOVE_ASSET,
    NOTIFY_ALBUM_DISMISS_ASSET,
    NOTIFY_THUMB_ADD,
    NOTIFY_THUMB_UPDATE,
    NOTIFY_INVALID
};

enum class RequestPhotoType : int32_t {
    REQUEST_ALL_THUMBNAILS = 0,
    REQUEST_FAST_THUMBNAIL = 1,
    REQUEST_QUALITY_THUMBNAIL = 2,
    REQUEST_TYPE_END
};

enum class BurstCoverLevelType : int32_t {
    COVER = 1,
    MEMBER = 2,
    DEFAULT = COVER
};

enum class CoverSatisfiedType : uint8_t {
    NO_SETTING = 0,
    DEFAULT_SETTING = 1,
    USER_SETTING = 2,
    USER_SETTING_EDITE = 3,
    BEAUTY_SETTING = 4,
    BEAUTY_SETTING_EDITE = 5
};

enum class WatermarkType : int32_t {
    DEFAULT = 0,
    BRAND_COMMON,
    COMMON,
    BRAND
};

enum CoverUriSource : int32_t {
    DEFAULT_COVER = 0,
    MANUAL_CLOUD_COVER = 1,
    MANUAL_LOCAL_COVER = 2,
    MANUAL_WAIT_PULL_COVER = 3,
};

enum OrderStyleType : int32_t {
    MIX = 0,
    SPLIT = 1
};

enum NotifyChangeType: int32_t {
    NOTIFY_CHANGE_INVALID = -1,
    NOTIFY_CHANGE_ADD = 0,
    NOTIFY_CHANGE_UPDATE,
    NOTIFY_CHANGE_REMOVE,
    NOTIFY_CHANGE_YUV_READY,
};

enum PhotoType: int32_t {
    PHOTOTYPE_IMAGE = 1,
    PHOTOTYPE_VIDEO = 2,
};

enum ThumbnailVisibility: int32_t {
    INVISIBLE = 0,
    VISIBLE = 1
};

} // namespace Media
} // namespace OHOS
#endif // OHOS_FILEMANAGEMENT_USERFILEMGR_TYPES_H