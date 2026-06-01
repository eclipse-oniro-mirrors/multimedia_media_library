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

#ifndef MEDIA_ALBUMS_CONTROLLER_SERVICE_FUZZER_H
#define MEDIA_ALBUMS_CONTROLLER_SERVICE_FUZZER_H

#define FUZZ_PROJECT_NAME "media_albums_controller_service_fuzzer"
#include <iostream>
#include <vector>
#include <utility>

#include "userfile_manager_types.h"

namespace OHOS::Media {

const std::vector<std::pair<PhotoAlbumType, PhotoAlbumSubType>> ALBUM_PAIRS = {
    {PhotoAlbumType::USER, PhotoAlbumSubType::USER_GENERIC},
    {PhotoAlbumType::SMART, PhotoAlbumSubType::PORTRAIT},
    {PhotoAlbumType::SMART, PhotoAlbumSubType::GROUP_PHOTO},
    {PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT},
    {PhotoAlbumType::SMART, PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS},
    {PhotoAlbumType::SMART, PhotoAlbumSubType::GEOGRAPHY_LOCATION},
    {PhotoAlbumType::SMART, PhotoAlbumSubType::GEOGRAPHY_CITY},
    {PhotoAlbumType::SOURCE, PhotoAlbumSubType::SOURCE_GENERIC},
    {PhotoAlbumType::SOURCE, PhotoAlbumSubType::SOURCE_GENERIC_FROM_FILE_MANAGER},
    {PhotoAlbumType::SYSTEM, PhotoAlbumSubType::TRASH},
    {PhotoAlbumType::SYSTEM, PhotoAlbumSubType::SYSTEM_START},
    {PhotoAlbumType::SYSTEM, PhotoAlbumSubType::HIDDEN}
};
} // namespace OHOS::Media

#endif