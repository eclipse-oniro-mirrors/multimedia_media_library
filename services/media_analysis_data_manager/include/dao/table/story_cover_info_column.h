
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_COVER_INFO_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_COVER_INFO_COLUMN_H

#include <string>

#include "base_data_uri.h"

namespace OHOS {
namespace Media {
// highlight cover info table name
const std::string HIGHLIGHT_COVER_INFO_TABLE = "tab_highlight_cover_info";

// create highlight cover info table
const std::string RATIO = "ratio";
const std::string BACKGROUND = "background";
const std::string FOREGROUND = "foreground";
const std::string WORDART = "wordart";
const std::string IS_COVERED = "is_covered";
const std::string COLOR = "color";
const std::string RADIUS = "radius";
const std::string SATURATION = "saturation";
const std::string BRIGHTNESS = "brightness";
const std::string BACKGROUND_COLOR_TYPE = "background_color_type";
const std::string SHADOW_LEVEL = "shadow_level";
const std::string TITLE_SCALE_X = "title_scale_x";
const std::string TITLE_SCALE_Y = "title_scale_y";
const std::string TITLE_RECT_WIDTH = "title_rect_width";
const std::string TITLE_RECT_HEIGHT = "title_rect_height";
const std::string BACKGROUND_SCALE_X = "background_scale_x";
const std::string BACKGROUND_SCALE_Y = "background_scale_y";
const std::string BACKGROUND_RECT_WIDTH = "background_rect_width";
const std::string BACKGROUND_RECT_HEIGHT  = "background_rect_height";
const std::string LAYOUT_INDEX = "layout_index";
const std::string COVER_ALGO_VERSION = "cover_algo_version";
const std::string COVER_KEY = "cover_key";
const std::string COVER_SERVICE_VERSION = "cover_service_version";
const std::string COVER_STATUS = "status";

const std::string URI_HIGHLIGHT_COVER_INFO = MEDIALIBRARY_DATA_URI + "/" + HIGHLIGHT_COVER_INFO_TABLE;
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_COVER_INFO_COLUMN_H