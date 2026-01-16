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

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "nlohmann/json.hpp"
#include "bundle_mgr_interface.h"
#include "common_event_manager.h"

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
    EXPORT static void UnsubscribeCotaUpdatedEvent();
    EXPORT static void ClearBundleInfoInCache();
private:
    static sptr<AppExecFwk::IBundleMgr> GetSysBundleManager();
    static int32_t ParseWhiteList(const nlohmann::json &checkListJson);
    static int32_t ParseDenyList(const nlohmann::json &checkListJson);
    static int32_t ReadCheckList();
    static void ClearCheckList();
    static int32_t SubscribeCotaUpdatedEvent();
    static bool isUseWhiteList_;
    // key: bundleName, value: version
    static std::unordered_map<std::string, std::string> whiteList_;
    // key: bundleName
    static std::unordered_set<std::string> denyList_;
    static sptr<AppExecFwk::IBundleMgr> bundleMgr_;
    static std::mutex bundleMgrMutex_;
    static std::shared_ptr<EventFwk::CommonEventSubscriber> cotaUpdateSubscriber_;
    class CotaUpdateReceiver;
};
}
}
#endif // HEIF_TRANSCODING_CHECK_UTILS_H