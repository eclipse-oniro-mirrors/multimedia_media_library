/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "BackgroundCloudFileProcessor"

#include "background_cloud_file_processor.h"

#include <sys/statvfs.h>

#include "abs_rdb_predicates.h"
#include "cloud_sync_manager.h"
#include "common_timer_errors.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "mimetype_utils.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "cloud_sync_utils.h"
#include "power_efficiency_manager.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"

namespace OHOS {
namespace Media {
using namespace FileManagement::CloudSync;
using namespace OHOS::Media::ORM;

static constexpr int32_t UPDATE_BATCH_CLOUD_SIZE = 2;
static constexpr int32_t UPDATE_BATCH_LOCAL_VIDEO_SIZE = 50;
static constexpr int32_t UPDATE_BATCH_LOCAL_IMAGE_SIZE = 200;
static constexpr int32_t MAX_RETRY_COUNT = 2;
static constexpr int32_t MIMETYPE_REPAIR_INTERVAL = 20000;
static constexpr int32_t CACHE_PHOTO_NUM = 100;

// The task can be performed only when the ratio of available storage capacity reaches this value
static constexpr double DEVICE_STORAGE_FREE_RATIO_HIGH = 0.15;
static constexpr double DEVICE_STORAGE_FREE_RATIO_LOW = 0.05;

static constexpr int64_t ONEDAY_TO_SEC = 60 * 60 * 24;
static constexpr int64_t DOWNLOAD_NUM_FREE_RATIO_HIGH = 1000;
static constexpr int64_t DOWNLOAD_DAY_FREE_RATIO_HIGH = 30;
static constexpr int64_t DOWNLOAD_NUM_FREE_RATIO_LOW = 250;
static constexpr int64_t DOWNLOAD_DAY_FREE_RATIO_LOW = 7;

static const int64_t DOWNLOAD_ID_DEFAULT = -1;

int32_t BackgroundCloudFileProcessor::processInterval_ = PROCESS_INTERVAL;  // 5 minute
int32_t BackgroundCloudFileProcessor::downloadInterval_ = DOWNLOAD_INTERVAL;  // 1 minute
int32_t BackgroundCloudFileProcessor::downloadDuration_ = DOWNLOAD_DURATION; // 10 seconds
recursive_mutex BackgroundCloudFileProcessor::mutex_;
Utils::Timer BackgroundCloudFileProcessor::timer_("background_cloud_file_processor");
uint32_t BackgroundCloudFileProcessor::cloudDataTimerId_ = 0;
uint32_t BackgroundCloudFileProcessor::startTimerId_ = 0;
uint32_t BackgroundCloudFileProcessor::stopTimerId_ = 0;
bool BackgroundCloudFileProcessor::isUpdating_ = true;
int32_t BackgroundCloudFileProcessor::cloudUpdateOffset_ = 0;
int32_t BackgroundCloudFileProcessor::localImageUpdateOffset_ = 0;
int32_t BackgroundCloudFileProcessor::localVideoUpdateOffset_ = 0;
int32_t BackgroundCloudFileProcessor::cloudRetryCount_ = 0;
std::mutex BackgroundCloudFileProcessor::downloadResultMutex_;
std::mutex BackgroundCloudFileProcessor::repairMimeTypeMutex_;
std::unordered_map<std::string, BackgroundCloudFileProcessor::DownloadStatus>
    BackgroundCloudFileProcessor::downloadResult_;
int64_t BackgroundCloudFileProcessor::downloadId_ = DOWNLOAD_ID_DEFAULT;

const std::string BACKGROUND_CLOUD_FILE_CONFIG = "/data/storage/el2/base/preferences/background_cloud_file_config.xml";
const std::string DOWNLOAD_CNT_CONFIG = "/data/storage/el2/base/preferences/download_count_config.xml";

const std::string DOWNLOAD_LATEST_FINISHED = "download_latest_finished";
const std::string LAST_DOWNLOAD_MILLISECOND = "last_download_millisecond";
const std::string LAST_LOCAL_MIMETYPE_REPAIR = "last_mimetype_repair";
// when kernel hibernates, the timer expires longer, and the number of download images needs to be compensated
const int32_t MIN_DOWNLOAD_NUM = 1;
const int32_t MAX_DOWNLOAD_NUM = 30;
const double HALF = 0.5;

// LCOV_EXCL_START
void BackgroundCloudFileProcessor::SetDownloadLatestFinished(bool downloadLatestFinished)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);

    prefs->PutBool(DOWNLOAD_LATEST_FINISHED, downloadLatestFinished);
    prefs->FlushSync();
    MEDIA_INFO_LOG("set preferences %{public}d", downloadLatestFinished);
}

bool BackgroundCloudFileProcessor::GetDownloadLatestFinished()
{
    int32_t errCode;
    bool downloadLatestFinished = false;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, false,
        "get preferences error: %{public}d", errCode);
    return prefs->GetBool(DOWNLOAD_LATEST_FINISHED, downloadLatestFinished);
}

