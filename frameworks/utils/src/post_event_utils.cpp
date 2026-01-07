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
#include "post_event_utils.h"

#include <unistd.h>
#include <set>

#include "hisysevent.h"
#include "ipc_skeleton.h"
#include "media_log.h"
namespace OHOS {
namespace Media {
using namespace std;

const string OPT_CREATE = "CREATE";
const string OPT_THUMB = "THUMB";
const string OPT_SCAN = "SCAN";
const string OPT_QUERY = "QUERY";
static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";

string PostEventUtils::GetOptType(const uint32_t &optType)
{
    string type = "";
    switch (optType) {
        case OptType::CREATE:
            type = OPT_CREATE;
            break;
        case OptType::THUMB:
            type = OPT_THUMB;
            break;
        case OptType::SCAN:
            type = OPT_SCAN;
            break;
        case OptType::QUERY:
            type = OPT_QUERY;
            break;
        default:
            break;
    }
    return type;
}

int32_t PostEventUtils::GetIntValue(const string &key, const VariantMap &map)
{
    int value = 0;
    auto iter = map.find(key);
    if (iter != map.end()) {
        if (holds_alternative<int32_t>(iter->second)) {
            return get<int32_t>(iter->second);
        }
    }
    return value;
}

int64_t PostEventUtils::GetInt64Value(const string &key, const VariantMap &map)
{
    int64_t value = 0;
    auto iter = map.find(key);
    if (iter != map.end()) {
        if (holds_alternative<int64_t>(iter->second)) {
            return get<int64_t>(iter->second);
        }
    }
    return value;
}

string PostEventUtils::GetStringValue(const string &key, const VariantMap &map)
{
    string value;
    auto iter = map.find(key);
    if (iter != map.end()) {
        if (holds_alternative<string>(iter->second)) {
            return get<string>(iter->second);
        }
    }
    return value;
}

void PostEventUtils::PostFileOptError(const VariantMap &error)
{
    uint32_t uid = getuid();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_FILE_OPT_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "UID", uid,
        "ERR_FILE", GetStringValue(KEY_ERR_FILE, error),
        "LINE", GetIntValue(KEY_ERR_LINE, error),
        "ERROR_CODE", GetIntValue(KEY_ERR_CODE, error),
        "FILE", GetStringValue(KEY_OPT_FILE, error),
        "TYPE", GetOptType(GetIntValue(KEY_OPT_TYPE, error)),
        "CALLING_ID", IPCSkeleton::GetCallingUid());
    if (ret != 0) {
        MEDIA_ERR_LOG("PostFileOptError error:%{public}d", ret);
    }
}

void PostEventUtils::PostRecoveryOptError(const VariantMap &error)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_META_RECOVERY_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "ERR_FILE", GetStringValue(KEY_ERR_FILE, error),
        "LINE", GetIntValue(KEY_ERR_LINE, error),
        "ERROR_CODE", GetIntValue(KEY_ERR_CODE, error),
        "TYPE", GetOptType(GetIntValue(KEY_OPT_TYPE, error)));
    if (ret != 0) {
        MEDIA_ERR_LOG("PostFileOptError error:%{public}d", ret);
    }
}

void PostEventUtils::PostDbOptError(const VariantMap &error)
{
    uint32_t uid = getuid();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DB_OPT_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "UID", uid,
        "ERR_FILE", GetStringValue(KEY_ERR_FILE, error),
        "LINE", GetIntValue(KEY_ERR_LINE, error),
        "ERROR_CODE", GetIntValue(KEY_ERR_CODE, error),
        "TYPE", GetOptType(GetIntValue(KEY_OPT_TYPE, error)),
        "CALLING_ID", IPCSkeleton::GetCallingUid());
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to PostDbOptError error:%{public}d", ret);
    }
}

void PostEventUtils::PostDbUpgradeError(const VariantMap &error)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DB_UPGRADE_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "ERR_FILE", GetStringValue(KEY_ERR_FILE, error),
        "LINE", GetIntValue(KEY_ERR_LINE, error));
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to PostDbUpgradeError err:%{public}d", ret);
    }
}

