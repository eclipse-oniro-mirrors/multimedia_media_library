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
#define MLOG_TAG "DfxManager"

#include <sstream>
#include "dfx_manager.h"
#ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
#include "cloud_media_context.h"
#endif
#include "dfx_cloud_manager.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "userfile_manager_types.h"
#include "medialibrary_bundle_manager.h"
#ifdef META_RECOVERY_SUPPORT
#include "medialibrary_meta_recovery.h"
#endif
#include "dfx_database_utils.h"
#include "dfx_deprecated_perm_usage.h"
#include "vision_aesthetics_score_column.h"
#include "parameters.h"
#include "photo_storage_operation.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "hi_audit.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "settings_data_manager.h"
#include "result_set_utils.h"

using namespace std;

namespace OHOS {
namespace Media {

shared_ptr<DfxManager> DfxManager::dfxManagerInstance_{nullptr};
mutex DfxManager::instanceLock_;

struct QueryParams {
    MediaType mediaType;
    PhotoPositionType positionType;
};

shared_ptr<DfxManager> DfxManager::GetInstance()
{
    lock_guard<mutex> lockGuard(instanceLock_);
    if (dfxManagerInstance_ == nullptr) {
        dfxManagerInstance_ = make_shared<DfxManager>();
        if (dfxManagerInstance_ != nullptr) {
            dfxManagerInstance_->Init();
        }
    }
    return dfxManagerInstance_;
}

DfxManager::DfxManager() : isInitSuccess_(false),
    downloadMeta_(DOWNLOAD_META_LEN),
    uploadMeta_(UPLOAD_META_LEN),
    downloadThumb_(DOWNLOAD_THUMB_LEN),
    downloadLcd_(DOWNLOAD_LCD_LEN),
    uploadAlbum_(UPLOAD_ALBUM_LEN),
    downloadAlbum_(DOWNLOAD_ALBUM_LEN),
    updateDetails_(UPDATE_DETAILS_LEN),
    uploadMetaErr_(UPLOAD_META_ERR_LEN)
{
}

DfxManager::~DfxManager()
{
}

void DfxManager::Init()
{
    MEDIA_INFO_LOG("Init DfxManager");
    dfxCollector_ = make_shared<DfxCollector>();
    dfxAnalyzer_ = make_shared<DfxAnalyzer>();
    dfxReporter_ = make_shared<DfxReporter>();
    dfxWorker_ = DfxWorker::GetInstance();
    dfxWorker_->Init();
    isInitSuccess_ = true;
}

void DfxManager::HandleTimeOutOperation(std::string &bundleName, int32_t type, int32_t object, int32_t time)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxReporter_->ReportTimeOutOperation(bundleName, type, object, time);
}

void DfxManager::HandleControllerServiceError(uint32_t operationCode, int32_t errorCode)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxReporter_->ReportControllerService(operationCode, errorCode);
}

int32_t DfxManager::HandleHighMemoryThumbnail(std::string &path, int32_t mediaType, int32_t width,
    int32_t height)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return NOT_INIT;
    }
    string suffix = MediaFileUtils::GetExtensionFromPath(path);
    if (mediaType == MEDIA_TYPE_IMAGE) {
        return dfxReporter_->ReportHighMemoryImageThumbnail(path, suffix, width, height);
    } else {
        return dfxReporter_->ReportHighMemoryVideoThumbnail(path, suffix, width, height);
    }
}

void DfxManager::HandleThumbnailError(const std::string &path, int32_t method, int32_t errorCode)
{
    string safePath = DfxUtils::GetSafePath(path);
    MEDIA_ERR_LOG("Failed to %{public}d, path: %{public}s, err: %{public}d", method, safePath.c_str(), errorCode);
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectThumbnailError(safePath, method, errorCode);
}

void DfxManager::HandleThumbnailGeneration(const ThumbnailData::GenerateStats &stats)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxReporter_->ReportThumbnailGeneration(stats);
}

void DfxManager::HandleCommonBehavior(string bundleName, int32_t type)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->AddCommonBahavior(bundleName, type);
}

static string GetDisplayName(const string &coverId, const std::map<std::string, std::string> &displayNames)
{
    auto it = std::find_if(displayNames.begin(), displayNames.end(),
        [&](const std::pair<std::string, std::string> &name) {
        return name.first == coverId;
    });
    string displayName = it == displayNames.end() ? "" : it->second;
    return DfxUtils::GetSafeDiaplayNameWhenChinese(displayName);
}

static string GetAlbumName(const string &coverId, const DeleteBehaviorData &deleteBehaviorData)
{
    auto itIds = std::find_if(deleteBehaviorData.ownerAlbumIds.begin(), deleteBehaviorData.ownerAlbumIds.end(),
        [&](const std::pair<std::string, std::string> &ownerAlbumId) {
            return ownerAlbumId.first == coverId;
        });
    if (itIds == deleteBehaviorData.ownerAlbumIds.end()) {
        return "";
    }
    string albumId = itIds->second;
    auto it = std::find_if(deleteBehaviorData.albumNames.begin(), deleteBehaviorData.albumNames.end(),
        [&](const std::pair<std::string, std::string> &albumName) {
            return albumName.first == albumId;
        });
    string albumName = it == deleteBehaviorData.albumNames.end() ? "" : it->second;
    return DfxUtils::GetSafeAlbumNameWhenChinese(albumName);
}

static void LogDelete(DfxData *data)
{
    if (data == nullptr) {
        return;
    }
    auto *taskData = static_cast<DeleteBehaviorTask *>(data);
    string id = taskData->id_;
    int32_t type = taskData->type_;
    int32_t size = taskData->size_;
    std::shared_ptr<DfxReporter> dfxReporter = taskData->dfxReporter_;
    MEDIA_INFO_LOG("id: %{public}s, type: %{public}d, size: %{public}d", id.c_str(), type, size);

    std::vector<std::string> uris = taskData->uris_;
    if (!uris.empty()) {
        for (auto& uri: uris) {
            string::size_type pos = uri.find_last_of('/');
            if (pos == string::npos) {
                continue;
            }
            string halfUri = uri.substr(0, pos);
            string::size_type pathPos = halfUri.find_last_of('/');
            if (pathPos == string::npos) {
                continue;
            }
            string coverId = MediaFileUri::GetPhotoId(uri);
            string displayName = GetDisplayName(coverId, taskData->deleteBehaviorData_.displayNames);
            string albumName = GetAlbumName(coverId, taskData->deleteBehaviorData_);
            AuditLog auditLog = { true, "USER BEHAVIOR", "DELETE", "io", 1, "running", "ok",
                id, type, size, (halfUri.substr(pathPos + 1)).c_str(), displayName, albumName };
            HiAudit::GetInstance().Write(auditLog);
            dfxReporter->ReportDeleteBehavior(id, type, halfUri.substr(pathPos + 1));
        }
    }
}