void BackgroundCloudFileProcessor::SetLastDownloadMilliSecond(int64_t lastDownloadMilliSecond)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);

    prefs->PutLong(LAST_DOWNLOAD_MILLISECOND, lastDownloadMilliSecond);
    prefs->FlushSync();
}

int64_t BackgroundCloudFileProcessor::GetLastDownloadMilliSecond()
{
    int32_t errCode;
    int64_t lastDownloadMilliSecond = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, false,
        "get preferences error: %{public}d", errCode);
    return prefs->GetLong(LAST_DOWNLOAD_MILLISECOND, lastDownloadMilliSecond);
}

void BackgroundCloudFileProcessor::ClearDownloadCnt()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DOWNLOAD_CNT_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    prefs->Clear();
    prefs->FlushSync();
}

void BackgroundCloudFileProcessor::UpdateDownloadCnt(std::string uri, int64_t cnt)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DOWNLOAD_CNT_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    prefs->PutLong(uri, cnt);
    prefs->FlushSync();
}

int64_t BackgroundCloudFileProcessor::GetDownloadCnt(std::string uri)
{
    int32_t errCode;
    int64_t defaultCnt = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DOWNLOAD_CNT_CONFIG, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, defaultCnt, "get preferences error: %{public}d", errCode);
    return prefs->GetLong(uri, defaultCnt);
}

void BackgroundCloudFileProcessor::DownloadCloudFiles()
{
    CHECK_AND_RETURN_LOG(CloudSyncUtils::IsCloudSyncSwitchOn(),
        "Cloud sync switch off, skip DownloadCloudFiles");
    MEDIA_DEBUG_LOG("Start downloading cloud files task");

    double freeRatio = 0.0;
    CHECK_AND_RETURN_LOG(GetStorageFreeRatio(freeRatio),
        "GetStorageFreeRatio failed, stop downloading cloud files");
    auto resultSet = QueryCloudFiles(freeRatio);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Failed to query cloud files!");

    DownloadFiles downloadFiles;
    ParseDownloadFiles(resultSet, downloadFiles);
    CHECK_AND_RETURN_LOG(!downloadFiles.uris.empty(), "No cloud files need to be downloaded");
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    int32_t ret = AddDownloadTask(downloadFiles);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to add download task! err: %{public}d", ret);
}

void BackgroundCloudFileProcessor::UpdateCloudData()
{
    MEDIA_DEBUG_LOG("Start update cloud data task");
    std::vector<QueryOption> queryList = {{false, true}, {false, false}, {true, true}};
    int32_t count = 0;
    UpdateData updateData;
    for (auto option : queryList) {
        std::shared_ptr<NativeRdb::ResultSet> resultSet = QueryUpdateData(option.isCloud, option.isVideo);
        if (resultSet == nullptr || resultSet->GetRowCount(count) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Failed to query data, %{public}d, %{public}d", option.isCloud, option.isVideo);
            continue;
        }
        if (count == 0) {
            MEDIA_DEBUG_LOG("no need to update, %{public}d, %{public}d", option.isCloud, option.isVideo);
            continue;
        }
        ParseUpdateData(resultSet, updateData, option.isCloud, option.isVideo);
        break;
    }

    if (updateData.abnormalData.empty()) {
        MEDIA_DEBUG_LOG("No data need to update");
        return;
    }
    int32_t ret = AddUpdateDataTask(updateData);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to add update task! err: %{public}d", ret);
    }
}

void UpdateMimeTypeByFileId(const string &mimetype, int32_t fileId)
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_MIME_TYPE, mimetype);
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t result = rdbStore->Update(changedRows, values, predicates);
    bool cond = (result != NativeRdb::E_OK || changedRows <= 0);
    CHECK_AND_RETURN_LOG(!cond, "Update operation failed. Result %{public}d. Updated %{public}d",
        result, changedRows);
}

std::vector<PhotosPo> GetRepairMimeTypeData(const int32_t &lastRecord)
{
    MEDIA_INFO_LOG("GetRepairMimeTypeData begin");
    std::vector<PhotosPo> photosPoVec;
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_TYPE, MediaColumn::MEDIA_MIME_TYPE, PhotoColumn::PHOTO_POSITION };
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photosPoVec, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0);
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, lastRecord);
    predicates.OrderByAsc(MediaColumn::MEDIA_ID);
    predicates.Limit(CACHE_PHOTO_NUM);
    
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, photosPoVec, "Failed to query.");
    
    ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPoVec);
    return photosPoVec;
}

