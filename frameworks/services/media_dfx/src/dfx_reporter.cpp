/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "DfxReporter"

#include "dfx_reporter.h"

#include <vector>

#include "dfx_const.h"
#include "dfx_utils.h"
#include "dfx_database_utils.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "hisysevent.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_inotify.h"
#include "medialibrary_astc_stat.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
constexpr uint64_t MB_SIZE = 1024 * 1024;

DfxReporter::DfxReporter()
{
}

DfxReporter::~DfxReporter()
{
}

void DfxReporter::ReportControllerService(uint32_t operationCode, int32_t errorCode)
{
    std::string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_SERVICE_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "BUNDLE_NAME", bundleName,
        "OPERATION_CODE", operationCode,
        "ERROR_CODE", errorCode);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportControllerService error:%{public}d", ret);
    }
}

void DfxReporter::ReportTimeOutOperation(std::string &bundleName, int32_t type, int32_t object, int32_t time)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_TIMEOUT_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "BUNDLE_NAME", bundleName,
        "OPERATION_TYPE", type,
        "OPERATION_OBJECT", object,
        "TIME", time);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportTimeoutOperation error:%{public}d", ret);
    }
}

int32_t DfxReporter::ReportHighMemoryImageThumbnail(std::string &path, std::string &suffix, int32_t width,
    int32_t height)
{
    if (suffix != "jpg" && suffix != "jpeg" && suffix != "jpe") {
        MEDIA_WARN_LOG("image %{public}s is %{public}s, width: %{public}d, height: %{public}d", path.c_str(),
            suffix.c_str(), width, height);
        return OTHER_FORMAT_IMAGE;
    } else if (width > IMAGE_MIN && height > IMAGE_MIN) {
        MEDIA_WARN_LOG("image %{public}s is too large, width: %{public}d, height: %{public}d", path.c_str(), width,
            height);
        return BIG_IMAGE;
    }
    return COMMON_IMAGE;
}

int32_t DfxReporter::ReportHighMemoryVideoThumbnail(std::string &path, std::string &suffix, int32_t width,
    int32_t height)
{
    if (width >= VIDEO_8K_MIN || height >= VIDEO_8K_MIN) {
        MEDIA_WARN_LOG("video %{public}s is too large, width: %{public}d, height: %{public}d", path.c_str(), width,
            height);
        return BIG_VIDEO;
    }
    return COMMON_VIDEO;
}

void DfxReporter::ReportThumbnailError()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(THUMBNAIL_ERROR_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    map<string, NativePreferences::PreferencesValue> errorMap = prefs->GetAll();
    for (auto &erroInfo : errorMap) {
        string key = erroInfo.first;
        string value = erroInfo.second;
        vector<string> thumbnailInfo = DfxUtils::Split(key, SPLIT_CHAR);
        if (thumbnailInfo.empty() || thumbnailInfo.size() < 3) { // 3 means length of key
            continue;
        }
        // 0 means index of path
        string path = thumbnailInfo[0];
        // 1 means index of method
        int32_t method = MediaLibraryDataManagerUtils::IsNumber(thumbnailInfo[1]) ? stoi(thumbnailInfo[1]) : 0;
        // 2 means index of error code
        int32_t errorCode = MediaLibraryDataManagerUtils::IsNumber(thumbnailInfo[2]) ? stoi(thumbnailInfo[2]) : 0;
        int64_t time = MediaLibraryDataManagerUtils::IsNumber(value) ? stol(value) : 0;
        int ret = HiSysEventWrite(
            MEDIA_LIBRARY,
            "MEDIALIB_THUMBNAIL_ERROR",
            HiviewDFX::HiSysEvent::EventType::FAULT,
            "PATH", path,
            "METHOD", method,
            "ERROR_CODE", errorCode,
            "TIME", time);
        if (ret != 0) {
            MEDIA_ERR_LOG("ReportThumbnailError error:%{public}d", ret);
        }
    }
    prefs->Clear();
    prefs->FlushSync();
}

void DfxReporter::ReportCommonBehavior()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(COMMON_BEHAVIOR_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    map<string, NativePreferences::PreferencesValue> errorMap = prefs->GetAll();
    for (auto &erroInfo : errorMap) {
        string bundleName = erroInfo.first;
        int32_t times = static_cast<int32_t>(erroInfo.second);
        int ret = HiSysEventWrite(
            MEDIA_LIBRARY,
            "MEDIALIB_COMMON_STATISTIC",
            HiviewDFX::HiSysEvent::EventType::STATISTIC,
            "BUNDLE_NAME", bundleName,
            "TIMES", times);
        if (ret != 0) {
            MEDIA_ERR_LOG("ReportCommonBehavior error:%{public}d", ret);
        }
    }
    prefs->Clear();
    prefs->FlushSync();
}

