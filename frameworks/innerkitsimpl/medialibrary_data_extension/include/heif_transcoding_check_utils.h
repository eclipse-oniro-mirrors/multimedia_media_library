/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef HEIF_TRANSCODING_CHECK_UTILS_H
#define HEIF_TRANSCODING_CHECK_UTILS_H

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "nlohmann/json.hpp"
#include "bundle_mgr_interface.h"
#include "common_event_manager.h"
#include "highpixel_bundle_info_cache.h"
#include "preferred_compatible_mode_check_utils.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class HeifTranscodingCheckUtils {
public:
    EXPORT HeifTranscodingCheckUtils(const HeifTranscodingCheckUtils&) = delete;
    EXPORT HeifTranscodingCheckUtils(HeifTranscodingCheckUtils&&) = delete;
    EXPORT HeifTranscodingCheckUtils& operator=(const HeifTranscodingCheckUtils&) = delete;
    EXPORT HeifTranscodingCheckUtils& operator=(HeifTranscodingCheckUtils&&) = delete;
    EXPORT static int32_t InitCheckList();
    EXPORT static bool CanSupportedCompatibleDuplicate(const std::string &bundleName);
    EXPORT static bool CanSupportedHighPixelPicture(const std::string &bundleName, const HighPixelType &type);
    EXPORT static void UnsubscribeCotaUpdatedEvent();
    EXPORT static void ClearBundleInfoInCache();
    EXPORT static TranscodeMode CheckTranscodeMode(const std::string &bundleName,
        bool isHighPixel, bool isHeifFile);
    EXPORT static bool IsDUEVersionHigher();
private:
    static sptr<AppExecFwk::IBundleMgr> GetSysBundleManager();
    static int32_t ParseWhiteList(const nlohmann::json &checkListJson);
    static int32_t ParseDenyList(const nlohmann::json &checkListJson);
    static int32_t ParseHighPixelCheckList(const nlohmann::json &checkListJson, const std::string &path);
    static int32_t ParsePixelWhiteListFromFile();
    static int32_t ReadCheckList();
    static bool CheckHighPixelWhiteList(const std::string &bundleName,
        const HighPixelType &pixelType, const std::unordered_map<std::string, std::string>* whiteList);
    static bool CheckHighPixelBlackList(const std::string &bundleName,
        const HighPixelType &pixelType, const std::unordered_map<std::string, std::string>* denyList);
    static void ClearCheckList();
    static int32_t SubscribeCotaUpdatedEvent();
    static bool isUseWhiteList_;
    static bool isUseWhiteList50_;
    static bool isUseWhiteList200_;
    // key: bundleName, value: version
    static std::unordered_map<std::string, std::string> whiteList_;
    // key: bundleName
    static std::unordered_set<std::string> denyList_;
    static std::unordered_map<std::string, std::string> whiteList50_;
    static std::unordered_map<std::string, std::string> denyList50_;
    static std::unordered_map<std::string, std::string> whiteList200_;
    static std::unordered_map<std::string, std::string> denyList200_;
    static sptr<AppExecFwk::IBundleMgr> bundleMgr_;
    static std::mutex bundleMgrMutex_;
    static std::shared_ptr<EventFwk::CommonEventSubscriber> cotaUpdateSubscriber_;
    static std::shared_mutex checkListMutex_;
    class CotaUpdateReceiver;
};
}
}
#endif // HEIF_TRANSCODING_CHECK_UTILS_H