void BackgroundCloudFileProcessor::HandleRepairMimeType(const int32_t &lastRecord)
{
    std::unique_lock<std::mutex> lock(repairMimeTypeMutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(lock.try_lock(), "Repairing mimetype has started, skipping this operation");
    MEDIA_INFO_LOG("Start repair mimetype from %{public}d", lastRecord);
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    bool terminate = false;
    int32_t repairRecord = lastRecord;
    std::vector<PhotosPo> photosPoVec = GetRepairMimeTypeData(repairRecord);
    do {
        for (PhotosPo photosPo : photosPoVec) {
            std::string path = photosPo.data.value_or("");
            int32_t fileId = photosPo.fileId.value_or(0);
            std::string mimeType = photosPo.mimeType.value_or("");
            int32_t position = photosPo.position.value_or(0);
            if (path == "" || fileId <= 0 || position <= 0) {
                repairRecord = fileId;
                continue;
            }
            if (position == static_cast<int32_t>(POSITION_CLOUD) && !MedialibrarySubscriber::IsWifiConnected()) {
                MEDIA_INFO_LOG("Break repair cause wifi not connect");
                terminate = true;
                break;
            }
            string mimeTypeNew = MimeTypeUtils::GetMimeTypeFromContent(path);
            if (mimeTypeNew != mimeType) {
                MEDIA_INFO_LOG("Update mimetype from: %{public}s to: %{public}s",
                    mimeType.c_str(), mimeTypeNew.c_str());
                UpdateMimeTypeByFileId(mimeTypeNew, fileId);
            }
            repairRecord = fileId;

            // reduce repair frequency
            this_thread::sleep_for(chrono::milliseconds(MIMETYPE_REPAIR_INTERVAL));
            if (!PowerEfficiencyManager::IsChargingAndScreenOff()) {
                MEDIA_INFO_LOG("Break repair cause invalid status");
                terminate = true;
                break;
            }
        }
        prefs->PutInt(LAST_LOCAL_MIMETYPE_REPAIR, repairRecord);
        prefs->FlushSync();
        MEDIA_INFO_LOG("repair mimetype to %{public}d", repairRecord);
        photosPoVec = GetRepairMimeTypeData(repairRecord);
    } while (photosPoVec.size() > 0 && !terminate);
}

void BackgroundCloudFileProcessor::RepairMimeType()
{
    int32_t errCode = 0;
    int64_t defaultCnt = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int32_t localRepairRecord = prefs->GetInt(LAST_LOCAL_MIMETYPE_REPAIR, defaultCnt);

    std::vector<PhotosPo> photosPoVec = GetRepairMimeTypeData(localRepairRecord);
    CHECK_AND_RETURN_LOG(photosPoVec.size() > 0, "no data for repair");
    MEDIA_INFO_LOG("need repair count %{public}d", static_cast<int>(photosPoVec.size()));
    std::thread([localRepairRecord]() {
        HandleRepairMimeType(localRepairRecord);
    }).detach();
}

void BackgroundCloudFileProcessor::ProcessCloudData()
{
    UpdateCloudData();
}

bool BackgroundCloudFileProcessor::GetStorageFreeRatio(double &freeRatio)
{
    struct statvfs diskInfo;
    int ret = statvfs("/data/storage/el2/database", &diskInfo);
    CHECK_AND_RETURN_RET_LOG(ret == 0, false, "Get file system status information failed, err: %{public}d", ret);

    double totalSize = static_cast<double>(diskInfo.f_bsize) * static_cast<double>(diskInfo.f_blocks);
    CHECK_AND_RETURN_RET_LOG(totalSize >= 1e-9, false,
        "Get file system total size failed, totalSize=%{public}f", totalSize);

    double freeSize = static_cast<double>(diskInfo.f_bsize) * static_cast<double>(diskInfo.f_bfree);
    freeRatio = freeSize / totalSize;
    return true;
}

std::shared_ptr<NativeRdb::ResultSet> BackgroundCloudFileProcessor::QueryCloudFiles(double freeRatio)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, nullptr, "uniStore is nullptr!");

    int64_t downloadNum;
    int64_t downloadMilliSecond;

    if (freeRatio >= DEVICE_STORAGE_FREE_RATIO_HIGH) {
        downloadNum = DOWNLOAD_NUM_FREE_RATIO_HIGH;
        downloadMilliSecond = DOWNLOAD_DAY_FREE_RATIO_HIGH * ONEDAY_TO_SEC * SEC_TO_MSEC;
    } else if ((freeRatio >= DEVICE_STORAGE_FREE_RATIO_LOW) && (freeRatio < DEVICE_STORAGE_FREE_RATIO_HIGH)) {
        downloadNum = DOWNLOAD_NUM_FREE_RATIO_LOW;
        downloadMilliSecond = DOWNLOAD_DAY_FREE_RATIO_LOW * ONEDAY_TO_SEC * SEC_TO_MSEC;
        MEDIA_INFO_LOG("freeRatio is %{public}.2f, available disk is low", freeRatio);
    } else {
        MEDIA_WARN_LOG("freeRatio is %{public}.2f less than %{public}.2f, stop downloading cloud files",
                       freeRatio, DEVICE_STORAGE_FREE_RATIO_LOW);
        return nullptr;
    }

    auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();

    string sql = "SELECT " + PhotoColumn::MEDIA_FILE_PATH + ", " + PhotoColumn::PHOTO_POSITION + ", " +
        PhotoColumn::MEDIA_ID + ", " + PhotoColumn::MEDIA_NAME +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        PhotoColumn::PHOTO_CLEAN_FLAG + " = " + std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN)) +
        " AND " + PhotoColumn::MEDIA_FILE_PATH + " IS NOT NULL AND " + PhotoColumn::MEDIA_FILE_PATH + " != '' AND " +
        MediaColumn::MEDIA_SIZE + " > 0 AND " + PhotoColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_IMAGE) +
        " AND " + MediaColumn::MEDIA_DATE_TAKEN + " > " + std::to_string(currentMilliSecond - downloadMilliSecond) +
        " ORDER BY " + MediaColumn::MEDIA_DATE_TAKEN + " DESC, " + PhotoColumn::MEDIA_ID + " DESC LIMIT " +
        std::to_string(downloadNum);
    MEDIA_DEBUG_LOG("ParseDownloadFiles SQL limit downloadNum: %{public}" PRId64, downloadNum);
    return uniStore->QuerySql(sql);
}