void DfxReporter::ReportDeleteStatistic()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DELETE_BEHAVIOR_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    map<string, NativePreferences::PreferencesValue> deleteMap = prefs->GetAll();
    for (auto &info : deleteMap) {
        string key = info.first;
        vector<string> deleteInfo = DfxUtils::Split(key, SPLIT_CHAR);
        if (deleteInfo.empty() || deleteInfo.size() < 2) { // 2 means length of key
            continue;
        }
        // 0 means index of bundleName
        string bundleName = deleteInfo[0];
        // 1 means index of type
        int32_t type = MediaLibraryDataManagerUtils::IsNumber(deleteInfo[1]) ? stoi(deleteInfo[1]) : 0;
        int32_t times = static_cast<int32_t>(info.second);
        int ret = HiSysEventWrite(
            MEDIA_LIBRARY,
            "MEDIALIB_DELETE_STATISTIC",
            HiviewDFX::HiSysEvent::EventType::STATISTIC,
            "BUNDLE_NAME", bundleName,
            "TYPE", type,
            "TIMES", times);
        if (ret != 0) {
            MEDIA_ERR_LOG("ReportDeleteBehavior error:%{public}d", ret);
        }
    }
    prefs->Clear();
    prefs->FlushSync();
}

void DfxReporter::ReportDeleteBehavior(string bundleName, int32_t type, std::string path)
{
    if (bundleName == "" || path == "") {
        return;
    }
    MEDIA_WARN_LOG("%{public}s do %{public}d on %{public}s", bundleName.c_str(), type, path.c_str());
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DELETE_BEHAVIOR",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "BUNDLE_NAME", bundleName,
        "TYPE", type,
        "PATH", path);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportDeleteBehavior error:%{public}d", ret);
    }
}

void DfxReporter::ReportThumbnailGeneration(const ThumbnailData::GenerateStats &stats)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_THUMBNAIL_GENERATION",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "URI", stats.uri,
        "SCENE", static_cast<int32_t>(stats.scene),
        "OPEN_THUMB_COST", stats.openThumbCost,
        "OPEN_LCD_COST", stats.openLcdCost,
        "SOURCE_TYPE", static_cast<int32_t>(stats.sourceType),
        "SOURCE_WIDTH", stats.sourceWidth,
        "SOURCE_HEIGHT", stats.sourceHeight,
        "TOTAL_COST", static_cast<int32_t>(stats.totalCost),
        "ERROR_CODE", stats.errorCode);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportThumbnailGeneration error:%{public}d", ret);
    }
}

void DfxReporter::ReportPhotoInfo(const PhotoStatistics& stats)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_PHOTO_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "LOCAL_IMAGE_COUNT", stats.localImageCount,
        "LOCAL_VIDEO_COUNT", stats.localVideoCount,
        "CLOUD_IMAGE_COUNT", stats.cloudImageCount,
        "CLOUD_VIDEO_COUNT", stats.cloudVideoCount,
        "SHARED_IMAGE_COUNT", stats.sharedImageCount,
        "SHARED_VIDEO_COUNT", stats.sharedVideoCount,
        "SOUTH_DEVICE_TYPE", stats.southDeviceType,
        "WAITING_COUNT", stats.tasksWaitingCount,
        "DOWNLOADING_COUNT", stats.tasksDownloadingCount,
        "PAUSE_COUNT", stats.tasksPauseCount,
        "FAILED_COUNT", stats.tasksFailedCount,
        "SUCC_COUNT", stats.tasksSuccessCount,
        "AUTO_PAUSE_COUNT", stats.tasksAutoPauseCount,
        "SUCC_TOTAL_SIZE", stats.tasksSuccessTotalSize,
        "SUCC_TOTAL_TIME", stats.tasksSuccessTotalTime,
        "USER_ALBUM_COUNT", stats.userAlbumCount,
        "SOURCE_ALBUM_COUNT", stats.sourceAlbumCount,
        "UPLOAD_USER_ALBUM_COUNT", stats.uploadUserAlbumCount,
        "UPLOAD_SOURCE_ALBUM_COUNT", stats.uploadSourceAlbumCount,
        "NOT_UPLOAD_ASSET_COUNT", stats.notUploadAssetCount);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportPhotoInfo error:%{public}d", ret);
    }
}

