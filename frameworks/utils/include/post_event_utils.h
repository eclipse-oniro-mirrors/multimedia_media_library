/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef POST_EVENT_UTILS_H
#define POST_EVENT_UTILS_H
#include <map>
#include <string>
#include <variant>
#include <list>
#include <set>

#include "singleton.h"
namespace OHOS {
namespace Media {
#define COMPILE_HIDDEN __attribute__ ((visibility ("hidden")))
struct SyncEventStat {
    int64_t startDownloadTime;
    int64_t endDownloadTime;
    int32_t downloadType;
    int32_t totalPhotoCount;
    int32_t totalAlbumNum;
    int32_t addAlbumNum;
    int32_t updateAlbumNum;
    int32_t deleteAlbumNum;
    int32_t totalAssetNum;
    int32_t addAssetNum;
    int32_t updateAssetNum;
    int32_t deleteAssetNum;
    int32_t avgRefreshImageVideoAlbumTime;
    int32_t refreshUserAndSourceAlbumCount;
    int32_t avgRefreshUserAndSourceAlbumTime;
    int32_t refreshAnalysisAlbumCount;
    int32_t avgRefreshAnalysisAlbumTime;
};

const std::string KEY_OPT_TYPE = "optType";

const std::string KEY_ERR_FILE = "errFile";
const std::string KEY_ERR_LINE = "errLine";
const std::string KEY_ERR_CODE = "errCode";
const std::string KEY_OPT_FILE = "optFile";

const std::string KEY_GNUMS = "gnums";
const std::string KEY_ANUMS = "anums";

const std::string KEY_PRE_VERSION = "preVersion";
const std::string KEY_AFTER_VERSION = "afterVersion";

const std::string KEY_COUNT = "count";

const std::string KEY_TIME_INTERVAL = "TIME_INTERVAL";

const std::string KEY_CALLING_PACKAGE = "CALLING_PACKAGE";
const std::string KEY_HIGH_QUALITY_COUNT = "HIGH_QUALITY_COUNT";
const std::string KEY_BALANCE_QUALITY_COUNT = "BALANCE_QUALITY_COUNT";
const std::string KEY_EMERGENCY_QUALITY_COUNT = "EMERGENCY_QUALITY_COUNT";

const std::string KEY_THIRD_PART_COUNT = "THIRD_PART_COUNT";
const std::string KEY_AUTO_COUNT = "AUTO_COUNT";

const std::string KEY_PHOTO_ID = "PHOTO_ID";
const std::string KEY_TOTAL_TIME_COST = "TOTAL_TIME_COST";

const std::string KEY_RESULT = "RESULT";
const std::string KEY_MEDIA_TYPE = "MEDIA_TYPE";

const std::string KEY_CLOUD_ENHANCEMENT_COMPLETE_TYPE = "COMPLETE_TYPE";
const std::string KEY_CLOUD_ENHANCEMENT_FINISH_TYPE = "FINISH_TYPE";
const std::string KEY_CLOUD_ENHANCEMENT_FILE_TYPE = "FILE_TYPE";

const std::string KEY_DB_CORRUPT = "DB_CORRUPT_DATE";

namespace {
const char* KEY_START_DOWNLOAD_TIME = "START_DOWNLOAD_T";
const char* KEY_END_DOWNLOAD_TIME = "END_DOWNLOAD_T";
const char* KEY_DOWNLOAD_TYPE = "DOWNLOAD_TYPE";
const char* KEY_TOTAL_PHOTO_COUNT = "TOTAL_PHOTO_C";
 
const char* KEY_TOTAL_ALBUM_NUM = "TOTAL_ALBUM_NUM";
const char* KEY_ADD_ALBUM_NUM = "ADD_ALBUM_NUM";
const char* KEY_UPDATE_ALBUM_NUM = "UPDATE_ALBUM_NUM";
const char* KEY_DELETE_ALBUM_NUM = "DELETE_ALBUM_NUM";
const char* KEY_TOTAL_ASSET_NUM = "TOTAL_ASSET_NUM";
const char* KEY_ADD_ASSET_NUM = "ADD_ASSET_NUM";
const char* KEY_UPDATE_ASSET_NUM = "UPDATE_ASSET_NUM";
const char* KEY_DELETE_ASSET_NUM = "DELETE_ASSET_NUM";
 
const char* KEY_REFRESH_IMAGEVIDEO_ALBUM_TOTAL_COUNT = "REF_IMAGEVIDEO_ALBUM_C";
const char* KEY_REFRESH_IMAGEVIDEO_ALBUM_TOTAL_TIME = "REF_IMAGEVIDEO_ALBUM_T";
const char* KEY_AVG_REFRESH_IMAGEVIDEO_ALBUM_TIME = "AVG_REF_IMAGEVIDEO_ALBUM_T";
 
const char* KEY_REFRESH_USER_AND_SOURCE_ALBUM_COUNT = "REF_USERSOURCE_ALBUM_C";
const char* KEY_REFRESH_USER_AND_SOURCE_ALBUM_TOTAL_COUNT = "REF_USERSOURCE_ALBUM_TOTAL_C";
const char* KEY_REFRESH_USER_AND_SOURCE_ALBUM_TOTAL_TIME = "REF_USERSOURCE_ALBUM_TOTAL_T";
const char* KEY_AVG_REFRESH_USER_AND_SOURCE_ALBUM_TIME = "AVG_REF_USERSOURCE_ALBUM_T";
 
const char* KEY_REFRESH_ANALYSIS_ALBUM_COUNT = "REF_ANALYSIS_ALBUM_C";
const char* KEY_REFRESH_ANALYSIS_ALBUM_TOTAL_COUNT = "REF_ANALYSIS_ALBUM_TOTAL_C";
const char* KEY_REFRESH_ANALYSIS_ALBUM_TOTAL_TIME = "REF_ANALYSIS_ALBUM_TOTAL_T";
const char* KEY_AVG_REFRESH_ANALYSIS_ALBUM_TIME = "AVG_REF_ANALYSIS_ALBUM_T";
}

enum OptType {
    CREATE = 0,
    THUMB,
    SCAN,
    QUERY,
};

enum ErrType {
    DEFAULT_ERR = 0,
    FILE_OPT_ERR,
    DB_OPT_ERR,
    DB_UPGRADE_ERR,
    DB_CORRUPT_ERR,
    RECOVERY_ERR,
};

enum StatType {
    DEFAULT_STAT = 0,
    THUMBNAIL_STAT,
    DB_UPGRADE_STAT,
    SYNC_STAT,
    AGING_STAT,
    MSC_FIRST_VISIT_STAT,
    MSC_REQUEST_POLICY_STAT,
    MSC_TRIGGER_RATIO_STAT,
    MSC_TOTAL_TIME_COST_STAT,
    MSC_RESULT_STAT,
    CLOUD_ENHANCEMENT_GET_COUNT_STAT,
};
using VariantMap = std::map<std::string, std::variant<int32_t, int64_t, std::string>>;

class PostEventUtils : public Singleton<PostEventUtils> {
public:
    void PostErrorProcess(const uint32_t &errType, const VariantMap &error);
    void PostStatProcess(const uint32_t &statType, const VariantMap &stat);
    void CreateCloudDownloadSyncStat(std::string& syncId);
    void UpdateCloudDownloadSyncStat(VariantMap& syncStat);
    void PostCloudDownloadSyncStat(std::string& syncId);

private:
    COMPILE_HIDDEN std::string GetOptType(const uint32_t &optType);
    COMPILE_HIDDEN void PostFileOptError(const VariantMap &errMap);
    COMPILE_HIDDEN void PostRecoveryOptError(const VariantMap &error);
    COMPILE_HIDDEN void PostDbOptError(const VariantMap &errMap);
    COMPILE_HIDDEN void PostDbUpgradeError(const VariantMap &errMap);
    COMPILE_HIDDEN void PostThumbnailStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostDbUpgradeStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostSyncStat();
    COMPILE_HIDDEN void PostAgingStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostMscFirstVisitStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostMscRequestPolicyStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostMscTriggerRatioStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostMscTotalTimeCostStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostMscResultStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostCloudEnhanceStat(const VariantMap &stat);
    COMPILE_HIDDEN void PostDatabaseCorruption(const VariantMap &errMap);

    COMPILE_HIDDEN int GetIntValue(const std::string &key, const VariantMap &map);
    COMPILE_HIDDEN int64_t GetInt64Value(const std::string &key, const VariantMap &map);
    COMPILE_HIDDEN std::string GetStringValue(const std::string &key, const VariantMap &map);
    COMPILE_HIDDEN SyncEventStat GetSyncEventStat(const VariantMap &stat);

    COMPILE_HIDDEN uint32_t thumbnailTimes_ = 0;
    COMPILE_HIDDEN uint32_t dbUpgradeTimes_ = 0;
    COMPILE_HIDDEN uint32_t syncTimes_ = 0;
    COMPILE_HIDDEN uint32_t recycleTimes_ = 0;
    std::mutex cloudDownloadSyncStatMutex_;
    std::string currentSyncId_;
    std::list<int32_t> analysis_alunm_count_;
    std::list<int32_t> user_alubm_count_;
    COMPILE_HIDDEN VariantMap cloudDownloadSyncStat_;
};
} // namespace Media
} // namespace OHOS
#endif