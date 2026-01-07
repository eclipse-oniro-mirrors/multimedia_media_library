/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "RepairFutureDateTask"

#include "repair_future_date_task.h"

#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "values_bucket.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "cloud_sync_utils.h"
#include "power_efficiency_manager.h"
#include "cloud_media_photos_dao.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "photos_po.h"
#include "lake_file_utils.h"
#include "medialibrary_related_system_state_manager.h"


namespace OHOS::Media::Background {
// LCOV_EXCL_START

const std::string REPAIR_FUTURE_DATE_TASK_CONFIG = "/data/storage/el2/base/preferences/repair_future_date_task.xml";
const std::string LAST_REPAIR_FILE_ID = "last_repair_file_id";
const int32_t REPAIR_BATCH_SIZE = 100;
const int32_t FUTURE_DATE_REPAIR_INTERVAL_CLOUD = 30000;
const int32_t FUTURE_DATE_REPAIR_INTERVAL_LOCAL = 2000;
const int64_t REPAIR_TIMESTAMP_LOWER_BOUND = 2'145'887'999'000;  // 2037-12-31 23:59:59

bool RepairFutureDateTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

int32_t RepairFutureDateTask::GetRepairDateData(const int32_t lastRecord, std::vector<PhotosPo> &photos)
{
    MEDIA_INFO_LOG("GetRepairDateData begin");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);

    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    predicates.EqualTo(PhotoColumn::MEDIA_TIME_PENDING, 0);
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0);

    int64_t nowTimestamp = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t timestampLowerBound = std::max(REPAIR_TIMESTAMP_LOWER_BOUND, nowTimestamp);
    predicates.GreaterThan(PhotoColumn::MEDIA_DATE_TAKEN, timestampLowerBound);
    predicates.GreaterThan(PhotoColumn::MEDIA_ID, lastRecord);
    predicates.OrderByAsc(PhotoColumn::MEDIA_ID);
    predicates.Limit(REPAIR_BATCH_SIZE);

    const std::vector<std::string> columns{
        PhotoColumn::MEDIA_ID, PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_POSITION};

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query.");

    return ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photos);
}

void RepairFutureDateTask::UpdateFutureDate(
    const CloudMediaScanService::ScanResult &scanResult, const int32_t fileId, const int32_t position)
{
    CHECK_AND_RETURN_LOG(scanResult.scanSuccess, "scan fileId %{public}d info failed", fileId);

    MEDIA_INFO_LOG("repair future date begin, fileId: %{public}d, position: %{public}d", fileId, position);
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_DATE_TAKEN, scanResult.dateTaken);
    values.Put(PhotoColumn::PHOTO_DETAIL_TIME, scanResult.detailTime);
    values.Put(PhotoColumn::PHOTO_DATE_YEAR, scanResult.dateYear);
    values.Put(PhotoColumn::PHOTO_DATE_MONTH, scanResult.dateMonth);
    values.Put(PhotoColumn::PHOTO_DATE_DAY, scanResult.dateDay);
    values.Put(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    int32_t changedRows = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    int32_t result = rdbStore->Update(changedRows, values, predicates);
    bool cond = (result != NativeRdb::E_OK || changedRows <= 0);
    CHECK_AND_RETURN_LOG(
        !cond, "repair future date failed, result:%{public}d. changedRows:%{public}d", result, changedRows);

    MEDIA_INFO_LOG("update succeed, fileId:%{public}d, dateTaken:%{public}lld, detailTime:%{public}s",
        fileId,
        static_cast<long long>(scanResult.dateTaken),
        scanResult.detailTime.c_str());
}

void RepairFutureDateTask::RepairPhotoDate(int32_t &currentRecord, bool &terminate, const std::vector<PhotosPo> &photos)
{
    for (const PhotosPo &photosPo : photos) {
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Break repair future date cause invalid status");
            terminate = true;
            return;
        }

        int32_t fileId = photosPo.fileId.value_or(0);
        std::string path = photosPo.data.value_or("");
        int32_t position = photosPo.position.value_or(0);
        if (fileId <= 0 || path.empty() || position <= 0) {
            MEDIA_ERR_LOG("Data anomaly, fileId:%{public}d, path:%{public}s, position:%{public}d",
                fileId,
                path.c_str(),
                position);
            continue;
        }

        if (position == static_cast<int32_t>(PhotoPosition::POSITION_CLOUD) &&
            !MedialibraryRelatedSystemStateManager::GetInstance()->IsNetAvailableInOnlyWifiCondition()) {
            MEDIA_INFO_LOG("Break repair future date cause wifi is invalid");
            terminate = true;
            return;
        }

        std::string tmpPath = LakeFileUtils::GetAssetRealPath(path);
        CloudMediaScanService scanService;
        CloudMediaScanService::ScanResult scanResult;
        scanService.ScanDownloadedFile(tmpPath, scanResult);
        UpdateFutureDate(scanResult, fileId, position);

        currentRecord = fileId;
        int32_t dateRepairInterval = FUTURE_DATE_REPAIR_INTERVAL_LOCAL;
        if (position == static_cast<int32_t>(PhotoPosition::POSITION_CLOUD)) {
            dateRepairInterval = FUTURE_DATE_REPAIR_INTERVAL_CLOUD;
        }
        this_thread::sleep_for(chrono::milliseconds(dateRepairInterval));
    }
}

void RepairFutureDateTask::Execute()
{
    MEDIA_INFO_LOG("Start repair future date task");
    std::unique_lock<std::mutex> lock(repairDateMutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(lock.try_lock(), "Repairing future date has started, skipping this operation");

    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(REPAIR_FUTURE_DATE_TASK_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "get preferences error: %{public}d", errCode);

    int32_t defaultFileId = 0;
    int32_t currentRecord = prefs->GetInt(LAST_REPAIR_FILE_ID, defaultFileId);
    MEDIA_INFO_LOG("Start repair future date from %{public}d", currentRecord);

    bool terminate = false;
    std::vector<PhotosPo> photos;
    auto ret = GetRepairDateData(currentRecord, photos);
    CHECK_AND_RETURN_LOG(ret == CloudSync::E_OK, "GetRepairDateData failed, ret: %{public}d", ret);
    CHECK_AND_RETURN_LOG(!photos.empty(), "no future date photo for repair");

    do {
        CHECK_AND_BREAK_INFO_LOG(
            !terminate && MedialibrarySubscriber::IsCurrentStatusOn(), "Current conditions are not met, break");

        MEDIA_INFO_LOG("need repair future date count %{public}d", static_cast<int32_t>(photos.size()));
        RepairPhotoDate(currentRecord, terminate, photos);
        prefs->PutInt(LAST_REPAIR_FILE_ID, currentRecord);
        prefs->FlushSync();
        MEDIA_INFO_LOG("repair future date to %{public}d", currentRecord);
        CHECK_AND_BREAK_INFO_LOG(
            !terminate && MedialibrarySubscriber::IsCurrentStatusOn(), "Current conditions are not met, break");

        photos = {};
        ret = GetRepairDateData(currentRecord, photos);
        CHECK_AND_RETURN_LOG(ret == CloudSync::E_OK, "GetRepairDateData failed, ret: %{public}d", ret);
    } while (!photos.empty());

    MEDIA_INFO_LOG("End repair future date, currentRecord:%{public}d", currentRecord);
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::Background