void DfxReporter::ReportAstcInfo(const LcdAndAstcCount& count)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ASTC_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "LOCAL_LCD_COUNT", count.localLcdCount,
        "LOCAL_ASTC_COUNT", count.localAstcCount,
        "CLOUD_LCD_COUNT", count.cloudLcdCount,
        "CLOUD_ASTC_COUNT", count.cloudAstcCount,
        "PHASE_DETAIL", MediaLibraryAstcStat::GetInstance().GetJson());
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportAstcInfo error:%{public}d", ret);
    } else {
        MediaLibraryAstcStat::GetInstance().ClearOldData();
    }
}

void DfxReporter::ReportAlbumInfo(const std::string &albumName, int32_t albumImageCount, int32_t albumVideoCount,
    bool isLocal)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ALBUM_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "ALBUM_NAME", albumName,
        "ALBUM_IMAGE_COUNT", albumImageCount,
        "ALBUM_VIDEO_COUNT", albumVideoCount,
        "IS_LOCAL", isLocal);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportAlbumInfo error:%{public}d", ret);
    }
}

void DfxReporter::ReportDirtyCloudPhoto(const std::string &data, int32_t dirty, int32_t cloudVersion)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DIRTY_CLOUD_PHOTO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "PATH", data,
        "DIRTY", dirty,
        "CLOUD_VERSION", cloudVersion);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportDirtyCloudPhoto error:%{public}d", ret);
    }
}

void DfxReporter::ReportCommonVersion(int32_t dbVersion)
{
    MEDIA_INFO_LOG("dbVersion: %{public}d, thumbnailVersion: %{public}d", dbVersion, THUMBNAIL_VERSION);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_COMMON_VERSION",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "DB_VERSION", dbVersion,
        "THUMBNAIL_VERSION", THUMBNAIL_VERSION);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportCommonVersion error:%{public}d", ret);
    }
}

void DfxReporter::ReportAnalysisVersion(const std::string &analysisName, int32_t version)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ANALYSIS_VERSION",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "NAME", analysisName,
        "VERSION", version);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportAnalysisVersion error:%{public}d", ret);
    }
}

void DfxReporter::ReportAdaptationToMovingPhoto()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ADAPTATION_TO_MOVING_PHOTO_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }

    string date = DfxUtils::GetCurrentDate();
    string unadaptedAppPackages = prefs->GetString(MOVING_PHOTO_KEY_UNADAPTED_PACKAGE);
    string adaptedAppPackages = prefs->GetString(MOVING_PHOTO_KEY_ADAPTED_PACKAGE);
    int32_t unadaptedAppNum = prefs->GetInt(MOVING_PHOTO_KEY_UNADAPTED_NUM);
    int32_t adaptedAppNum = prefs->GetInt(MOVING_PHOTO_KEY_ADAPTED_NUM);

    prefs->Clear();
    prefs->FlushSync();

    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_MOVING_PHOTO_ADAPT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "DATE", date,
        "UNADAPTED_APP_NUM", unadaptedAppNum,
        "UNADAPTED_APP_PACKAGE", unadaptedAppPackages,
        "ADAPTED_APP_NUM", adaptedAppNum,
        "ADAPTED_APP_PACKAGE", adaptedAppPackages);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report adaptation to moving photo error:%{public}d", ret);
    }

    prefs->Clear();
    prefs->FlushSync();
}

void DfxReporter::ReportCinematicVideo()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_CINEMATIC_VIDEO_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }

    string date = DfxUtils::GetCurrentDate();
    int32_t lowQualityAccessTimes = prefs->GetInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES);
    int32_t highQualityAccessTimes = prefs->GetInt(CINEMATIC_VIDEO_KEY_HIGH_QUALITY_ACCESS_TIMES);
    int32_t lowQualityUriAccessTimes = prefs->GetInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_URI_ACCESS_TIMES);
    int32_t highQualityUriAccessTimes = prefs->GetInt(CINEMATIC_VIDEO_KEY_HIGH_QUALITY_URI_ACCESS_TIMES);
    int32_t cancelNum = prefs->GetInt(CINEMATIC_VIDEO_KEY_CANCEL_NUM);
    int32_t cancelWaitAvgTime = prefs->GetInt(CINEMATIC_VIDEO_KEY_CANCEL_WAIT_AVG_TIME);
    int32_t processAvgTime = prefs->GetInt(CINEMATIC_VIDEO_PROCESS_AVG_TIME);
    int32_t multistageSuccessTime = prefs->GetInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_SUCCESS_TIMES);
    int32_t multistageFailedTime = prefs->GetInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_FAILED_TIMES);

    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "CINEMATIC_VIDEO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "DATE", date,
        "LOW_QUALITY_ACCESS_TIMES", lowQualityAccessTimes,
        "HIGH_QUALITY_ACCESS_TIMES", highQualityAccessTimes,
        "LOW_QUALITY_ACCESS_URI_TIMES", lowQualityUriAccessTimes,
        "HIGH_QUALITY_ACCESS_URI_TIMES", highQualityUriAccessTimes,
        "CANCEL_NUM", cancelNum,
        "CANCEL_WAIT_AVG_TIME", cancelWaitAvgTime,
        "PROCESS_AVG_TIME", processAvgTime,
        "MULTISTAGE_SUCCESS_TIMES", multistageSuccessTime,
        "MULTISTAGE_FAILED_TIMES", multistageFailedTime);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report adaptation to moving photo error:%{public}d", ret);
    }

    prefs->Clear();
    prefs->FlushSync();
}

