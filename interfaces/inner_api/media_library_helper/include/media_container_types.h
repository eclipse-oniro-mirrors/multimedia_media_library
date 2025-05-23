/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_CONTAINER_TYPES_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_CONTAINER_TYPES_H

#include <string>

namespace OHOS {
namespace Media {

/** Supported audio container types */
const std::string AUDIO_CONTAINER_TYPE_AAC = "aac";
const std::string AUDIO_CONTAINER_TYPE_MP3 = "mp3";
const std::string AUDIO_CONTAINER_TYPE_FLAC = "flac";
const std::string AUDIO_CONTAINER_TYPE_WAV = "wav";
const std::string AUDIO_CONTAINER_TYPE_OGG = "ogg";
const std::string AUDIO_CONTAINER_TYPE_M4A = "m4a";

/** Supported video container types */
const std::string VIDEO_CONTAINER_TYPE_MP4 = "mp4";
const std::string VIDEO_CONTAINER_TYPE_3GP = "3gp";
const std::string VIDEO_CONTAINER_TYPE_MPG = "mpg";
const std::string VIDEO_CONTAINER_TYPE_MOV = "mov";
const std::string VIDEO_CONTAINER_TYPE_WEBM = "webm";
const std::string VIDEO_CONTAINER_TYPE_MKV = "mkv";
const std::string VIDEO_CONTAINER_TYPE_H264 = "h264";
const std::string VIDEO_CONTAINER_TYPE_MPEG = "mpeg";
const std::string VIDEO_CONTAINER_TYPE_TS = "ts";
const std::string VIDEO_CONTAINER_TYPE_M4V = "m4v";
const std::string VIDEO_CONTAINER_TYPE_3G2 = "3g2";

/** Supported image types */
const std::string IMAGE_CONTAINER_TYPE_BMP = "bmp";
const std::string IMAGE_CONTAINER_TYPE_BM = "bm";
const std::string IMAGE_CONTAINER_TYPE_GIF = "gif";
const std::string IMAGE_CONTAINER_TYPE_JPG = "jpg";
const std::string IMAGE_CONTAINER_TYPE_JPEG = "jpeg";
const std::string IMAGE_CONTAINER_TYPE_JPE = "jpe";
const std::string IMAGE_CONTAINER_TYPE_PNG = "png";
const std::string IMAGE_CONTAINER_TYPE_WEBP = "webp";
const std::string IMAGE_CONTAINER_TYPE_RAW = "raw";
const std::string IMAGE_CONTAINER_TYPE_DNG = "dng";
const std::string IMAGE_CONTAINER_TYPE_SVG = "svg";
const std::string IMAGE_CONTAINER_TYPE_HEIF = "heif";
const std::string IMAGE_CONTAINER_TYPE_HEIC = "heic";
const std::string DIR_ALL_CONTAINER_TYPE = ".ALLTYPE";
const std::string DIR_ALL_TYPE_VALUES = "ALLTYPE";

const std::string DOC_EXTENSION_VALUES = DIR_ALL_CONTAINER_TYPE;

const std::string DOC_TYPE_VALUES = DIR_ALL_TYPE_VALUES;

const std::string DOWNLOAD_EXTENSION_VALUES = DIR_ALL_CONTAINER_TYPE;

const std::string DOWNLOAD_TYPE_VALUES = DIR_ALL_TYPE_VALUES;
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_CONTAINER_TYPES_H