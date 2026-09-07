/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "lake_scan_rule_config.h"

namespace OHOS::Media {
const std::unordered_set<std::string> LAKE_BLACK_LIST = {
    "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Pictures/oh_pictures",
};
const std::regex LAKE_PATTERN_RELATIVE_PATH(
    R"(^/storage/media/local/files/Docs/HO_DATA_EXT_MISC)",
    std::regex_constants::icase
);
const std::regex LAKE_PATTERN_VISIBLE(
    R"(^/storage/media/local/files/Docs/HO_DATA_EXT_MISC$)",
    std::regex_constants::icase
);
const std::regex LAKE_PATTERN_INVISIBLE(
    R"(^/storage/media/local/files/Docs/HO_DATA_EXT_MISC/)"
    R"((Android/(data|obb|sandbox)|)"
    R"(\.transforms|)"
    R"(Pictures/\.Gallery2|)"
    R"(\.File_Recycle|)"
    R"(Huawei/CloudDrive/\.thumbnail|)"
    R"(Pictures/\.hwthumbnails|)"
    R"(Pictures/oh_pictures|)"
    R"((Movies|Music|Pictures)/\.thumbnails)$)",
    std::regex_constants::icase
);
const std::regex LAKE_PATTERN_TENCENT_CACHE(
    R"(.*Tencent/MicroMsg/[a-zA-Z0-9_]{32})",
    std::regex_constants::icase
);
const std::unordered_set<std::string> LAKE_DEFAULT_FOLDER_NAMES = {
    "Music",
    "Podcasts",
    "Ringtones",
    "Alarms",
    "Notifications",
    "Pictures",
    "Movies",
    "Download",
    "DCIM",
    "Documents",
    "Audiobooks",
    "Recordings",
};

const ScanRuleConfig LAKE_SCAN_RULE_CONFIG = {
    .rootPath = std::string(LAKE_ROOT_PATH),
    .blackList = LAKE_BLACK_LIST,
    .relativePathPattern = LAKE_PATTERN_RELATIVE_PATH,
    .visiblePattern = LAKE_PATTERN_VISIBLE,
    .invisiblePattern = LAKE_PATTERN_INVISIBLE,
    .tencentCachePattern = LAKE_PATTERN_TENCENT_CACHE,
    .defaultFolderNames = LAKE_DEFAULT_FOLDER_NAMES,
    .skipHiddenFile = true,
    .skipHiddenDirectory = true,
    .skipBlackList = true,
    .skipTencentCache = true,
    .createNomediaForInvisibleDirectory = true,
    .cleanNomediaInDefaultDirs = true,
    .skipDirectoryWithNomedia = true
};

const ScanRuleConfig &GetLakeScanRuleConfig()
{
    return LAKE_SCAN_RULE_CONFIG;
}
} // namespace OHOS::Media