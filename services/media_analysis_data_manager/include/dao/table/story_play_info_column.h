
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_PLAY_INFO_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_PLAY_INFO_COLUMN_H

#include <string>

namespace OHOS {
namespace Media {
// highlight play info table name
const std::string HIGHLIGHT_PLAY_INFO_TABLE = "tab_highlight_play_info";

// create highlight play info table
const std::string MUSIC = "music";
const std::string FILTER = "filter";
const std::string HIGHLIGHT_PLAY_INFO = "play_info";
const std::string PLAY_INFO_VERSION = "play_info_version";
const std::string IS_CHOSEN = "is_chosen";
const std::string PLAY_INFO_ID = "play_info_id";
const std::string HIGHLIGHTING_ALGO_VERSION = "highlighting_algo_version";
const std::string CAMERA_MOVEMENT_ALGO_VERSION = "camera_movement_algo_version";
const std::string TRANSITION_ALGO_VERSION = "transition_algo_version";
const std::string PLAY_SERVICE_VERSION = "play_service_version";
const std::string PLAY_INFO_STATUS = "status";

const std::string URI_HIGHLIGHT_PLAY_INFO = MEDIALIBRARY_DATA_URI + "/" + HIGHLIGHT_PLAY_INFO_TABLE;
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_COVER_INFO_COLUMN_H