void DfxManager::HandleNoPermmison(int32_t type, int32_t object, int32_t error)
{
    MEDIA_INFO_LOG("permission deny: {%{public}d, %{public}d, %{public}d}", type, object, error);
}

void DfxManager::HandleDeleteBehavior(int32_t type, int32_t size, std::vector<std::string> &uris, string bundleName,
    const DeleteBehaviorData &deleteBehaviorData)
{
    if (bundleName == "") {
        bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    }
    dfxCollector_->CollectDeleteBehavior(bundleName, type, size);
    if (dfxWorker_ == nullptr) {
        MEDIA_ERR_LOG("Can not get dfxWork_");
        return;
    }
    string id = bundleName == "" ? to_string(IPCSkeleton::GetCallingUid()) : bundleName;
    auto *taskData = new (nothrow) DeleteBehaviorTask(id, type, size, uris, dfxReporter_, deleteBehaviorData);
    if (taskData == nullptr) {
        MEDIA_ERR_LOG("Failed to new taskData");
        return;
    }
    auto deleteBehaviorTask = make_shared<DfxTask>(LogDelete, taskData);
    if (deleteBehaviorTask == nullptr) {
        MEDIA_ERR_LOG("Failed to create async task for deleteBehaviorTask.");
        return;
    }
    dfxWorker_->AddTask(deleteBehaviorTask);
}

const std::string SQL_BATCH_DOWNLOAD_INFO_COUNT = "\
    SELECT \
        COUNT(CASE WHEN download_status = 0 THEN 1 END) AS WAITING_COUNT, \
        COUNT(CASE WHEN download_status = 1 THEN 1 END) AS DOWNLOADING_COUNT, \
        COUNT(CASE WHEN download_status = 2 THEN 1 END) AS PAUSE_COUNT, \
        COUNT(CASE WHEN download_status = 3 THEN 1 END) AS FAILED_COUNT, \
        COUNT(CASE WHEN download_status = 4 THEN 1 END) AS SUCC_COUNT, \
        COUNT(CASE WHEN download_status = 5 THEN 1 END) AS AUTO_PAUSE_COUNT, \
        SUM(CASE WHEN download_status = 4 THEN size ELSE 0 END) AS SUCC_TOTAL_SIZE, \
        SUM(CASE WHEN download_status = 4 THEN (finish_time - add_time) ELSE 0 END) AS SUCC_TOTAL_TIME \
    FROM download_resources_task_records;";

static void AddDownloadTaskInfo(PhotoStatistics &stats)
{
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    std::vector<NativeRdb::ValueObject> params = {};
    auto resultSet = rdbStore->QuerySql(SQL_BATCH_DOWNLOAD_INFO_COUNT, params);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is nullptr");

    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        stats.tasksWaitingCount = GetInt32Val("WAITING_COUNT", resultSet);
        stats.tasksDownloadingCount = GetInt32Val("DOWNLOADING_COUNT", resultSet);
        stats.tasksPauseCount = GetInt32Val("PAUSE_COUNT", resultSet);
        stats.tasksFailedCount = GetInt32Val("FAILED_COUNT", resultSet);
        stats.tasksSuccessCount = GetInt32Val("SUCC_COUNT", resultSet);
        stats.tasksAutoPauseCount = GetInt32Val("AUTO_PAUSE_COUNT", resultSet);
        stats.tasksSuccessTotalSize = GetInt64Val("SUCC_TOTAL_SIZE", resultSet);
        stats.tasksSuccessTotalTime = GetInt64Val("SUCC_TOTAL_TIME", resultSet);
    };
    resultSet->Close();
}

const std::string SQL_ALBUM_UPLOAD_STATISTICS = "\
    SELECT \
        COUNT(CASE WHEN album_type = 2048 THEN 1 END) AS SOURCE_ALBUM_COUNT, \
        COUNT(CASE WHEN album_type = 0 THEN 1 END) AS USER_ALBUM_COUNT, \
        COUNT(CASE WHEN album_type = 2048 AND upload_status = 1 THEN 1 END) AS UPLOAD_SOURCE_ALBUM_COUNT, \
        COUNT(CASE WHEN album_type = 0 AND upload_status = 1 THEN 1 END) AS UPLOAD_USER_ALBUM_COUNT \
    FROM PhotoAlbum;";

const std::string SQL_NOT_UPLOAD_ASSET_COUNT = "\
    WITH NOT_UPLOAD_ALBUM AS ( \
        SELECT album_id \
        FROM PhotoAlbum \
        WHERE album_type IN (0, 2048) AND \
            upload_status = 0 \
    ) \
    SELECT \
        COUNT(CASE WHEN dirty = 1 THEN 1 END) AS NOT_UPLOAD_ASSET_COUNT \
    FROM Photos \
    WHERE owner_album_id IN (SELECT album_id FROM NOT_UPLOAD_ALBUM);";

static void QueryAlbumCountByUpload(PhotoStatistics &stats)
{
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "QueryAlbumCountByUpload rdbStore is nullptr");
    std::vector<NativeRdb::ValueObject> params = {};
    auto resultSet = rdbStore->QuerySql(SQL_ALBUM_UPLOAD_STATISTICS, params);
    CHECK_AND_RETURN_WARN_LOG(resultSet != nullptr, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        stats.sourceAlbumCount = GetInt32Val("SOURCE_ALBUM_COUNT", resultSet);
        stats.userAlbumCount = GetInt32Val("USER_ALBUM_COUNT", resultSet);
        stats.uploadSourceAlbumCount = GetInt32Val("UPLOAD_SOURCE_ALBUM_COUNT", resultSet);
        stats.uploadUserAlbumCount = GetInt32Val("UPLOAD_USER_ALBUM_COUNT", resultSet);
    }
    resultSet->Close();
}

static void QueryAssetCountByUpload(PhotoStatistics &stats)
{
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "QueryAssetCountByUpload rdbStore is nullptr");
    std::vector<NativeRdb::ValueObject> params = {};
    auto resultSet = rdbStore->QuerySql(SQL_NOT_UPLOAD_ASSET_COUNT, params);
    CHECK_AND_RETURN_WARN_LOG(resultSet != nullptr, "resultSet is nullptr");
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        stats.notUploadAssetCount = GetInt32Val("NOT_UPLOAD_ASSET_COUNT", resultSet);
    }
    resultSet->Close();
}

