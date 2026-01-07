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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PLAYBACK_FORMATS_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PLAYBACK_FORMATS_H_
#include <stdint.h>
#include "mtp_constants.h"

static const uint16_t PLAYBACK_FORMATS[] = {
    MTP_FORMAT_UNDEFINED_CODE,
    MTP_FORMAT_ASSOCIATION_CODE,
    MTP_FORMAT_TEXT_CODE,
    MTP_FORMAT_HTML_CODE,
    MTP_FORMAT_WAV_CODE,
    MTP_FORMAT_MP3_CODE,
    MTP_FORMAT_MPEG_CODE,
    MTP_FORMAT_EXIF_JPEG_CODE,
    MTP_FORMAT_TIFF_EP_CODE,
    MTP_FORMAT_BMP_CODE,
    MTP_FORMAT_GIF_CODE,
    MTP_FORMAT_JFIF_CODE,
    MTP_FORMAT_PNG_CODE,
    MTP_FORMAT_TIFF_CODE,
    MTP_FORMAT_WMA_CODE,
    MTP_FORMAT_OGG_CODE,
    MTP_FORMAT_AAC_CODE,
    MTP_FORMAT_MP4_CONTAINER_CODE,
    MTP_FORMAT_MP2_CODE,
    MTP_FORMAT_3GP_CONTAINER_CODE,
    MTP_FORMAT_ABSTRACT_AUDIO_VIDEO_PLAYLIST_CODE,
    MTP_FORMAT_WPL_PLAYLIST_CODE,
    MTP_FORMAT_M3U_PLAYLIST_CODE,
    MTP_FORMAT_PLS_PLAYLIST_CODE,
    MTP_FORMAT_XML_DOCUMENT_CODE,
    MTP_FORMAT_FLAC_CODE,
    MTP_FORMAT_DNG_CODE,
    MTP_FORMAT_HEIF_CODE,
};

#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PLAYBACK_FORMATS_H_