void BackgroundCloudFileProcessor::CheckAndUpdateDownloadCnt(std::string uri, int64_t cnt)
{
    bool updateDownloadCntFlag = true;
    unique_lock<mutex> downloadLock(downloadResultMutex_);
    if (downloadResult_.find(uri) != downloadResult_.end()) {
        bool cond = ((downloadResult_[uri] == DownloadStatus::NETWORK_UNAVAILABLE) ||
            (downloadResult_[uri] == DownloadStatus::STORAGE_FULL));
        CHECK_AND_EXECUTE(!cond, updateDownloadCntFlag = false);
    }
    downloadLock.unlock();

    if (updateDownloadCntFlag) {
        UpdateDownloadCnt(uri, cnt + 1);
    }
}

void BackgroundCloudFileProcessor::GetDownloadNum(int64_t &downloadNum)
{
    int64_t currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t lastDownloadMilliSecond = GetLastDownloadMilliSecond();

    int64_t minutes = (currentMilliSecond - lastDownloadMilliSecond) / downloadInterval_;
    if (minutes < MIN_DOWNLOAD_NUM) {
        downloadNum = MIN_DOWNLOAD_NUM;
    } else if (minutes >= MAX_DOWNLOAD_NUM) {
        downloadNum = MAX_DOWNLOAD_NUM;
    } else {
        downloadNum = minutes;
        bool cond = ((currentMilliSecond - lastDownloadMilliSecond) - (minutes * downloadInterval_)
            > (HALF * downloadInterval_));
        CHECK_AND_EXECUTE(!cond, downloadNum++);
    }
    MEDIA_DEBUG_LOG("GetDownloadNum downloadNum: %{public}" PRId64, downloadNum);
}

void BackgroundCloudFileProcessor::DownloadLatestFinished()
{
    SetDownloadLatestFinished(true);
    ClearDownloadCnt();

    unique_lock<mutex> downloadLock(downloadResultMutex_);
    downloadResult_.clear();
    downloadLock.unlock();

    lock_guard<recursive_mutex> lock(mutex_);
    if (startTimerId_ > 0) {
        timer_.Unregister(startTimerId_);
        startTimerId_ = 0;
    }
    if (stopTimerId_ > 0) {
        timer_.Unregister(stopTimerId_);
        stopTimerId_ = 0;
    }
}

void BackgroundCloudFileProcessor::ParseDownloadFiles(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    DownloadFiles &downloadFiles)
{
    int64_t downloadNum;
    GetDownloadNum(downloadNum);
    auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
    SetLastDownloadMilliSecond(currentMilliSecond);

    bool downloadLatestFinished = true;
    downloadFiles.uris.clear();
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string path =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        if (path.empty()) {
            MEDIA_WARN_LOG("Failed to get cloud file path!");
            continue;
        }

        int32_t position =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_POSITION, resultSet, TYPE_INT32));
        if (position != static_cast<int32_t>(POSITION_CLOUD)) {
            continue;
        }
        int32_t fileId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        MEDIA_DEBUG_LOG("ParseDownloadFiles fileId: %{public}d", fileId);
        std::string displayName =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_NAME, resultSet, TYPE_STRING));
        if (displayName.empty()) {
            MEDIA_WARN_LOG("Failed to get cloud file displayName!");
            continue;
        }
        std::string uri = "";
        uri = MediaFileUri::GetPhotoUri(to_string(fileId), path, displayName);

        int64_t cnt = GetDownloadCnt(uri);
        if (cnt < DOWNLOAD_FAIL_MAX_TIMES) {
            downloadLatestFinished = false;
            downloadFiles.uris.push_back(uri);
            downloadFiles.mediaType = MEDIA_TYPE_IMAGE;
            CheckAndUpdateDownloadCnt(uri, cnt);
            if ((int64_t)downloadFiles.uris.size() >= downloadNum) {
                break;
            }
        }
    }
    if (downloadLatestFinished) {
        DownloadLatestFinished();
    }
    MEDIA_DEBUG_LOG("ParseDownloadFiles downloadLatestFinished: %{public}d", downloadLatestFinished);
}