static void QueryAlbumAndAssetCountByUpload(PhotoStatistics &stats)
{
    QueryAlbumCountByUpload(stats);
    QueryAssetCountByUpload(stats);
}

static void AddPhotoStats(PhotoStatistics &stats)
{
    const std::vector<QueryParams> queryParamsList = {
        {MediaType::MEDIA_TYPE_IMAGE, PhotoPositionType::LOCAL},
        {MediaType::MEDIA_TYPE_VIDEO, PhotoPositionType::LOCAL},
        {MediaType::MEDIA_TYPE_IMAGE, PhotoPositionType::CLOUD},
        {MediaType::MEDIA_TYPE_VIDEO, PhotoPositionType::CLOUD},
        {MediaType::MEDIA_TYPE_IMAGE, PhotoPositionType::LOCAL_AND_CLOUD},
        {MediaType::MEDIA_TYPE_VIDEO, PhotoPositionType::LOCAL_AND_CLOUD}
    };

    int32_t* countPtrs[] = {
        &stats.localImageCount,
        &stats.localVideoCount,
        &stats.cloudImageCount,
        &stats.cloudVideoCount,
        &stats.sharedImageCount,
        &stats.sharedVideoCount
    };

    for (size_t i = 0; i < queryParamsList.size(); i++) {
        const auto& params = queryParamsList[i];
        *countPtrs[i] = DfxDatabaseUtils::QueryFromPhotos(params.mediaType, static_cast<int32_t>(params.positionType));
    }
}

static void AddSouthDeviceType(PhotoStatistics &stats)
{
    stats.southDeviceType = static_cast<int32_t>(SettingsDataManager::GetPhotosSyncSwitchStatus());
}

static void HandlePhotoInfo(std::shared_ptr<DfxReporter>& dfxReporter)
{
    PhotoStatistics stats = {};
    AddPhotoStats(stats);
    AddDownloadTaskInfo(stats);
    AddSouthDeviceType(stats);
    QueryAlbumAndAssetCountByUpload(stats);
    MEDIA_INFO_LOG("localImageCount: %{public}d, localVideoCount: %{public}d, "
                   "cloudImageCount: %{public}d, cloudVideoCount: %{public}d, "
                   "sharedImageCount: %{public}d, sharedVideoCount: %{public}d, "
                   "DownloadTasksWaitingCount: %{public}d, DownloadTasksDownloadingCount: %{public}d, "
                   "DownloadTasksPauseCount: %{public}d, DownloadTasksFailedCount: %{public}d, "
                   "DownloadTasksSuccessCount: %{public}d, DownloadTasksAutoPauseCount: %{public}d, "
                   "DownloadTasksSuccessTotalSize: %{public}" PRId64 " bytes, "
                   "DownloadTasksSuccessTotalTime: %{public}" PRId64 " seconds",
                   stats.localImageCount, stats.localVideoCount,
                   stats.cloudImageCount, stats.cloudVideoCount,
                   stats.sharedImageCount, stats.sharedVideoCount,
                   stats.tasksWaitingCount, stats.tasksDownloadingCount,
                   stats.tasksPauseCount, stats.tasksFailedCount,
                   stats.tasksSuccessCount, stats.tasksAutoPauseCount,
                   stats.tasksSuccessTotalSize, stats.tasksSuccessTotalTime
                   );
    dfxReporter->ReportPhotoInfo(stats);
}

static void HandleAlbumInfoBySubtype(std::shared_ptr<DfxReporter> &dfxReporter, int32_t albumSubType)
{
    AlbumInfo albumInfo = DfxDatabaseUtils::QueryAlbumInfoBySubtype(albumSubType);
    string albumName = ALBUM_MAP.at(albumSubType);
    MEDIA_INFO_LOG("album %{public}s: {count:%{public}d, imageCount:%{public}d, videoCount:%{public}d, \
        isLocal:%{public}d}", albumName.c_str(), albumInfo.count, albumInfo.imageCount, albumInfo.videoCount,
        albumInfo.isLocal);
    dfxReporter->ReportAlbumInfo(albumName.c_str(), albumInfo.imageCount, albumInfo.videoCount, albumInfo.isLocal);
}

static void HandleAlbumInfoByUploadStatus(std::shared_ptr<DfxReporter> &dfxReporter, const bool uploadStatus)
{
    vector<string> albumNames = DfxDatabaseUtils::QueryAlbumNamesByUploadStatus(static_cast<int32_t>(uploadStatus));
    for (const auto& albumName : albumNames) {
        MEDIA_INFO_LOG("albumNames: %{public}s, uploadStatus: %{public}d", albumName.c_str(), uploadStatus);
        dfxReporter->ReportAlbumInfo(albumName.c_str(), 0, 0, uploadStatus);
    }
}

static void HandleAlbumInfo(std::shared_ptr<DfxReporter> &dfxReporter)
{
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::IMAGE));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::VIDEO));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::FAVORITE));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::HIDDEN));
    HandleAlbumInfoBySubtype(dfxReporter, static_cast<int32_t>(PhotoAlbumSubType::TRASH));
    HandleAlbumInfoByUploadStatus(dfxReporter, false);
    HandleAlbumInfoByUploadStatus(dfxReporter, true);
}

static void HandleDirtyCloudPhoto(std::shared_ptr<DfxReporter> &dfxReporter)
{
    vector<PhotoInfo> photoInfoList = DfxDatabaseUtils::QueryDirtyCloudPhoto();
    if (photoInfoList.empty()) {
        return;
    }
    for (auto& photoInfo: photoInfoList) {
        dfxReporter->ReportDirtyCloudPhoto(photoInfo.data, photoInfo.dirty, photoInfo.cloudVersion);
    }
}