void DfxReporter::ReportStartResult(int32_t scene, int32_t error, int32_t start)
{
    int32_t cost = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - start);
    MEDIA_ERR_LOG("SCENE:%{public}d, ERROR:%{public}d, TIME:%{public}d", scene, error, cost);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_START_RESULT",
        HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        "SCENE", scene,
        "ERROR", error,
        "TIME", cost);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report startResult error:%{public}d", ret);
    }
}

std::string SecondsToTime(const int64_t& seconds)
{
    int32_t remain_seconds = seconds;
    int32_t hour = seconds / ONE_HOUR;
    remain_seconds = seconds - ONE_HOUR * hour;
    int32_t minute = remain_seconds / ONE_MINUTE;
    remain_seconds = remain_seconds - minute * ONE_MINUTE;
    return std::to_string(hour) + "_h_" + std::to_string(minute) + "_m_" + std::to_string(remain_seconds) + "_s";
}

int32_t DfxReporter::ReportCloudSyncThumbGenerationStatus(const int32_t& downloadedThumb,
    const int32_t& generatedThumb, const int32_t& totalDownload, const int32_t& southDeviceType)
{
    if (totalDownload == 0) {
        return 0;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return 0;
    }
    int64_t start = prefs->GetLong(CLOUD_SYNC_START_TIME, 0);
    int64_t now = MediaFileUtils::UTCTimeSeconds();
    int64_t cost = now - start;
    time_t startTime = start + ONE_HOUR * 8;
    std::string astcStartTime = asctime(gmtime(&startTime));
    float total = static_cast<float>(totalDownload);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "CLOUD_THUMB_GENERATE_STATISTIC",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "CLOUD_DOWN_START", astcStartTime,
        "DOWNLOADED_THUMB_NUM", downloadedThumb,
        "DOWNLOADED_THUMB_RATIO", downloadedThumb / total,
        "GENERATED_THUMB_NUM", generatedThumb,
        "GENERATED_THUMB_RATIO", generatedThumb / total,
        "CLOUD_DOWN_TOTAL_DURATION", SecondsToTime(cost),
        "SOUTH_DEVICE_TYPE", southDeviceType);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report CloudSyncThumbGenerationStatus error:%{public}d", ret);
    }
    return ret;
}

static string GetWatchListInfo()
{
    auto watch = MediaLibraryInotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("MediaLibraryInotify GetInstance fail");
        return "";
    }
    return watch->BuildDfxInfo();
}

