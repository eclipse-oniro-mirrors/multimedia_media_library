/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_DFXDATABASEUTILS_FUZZER_H
#define MEDIALIBRARY_DFXDATABASEUTILS_FUZZER_H

#define FUZZ_PROJECT_NAME "medialibrarydfxdatabaseutils_fuzzer"
#include <vector>
#include "cloud_media_operation_code.h"
#include "thumbnail_const.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
using namespace OHOS::Media::CloudSync;

const int32_t FILE_HEIGHT_AND_WIDTH_120 = 120;
const int32_t FILE_HEIGHT_AND_WIDTH_240 = 240;
const int32_t FILE_HEIGHT_AND_WIDTH_360 = 360;
const int32_t FILE_HEIGHT_AND_WIDTH_480 = 480;
const int32_t FILE_HEIGHT_AND_WIDTH_720 = 720;
const int32_t FILE_HEIGHT_AND_WIDTH_1080 = 1080;
const int32_t FILE_HEIGHT_AND_WIDTH_1440 = 1440;
const int32_t FILE_HEIGHT_AND_WIDTH_2000 = 2000;
const int32_t FILE_HEIGHT_AND_WIDTH_4000 = 4000;
const int32_t FILE_HEIGHT_AND_WIDTH_5000 = 5000;

const std::vector<PhotoPositionType> PHOTO_POSITION_TYPE_LIST = {
    PhotoPositionType::LOCAL,
    PhotoPositionType::CLOUD,
    PhotoPositionType::LOCAL_AND_CLOUD
};

const std::vector<ThumbnailReady> THUMBNAIL_READY_LIST = {
    ThumbnailReady::GENERATE_THUMB_RETRY,
    ThumbnailReady::GENERATE_THUMB_COMPLETED
};


const std::vector<ThumbState> THUMB_STATUS_LIST = {
    ThumbState::DOWNLOADED,
    ThumbState::THM_TO_DOWNLOAD
};

const std::vector<MediaType> MEDIA_TYPE_LISTS = {
    MediaType::MEDIA_TYPE_IMAGE,
    MediaType::MEDIA_TYPE_VIDEO,
    MediaType::MEDIA_TYPE_AUDIO
};

const std::vector<int32_t> FILE_HEIGHT_AND_WIDTH_LISTS = {
    FILE_HEIGHT_AND_WIDTH_120,
    FILE_HEIGHT_AND_WIDTH_240,
    FILE_HEIGHT_AND_WIDTH_360,
    FILE_HEIGHT_AND_WIDTH_480,
    FILE_HEIGHT_AND_WIDTH_720,
    FILE_HEIGHT_AND_WIDTH_1080,
    FILE_HEIGHT_AND_WIDTH_1440,
    FILE_HEIGHT_AND_WIDTH_2000,
    FILE_HEIGHT_AND_WIDTH_4000,
    FILE_HEIGHT_AND_WIDTH_5000
};

const std::vector<std::string> MIMETYPE_LISTS = {
    "image/jpeg",
    "audio/mpeg",
    "video/mp4",
    "text/xml"
    "invalid"
};
} // namespace Media
} // namespace OHOS
#endif