void BackgroundCloudFileProcessor::removeFinishedResult(const std::vector<std::string>& downloadingPaths)
{
    lock_guard<mutex> downloadLock(downloadResultMutex_);
    for (auto it = downloadResult_.begin(); it != downloadResult_.end();) {
        if (find(downloadingPaths.begin(), downloadingPaths.end(), it->first) == downloadingPaths.end()) {
            it = downloadResult_.erase(it);
        } else {
            it++;
        }
    }
}

int32_t BackgroundCloudFileProcessor::AddDownloadTask(const DownloadFiles &downloadFiles)
{
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL, "Failed to get async worker instance!");

    auto *taskData = new (std::nothrow) DownloadCloudFilesData(downloadFiles);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_NO_MEMORY,
        "Failed to alloc async data for downloading cloud files!");

    auto asyncTask = std::make_shared<MediaLibraryAsyncTask>(DownloadCloudFilesExecutor, taskData);
    asyncWorker->AddTask(asyncTask, false);
    return E_OK;
}

void BackgroundCloudFileProcessor::DownloadCloudFilesExecutor(AsyncTaskData *data)
{
    auto *taskData = static_cast<DownloadCloudFilesData *>(data);
    auto downloadFiles = taskData->downloadFiles_;

    MEDIA_DEBUG_LOG("Try to download %{public}zu cloud files.", downloadFiles.uris.size());

    unique_lock<mutex> downloadLock(downloadResultMutex_);
    for (const auto &uri : downloadFiles.uris) {
        downloadResult_[uri] = DownloadStatus::INIT;
        MEDIA_INFO_LOG("Start to download cloud file, uri: %{public}s", MediaFileUtils::DesensitizePath(uri).c_str());
    }
    downloadLock.unlock();

    removeFinishedResult(downloadFiles.uris);

    std::shared_ptr<BackgroundCloudFileDownloadCallback> downloadCallback = nullptr;
    downloadCallback = std::make_shared<BackgroundCloudFileDownloadCallback>();
    CHECK_AND_RETURN_LOG(downloadCallback != nullptr, "downloadCallback is null.");
    int32_t ret = CloudSyncManager::GetInstance().StartFileCache(downloadFiles.uris, downloadId_,
        FieldKey::FIELDKEY_CONTENT, downloadCallback);
    if (ret != E_OK || downloadId_ == DOWNLOAD_ID_DEFAULT) {
        MEDIA_ERR_LOG("failed to StartFileCache, ret: %{public}d, downloadId_: %{public}s.",
            ret, to_string(downloadId_).c_str());
        downloadId_ = DOWNLOAD_ID_DEFAULT;
        return;
    }

    MEDIA_INFO_LOG("Success, downloadId: %{public}d, downloadNum: %{public}d.",
        static_cast<int32_t>(downloadId_), static_cast<int32_t>(downloadFiles.uris.size()));

    lock_guard<recursive_mutex> lock(mutex_);

    if (downloadFiles.mediaType == MEDIA_TYPE_VIDEO) {
        if (stopTimerId_ > 0) {
            timer_.Unregister(stopTimerId_);
        }
        stopTimerId_ = timer_.Register(StopDownloadFiles, downloadDuration_, true);
    }
}

void BackgroundCloudFileProcessor::StopDownloadFiles()
{
    if (downloadId_ != DOWNLOAD_ID_DEFAULT) {
        int32_t ret = CloudSyncManager::GetInstance().StopFileCache(downloadId_);
        MEDIA_INFO_LOG("Stop downloading cloud file, err: %{public}d, downloadId_: %{public}d",
            ret, static_cast<int32_t>(downloadId_));
    }
}

void BackgroundCloudFileProcessor::HandleSuccessCallback(const DownloadProgressObj& progress)
{
    lock_guard<mutex> downloadLock(downloadResultMutex_);
    bool cond = (progress.downloadId != downloadId_ || downloadResult_.find(progress.path) == downloadResult_.end());
    CHECK_AND_RETURN_WARN_LOG(!cond,
        "downloadId or uri is err, uri: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
        MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(progress.downloadId).c_str(),
        to_string(downloadId_).c_str());

    downloadResult_[progress.path] = DownloadStatus::SUCCESS;
    MEDIA_INFO_LOG("download success, uri: %{public}s.", MediaFileUtils::DesensitizePath(progress.path).c_str());
}

void BackgroundCloudFileProcessor::HandleFailedCallback(const DownloadProgressObj& progress)
{
    lock_guard<mutex> downloadLock(downloadResultMutex_);
    bool cond = (progress.downloadId != downloadId_ || downloadResult_.find(progress.path) == downloadResult_.end());
    CHECK_AND_RETURN_WARN_LOG(!cond,
        "downloadId or uri is err, uri: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
        MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(progress.downloadId).c_str(),
        to_string(downloadId_).c_str());

    MEDIA_ERR_LOG("download failed, error type: %{public}d, uri: %{public}s.", progress.downloadErrorType,
        MediaFileUtils::DesensitizePath(progress.path).c_str());
    switch (progress.downloadErrorType) {
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::NETWORK_UNAVAILABLE): {
            downloadResult_[progress.path] = DownloadStatus::NETWORK_UNAVAILABLE;
            break;
        }
        case static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::LOCAL_STORAGE_FULL): {
            downloadResult_[progress.path] = DownloadStatus::STORAGE_FULL;
            break;
        }
        default: {
            downloadResult_[progress.path] = DownloadStatus::UNKNOWN;
            MEDIA_WARN_LOG("download error type not exit.");
            break;
        }
    }
}