static void HandleLocalVersion(std::shared_ptr<DfxReporter> &dfxReporter)
{
    int32_t dbVersion = DfxDatabaseUtils::QueryDbVersion();
    dfxReporter->ReportCommonVersion(dbVersion);
    int32_t aestheticsVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_aesthetics_score",
        "aesthetics_version");
    dfxReporter->ReportAnalysisVersion("aesthetics_score", aestheticsVersion);
    int32_t compositionVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_composition",
        "composition_version");
    dfxReporter->ReportAnalysisVersion("composition", compositionVersion);
    int32_t headVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_head", "head_version");
    dfxReporter->ReportAnalysisVersion("head", headVersion);
    int32_t imageFaceVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_image_face", "features_version");
    dfxReporter->ReportAnalysisVersion("image_face", imageFaceVersion);
    int32_t labelVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_label", "label_version");
    dfxReporter->ReportAnalysisVersion("label", labelVersion);
    int32_t objectVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_object", "object_version");
    dfxReporter->ReportAnalysisVersion("object", objectVersion);
    int32_t ocrVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_ocr", "ocr_version");
    dfxReporter->ReportAnalysisVersion("ocr", ocrVersion);
    int32_t poseVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_pose", "pose_version");
    dfxReporter->ReportAnalysisVersion("pose", poseVersion);
    int32_t recommendationVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_recommendation",
        "recommendation_version");
    dfxReporter->ReportAnalysisVersion("recommendation", recommendationVersion);
    int32_t saliencyDetectVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_saliency_detect",
        "saliency_version");
    dfxReporter->ReportAnalysisVersion("saliency_detect", saliencyDetectVersion);
    int32_t segmentationVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_segmentation",
        "segmentation_version");
    dfxReporter->ReportAnalysisVersion("segmentation", segmentationVersion);
    int32_t videoLabelVersion = DfxDatabaseUtils::QueryAnalysisVersion("tab_analysis_video_label", "algo_version");
    dfxReporter->ReportAnalysisVersion("video_label", videoLabelVersion);
}

void HandleAstcInfo(std::shared_ptr<DfxReporter>& dfxReporter)
{
    LcdAndAstcCount count = {};
    count.localAstcCount = DfxDatabaseUtils::QueryASTCThumb(true);
    count.cloudAstcCount = DfxDatabaseUtils::QueryASTCThumb(false);
    count.localLcdCount = DfxDatabaseUtils::QueryLCDThumb(true);
    count.cloudLcdCount = DfxDatabaseUtils::QueryLCDThumb(false);

    MEDIA_INFO_LOG("localLcdCount: %{public}d, localAstcCount: %{public}d, "
                   "cloudLcdCount: %{public}d, cloudAstcCount: %{public}d",
                   count.localLcdCount, count.localAstcCount,
                   count.cloudLcdCount, count.cloudAstcCount);

    dfxReporter->ReportAstcInfo(count);
}

static void HandleStatistic(DfxData *data)
{
    if (data == nullptr) {
        return;
    }
    auto *taskData = static_cast<StatisticData *>(data);
    std::shared_ptr<DfxReporter> dfxReporter = taskData->dfxReporter_;
    HandlePhotoInfo(dfxReporter);
    HandleAlbumInfo(dfxReporter);
    HandleDirtyCloudPhoto(dfxReporter);
    HandleLocalVersion(dfxReporter);
    HandleAstcInfo(dfxReporter);
#ifdef META_RECOVERY_SUPPORT
    MediaLibraryMetaRecovery::GetInstance().RecoveryStatistic();
#endif
    DfxDeprecatedPermUsage::Statistics();
}

static void CheckPhotoError(std::shared_ptr<DfxReporter>& dfxReporter)
{
    CHECK_AND_RETURN_INFO_LOG(dfxReporter != nullptr, "E_OK");
    auto photoCount = DfxDatabaseUtils::QueryPhotoErrorCount();
    CHECK_AND_RETURN_INFO_LOG(photoCount, "E_OK");
    MEDIA_ERR_LOG("DFX ReportPhotoError count is %{public}d", photoCount);
    PhotoErrorCount photoError = { {PhotoErrorType::PHOTO_MISS_TYPE}, {photoCount} };
    dfxReporter->ReportPhotoError(photoError);
}

static void HandleCheckTwoDayTask(DfxData *data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Failed to get data for Handle Check Two Day Task!");
    auto *taskData = static_cast<StatisticData *>(data);
    std::shared_ptr<DfxReporter> dfxReporter = taskData->dfxReporter_;
    CHECK_AND_RETURN_LOG(dfxReporter != nullptr, "Failed to get dfxReporter for Handle Check Two Day Task!");
    CheckPhotoError(dfxReporter);
}

void DfxManager::HandleHalfDayMissions()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    int64_t lastReportTime = prefs->GetLong(LAST_HALF_DAY_REPORT_TIME, 0);
    if (MediaFileUtils::UTCTimeSeconds() - lastReportTime > HALF_DAY && dfxWorker_ != nullptr) {
        MEDIA_INFO_LOG("start handle statistic behavior");
        auto *taskData = new (nothrow) StatisticData(dfxReporter_);
        if (taskData == nullptr) {
            MEDIA_ERR_LOG("Failed to alloc async data for Handle Half Day Missions!");
            return;
        }
        auto statisticTask = make_shared<DfxTask>(HandleStatistic, taskData);
        if (statisticTask == nullptr) {
            MEDIA_ERR_LOG("Failed to create statistic task.");
            return;
        }
        dfxWorker_->AddTask(statisticTask);
        int64_t time = MediaFileUtils::UTCTimeSeconds();
        prefs->PutLong(LAST_HALF_DAY_REPORT_TIME, time);
        prefs->FlushSync();
    }
}

void DfxManager::HandleTwoDayMissions()
{
    CHECK_AND_RETURN_LOG(isInitSuccess_, "DfxManager not init");
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    int64_t lastReportTime = prefs->GetLong(LAST_TWO_DAY_REPORT_TIME, 0);
    if (MediaFileUtils::UTCTimeSeconds() - lastReportTime > TWO_DAY && dfxWorker_ != nullptr) {
        MEDIA_INFO_LOG("start handle statistic behavior");
        auto *taskData = new (nothrow) StatisticData(dfxReporter_);
        CHECK_AND_RETURN_LOG(taskData != nullptr, "Failed to alloc async data for Handle Two Day Missions!");
        auto twoDayTask = make_shared<DfxTask>(HandleCheckTwoDayTask, taskData);
        CHECK_AND_RETURN_LOG(twoDayTask != nullptr, "Failed to create dfx task.");
        dfxWorker_->AddTask(twoDayTask);
        int64_t time = MediaFileUtils::UTCTimeSeconds();
        prefs->PutLong(LAST_TWO_DAY_REPORT_TIME, time);
        prefs->FlushSync();
    }
}