void PostEventUtils::PostThumbnailStat(const VariantMap &stat)
{
    uint32_t uid = getuid();
    thumbnailTimes_++;
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_THUMBNAIL_STAT",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "UID", uid,
        "TIMES", thumbnailTimes_,
        "GNUMS", GetIntValue(KEY_GNUMS, stat),
        "ANUMS", GetIntValue(KEY_ANUMS, stat));
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to PostThumbnailStat error:%{public}d ", ret);
    }
}

void PostEventUtils::PostDbUpgradeStat(const VariantMap &stat)
{
    int32_t preVersion = GetIntValue(KEY_PRE_VERSION, stat);
    int32_t afterVersion = GetIntValue(KEY_AFTER_VERSION, stat);
    dbUpgradeTimes_++;
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DB_UPGRADE_STAT",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "PRE_VERSION", preVersion,
        "AFTER_VERSION", afterVersion,
        "COUNT", dbUpgradeTimes_);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostDbUpgradeStat preVersion:%{public}d afterVersion:%{public}d error:%{public}d",
            preVersion, afterVersion, ret);
    }
}

void PostEventUtils::PostSyncStat()
{
    syncTimes_++;
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_SYNC_STAT",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "TIMES", syncTimes_);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostSyncStat ret:%{public}d", ret);
    }
}

void PostEventUtils::PostAgingStat(const VariantMap &stat)
{
    recycleTimes_++;
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_AGING_STAT",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "TIMES", recycleTimes_,
        "COUNT", GetIntValue(KEY_COUNT, stat));
    if (ret != 0) {
        MEDIA_ERR_LOG("PostAgingStat error:%{public}d", ret);
    }
}

void PostEventUtils::PostMscFirstVisitStat(const VariantMap &stat)
{
    string photoId = GetStringValue(KEY_PHOTO_ID, stat);
    int64_t timeInterval = GetInt64Value(KEY_TIME_INTERVAL, stat);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MSC_FIRST_VISIT_STAT",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        KEY_PHOTO_ID, photoId,
        KEY_TIME_INTERVAL, timeInterval);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostMscFirstVisitStat error:%{public}d", ret);
    }
}

void PostEventUtils::PostMscRequestPolicyStat(const VariantMap &stat)
{
    string callingPackage = GetStringValue(KEY_CALLING_PACKAGE, stat);
    int32_t highQualityCount = GetIntValue(KEY_HIGH_QUALITY_COUNT, stat);
    int32_t balanceQualityCount = GetIntValue(KEY_BALANCE_QUALITY_COUNT, stat);
    int32_t emergencyQualityCount = GetIntValue(KEY_EMERGENCY_QUALITY_COUNT, stat);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MSC_REQUST_POLICY_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_CALLING_PACKAGE, callingPackage,
        KEY_HIGH_QUALITY_COUNT, highQualityCount,
        KEY_BALANCE_QUALITY_COUNT, balanceQualityCount,
        KEY_EMERGENCY_QUALITY_COUNT, emergencyQualityCount);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostMscRequestPolicyStat error:%{public}d", ret);
    }
}

void PostEventUtils::PostMscTriggerRatioStat(const VariantMap &stat)
{
    int32_t thirdPartCount = GetIntValue(KEY_THIRD_PART_COUNT, stat);
    int32_t autoCount = GetIntValue(KEY_AUTO_COUNT, stat);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MSC_TRIGGER_RATIO_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_THIRD_PART_COUNT, thirdPartCount,
        KEY_AUTO_COUNT, autoCount);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostMscTriggerRatioStat error:%{public}d", ret);
    }
}

