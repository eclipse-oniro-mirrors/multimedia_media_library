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
 
#ifndef OHOS_MEDIALIBRARY_UPGRADE_UTILS_H
#define OHOS_MEDIALIBRARY_UPGRADE_UTILS_H
 
#include "preferences.h"
#include "preferences_helper.h"
#include "upgrade_visibility.h"
 
namespace OHOS {
namespace Media {
#define RDB_UPGRADE_EVENT "/data/storage/el2/base/preferences/rdb_upgrade_events.xml"
#define RDB_CONFIG "/data/storage/el2/base/preferences/rdb_config.xml"
#define RDB_FIX_RECORDS "/data/storage/el2/base/preferences/rdb_fix_records.xml"
#define DETAIL_TIME_FIXED "detail_time_fixed"
#define THUMBNAIL_VISIBLE_FIXED "thumbnail_visible_fixed"
// 需要状态管理的起始版本号 VERSION_FIX_DB_UPGRADE_TO_API20
constexpr int32_t STATUS_MANAGEMENT_START_VERSION = 350;
static const int32_t NEED_FIXED = 1;
static const int32_t ALREADY_FIXED = 2;
static constexpr uint32_t UPGRADE_EXCEPTION_VERSIONS_STR_LIMIT = 1024;
 
enum UPGRADE_STATUS : int32_t {
    UNKNOWN,
    SYNC,
    ASYNC,
    ALL,
};
 
class RdbUpgradeUtils {
public:
    RdbUpgradeUtils() = delete;
    ~RdbUpgradeUtils() = delete;
 
    UPGRADE_EXPORT static bool HasUpgraded(int32_t version, bool isSync, const std::string& path = RDB_UPGRADE_EVENT);
    UPGRADE_EXPORT static void SetUpgradeStatus(int32_t version, bool isSync,
        const std::string& path = RDB_UPGRADE_EVENT);
    static void AddMapValueToPreference();
    static void AddUpgradeDfxMessages(int32_t version, int32_t index, int32_t error);
    static void ReportUpgradeDfxMessages(int64_t startTime, int32_t srcVersion,
        int32_t dstVersion, bool isSync);

    static void SetOldVersion(int32_t oldVersion);
    static int32_t GetOldVersion();
};
 
} // Media
} // OHOS
#endif // OHOS_MEDIALIBRARY_UPGRADE_UTILS_H