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

#include "file_manager_scan_rule_config.h"

namespace OHOS::Media {
const std::unordered_set<std::string> FILE_MANAGER_BLACK_LIST = {
    "/storage/media/local/files/Docs/HO_DATA_EXT_MISC",
    "/storage/media/local/files/Docs/.thumbs",
    "/storage/media/local/files/Docs/.Recent",
    "/storage/media/local/files/Docs/.backup",
    "/storage/media/local/files/Docs/.Trash",
};
const std::regex FILE_MANAGER_PATTERN_RELATIVE_PATH(
    R"(^/storage/media/local/files/Docs)",
    std::regex_constants::icase
);
const std::regex FILE_MANAGER_PATTERN_VISIBLE(R"($^)", std::regex_constants::icase);
const std::regex FILE_MANAGER_PATTERN_INVISIBLE(R"($^)", std::regex_constants::icase);
const std::regex FILE_MANAGER_PATTERN_TENCENT_CACHE(R"($^)", std::regex_constants::icase);
const std::unordered_set<std::string> FILE_MANAGER_DEFAULT_FOLDER_NAMES = {};

const ScanRuleConfig FILE_MANAGER_SCAN_RULE_CONFIG = {
    .rootPath = std::string(FILE_MANAGER_ROOT_PATH),
    .blackList = FILE_MANAGER_BLACK_LIST,
    .relativePathPattern = FILE_MANAGER_PATTERN_RELATIVE_PATH,
    .visiblePattern = FILE_MANAGER_PATTERN_VISIBLE,
    .invisiblePattern = FILE_MANAGER_PATTERN_INVISIBLE,
    .tencentCachePattern = FILE_MANAGER_PATTERN_TENCENT_CACHE,
    .defaultFolderNames = FILE_MANAGER_DEFAULT_FOLDER_NAMES,
    .skipHiddenFile = false,
    .skipHiddenDirectory = false,
    .skipBlackList = true,
    .skipTencentCache = false,
    .createNomediaForInvisibleDirectory = false,
    .cleanNomediaInDefaultDirs = false,
    .skipDirectoryWithNomedia = false
};

const ScanRuleConfig &GetFileManagerScanRuleConfig()
{
    return FILE_MANAGER_SCAN_RULE_CONFIG;
}
} // namespace OHOS::Media