void DfxReporter::ReportPhotoRecordInfo()
{
    PhotoRecordInfo photoRecordInfo;
    int32_t result = DfxDatabaseUtils::QueryPhotoRecordInfo(photoRecordInfo);
    if (result != 0) {
        MEDIA_ERR_LOG("QueryPhotoRecordInfo error:%{public}d", result);
        return;
    }
    int32_t imageCount = photoRecordInfo.imageCount;
    int32_t videoCount = photoRecordInfo.videoCount;
    int32_t abnormalSizeCount = photoRecordInfo.abnormalSizeCount;
    int32_t abnormalWidthOrHeightCount = photoRecordInfo.abnormalWidthOrHeightCount;
    int32_t abnormalVideoDurationCount = photoRecordInfo.abnormalVideoDurationCount;
    int32_t toBeUpdatedRecordCount = photoRecordInfo.toBeUpdatedRecordCount;
    int64_t dbFileSize = photoRecordInfo.dbFileSize;
    int32_t duplicateLpathCount = photoRecordInfo.duplicateLpathCount;
    int32_t abnormalLpathCount = photoRecordInfo.abnormalLpathCount;
    int32_t photoWaitUploadCloudCount = photoRecordInfo.photoWaitUploadCloudCount;
    int32_t photoWaitUploadHdcCount = photoRecordInfo.photoWaitUploadHdcCount;
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DATABASE_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "DB_FILE_SIZE", dbFileSize,
        "REPLICA_DB_FILE_SIZE", photoRecordInfo.slaveDbFileSize,
        "IMAGE_COUNT", imageCount,
        "VIDEO_COUNT", videoCount,
        "ABNORMAL_SIZE_COUNT", abnormalSizeCount,
        "ABNORMAL_WIDTH_OR_HEIGHT_COUNT", abnormalWidthOrHeightCount,
        "ABNORMAL_VIDEO_DURATION_COUNT", abnormalVideoDurationCount,
        "ABNORMAL_COUNT_TO_UPDATE", toBeUpdatedRecordCount,
        "DUPLICATE_LPATH_COUNT", duplicateLpathCount,
        "ABNORMAL_LPATH_COUNT", abnormalLpathCount,
        "PHOTO_WAIT_UPLOAD_CLOUD_COUNT", photoWaitUploadCloudCount,
        "PHOTO_WAIT_UPLOAD_HDC_COUNT", photoWaitUploadHdcCount,
        "WATCH_LIST_INFO", GetWatchListInfo());
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportPhotoRecordInfo error:%{public}d", ret);
    }
}

void DfxReporter::ReportOperationRecordInfo()
{
    OperationRecordInfo operationRecordInfo;
    int32_t result = DfxDatabaseUtils::QueryOperationRecordInfo(operationRecordInfo);
    if (result != 0) {
        MEDIA_ERR_LOG("ReportOperationRecordInfo error:%{public}d", result);
        return;
    }
    int32_t addTotalCount = operationRecordInfo.addTotalCount;
    int32_t delTotalCount = operationRecordInfo.delTotalCount;
    int32_t updateTotalCount = operationRecordInfo.updateTotalCount;
    int32_t totalCount = operationRecordInfo.totalCount;
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_OPRN_CURRENT_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "CURRENT_OPT_ADD_COUNT", addTotalCount,
        "CURRENT_OPT_DELETE_COUNT", delTotalCount,
        "CURRENT_OPT_UPDATE_COUNT", updateTotalCount,
        "CURRENT_OPT_TOTAL_COUNT", totalCount,
        "CURRENT_TIME", currentTime);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportOperationRecordInfo error:%{public}d", ret);
    }

    static int32_t lastAddTotalCount = 0;
    static int32_t lastDelTotalCount = 0;
    static int32_t lastUpdateTotalCount = 0;
    static int32_t lastTotalCount = 0;
    static int64_t lastOptQueryTime = 0;
    ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_OPRN_CHANGE_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "OPT_ADD_COUNT", addTotalCount - lastAddTotalCount,
        "OPT_DELETE_COUNT", delTotalCount - lastDelTotalCount,
        "OPT_UPDATE_COUNT", updateTotalCount - lastUpdateTotalCount,
        "OPT_TOTAL_COUNT", totalCount - lastTotalCount,
        "OPT_START_TIME", lastOptQueryTime,
        "OPT_END_TIME", currentTime);

    lastAddTotalCount = addTotalCount;
    lastDelTotalCount = delTotalCount;
    lastUpdateTotalCount = updateTotalCount;
    lastTotalCount = totalCount;
    lastOptQueryTime = currentTime;
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportOperationRecordInfo last info error:%{public}d", ret);
    }
}

int32_t DfxReporter::ReportMedialibraryAPI(const string& callerPackage, const string& saveUri)
{
    string currentDate = DfxUtils::GetCurrentDate();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_DEPRECATED_API_USAGE",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "DATE", currentDate,
        "CALLER_APP_PACKAGE", callerPackage,
        "SAVE_URI", saveUri,
        "READ_URI", "");
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportMedialibraryAPI failed, ret: %{public}d", ret);
        return E_FAIL;
    }
    return E_SUCCESS;
}