void DfxManager::IsDirectoryExist(const string& dirName)
{
    struct stat statInfo {};
    if (stat(dirName.c_str(), &statInfo) == E_SUCCESS) {
        if (statInfo.st_mode & S_IFDIR) {
            return;
        }
        MEDIA_ERR_LOG("Not Is DIR, errno is %{public}d", errno);
        return;
    }
    MEDIA_ERR_LOG("Directory Not Exist, errno is %{public}d", errno);
    return;
}

void DfxManager::CheckStatus()
{
    const std::string CLOUD_FILE_PATH = "/storage/cloud/files";
    IsDirectoryExist(CLOUD_FILE_PATH);
}

void DfxManager::HandleFiveMinuteTask()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    std::unordered_map<string, CommonBehavior> commonBehavior = dfxCollector_->GetCommonBehavior();
    dfxAnalyzer_->FlushCommonBehavior(commonBehavior);
    HandleDeleteBehaviors();
    std::unordered_map<std::string, ThumbnailErrorInfo> result = dfxCollector_->GetThumbnailError();
    dfxAnalyzer_->FlushThumbnail(result);
    AdaptationToMovingPhotoInfo adaptationInfo = dfxCollector_->GetAdaptationToMovingPhotoInfo();
    dfxAnalyzer_->FlushAdaptationToMovingPhoto(adaptationInfo);
    CinematicVideoInfo cinematicVideoInfo = dfxCollector_->GetCinematicVideoInfo();
    dfxAnalyzer_->FlushCinematicVideoInfo(cinematicVideoInfo);
    CheckStatus();
}

void DfxManager::HandleDeleteBehaviors()
{
    std::unordered_map<string, int32_t> deleteAssetToTrash =
        dfxCollector_->GetDeleteBehavior(DfxType::TRASH_PHOTO);
    dfxAnalyzer_->FlushDeleteBehavior(deleteAssetToTrash, DfxType::TRASH_PHOTO);
    std::unordered_map<string, int32_t> deleteAssetFromDisk =
        dfxCollector_->GetDeleteBehavior(DfxType::ALBUM_DELETE_ASSETS);
    dfxAnalyzer_->FlushDeleteBehavior(deleteAssetToTrash, DfxType::ALBUM_DELETE_ASSETS);
    std::unordered_map<string, int32_t> removeAssets =
        dfxCollector_->GetDeleteBehavior(DfxType::ALBUM_REMOVE_PHOTOS);
    dfxAnalyzer_->FlushDeleteBehavior(removeAssets, DfxType::ALBUM_REMOVE_PHOTOS);
}

int64_t DfxManager::HandleMiddleReport()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return MediaFileUtils::UTCTimeSeconds();
    }
    dfxReporter_->ReportCommonBehavior();
    dfxReporter_->ReportDeleteStatistic();
    return MediaFileUtils::UTCTimeSeconds();
}

int64_t DfxManager::HandleOneDayReport()
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return MediaFileUtils::UTCTimeSeconds();
    }
    dfxReporter_->ReportThumbnailError();
    dfxReporter_->ReportAdaptationToMovingPhoto();
    dfxReporter_->ReportPhotoRecordInfo();
    dfxReporter_->ReportOperationRecordInfo();
    dfxReporter_->ReportCinematicVideo();
    dfxReporter_->ReportAlibHeifDuplicate();
    return MediaFileUtils::UTCTimeSeconds();
}

void DfxManager::HandleAdaptationToMovingPhoto(const string &appName, bool adapted)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectAdaptationToMovingPhotoInfo(appName, adapted);
}

static bool QueryCinematicVideoStatus(const string &fileId, MultiStagesCaptureVideoStatus &photoStatus)
{
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns { PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_QUALITY };
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);

    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return false;
    }

    int32_t columnIndexSubtype = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_SUBTYPE, columnIndexSubtype);
    int32_t currentSubtype = 0;
    resultSet->GetInt(columnIndexSubtype, currentSubtype);
    if (currentSubtype != static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
        MEDIA_INFO_LOG("Current photo is not cinematic video");
        return false;
    }

    int32_t columnIndexQuality = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_QUALITY, columnIndexQuality);
    int32_t currentPhotoQuality = 0;
    resultSet->GetInt(columnIndexQuality, currentPhotoQuality);
    photoStatus = currentPhotoQuality ? MultiStagesCaptureVideoStatus::LOW_QUALITY :
        MultiStagesCaptureVideoStatus::HIGH_QUALITY;
    return true;
}

void DfxManager::HandleCinematicVideoAccessTimes(bool isByUri, bool isHighQualityRequest, const std::string &fileId)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }

    if (!fileId.empty()) {
        MultiStagesCaptureVideoStatus photoStatus = MultiStagesCaptureVideoStatus::LOW_QUALITY;
        CHECK_AND_RETURN_INFO_LOG(QueryCinematicVideoStatus(fileId, photoStatus),
            "QueryCinematicVideoStatus failed");
        isHighQualityRequest = photoStatus == MultiStagesCaptureVideoStatus::HIGH_QUALITY ?
            true : false;
    }
    dfxCollector_->CollectCinematicVideoAccessTimes(isByUri, isHighQualityRequest);
}

void DfxManager::HandleCinematicVideoAddStartTime(const CinematicWaitType waitType, const string &videoId)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectCinematicVideoAddStartTime(waitType, videoId);
}

void DfxManager::HandleCinematicVideoAddEndTime(const CinematicWaitType waitType, const string &videoId)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectCinematicVideoAddEndTime(waitType, videoId);
}

void DfxManager::HandleCinematicVideoMultistageResult(bool multistageResult)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    dfxCollector_->CollectCinematicVideoMultistageResult(multistageResult);
}