void BackgroundCloudFileProcessor::HandleStoppedCallback(const DownloadProgressObj& progress)
{
    lock_guard<mutex> downloadLock(downloadResultMutex_);
    bool cond = (progress.downloadId != downloadId_ || downloadResult_.find(progress.path) == downloadResult_.end());
    CHECK_AND_RETURN_WARN_LOG(!cond,
        "downloadId or uri is err, uri: %{public}s, downloadId: %{public}s, downloadId_: %{public}s.",
        MediaFileUtils::DesensitizePath(progress.path).c_str(), to_string(progress.downloadId).c_str(),
        to_string(downloadId_).c_str());

    downloadResult_[progress.path] = DownloadStatus::STOPPED;
    UpdateDownloadCnt(progress.path, 0);
    MEDIA_ERR_LOG("download stopped, uri: %{public}s.", MediaFileUtils::DesensitizePath(progress.path).c_str());
}

void BackgroundCloudFileProcessor::SetPredicates(NativeRdb::RdbPredicates &predicates, bool isCloud, bool isVideo)
{
    if (isCloud) {
        predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD))
            ->OrderByAsc(MediaColumn::MEDIA_ID)
            ->Limit(cloudUpdateOffset_, UPDATE_BATCH_CLOUD_SIZE);
    } else {
        if (isVideo) {
            predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD))->And()
                ->BeginWrap()
                ->EqualTo(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_VIDEO))->And()
                ->BeginWrap()
                ->EqualTo(MediaColumn::MEDIA_DURATION, 0)->Or()
                ->IsNull(MediaColumn::MEDIA_DURATION)
                ->EndWrap()
                ->EndWrap()
                ->OrderByAsc(MediaColumn::MEDIA_ID)
                ->Limit(localVideoUpdateOffset_, UPDATE_BATCH_LOCAL_VIDEO_SIZE);
        } else {
            predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD))->And()
                ->NotEqualTo(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_VIDEO))
                ->OrderByAsc(MediaColumn::MEDIA_ID)
                ->Limit(localImageUpdateOffset_, UPDATE_BATCH_LOCAL_IMAGE_SIZE);
        }
    }
}

std::shared_ptr<NativeRdb::ResultSet> BackgroundCloudFileProcessor::QueryUpdateData(bool isCloud, bool isVideo)
{
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_TYPE, MediaColumn::MEDIA_SIZE, MediaColumn::MEDIA_NAME,
        PhotoColumn::PHOTO_WIDTH, PhotoColumn::PHOTO_HEIGHT,
        MediaColumn::MEDIA_MIME_TYPE, MediaColumn::MEDIA_DURATION, PhotoColumn::PHOTO_MEDIA_SUFFIX };

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.BeginWrap()
        ->EqualTo(MediaColumn::MEDIA_SIZE, 0)
        ->Or()
        ->IsNull(MediaColumn::MEDIA_SIZE)
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_WIDTH, 0)
        ->Or()
        ->IsNull(PhotoColumn::PHOTO_WIDTH)
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_HEIGHT, 0)
        ->Or()
        ->IsNull(PhotoColumn::PHOTO_HEIGHT)
        ->Or()
        ->EqualTo(MediaColumn::MEDIA_MIME_TYPE, "")
        ->Or()
        ->IsNull(MediaColumn::MEDIA_MIME_TYPE)
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_MEDIA_SUFFIX, "")
        ->Or()
        ->IsNull(PhotoColumn::PHOTO_MEDIA_SUFFIX)
        ->Or()
        ->BeginWrap()
        ->EqualTo(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_VIDEO))
        ->And()
        ->BeginWrap()
        ->EqualTo(MediaColumn::MEDIA_DURATION, 0)
        ->Or()
        ->IsNull(MediaColumn::MEDIA_DURATION)
        ->EndWrap()
        ->EndWrap()
        ->EndWrap()
        ->And()
        ->EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0)
        ->And()
        ->EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE))
        ->And()
        ->EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN))
        ->And();
    SetPredicates(predicates, isCloud, isVideo);
    return MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
}