int32_t DfxReporter::ReportAlbumFusion(const AlbumFusionDfxDataPoint& reportData)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ALBUM_FUSION_SINGLE",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "ALBUM_FUSION_TAG", reportData.albumFusionTag,
        "REPORT_TIME_STAMP", reportData.reportTimeStamp,
        "ALBUM_FUSION_STATE", reportData.albumFusionState,
        "IMAGE_ASSET_COUNT", reportData.imageAssetCount,
        "VIDEO_ASSET_COUNT", reportData.videoAssetCount,
        "NUMBER_OF_SOURCE_ALBUM", reportData.numberOfSourceAlbum,
        "NUMBER_OF_USER_ALBUM", reportData.numberOfUserAlbum,
        "TOTAL_ASSETS_IN_SOURCE_ALBUMS", reportData.totalAssetsInSourceAlbums,
        "TOTAL_ASSETS_IN_USER_ALBUMS", reportData.totalAssetsInUserAlbums,
        "ALBUM_DETAILS", reportData.albumDetails,
        "HIDDEN_ASSET_COUNT", reportData.hiddenAssetInfo);
    if (ret != 0) {
        MEDIA_ERR_LOG("ALBUM FUSION report data failed, ret: %{public}d", ret);
        return E_FAIL;
    }
    return E_SUCCESS;
}

int32_t DfxReporter::ReportCustomRestoreFusion(const CustomRestoreDfxDataPoint& reportData)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_CUSTOM_RESTORE",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "CUSTOM_RESTORE_PACKAGE_NAME", reportData.customRestorePackageName,
        "ALBUM_LPATH", reportData.albumLPath,
        "KEY_PATH", reportData.keyPath,
        "TOTAL_NUM", reportData.totalNum,
        "SUCCESS_NUM", reportData.successNum,
        "FAILED_NUM", reportData.failedNum,
        "SAME_NUM", reportData.sameNum,
        "CANCEL_NUM", reportData.cancelNum,
        "TOTAL_TIME", reportData.totalTime);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report CustomRestoreFusion error: %{public}d", ret);
    }
    return ret;
}

int32_t DfxReporter::ReportPhotoError(const PhotoErrorCount& reportData)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_PHOTO_ERROR_STATISTIC",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "PHOTO_ERROR_TYPE", reportData.photoErrorTypes,
        "PHOTO_ERROR_COUNT", reportData.photoErrorCounts);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report ReportPhotoError error: %{public}d", ret);
    }
    return ret;
}

int32_t DfxReporter::ReportSyncFault(const std::string& taskId, const std::string& position,
    const SyncFaultEvent& event, const int32_t& southDeviceType)
{
    int32_t ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_SYNC_FAULT",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "TASK_ID", taskId,
        "FAULT_SCENARIO", static_cast<uint32_t>(event.scenario),
        "FAULT_TYPE", static_cast<uint32_t>(event.type),
        "FAULT_ERROR_CODE", event.errorCode,
        "FUNCTION_NAME", position,
        "MESSAGE", event.message,
        "SOUTH_DEVICE_TYPE", southDeviceType);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("report ReportSyncFault error %{public}d", ret);
    }
    return ret;
}

int32_t DfxReporter::ReportSyncStat(const std::string& taskId, const CloudSyncInfo& info, const CloudSyncStat& stat,
    const std::string& syncInfo, const int32_t& southDeviceType)
{
    int32_t ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_SYNC_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "TASK_ID", taskId,
        "SYNC_REASON", info.syncReason,
        "STOP_REASON", info.stopReason,
        "START_TIME", info.startTime,
        "DURATION", info.duration,
        "UPLOAD_META", stat.uploadMeta,
        "DOWNLOAD_META", stat.downloadMeta,
        "DOWNLOAD_THM", stat.downloadThumb,
        "DOWNLOAD_LCD", stat.downloadLcd,
        "UPLOAD_ALBUM", stat.uploadAlbum,
        "DOWNLOAD_ALBUM", stat.downloadAlbum,
        "UPLOAD_META_ERR", stat.uploadMetaErr,
        "SYNC_INFO", syncInfo,
        "SOUTH_DEVICE_TYPE", southDeviceType);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("report ReportSyncStat error %{public}d", ret);
    }
    return ret;
}