static void GetPhotoAndPhotoExtSizes(QuerySizeAndResolution &queryInfo)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "RdbStore is null");

    PhotoStorageOperation photoStorageOperation;
    int64_t cacheSize = photoStorageOperation.GetCacheSize();
    int64_t highlightSize = photoStorageOperation.GetHighlightSizeFromPreferences();

    TotalThumbnailSizeResult totalThumbnailSizeResult = {};
    photoStorageOperation.GetTotalThumbnailSize(rdbStore, totalThumbnailSizeResult);

    TotalEditdataSizeResult totalEditdataSizeRusult = {};
    photoStorageOperation.GetTotalEditdataSize(rdbStore, totalEditdataSizeRusult);

    int64_t totalExtSize = cacheSize + highlightSize + totalThumbnailSizeResult.totalThumbnailSize +
        totalEditdataSizeRusult.totalEditdataSize;
    LocalPhotoSizeResult localPhotoSizeResult = {};
    photoStorageOperation.GetLocalPhotoSize(rdbStore, localPhotoSizeResult, totalExtSize);

    int64_t totalSize = localPhotoSizeResult.localImageSize + localPhotoSizeResult.localVideoSize + totalExtSize;

    queryInfo.cacheRomSize = std::to_string(cacheSize);
    queryInfo.highlightRomSize = std::to_string(highlightSize);
    queryInfo.ThumbnailRomSize = std::to_string(totalThumbnailSizeResult.totalThumbnailSize);
    queryInfo.EditdataRomSize = std::to_string(totalEditdataSizeRusult.totalEditdataSize);
    queryInfo.localImageRomSize = std::to_string(localPhotoSizeResult.localImageSize);
    queryInfo.localVideoRomSize = std::to_string(localPhotoSizeResult.localVideoSize);
    queryInfo.totalSize = std::to_string(totalSize);
    MEDIA_INFO_LOG("localImageRomSize: %{public}s, localVideoRomSize: %{public}s, totalThumbnailSize: %{public}s, "
                   "totalEditdataSize: %{public}s, cacheSize: %{public}s, highlightSize: %{public}s, "
                   "totalSize: %{public}s",
                   queryInfo.localImageRomSize.c_str(), queryInfo.localVideoRomSize.c_str(),
                   queryInfo.ThumbnailRomSize.c_str(), queryInfo.EditdataRomSize.c_str(),
                   queryInfo.cacheRomSize.c_str(), queryInfo.highlightRomSize.c_str(),
                   queryInfo.totalSize.c_str());
}

static void HandleGetSizeAndResolutionInfo(std::shared_ptr<DfxReporter>& dfxReporter)
{
    MEDIA_INFO_LOG("HandleGetSizeAndResolutionInfo start");
    CHECK_AND_RETURN_LOG(dfxReporter != nullptr, "Failed to get dfxReporter for HandleGetSizeAndResolutionInfo!");
    QuerySizeAndResolution queryInfo = {};
    bool bQueryInfo = DfxDatabaseUtils::GetSizeAndResolutionInfo(queryInfo);
    CHECK_AND_RETURN_LOG(bQueryInfo, "E_OK");
    std::string photoMimeType;
    DfxDatabaseUtils::GetPhotoMimeType(photoMimeType);
    GetPhotoAndPhotoExtSizes(queryInfo);
    dfxReporter->ReportPhotoSizeAndResolutionInfo(queryInfo, photoMimeType);
}

static void HandleCheckOneWeekDayTask(DfxData *data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Failed to get data for Handle Check One Week Day Task!");
    auto *taskData = static_cast<StatisticData *>(data);
    std::shared_ptr<DfxReporter> dfxReporter = taskData->dfxReporter_;
    CHECK_AND_RETURN_LOG(dfxReporter != nullptr, "Failed to get dfxReporter for Handle Check One Week Day Task!");
    HandleGetSizeAndResolutionInfo(dfxReporter);
}

void DfxManager::HandleOneWeekMissions()
{
    MEDIA_INFO_LOG("HandlePhotoInfo start");
    CHECK_AND_RETURN_LOG(isInitSuccess_, "DfxManager not init");
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
    NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    int64_t lastReportTime = prefs->GetLong(LAST_WEEK_REPORT_TIME, 0);
    if (MediaFileUtils::UTCTimeSeconds() - lastReportTime > ONE_WEEK && dfxWorker_ != nullptr) {
        MEDIA_INFO_LOG("start handle statistic behavior");
        auto *taskData = new (nothrow) StatisticData(dfxReporter_);
        CHECK_AND_RETURN_LOG(taskData != nullptr, "Failed to alloc async data for Handle Week Day Missions!");
        auto oneWeekTask = make_shared<DfxTask>(HandleCheckOneWeekDayTask, taskData);
        CHECK_AND_RETURN_LOG(oneWeekTask != nullptr, "Failed to create dfx task.");
        dfxWorker_->AddTask(oneWeekTask);
        int64_t time = MediaFileUtils::UTCTimeSeconds();
        prefs->PutLong(LAST_WEEK_REPORT_TIME, time);
        prefs->FlushSync();
    }
}

bool IsReported()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return false;
    }
    return prefs->GetBool(IS_REPORTED, false);
}

void SetReported(bool isReported)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutBool(IS_REPORTED, isReported);
}

CloudSyncDfxManager::~CloudSyncDfxManager()
{
    ShutDownTimer();
}

CloudSyncDfxManager& CloudSyncDfxManager::GetInstance()
{
    static CloudSyncDfxManager cloudSyncDfxManager;
    return cloudSyncDfxManager;
}

CloudSyncStatus GetCloudSyncStatus()
{
    return static_cast<CloudSyncStatus>(system::GetParameter(CLOUDSYNC_STATUS_KEY, "0").at(0) - '0');
}

CloudSyncDfxManager::CloudSyncDfxManager()
{
    InitSyncState();
    uint16_t newState = static_cast<uint16_t>(syncState_);
    stateProcessFuncs_[newState].Process(*this);
}

void CloudSyncDfxManager::InitSyncState()
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::BEGIN:
        case CloudSyncStatus::SYNC_SWITCHED_OFF:
            syncState_ = SyncState::INIT_STATE;
            return;
        case CloudSyncStatus::FIRST_FIVE_HUNDRED:
        case CloudSyncStatus::TOTAL_DOWNLOAD:
            syncState_ = SyncState::START_STATE;
            return;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            syncState_ = SyncState::END_STATE;
            return;
        default:
            return;
    }
}

bool InitState::StateSwitch(CloudSyncDfxManager& manager)
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::FIRST_FIVE_HUNDRED:
        case CloudSyncStatus::TOTAL_DOWNLOAD:
            manager.syncState_ = SyncState::START_STATE;
            return true;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            manager.syncState_ = SyncState::END_STATE;
            MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
            return true;
        default:
            return false;
    }
}

void InitState::Process(CloudSyncDfxManager& manager)
{
    MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
    manager.ResetStartTime();
    manager.ShutDownTimer();
    SetReported(false);
}

void CloudSyncDfxManager::RunDfx()
{
    uint16_t oldState = static_cast<uint16_t>(syncState_);
    if (stateProcessFuncs_[oldState].StateSwitch(*this)) {
        uint16_t newState = static_cast<uint16_t>(syncState_);
        stateProcessFuncs_[newState].Process(*this);
    }
}