void BackgroundCloudFileProcessor::ParseUpdateData(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    UpdateData &updateData, bool isCloud, bool isVideo)
{
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        int64_t size =
            get<int64_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_SIZE, resultSet, TYPE_INT64));
        int32_t width =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_WIDTH, resultSet, TYPE_INT32));
        int32_t height =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_HEIGHT, resultSet, TYPE_INT32));
        int32_t duration =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_DURATION, resultSet, TYPE_INT32));
        std::string path =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        std::string displayName =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_NAME, resultSet, TYPE_STRING));
        if (path.empty()) {
            MEDIA_WARN_LOG("Failed to get data path");
            continue;
        }
        std::string mimeType =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_MIME_TYPE, resultSet, TYPE_STRING));
        std::string suffix =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_MEDIA_SUFFIX, resultSet, TYPE_STRING));
        int32_t mediaType =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TYPE, resultSet, TYPE_INT32));

        AbnormalData abnormalData;
        abnormalData.fileId = fileId;
        abnormalData.path = path;
        abnormalData.displayName = displayName;
        abnormalData.size = size;
        abnormalData.width = width;
        abnormalData.height = height;
        abnormalData.duration = duration;
        abnormalData.mimeType = mimeType;
        abnormalData.mediaSuffix = suffix;
        abnormalData.isCloud = isCloud;
        abnormalData.isVideo = isVideo;

        if (isCloud && mediaType == static_cast<int32_t>(MEDIA_TYPE_VIDEO)) {
            updateData.abnormalData.clear();
            abnormalData.mediaType = MEDIA_TYPE_VIDEO;
            updateData.abnormalData.push_back(abnormalData);
            return;
        }
        abnormalData.mediaType = static_cast<MediaType>(mediaType);
        updateData.abnormalData.push_back(abnormalData);
    }
}

int32_t BackgroundCloudFileProcessor::AddUpdateDataTask(const UpdateData &updateData)
{
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_FAIL, "Failed to get async worker instance!");

    auto *taskData = new (std::nothrow) UpdateAbnormalData(updateData);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_NO_MEMORY, "Failed to alloc async data for update cloud data!");

    auto asyncTask = std::make_shared<MediaLibraryAsyncTask>(UpdateCloudDataExecutor, taskData);
    asyncWorker->AddTask(asyncTask, false);
    return E_OK;
}

void BackgroundCloudFileProcessor::UpdateCurrentOffset(bool isCloud, bool isVideo)
{
    if (isCloud) {
        if (cloudRetryCount_ >= MAX_RETRY_COUNT) {
            cloudUpdateOffset_ += 1;
            cloudRetryCount_ = 0;
        } else {
            cloudRetryCount_ += 1;
        }
        MEDIA_INFO_LOG("cloudUpdateOffset_ is %{public}d, cloudRetryCount_ is %{public}d",
            cloudUpdateOffset_, cloudRetryCount_);
        return;
    }
    if (isVideo) {
        localVideoUpdateOffset_++;
        MEDIA_INFO_LOG("localVideoUpdateOffset_ is %{public}d", localVideoUpdateOffset_);
    } else {
        localImageUpdateOffset_++;
        MEDIA_INFO_LOG("localImageUpdateOffset_ is %{public}d", localImageUpdateOffset_);
    }
}

void BackgroundCloudFileProcessor::UpdateCloudDataExecutor(AsyncTaskData *data)
{
    auto *taskData = static_cast<UpdateAbnormalData *>(data);
    auto updateData = taskData->updateData_;

    MEDIA_INFO_LOG("start update %{public}zu cloud files.", updateData.abnormalData.size());
    for (const auto &abnormalData : updateData.abnormalData) {
        CHECK_AND_RETURN_LOG(isUpdating_, "stop update data, isUpdating_ is %{public}d.", isUpdating_);
        std::unique_ptr<Metadata> metadata = make_unique<Metadata>();
        metadata->SetFilePath(abnormalData.path);
        metadata->SetFileMediaType(abnormalData.mediaType);
        metadata->SetFileId(abnormalData.fileId);
        metadata->SetFileDuration(abnormalData.duration);
        metadata->SetFileHeight(abnormalData.height);
        metadata->SetFileWidth(abnormalData.width);
        metadata->SetFileSize(abnormalData.size);
        metadata->SetFileMimeType(abnormalData.mimeType);
        GetSizeAndMimeType(metadata);
        metadata->SetFileExtension(ScannerUtils::GetFileExtension(abnormalData.displayName));
        if (abnormalData.size == 0 || abnormalData.mimeType.empty()) {
            int64_t fileSize = metadata->GetFileSize();
            string mimeType =  metadata->GetFileMimeType();
            metadata->SetFileSize(fileSize == 0 ? -1: fileSize);
            metadata->SetFileMimeType(mimeType.empty() ? DEFAULT_IMAGE_MIME_TYPE : mimeType);
        }
        if (abnormalData.width == 0 || abnormalData.height == 0
            || (abnormalData.duration == 0 && abnormalData.mediaType == MEDIA_TYPE_VIDEO)) {
            int32_t ret = GetExtractMetadata(metadata);
            if (ret != E_OK && MediaFileUtils::IsFileExists(abnormalData.path)) {
                UpdateCurrentOffset(abnormalData.isCloud, abnormalData.isVideo);
                MEDIA_ERR_LOG("failed to get extract metadata! err: %{public}d.", ret);
                continue;
            }
            int32_t width = metadata->GetFileWidth();
            int32_t height = metadata->GetFileHeight();
            int32_t duration = metadata->GetFileDuration();
            metadata->SetFileWidth(width == 0 ? -1: width);
            metadata->SetFileHeight(height == 0 ? -1: height);
            metadata->SetFileDuration((duration == 0 && abnormalData.mediaType == MEDIA_TYPE_VIDEO) ? -1: duration);
        }
        UpdateAbnormaldata(metadata, PhotoColumn::PHOTOS_TABLE);
        if (abnormalData.isCloud) {
            cloudRetryCount_ = 0;
        }
    }
}