void DfxReporter::ReportPhotoSizeAndResolutionInfo(const QuerySizeAndResolution& querySizeAndResolution,
    const std::string& photoMimeType)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "PHOTO_INFO_EXT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "LOCAL_IMAGE_SIZE", querySizeAndResolution.localImageSize,
        "LOCAL_VIDEO_SIZE", querySizeAndResolution.localVideoSize,
        "CLOUD_IMAGE_SIZE", querySizeAndResolution.cloudImageSize,
        "CLOUD_VIDEO_SIZE", querySizeAndResolution.cloudVideoSize,
        "PHOTO_MIMETYPE", photoMimeType,
        "LOCAL_IMAGE_RESOLUTION", querySizeAndResolution.localImageResolution,
        "LOCAL_VIDEO_RESOLUTION", querySizeAndResolution.localVideoResolution,
        "CLOUD_IMAGE_RESOLUTION", querySizeAndResolution.cloudImageResolution,
        "CLOUD_VIDEO_RESOLUTION", querySizeAndResolution.cloudVideoResolution,
        "LOCAL_IMAGE_ROM_SIZE", querySizeAndResolution.localImageRomSize,
        "LOCAL_VIDEO_ROM_SIZE", querySizeAndResolution.localVideoRomSize,
        "CACHE_ROM_SIZE", querySizeAndResolution.cacheRomSize,
        "HIGHLIGHT_ROM_SIZE", querySizeAndResolution.highlightRomSize,
        "THUMBNAIL_ROM_SIZE", querySizeAndResolution.ThumbnailRomSize,
        "EDITDATA_ROM_SIZE", querySizeAndResolution.EditdataRomSize);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report ReportPhotoSizeAndResolutionInfo error: %{public}d", ret);
    }
}

void DfxReporter::ReportAccurateRefreshResult(const AccurateRefreshDfxDataPoint& reportData)
{
    std::string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_TIMEOUT_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "BUNDLE_NAME", bundleName,
        "SQL", reportData.sqlStr,
        "TARGET_BUSINESS", reportData.targetBusiness,
        "TOTAL_COST_TIME", reportData.totalCostTime,
        "STANDARD_COST_TIME", reportData.standardCostTime,
        "PHOTO_OPERATION_TOTAL_TIME", reportData.photoOperationTotalTime,
        "ALBUM_OPERATION_TOTAL_TIME", reportData.albumOperationTotalTime,
        "ALBUM_ID", reportData.albumId,
        "ALBUM_OPERATION_TIME", reportData.albumOperationTime,
        "ALBUM_HIDDEN_INFO_OPERATION_TIME", reportData.albumHiddenInfoOperationTime);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportAccurateRefreshService error:%{public}d", ret);
    }
}

int32_t DfxReporter::reportHeifAgingStatistics(const HeifAgingStatistics& heifAgingStatistics)
{
    string date = DfxUtils::GetCurrentDate();
    uint64_t transcodeTotalSize = (heifAgingStatistics.transcodeTotalSize) / MB_SIZE;
    uint64_t agingTotalSize = (heifAgingStatistics.agingTotalSize) / MB_SIZE;
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_HEIF_AGING",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "TRANSCODE_FILE_NUM", heifAgingStatistics.transcodeFileNum,
        "TRANSCODE_TOTAL_SIZE", transcodeTotalSize,
        "AGING_FILE_NUM", heifAgingStatistics.agingFileNum,
        "AGING_TOTAL_SIZE", agingTotalSize);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportHeifAgingStatistics error:%{public}d", ret);
    }
    return ret;
}

void DfxReporter::ReportAlibHeifDuplicate()
{
    MEDIA_INFO_LOG("ReportAlibHeifDuplicate start");
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ALIB_HEIF_DUPLICATE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    string date = DfxUtils::GetCurrentDate();
    int32_t transcodeAccessTimes = prefs->GetInt(TRANSCODE_ACCESS_TIMES, 0);
    int32_t transcodeAccessMedialib = prefs->GetInt(TRANSCODE_ACCESS_MEDIALIB, 0);
    int32_t transcodeAccessLibc = prefs->GetInt(TRANSCODE_ACCESS_LIBC, 0);
    int32_t transcodeAvgTime = 0;
    int32_t transcodeTimes = prefs->GetInt(TRANSCODE_TIMES, 0);
    if (transcodeTimes != 0) {
        transcodeAvgTime = prefs->GetInt(TRANSCODE_AVG_TIME, 0) / prefs->GetInt(TRANSCODE_TIMES, 0);
    }
    int32_t transcodeFailedTimes = prefs->GetInt(TRANSCODE_FAILED_TIMES, 0);
    int32_t innerFailedTimes = prefs->GetInt(INNER_FAILED_TIMES, 0);
    int32_t codecFailedTimes = prefs->GetInt(CODEC_FAILED_TIMES, 0);
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_HEIF_ACCESS_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "TRANSCODE_ACCESS_TIMES", transcodeAccessTimes,
        "TRANSCODE_ACCESS_MEDIALIB", transcodeAccessMedialib,
        "TRANSCODE_ACCESS_LIBC", transcodeAccessLibc,
        "TRANSCODE_AVG_TIME", transcodeAvgTime,
        "TRANSCODE_TIMES", transcodeTimes,
        "TRANSCODE_FAILED_TIMES", transcodeFailedTimes,
        "INNER_FAILED_TIMES", innerFailedTimes,
        "CODEC_FAILED_TIMES", codecFailedTimes);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report alib heif duplicate error:%{public}d", ret);
    }

    prefs->Clear();
    prefs->FlushSync();
}

