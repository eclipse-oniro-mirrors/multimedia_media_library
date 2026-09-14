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
#ifndef OHOS_MEDIA_SCAN_RULE_CONFIG_H
#define OHOS_MEDIA_SCAN_RULE_CONFIG_H

#include <regex>
#include <string>
#include <unordered_set>

namespace OHOS::Media {
struct ScanRuleConfig {
    std::string rootPath;
    std::unordered_set<std::string> blackList;
    std::regex relativePathPattern;
    std::regex visiblePattern;
    std::regex invisiblePattern;
    std::regex appCachePattern;
    std::unordered_set<std::string> defaultFolderNames;
    bool skipHiddenFile {true};
    bool skipHiddenDirectory {true};
    bool skipBlackList {true};
    bool skipAppCache {true};
    bool createNomediaForInvisibleDirectory {true};
    bool cleanNomediaInDefaultDirs {true};
    bool skipDirectoryWithNomedia {true};
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_SCAN_RULE_CONFIG_H