void PostEventUtils::PostMscTotalTimeCostStat(const VariantMap &stat)
{
    string photoId = GetStringValue(KEY_PHOTO_ID, stat);
    int64_t totalTimeCost = GetInt64Value(KEY_TOTAL_TIME_COST, stat);
    int32_t mediaType = GetIntValue(KEY_MEDIA_TYPE, stat);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MSC_TOTAL_TIME_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_PHOTO_ID, photoId,
        KEY_TOTAL_TIME_COST, totalTimeCost,
        KEY_MEDIA_TYPE, mediaType);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostMscTotalTimeCostStat error:%{public}d", ret);
    }
}

void PostEventUtils::PostMscResultStat(const VariantMap &stat)
{
    string photoId = GetStringValue(KEY_PHOTO_ID, stat);
    int32_t result = GetIntValue(KEY_RESULT, stat);
    int32_t mediaType = GetIntValue(KEY_MEDIA_TYPE, stat);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MSC_RESULT_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_PHOTO_ID, photoId,
        KEY_RESULT, result,
        KEY_MEDIA_TYPE, mediaType);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostMscResultStat error:%{public}d", ret);
    }
}

void PostEventUtils::PostDatabaseCorruption(const VariantMap &errMap)
{
    string date = GetStringValue(KEY_DB_CORRUPT, errMap);
    MEDIA_ERR_LOG("ReportDatabaseCorruption periodTime:%{public}s", date.c_str());
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "DATABASE_CORRUPTION_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "DATE", date);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportDatabaseCorruption error:%{public}d", ret);
    }
}

void PostEventUtils::PostErrorProcess(const uint32_t &errType, const VariantMap &error)
{
    switch (errType) {
        case ErrType::FILE_OPT_ERR:
        case ErrType::DB_OPT_ERR:
        case ErrType::DB_UPGRADE_ERR:
            break;
        case ErrType::DB_CORRUPT_ERR:
            PostDatabaseCorruption(error);
            break;
        case ErrType::RECOVERY_ERR:
            PostRecoveryOptError(error);
            break;
        default:
            PostFileOptError(error);
            PostDbOptError(error);
            PostDbUpgradeError(error);
            break;
    }
}

void PostEventUtils::PostCloudEnhanceStat(const VariantMap &stat)
{
    std::string photoId = GetStringValue(KEY_PHOTO_ID, stat);
    std::string completeType = GetStringValue(KEY_CLOUD_ENHANCEMENT_COMPLETE_TYPE, stat);
    int64_t totalTimeCost = GetInt64Value(KEY_TOTAL_TIME_COST, stat);
    int32_t finishType = GetIntValue(KEY_CLOUD_ENHANCEMENT_FINISH_TYPE, stat);
    int32_t fileType = GetIntValue(KEY_CLOUD_ENHANCEMENT_FILE_TYPE, stat);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_CLOUDENHANCEMENT_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_PHOTO_ID, photoId,
        KEY_TOTAL_TIME_COST, totalTimeCost,
        KEY_CLOUD_ENHANCEMENT_COMPLETE_TYPE, completeType,
        KEY_CLOUD_ENHANCEMENT_FINISH_TYPE, finishType,
        KEY_CLOUD_ENHANCEMENT_FILE_TYPE, fileType);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostCloudEnhanceStat:%{public}d", ret);
    }
}

void PostEventUtils::CreateCloudDownloadSyncStat(std::string& syncId)
{
    std::lock_guard<std::mutex> lock(cloudDownloadSyncStatMutex_);
    cloudDownloadSyncStat_.clear();
    user_alubm_count_.clear();
    analysis_alunm_count_.clear();
    currentSyncId_ = syncId;
    MEDIA_INFO_LOG("currentSyncId is: %{public}s,", currentSyncId_.c_str());
}
 