static void SetAbnormalValuesFromMetaData(std::unique_ptr<Metadata> &metadata, NativeRdb::ValuesBucket &values)
{
    values.PutLong(MediaColumn::MEDIA_SIZE, metadata->GetFileSize());
    values.PutInt(MediaColumn::MEDIA_DURATION, metadata->GetFileDuration());
    values.PutInt(PhotoColumn::PHOTO_HEIGHT, metadata->GetFileHeight());
    values.PutInt(PhotoColumn::PHOTO_WIDTH, metadata->GetFileWidth());
    double aspectRatio =
        MediaFileUtils::CalculateAspectRatio(metadata->GetFileHeight(), metadata->GetFileWidth());
    values.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, aspectRatio);
    values.PutString(MediaColumn::MEDIA_MIME_TYPE, metadata->GetFileMimeType());
    values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, metadata->GetFileExtension());
}

void BackgroundCloudFileProcessor::UpdateAbnormaldata(std::unique_ptr<Metadata> &metadata, const std::string &tableName)
{
    int32_t updateCount(0);
    NativeRdb::ValuesBucket values;
    string whereClause = MediaColumn::MEDIA_ID + " = ?";
    vector<string> whereArgs = { to_string(metadata->GetFileId()) };
    SetAbnormalValuesFromMetaData(metadata, values);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Update operation failed. rdbStore is null");
    CHECK_AND_RETURN_LOG(isUpdating_, "stop update data,isUpdating_ is %{public}d.", isUpdating_);

    int32_t result = rdbStore->Update(updateCount, tableName, values, whereClause, whereArgs);
    bool cond = (result != NativeRdb::E_OK || updateCount <= 0);
    CHECK_AND_RETURN_LOG(!cond, "Update operation failed. Result %{public}d. Updated %{public}d",
        result, updateCount);
}

void BackgroundCloudFileProcessor::GetSizeAndMimeType(std::unique_ptr<Metadata> &metadata)
{
    std::string path = metadata->GetFilePath();
    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("stat syscall err %{public}d", errno);
        metadata->SetFileSize(static_cast<int64_t>(0));
    } else {
        metadata->SetFileSize(statInfo.st_size);
    }
    string extension = ScannerUtils::GetFileExtension(path);
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    metadata->SetFileExtension(extension);
    metadata->SetFileMimeType(mimeType);
}

int32_t BackgroundCloudFileProcessor::GetExtractMetadata(std::unique_ptr<Metadata> &metadata)
{
    int32_t err = 0;
    if (metadata->GetFileMediaType() == MEDIA_TYPE_IMAGE) {
        err = MetadataExtractor::ExtractImageMetadata(metadata);
    } else {
        err = MetadataExtractor::ExtractAVMetadata(metadata);
    }

    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "failed to extract data");
    return E_OK;
}

void BackgroundCloudFileProcessor::StopUpdateData()
{
    isUpdating_ = false;
}

void BackgroundCloudFileProcessor::StartTimer()
{
    lock_guard<recursive_mutex> lock(mutex_);
    MEDIA_INFO_LOG("Turn on the background download cloud file timer");
    CHECK_AND_EXECUTE(cloudDataTimerId_ <= 0, timer_.Unregister(cloudDataTimerId_));
    CHECK_AND_EXECUTE(startTimerId_ <= 0, timer_.Unregister(startTimerId_));
    uint32_t ret = timer_.Setup();
    CHECK_AND_PRINT_LOG(ret == Utils::TIMER_ERR_OK,
        "Failed to start background download cloud files timer, err: %{public}d", ret);
    isUpdating_ = true;
    cloudDataTimerId_ = timer_.Register(ProcessCloudData, processInterval_);
    if (!GetDownloadLatestFinished()) {
        auto currentMilliSecond = MediaFileUtils::UTCTimeMilliSeconds();
        SetLastDownloadMilliSecond(currentMilliSecond);
        startTimerId_ = timer_.Register(DownloadCloudFiles, downloadInterval_);
    }
}

void BackgroundCloudFileProcessor::StopTimer()
{
    lock_guard<recursive_mutex> lock(mutex_);
    MEDIA_INFO_LOG("Turn off the background download cloud file timer");

    timer_.Unregister(cloudDataTimerId_);
    timer_.Unregister(startTimerId_);
    timer_.Unregister(stopTimerId_);
    timer_.Shutdown();
    cloudDataTimerId_ = 0;
    startTimerId_ = 0;
    stopTimerId_ = 0;
    StopUpdateData();
    StopDownloadFiles();
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
