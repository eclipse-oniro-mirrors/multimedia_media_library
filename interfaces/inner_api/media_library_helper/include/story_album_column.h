
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_ALBUM_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_ALBUM_COLUMN_H

#include <string>

namespace OHOS {
namespace Media {
// highlight album table name
const std::string HIGHLIGHT_ALBUM_TABLE = "tab_highlight_album";

// create highlight album table
const std::string AI_ALBUM_ID = "ai_album_id";
const std::string SUB_TITLE = "sub_title";
const std::string CLUSTER_TYPE = "cluster_type";
const std::string CLUSTER_SUB_TYPE = "cluster_sub_type";
const std::string CLUSTER_CONDITION = "cluster_condition";
const std::string MIN_DATE_ADDED = "min_date_added";
const std::string MAX_DATE_ADDED = "max_date_added";
const std::string GENERATE_TIME = "generate_time";
const std::string HIGHLIGHT_VERSION = "highlight_version";
const std::string REMARKS = "remarks";
const std::string HIGHLIGHT_STATUS = "highlight_status";
const std::string HIGHLIGHT_INSERT_PIC_COUNT = "insert_pic_count";
const std::string HIGHLIGHT_REMOVE_PIC_COUNT = "remove_pic_count";
const std::string HIGHLIGHT_SHARE_SCREENSHOT_COUNT = "share_screenshot_count";
const std::string HIGHLIGHT_SHARE_COVER_COUNT = "share_cover_count";
const std::string HIGHLIGHT_RENAME_COUNT = "rename_count";
const std::string HIGHLIGHT_CHANGE_COVER_COUNT = "change_cover_count";
const std::string HIGHLIGHT_RENDER_VIEWED_TIMES = "render_viewed_times";
const std::string HIGHLIGHT_RENDER_VIEWED_DURATION = "render_viewed_duration";
const std::string HIGHLIGHT_ART_LAYOUT_VIEWED_TIMES = "art_layout_viewed_times";
const std::string HIGHLIGHT_ART_LAYOUT_VIEWED_DURATION = "art_layout_viewed_duration";

const std::string URI_HIGHLIGHT_ALBUM = MEDIALIBRARY_DATA_URI + "/" + HIGHLIGHT_ALBUM_TABLE;
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_STORY_ALBUM_COLUMN_H