void PostEventUtils::UpdateCloudDownloadSyncStat(VariantMap &syncStat)
{
    static const std::set<std::string> KEY_SET = {
        KEY_START_DOWNLOAD_TIME, KEY_END_DOWNLOAD_TIME, KEY_DOWNLOAD_TYPE, KEY_TOTAL_PHOTO_COUNT};
    std::lock_guard<std::mutex> lock(cloudDownloadSyncStatMutex_);
    for (const auto &[key, value] : syncStat) {
        if (key == KEY_REFRESH_ANALYSIS_ALBUM_TOTAL_COUNT) {
            analysis_alunm_count_.push_back(get<int32_t>(value));
        } else if (key == KEY_REFRESH_USER_AND_SOURCE_ALBUM_TOTAL_COUNT) {
            user_alubm_count_.push_back(get<int32_t>(value));
        } else if (KEY_SET.count(key) > 0 || cloudDownloadSyncStat_.count(key) == 0) {
            cloudDownloadSyncStat_[key] = value;
        } else {
            cloudDownloadSyncStat_[key] = get<int32_t>(cloudDownloadSyncStat_[key]) + get<int32_t>(value);
        }
    }
}
 
SyncEventStat PostEventUtils::GetSyncEventStat(const VariantMap &stat)
{
    SyncEventStat syncEventStat;
    syncEventStat.startDownloadTime = GetInt64Value(KEY_START_DOWNLOAD_TIME, stat);
    syncEventStat.endDownloadTime = GetInt64Value(KEY_END_DOWNLOAD_TIME, stat);
    syncEventStat.downloadType = GetIntValue(KEY_DOWNLOAD_TYPE, stat);
    syncEventStat.totalPhotoCount = GetIntValue(KEY_TOTAL_PHOTO_COUNT, stat);
 
    syncEventStat.totalAlbumNum = GetIntValue(KEY_TOTAL_ALBUM_NUM, stat);
    syncEventStat.addAlbumNum = GetIntValue(KEY_ADD_ALBUM_NUM, stat);
    syncEventStat.updateAlbumNum = GetIntValue(KEY_UPDATE_ALBUM_NUM, stat);
    syncEventStat.deleteAlbumNum = GetIntValue(KEY_DELETE_ALBUM_NUM, stat);
 
    syncEventStat.totalAssetNum = GetIntValue(KEY_TOTAL_ASSET_NUM, stat);
    syncEventStat.addAssetNum = GetIntValue(KEY_ADD_ASSET_NUM, stat);
    syncEventStat.updateAssetNum = GetIntValue(KEY_UPDATE_ASSET_NUM, stat);
    syncEventStat.deleteAssetNum = GetIntValue(KEY_DELETE_ASSET_NUM, stat);
 
    syncEventStat.avgRefreshImageVideoAlbumTime = GetIntValue(KEY_REFRESH_IMAGEVIDEO_ALBUM_TOTAL_TIME, stat) /
                                            std::max(GetIntValue(KEY_REFRESH_IMAGEVIDEO_ALBUM_TOTAL_COUNT, stat), 1);
 
    std::set<int32_t> user_set(user_alubm_count_.begin(), user_alubm_count_.end());
    syncEventStat.refreshUserAndSourceAlbumCount = static_cast<int32_t>(user_set.size());
    syncEventStat.avgRefreshUserAndSourceAlbumTime =
        GetIntValue(KEY_REFRESH_USER_AND_SOURCE_ALBUM_TOTAL_TIME, stat) /
        std::max(static_cast<int32_t>(user_alubm_count_.size()), 1);
 
    std::set<int32_t> analysis_set(analysis_alunm_count_.begin(), analysis_alunm_count_.end());
    syncEventStat.refreshAnalysisAlbumCount = static_cast<int32_t>(analysis_set.size());
    syncEventStat.avgRefreshAnalysisAlbumTime = GetIntValue(KEY_REFRESH_ANALYSIS_ALBUM_TOTAL_TIME, stat) /
                                                std::max(static_cast<int32_t>(analysis_alunm_count_.size()), 1);
    return syncEventStat;
}

