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
#ifndef OHOS_MEDIA_DIR_SCAN_ANOMALY_HELPER_H
#define OHOS_MEDIA_DIR_SCAN_ANOMALY_HELPER_H

#include <mutex>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "preferences_helper.h"

namespace OHOS::Media {

class DirScanAnomalyHelper {
public:
    static int32_t AddAnomalyDir(const std::string &dirPath);
    static bool MatchesAnomalyDir(const std::string &dirPath);
    static int32_t RemoveAnomalyDir(const std::string &dirPath);
    static void ClearAll();

private:
    static std::shared_ptr<NativePreferences::Preferences> GetPreferences();
    static nlohmann::json GetDirsLocked();
    static void SaveDirsLocked(const nlohmann::json &dirs);

    static const std::string XML_PATH;
    static const std::string KEY_ANOMALY_DIRS;
    static std::mutex mutex_;
    static nlohmann::json cachedDirs_;
    static bool cacheLoaded_;
};

} // namespace OHOS::Media

#endif // OHOS_MEDIA_DIR_SCAN_ANOMALY_HELPER_H
