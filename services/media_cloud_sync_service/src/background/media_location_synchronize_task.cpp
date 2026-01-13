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

#define MLOG_TAG "Media_Background"

#include "media_location_synchronize_task.h"

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
#include "cloud_media_scan_service.h"
#include "cloud_media_dao_utils.h"
#include "medialibrary_related_system_state_manager.h"

namespace OHOS::Media::Background {

using namespace OHOS::Media::CloudSync;

const std::string BACKGROUND_CLOUD_FILE_CONFIG = "/data/storage/el2/base/preferences/background_cloud_file_config.xml";
const std::string LAST_LOCAL_LOCATION_REPAIR = "last_location_repair";
static constexpr int32_t LOCATION_REPAIR_INTERVAL_CLOUD = 15000;
static constexpr int32_t LOCATION_REPAIR_INTERVAL_LOCAL = 1000;
static constexpr int32_t CACHE_PHOTO_NUM = 100;
constexpr double DOUBLE_EPSILON = 1e-15;
constexpr double DEFAULT_LONGITUDE = 0.0;
constexpr double DEFAULT_LATITUDE = 0.0;
std::mutex MediaLocationSynchronizeTask::repairLocationMutex_;

bool MediaLocationSynchronizeTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

int32_t MediaLocationSynchronizeTask::GetNeedRepairCount(const int32_t repairRecord)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    std::vector<NativeRdb::ValueObject> bindArgs = {repairRecord};
    auto resultSet = rdbStore->QuerySql(SQL_GET_REPAIR_LOCATION_COUNT, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "resultSet is null");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        MEDIA_DEBUG_LOG("fail to query");
        return 0;
    }
    int32_t count = GetInt32Val("count", resultSet);
    MEDIA_INFO_LOG("GetNeedRepairCount Rowcount: %{public}d", count);
    resultSet->Close();
    return count;
}

int32_t GetRepairLocationData(const int32_t lastRecord, std::vector<PhotosPo> &photosPoVec)
{
    MEDIA_INFO_LOG("GetRepairLocationData begin");
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::PHOTO_POSITION, PhotoColumn::PHOTO_CLOUD_ID};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.IsNull(PhotoColumn::PHOTO_LATITUDE)->Or()->EqualTo(PhotoColumn::PHOTO_LATITUDE, 0);
    predicates.IsNull(PhotoColumn::PHOTO_LONGITUDE)->Or()->EqualTo(PhotoColumn::PHOTO_LONGITUDE, 0);
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, 0);
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, 0);
    predicates.EqualTo(PhotoColumn::MEDIA_TIME_PENDING, 0);
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, lastRecord);
    predicates.OrderByAsc(MediaColumn::MEDIA_ID);
    predicates.Limit(CACHE_PHOTO_NUM);
    
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "Failed to query.");
    
    return ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPoVec);
}

void RepairPhotoLocation(int32_t &repairRecord, bool &terminate, std::vector<PhotosPo> &photosPoVec)
{
    for (PhotosPo photosPo : photosPoVec) {
        std::string path = photosPo.data.value_or("");
        int32_t fileId = photosPo.fileId.value_or(0);
        std::string cloudId = photosPo.cloudId.value_or("");
        int32_t position = photosPo.position.value_or(0);
        if (path == "" || position <= 0) {
            repairRecord = fileId;
            MEDIA_INFO_LOG("fileId = %{public}d is invaild", fileId);
            continue;
        }
        if (position == static_cast<int32_t>(PhotoPosition::POSITION_CLOUD) &&
            !MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnected()) {
            MEDIA_INFO_LOG("Break repair cause wifi not connect");
            terminate = true;
            break;
        }
        repairRecord = fileId;
        this_thread::sleep_for(chrono::milliseconds(LOCATION_REPAIR_INTERVAL_CLOUD));
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Break repair cause invalid status");
            terminate = true;
            break;
        }
    }
}

void MediaLocationSynchronizeTask::HandleRepairLocation(const int32_t lastRecord)
{
    std::unique_lock<std::mutex> lock(repairLocationMutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(lock.try_lock(), "Repairing location has started, skipping this operation");
    MEDIA_INFO_LOG("Start repair location from %{public}d", lastRecord);
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    bool terminate = false;
    int32_t repairRecord = lastRecord;
    std::vector<PhotosPo> photosPoVec;
    GetRepairLocationData(repairRecord, photosPoVec);
    do {
        MEDIA_INFO_LOG("need repair location count %{public}d", static_cast<int>(photosPoVec.size()));
        RepairPhotoLocation(repairRecord, terminate, photosPoVec);
        prefs->PutInt(LAST_LOCAL_LOCATION_REPAIR, repairRecord);
        prefs->FlushSync();
        MEDIA_INFO_LOG("repair location to %{public}d", repairRecord);
        GetRepairLocationData(repairRecord, photosPoVec);
    } while (photosPoVec.size() > 0 && !terminate && MedialibrarySubscriber::IsCurrentStatusOn());
}

void MediaLocationSynchronizeTask::Execute()
{
    MEDIA_INFO_LOG("Start location synchronizing task");
    int32_t errCode = 0;
    int32_t defaultCnt = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(BACKGROUND_CLOUD_FILE_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int32_t localRepairRecord = prefs->GetInt(LAST_LOCAL_LOCATION_REPAIR, defaultCnt);

    int32_t needRepairCount = GetNeedRepairCount(localRepairRecord);
    CHECK_AND_RETURN_LOG(needRepairCount > 0, "GetNeedRepairCount Empty");
    std::thread([localRepairRecord]() {
        HandleRepairLocation(localRepairRecord);
    }).detach();
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::Background