int32_t DfxReporter::ReportUpgradeFault(const UpgradeExceptionInfo& reportData)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_UPGRADE_EXCEPTION",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "SRC_VERSION", reportData.srcVersion,
        "DTS_VERSION", reportData.dstVersion,
        "IS_SYNC", reportData.isSync,
        "EXCEPTION_VERSIONS", reportData.exceptionVersions,
        "DURATION", reportData.duration);
    if (ret != 0) {
        MEDIA_ERR_LOG("ReportUpgradeFault error:%{public}d", ret);
    }
    return ret;
}

int32_t DfxReporter::ReportAncoCheckInfo(const AncoCheckInfo& reportData)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ANCO_CHECK_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "CHECK_START_TIME", reportData.checkStartTime,
        "CHECK_END_TIME", reportData.checkEndTime,
        "CHECK_ADD", reportData.checkAdd,
        "CHECK_UPDATE", reportData.checkUpdate,
        "CHECK_DELETE", reportData.checkDelete);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report AncoCheckInfo error: %{public}d", ret);
    }
    return ret;
}
 
int32_t DfxReporter::ReportAncoOperationChangeInfo(const AncoOperationChangeInfo& reportData)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ANCO_OPRN_CHANGE_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "PHOTO_OPT_ADD_COUNT", reportData.photoOptAddCount,
        "PHOTO_OPT_UPDATE_COUNT", reportData.photoOptUpdateCount,
        "PHOTO_OPT_DELETE_COUNT", reportData.photoOptDeleteCount,
        "ALBUM_OPT_ADD_COUNT", reportData.albumOptAddCount,
        "ALBUM_OPT_UPDATE_COUNT", reportData.albumOptUpdateCount,
        "ALBUM_OPT_DELETE_COUNT", reportData.albumOptDeleteCount,
        "TOTAL_OPT_COUNT", reportData.totalOptCount);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report AncoOperationChangeInfo error: %{public}d", ret);
    }
    return ret;
}

int32_t DfxReporter::ReportAncoCountFormatInfo(const AncoCountFormatInfo& reportData, bool firstLoad)
{
    int ret = -1;
    if (firstLoad) {
        ret = HiSysEventWrite(
            MEDIA_LIBRARY,
            "MEDIALIB_ANCO_COUNT_FORMAT_INFO",
            HiviewDFX::HiSysEvent::EventType::STATISTIC,
            "LOAD_START_TIME", reportData.loadStartTime,
            "LOAD_END_TIME", reportData.loadEndTime,
            "ALBUM_COUNT", reportData.albumCount,
            "IMAGE_COUNT", reportData.imageCount,
            "VIDEO_COUNT", reportData.videoCount,
            "ASSET_FORMAT_DISTRIBUTION", reportData.assetFormatDistribution);
        MEDIA_INFO_LOG("Report AncoFirstLoadInfo ret: %{public}d", ret);
    } else {
        ret = HiSysEventWrite(
            MEDIA_LIBRARY,
            "MEDIALIB_ANCO_COUNT_FORMAT_INFO",
            HiviewDFX::HiSysEvent::EventType::STATISTIC,
            "ALBUM_COUNT", reportData.albumCount,
            "IMAGE_COUNT", reportData.imageCount,
            "VIDEO_COUNT", reportData.videoCount,
            "ASSET_FORMAT_DISTRIBUTION", reportData.assetFormatDistribution);
    }
    if (ret != 0) {
        MEDIA_ERR_LOG("Report AncoCountFormatInfo error: %{public}d", ret);
    }
    return ret;
}

int32_t DfxReporter::ReportThmInodeCleanInfo(const ThmInodeCleanInfo &info)
{
    int ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_THM_INODE_CLEAN_INFO",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "RESULT", info.result,
        "IS_CONFIG_XATTR", info.isConfigXattr,
        "XATTR_INFO", info.xattrInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "ReportThmInodeCleanInfo error:%{public}d", ret);
    return ret;
}
} // namespace Media
} // namespace OHOS