void PostEventUtils::PostCloudDownloadSyncStat(std::string& syncId)
{
    if (syncId != currentSyncId_) {
        MEDIA_ERR_LOG("Invaild syncid! syncid is %{public}s and currentSyncId is %{public}s.",
            syncId.c_str(),
            currentSyncId_.c_str());
        return;
    }
    std::unique_lock<std::mutex> lock(cloudDownloadSyncStatMutex_);
    VariantMap stat;
    stat.swap(cloudDownloadSyncStat_);
    lock.unlock();
    SyncEventStat syncEventStat = PostEventUtils::GetSyncEventStat(stat);
    if (syncEventStat.totalAlbumNum == 0 && syncEventStat.totalAssetNum == 0 &&
        syncEventStat.avgRefreshImageVideoAlbumTime == 0 && syncEventStat.avgRefreshUserAndSourceAlbumTime == 0 &&
        syncEventStat.avgRefreshAnalysisAlbumTime == 0) {
        return;
    }
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_CLOUD_SYNC_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        KEY_START_DOWNLOAD_TIME, syncEventStat.startDownloadTime,
        KEY_END_DOWNLOAD_TIME, syncEventStat.endDownloadTime,
        KEY_DOWNLOAD_TYPE, syncEventStat.downloadType,
        KEY_TOTAL_PHOTO_COUNT, syncEventStat.totalPhotoCount,
        KEY_TOTAL_ALBUM_NUM, syncEventStat.totalAlbumNum,
        KEY_ADD_ALBUM_NUM, syncEventStat.addAlbumNum,
        KEY_UPDATE_ALBUM_NUM, syncEventStat.updateAlbumNum,
        KEY_DELETE_ALBUM_NUM, syncEventStat.deleteAlbumNum,
        KEY_TOTAL_ASSET_NUM, syncEventStat.totalAssetNum,
        KEY_ADD_ASSET_NUM, syncEventStat.addAssetNum,
        KEY_UPDATE_ASSET_NUM, syncEventStat.updateAssetNum,
        KEY_DELETE_ASSET_NUM, syncEventStat.deleteAssetNum,
        KEY_AVG_REFRESH_IMAGEVIDEO_ALBUM_TIME, syncEventStat.avgRefreshImageVideoAlbumTime,
        KEY_REFRESH_USER_AND_SOURCE_ALBUM_COUNT, syncEventStat.refreshUserAndSourceAlbumCount,
        KEY_AVG_REFRESH_USER_AND_SOURCE_ALBUM_TIME, syncEventStat.avgRefreshUserAndSourceAlbumTime,
        KEY_REFRESH_ANALYSIS_ALBUM_COUNT, syncEventStat.refreshAnalysisAlbumCount,
        KEY_AVG_REFRESH_ANALYSIS_ALBUM_TIME, syncEventStat.avgRefreshAnalysisAlbumTime);
    if (ret != 0) {
        MEDIA_ERR_LOG("PostCloudDownloadSyncStat:%{public}d", ret);
    }
}

void PostEventUtils::PostStatProcess(const uint32_t &statType, const VariantMap &stat)
{
    switch (statType) {
        case StatType::THUMBNAIL_STAT:
        case StatType::DB_UPGRADE_STAT:
        case StatType::SYNC_STAT:
        case StatType::AGING_STAT:
            break;
        case StatType::MSC_FIRST_VISIT_STAT:
            PostMscFirstVisitStat(stat);
            break;
        case StatType::MSC_REQUEST_POLICY_STAT:
            PostMscRequestPolicyStat(stat);
            break;
        case StatType::MSC_TRIGGER_RATIO_STAT:
            PostMscTriggerRatioStat(stat);
            break;
        case StatType::MSC_TOTAL_TIME_COST_STAT:
            PostMscTotalTimeCostStat(stat);
            break;
        case StatType::MSC_RESULT_STAT:
            PostMscResultStat(stat);
            break;
        case StatType::CLOUD_ENHANCEMENT_GET_COUNT_STAT:
            PostCloudEnhanceStat(stat);
            break;
        default:
            PostThumbnailStat(stat);
            PostDbUpgradeStat(stat);
            PostSyncStat();
            PostAgingStat(stat);
            break;
    }
}
}  // namespace Media
}  // namespace OHOS
