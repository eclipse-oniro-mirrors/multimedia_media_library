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
};

enum HighlightAlbumInfoType : int32_t {
    INVALID_INFO = -1,
    COVER_INFO,
    PLAY_INFO
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

enum PhotoAlbumType : int32_t {
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
    SYSTEM_END = IMAGE,
    SOURCE_GENERIC = 2049,
    ANALYSIS_START = 4097,
    CLASSIFY = ANALYSIS_START,
    GEOGRAPHY_LOCATION = 4099,
    GEOGRAPHY_CITY,
    SHOOTING_MODE,
    PORTRAIT,
    HIGHLIGHT = 4104,
    HIGHLIGHT_SUGGESTIONS,
    ANALYSIS_END = HIGHLIGHT_SUGGESTIONS,
    ANY = std::numeric_limits<int32_t>::max()
};

enum class PhotoSubType : int32_t {
    DEFAULT,
    SCREENSHOT,
    CAMERA,
    MOVING_PHOTO,
    SUBTYPE_END
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
    NOTIFY_THUMB_UPDATE
};

enum class RequestPhotoType : int32_t {
    REQUEST_ALL_THUMBNAILS = 0,
    REQUEST_FAST_THUMBNAIL = 1,
    REQUEST_QUALITY_THUMBNAIL = 2,
    REQUEST_TYPE_END
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_FILEMANAGEMENT_USERFILEMGR_TYPES_H
