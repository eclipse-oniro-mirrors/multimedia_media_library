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
 
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const std::string RDB_UPGRADE_EVENT = "/data/storage/el2/base/preferences/rdb_upgrade_events.xml";
static const std::string RDB_FIX_RECORDS = "/data/storage/el2/base/preferences/rdb_fix_records.xml";
static const std::string DETAIL_TIME_FIXED = "detail_time_fixed";
static const std::string THUMBNAIL_VISIBLE_FIXED = "thumbnail_visible_fixed";
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
 
    EXPORT static bool HasUpgraded(int32_t version, bool isSync);
    EXPORT static void SetUpgradeStatus(int32_t version, bool isSync);
    static void AddMapValueToPreference();
    static void AddUpgradeDfxMessages(int32_t version, int32_t index, int32_t error);
    static void ReportUpgradeDfxMessages(int64_t startTime, int32_t srcVersion,
        int32_t dstVersion, bool isSync);
};
 
} // Media
} // OHOS
#endif // OHOS_MEDIALIBRARY_UPGRADE_UTILS_H