bool StartState::StateSwitch(CloudSyncDfxManager& manager)
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::BEGIN:
        case CloudSyncStatus::SYNC_SWITCHED_OFF:
            manager.syncState_ = SyncState::INIT_STATE;
            return true;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            manager.syncState_ = SyncState::END_STATE;
            MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
            return true;
        default:
            return false;
    }
}

void StartState::Process(CloudSyncDfxManager& manager)
{
    MEDIA_INFO_LOG("CloudSyncDfxManager new status:%{public}hu", manager.syncState_);
    manager.SetStartTime();
    manager.StartTimer();
    SetReported(false);
}

bool EndState::StateSwitch(CloudSyncDfxManager& manager)
{
    CloudSyncStatus cloudSyncStatus = GetCloudSyncStatus();
    switch (cloudSyncStatus) {
        case CloudSyncStatus::BEGIN:
        case CloudSyncStatus::SYNC_SWITCHED_OFF:
            manager.syncState_ = SyncState::INIT_STATE;
            return true;
        case CloudSyncStatus::TOTAL_DOWNLOAD_FINISH:
            return true;
        default:
            return false;
    }
}

void EndState::Process(CloudSyncDfxManager& manager)
{
    std::unique_lock<std::mutex> lock(manager.endStateMutex_);
    if (IsReported()) {
        manager.ShutDownTimer();
        return;
    }
    manager.SetStartTime();
    manager.StartTimer();
    int32_t downloadedThumb = 0;
    int32_t generatedThumb = 0;
    if (!DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb)) {
        if (downloadedThumb != generatedThumb) {
            return;
        }
        int32_t totalDownload = 0;
        DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);
        if (totalDownload != downloadedThumb) {
            return;
        }
        SetReported(true);
        manager.ShutDownTimer();
        DfxReporter::ReportCloudSyncThumbGenerationStatus(downloadedThumb, generatedThumb, totalDownload,
            static_cast<int32_t>(SettingsDataManager::GetPhotosSyncSwitchStatus()));
    }
}

void CloudSyncDfxManager::StartTimer()
{
    std::unique_lock<std::mutex> lock(timerMutex_);
    if (timerId_ != 0) {
        return;
    }
    if (timer_.Setup() != ERR_OK) {
        MEDIA_INFO_LOG("CloudSync Dfx Set Timer Failed");
        return;
    }
    Utils::Timer::TimerCallback timerCallback = [this]() {
        (void)this;
        if (IsReported()) {
            return;
        }
        int32_t generatedThumb = 0;
        int32_t downloadedThumb = 0;
        if (!DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb)) {
            int32_t totalDownload = 0;
            DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);
            if (downloadedThumb == generatedThumb && totalDownload == generatedThumb) {
                MEDIA_INFO_LOG("CloudSyncDfxManager Dfx report Thumb generation status, "
                    "download: %{public}d, generate: %{public}d", downloadedThumb, generatedThumb);
                SetReported(true);
            }
            DfxReporter::ReportCloudSyncThumbGenerationStatus(downloadedThumb, generatedThumb, totalDownload,
                static_cast<int32_t>(SettingsDataManager::GetPhotosSyncSwitchStatus()));
        }
    };
    timerId_ = timer_.Register(timerCallback, SIX_HOUR * TO_MILLION, false);
    MEDIA_INFO_LOG("CloudSyncDfxManager StartTimer id:%{public}d", timerId_);
}

void CloudSyncDfxManager::ShutDownTimer()
{
    std::unique_lock<std::mutex> lock(timerMutex_);
    if (timerId_ == 0) {
        return;
    }
    MEDIA_INFO_LOG("CloudSyncDfxManager ShutDownTimer id:%{public}d", timerId_);
    timer_.Unregister(timerId_);
    timerId_ = 0;
    timer_.Shutdown();
}

void CloudSyncDfxManager::ResetStartTime()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return;
    }
    prefs->PutLong(CLOUD_SYNC_START_TIME, 0);
    prefs->FlushSync();
}

void CloudSyncDfxManager::SetStartTime()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_COMMON_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get dfx common preferences error: %{public}d", errCode);
        return;
    }
    int64_t time = prefs->GetLong(CLOUD_SYNC_START_TIME, 0);
    // if startTime exists, no need to reset startTime
    if (time != 0) {
        return;
    }
    time = MediaFileUtils::UTCTimeSeconds();
    prefs->PutLong(CLOUD_SYNC_START_TIME, time);
    prefs->FlushSync();
}

void DfxManager::ResetStatistic()
{
    for (auto &value : downloadMeta_) {
        value.store(0);
    }
    for (auto &value : uploadMeta_) {
        value.store(0);
    }
    for (auto &value : downloadThumb_) {
        value.store(0);
    }
    for (auto &value : downloadLcd_) {
        value.store(0);
    }
    for (auto &value : uploadAlbum_) {
        value.store(0);
    }
    for (auto &value : downloadAlbum_) {
        value.store(0);
    }
    for (auto &value : updateDetails_) {
        value.store(0);
    }
    for (auto &value : uploadMetaErr_) {
        value.store(0);
    }
}

void DfxManager::CopyStatistic(CloudSyncStat& stat)
{
    stat.downloadMeta.reserve(downloadMeta_.size());
    for (auto &value : downloadMeta_) {
        stat.downloadMeta.push_back(value.load());
    }
    stat.uploadMeta.reserve(uploadMeta_.size());
    for (auto &value : uploadMeta_) {
        stat.uploadMeta.push_back(value.load());
    }
    stat.downloadThumb.reserve(downloadThumb_.size());
    for (auto &value : downloadThumb_) {
        stat.downloadThumb.push_back(value.load());
    }
    stat.downloadLcd.reserve(downloadLcd_.size());
    for (auto &value : downloadLcd_) {
        stat.downloadLcd.push_back(value.load());
    }

    stat.uploadAlbum.reserve(uploadAlbum_.size());
    for (auto &value : uploadAlbum_) {
        stat.uploadAlbum.push_back(value.load());
    }
    stat.downloadAlbum.reserve(downloadAlbum_.size());
    for (auto &value : downloadAlbum_) {
        stat.downloadAlbum.push_back(value.load());
    }

    stat.updateDetails.reserve(updateDetails_.size());
    for (auto &value : updateDetails_) {
        stat.updateDetails.push_back(value.load());
    }
    stat.uploadMetaErr.reserve(uploadMetaErr_.size());
    for (auto &value : uploadMetaErr_) {
        stat.uploadMetaErr.push_back(value.load());
    }
}

void DfxManager::HandleSyncStart(const std::string& taskId, const int32_t syncReason)
{
    ResetStatistic();
    syncInfo_.startTime = static_cast<uint64_t>(MediaFileUtils::UTCTimeSeconds());
    syncInfo_.syncReason = syncReason;
    taskId_ = taskId;
}

void DfxManager::HandleUpdateMetaStat(uint32_t index, uint64_t diff, uint32_t syncType)
{
    if (index < DOWNLOAD_META_LEN) {
        downloadMeta_[index].fetch_add(diff);
    } else if (index - INDEX_UL_META_SUCCESS < UPLOAD_META_LEN) {
        uploadMeta_[index - INDEX_UL_META_SUCCESS].fetch_add(diff);
    } else {
        MEDIA_DEBUG_LOG("HandleUpdateMetaStat get a invalid index");
    }

    if (syncType > 0 && syncType < UPDATE_DETAILS_LEN &&
        (index == INDEX_UL_META_SUCCESS || index == INDEX_DL_META_SUCCESS)) {
        updateDetails_[syncType].fetch_add(diff);
    }
}

void DfxManager::HandleUpdateAttachmentStat(uint32_t index, uint64_t diff)
{
    if (index < DOWNLOAD_THUMB_LEN) {
        downloadThumb_[index].fetch_add(diff);
    } else if (index - INDEX_LCD_SUCCESS < DOWNLOAD_LCD_LEN) {
        downloadLcd_[index - INDEX_LCD_SUCCESS].fetch_add(diff);
    } else {
        MEDIA_DEBUG_LOG("HandleUpdateAttachmentStat get a invalid index");
    }
}

void DfxManager::HandleUpdateAlbumStat(uint32_t index, uint64_t diff)
{
    if (index < DOWNLOAD_ALBUM_LEN) {
        downloadAlbum_[index].fetch_add(diff);
    } else if (index - INDEX_UL_ALBUM_SUCCESS < UPLOAD_ALBUM_LEN) {
        uploadAlbum_[index - INDEX_UL_ALBUM_SUCCESS].fetch_add(diff);
    } else {
        MEDIA_DEBUG_LOG("HandleUpdateAlbumStat get a invalid index");
    }
}

void DfxManager::HandleUpdateUploadMetaStat(uint32_t index, uint64_t diff)
{
    if (index < UPLOAD_META_ERR_LEN) {
        uploadMetaErr_[index].fetch_add(diff);
    }
}

void DfxManager::HandleUpdateUploadDetailError(int32_t error)
{
    if (error == E_CLOUD_STORAGE_FULL) {
        uploadMetaErr_[INDEX_UL_META_ERR_STORAGE].fetch_add(1);
    }
}

static std::string VectorToString(const std::vector<uint64_t>& vec)
{
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (0 != i) {
            oss << " ";
        }
        oss << vec[i];
    }
    return oss.str();
}

void DfxManager::HandleSyncEnd(const int32_t stopReason)
{
    int64_t endTime = MediaFileUtils::UTCTimeSeconds();
    if (static_cast<uint64_t>(endTime) >= syncInfo_.startTime) {
        syncInfo_.duration = static_cast<uint64_t>(endTime) - syncInfo_.startTime;
    } else {
        syncInfo_.duration = 0;
    }
    syncInfo_.stopReason = stopReason;
    CloudSyncStat stat;
    CopyStatistic(stat);

    std::string syncInfo;
    for (auto &detail : stat.updateDetails) {
        if (detail > 0) {
            stat.updateDetails[0] = static_cast<uint64_t>(MediaFileUtils::UTCTimeSeconds());
            syncInfo = "[" + taskId_ + " " + std::to_string(syncInfo_.startTime) + " " +
            VectorToString(stat.updateDetails) + "]";
            break;
        }
    }
    #ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
    DfxReporter::ReportSyncStat(taskId_, syncInfo_, stat, syncInfo,
        CloudSync::CloudMediaContext::GetInstance().GetCloudType());
    #endif
}

void DfxManager::HandleReportSyncFault(const std::string& position, const SyncFaultEvent& event)
{
    #ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
    DfxReporter::ReportSyncFault(taskId_, position, event,
        CloudSync::CloudMediaContext::GetInstance().GetCloudType());
    #endif
}

void DfxManager::HandleAccurateRefreshTimeOut(const AccurateRefreshDfxDataPoint& reportData)
{
    if (!isInitSuccess_) {
        MEDIA_WARN_LOG("DfxManager not init");
        return;
    }
    MEDIA_INFO_LOG("enter HandleAccurateRefreshTimeOut");
    dfxReporter_->ReportAccurateRefreshResult(reportData);
}

void DfxManager::HandleTranscodeAccessTime(const TranscodeAccessType type)
{
    MEDIA_INFO_LOG("HandleTranscodeAccessTime type: %{public}d", type);
    CHECK_AND_RETURN_LOG(isInitSuccess_, "DfxManager not init");
    CHECK_AND_RETURN_LOG(dfxAnalyzer_, "dfxAnalyzer_ is nullptr");
    dfxAnalyzer_->FlushTranscodeAccessTimes(type);
}

void DfxManager::HandleTranscodeFailed(const TranscodeErrorType type)
{
    MEDIA_INFO_LOG("HandleTranscodeFailed type: %{public}d", type);
    CHECK_AND_RETURN_LOG(isInitSuccess_, "DfxManager not init");
    CHECK_AND_RETURN_LOG(dfxAnalyzer_, "dfxAnalyzer_ is nullptr");
    dfxAnalyzer_->FlushTranscodeFailed(type);
}

void DfxManager::HandleTranscodeCostTime(const int32_t costTime)
{
    MEDIA_INFO_LOG("HandleTranscodeCostTime type: %{public}d", costTime);
    CHECK_AND_RETURN_LOG(isInitSuccess_, "DfxManager not init");
    CHECK_AND_RETURN_LOG(dfxAnalyzer_, "dfxAnalyzer_ is nullptr");
    dfxAnalyzer_->FlushTranscodeCostTime(costTime);
}

void DfxManager::HandleUpgradeFault(const UpgradeExceptionInfo& reportData)
{
    dfxReporter_->ReportUpgradeFault(reportData);
}

int32_t DfxManager::HandleThmInodeCleanInfo(const ThmInodeCleanInfo &info)
{
    CHECK_AND_RETURN_RET_LOG(dfxReporter_ != nullptr, E_ERR, "DfxReporter is nullptr");
    return dfxReporter_->ReportThmInodeCleanInfo(info);
}
} // namespace Media
} // namespace OHOS
