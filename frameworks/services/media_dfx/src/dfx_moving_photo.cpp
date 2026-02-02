/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "DfxMovingPhoto"

#include "dfx_moving_photo.h"

#include "hisysevent.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_subscriber.h"
#include "media_edit_utils.h"
#include "preferences_helper.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {
constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
const std::string DFX_MOVING_PHOTO_XML = "/data/storage/el2/base/preferences/dfx_moving_photo.xml";
const std::string STATISTICS_FINISHED = "STATISTICS_FINISHED";
const std::string CURRENT_FILE_ID = "CURRENT_FILE_ID";
const std::string MOVING_PHOTO_TOTAL_COUNT = "MOVING_PHOTO_TOTAL_COUNT";
const std::string DIRTY_MOVING_PHOTO_TOTAL_COUNT = "DIRTY_MOVING_PHOTO_TOTAL_COUNT";
const std::string MOVING_PHOTO_NOT_LOCAL = "MOVING_PHOTO_NOT_LOCAL";
const std::string CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD = "CAMERA_NOT_EDIT_NOT_CLOUD";
const std::string CAMERA_AND_EDIT_AND_NOT_CLOUD = "CAMERA_EDIT_NOT_CLOUD";
const std::string CAMERA_AND_NOT_EDIT_AND_CLOUD = "CAMERA_NOT_EDIT_CLOUD";
const std::string CAMERA_AND_EDIT_AND_CLOUD = "CAMERA_EDIT_CLOUD";
const std::string NOT_CAMERA_AND_EDIT_AND_CLOUD = "NOT_CAMERA_EDIT_CLOUD";
const std::string NOT_CAMERA_AND_EDIT_AND_NOT_CLOUD = "NOT_CAMERA_EDIT_NOT_CLOUD";
const std::string NOT_CAMERA_AND_NOT_EDIT_AND_CLOUD = "NOT_CAMERA_NOT_EDIT_CLOUD";
const std::string NOT_CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD = "NOT_CAMERA_NOT_EDIT_NOT_CLOUD";
const std::int32_t BATCH_SIZE = 500;

std::mutex DfxMovingPhoto::mutex_;
bool DfxMovingPhoto::statisticsFinished_{false};

int32_t DfxMovingPhoto::QueryMovingPhotoCount(const int32_t startFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId};
    std::string sql = "SELECT"
                      " COUNT( * ) AS Count "
                      "FROM"
                      " Photos "
                      "WHERE"
                      " ( subtype = 3 OR (subtype = 0 AND moving_photo_effect_mode = 10))"
                      " AND file_id > ?;";
    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), E_HAS_DB_ERROR, "try to go to first row failed");
    return GetInt32Val("Count", resultSet);
}

std::vector<MovingPhotoInfo> DfxMovingPhoto::QueryMovingPhotos(const int32_t startFileId)
{
    std::vector<MovingPhotoInfo> photoInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photoInfos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, BATCH_SIZE};
    std::string sql = "SELECT"
                      " file_id,"
                      " data,"
                      " position "
                      "FROM"
                      " Photos "
                      "WHERE"
                      " (subtype = 3 OR (subtype = 0 AND moving_photo_effect_mode = 10))"
                      " AND file_id > ?"
                      " ORDER BY"
                      " file_id ASC"
                      " LIMIT ?;";
    auto resultSet = rdbStore->QuerySql(sql, bindArgs);
    bool cond = resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_RETURN_RET_LOG(cond, photoInfos, "resultSet is null or count is 0");

    do {
        MovingPhotoInfo photoInfo;
        photoInfo.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        photoInfo.path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        photoInfo.position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        photoInfos.push_back(photoInfo);
    } while (MedialibrarySubscriber::IsCurrentStatusOn() && resultSet->GoToNextRow() == NativeRdb::E_OK);
    return photoInfos;
}

void DfxMovingPhoto::StatisticsMovingPhotos(
    const std::vector<MovingPhotoInfo> &photoInfos, MovingPhotoStatistics &statistics, int32_t &curFileId)
{
    for (const MovingPhotoInfo &photoInfo : photoInfos) {
        CHECK_AND_BREAK_INFO_LOG(MedialibrarySubscriber::IsCurrentStatusOn(), "current status is off, break");
        curFileId = photoInfo.fileId;
        statistics.movingPhotoTotalCount++;

        const auto position = static_cast<PhotoPositionType>(photoInfo.position);
        if (position == PhotoPositionType::CLOUD) {
            statistics.movingPhotoNotLocal++;
            continue;
        }

        const string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(photoInfo.path);
        CHECK_AND_CONTINUE(!MediaFileUtils::IsFileExists(videoPath));

        statistics.dirtyMovingPhotoTotalCount++;

        const uint8_t isCloud = position == PhotoPositionType::LOCAL_AND_CLOUD ? 1 : 0;
        const uint8_t hasEditData =
            MediaFileUtils::IsFileExists(MediaEditUtils::GetEditDataPath(photoInfo.path)) ? 1 : 0;
        const uint8_t hasEditDataCamera =
            MediaFileUtils::IsFileExists(MediaEditUtils::GetEditDataCameraPath(photoInfo.path)) ? 1 : 0;

        const uint8_t stateFlags = (hasEditDataCamera << 2) | (hasEditData << 1) | isCloud;
        switch (stateFlags) {
            case 0b000:
                statistics.notCameraAndNotEditAndNotCloud++;
                break;
            case 0b001:
                statistics.notCameraAndNotEditAndCloud++;
                break;
            case 0b010:
                statistics.notCameraAndEditAndNotCloud++;
                break;
            case 0b011:
                statistics.notCameraAndEditAndCloud++;
                break;
            case 0b100:
                statistics.cameraAndNotEditAndNotCloud++;
                break;
            case 0b101:
                statistics.cameraAndNotEditAndCloud++;
                break;
            case 0b110:
                statistics.cameraAndEditAndNotCloud++;
                break;
            case 0b111:
                statistics.cameraAndEditAndCloud++;
                break;
            default:
                MEDIA_ERR_LOG("invalid position: %{public}d", photoInfo.position);
                break;
        }
    }
}

void DfxMovingPhoto::UpdateStatisticsFromXml(
    const std::shared_ptr<NativePreferences::Preferences> prefs, MovingPhotoStatistics &statistics)
{
    statistics.movingPhotoTotalCount += prefs->GetInt(MOVING_PHOTO_TOTAL_COUNT, 0);
    statistics.dirtyMovingPhotoTotalCount += prefs->GetInt(DIRTY_MOVING_PHOTO_TOTAL_COUNT, 0);
    statistics.movingPhotoNotLocal += prefs->GetInt(MOVING_PHOTO_NOT_LOCAL, 0);
    statistics.cameraAndNotEditAndNotCloud += prefs->GetInt(CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD, 0);
    statistics.cameraAndEditAndNotCloud += prefs->GetInt(CAMERA_AND_EDIT_AND_NOT_CLOUD, 0);
    statistics.cameraAndNotEditAndCloud += prefs->GetInt(CAMERA_AND_NOT_EDIT_AND_CLOUD, 0);
    statistics.cameraAndEditAndCloud += prefs->GetInt(CAMERA_AND_EDIT_AND_CLOUD, 0);
    statistics.notCameraAndEditAndCloud += prefs->GetInt(NOT_CAMERA_AND_EDIT_AND_CLOUD, 0);
    statistics.notCameraAndEditAndNotCloud += prefs->GetInt(NOT_CAMERA_AND_EDIT_AND_NOT_CLOUD, 0);
    statistics.notCameraAndNotEditAndCloud += prefs->GetInt(NOT_CAMERA_AND_NOT_EDIT_AND_CLOUD, 0);
    statistics.notCameraAndNotEditAndNotCloud += prefs->GetInt(NOT_CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD, 0);
}

void DfxMovingPhoto::WriteStatisticsToXml(
    const std::shared_ptr<NativePreferences::Preferences> prefs, const MovingPhotoStatistics &statistics)
{
    prefs->PutInt(MOVING_PHOTO_TOTAL_COUNT, statistics.movingPhotoTotalCount);
    prefs->PutInt(DIRTY_MOVING_PHOTO_TOTAL_COUNT, statistics.dirtyMovingPhotoTotalCount);
    prefs->PutInt(MOVING_PHOTO_NOT_LOCAL, statistics.movingPhotoNotLocal);
    prefs->PutInt(CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD, statistics.cameraAndNotEditAndNotCloud);
    prefs->PutInt(CAMERA_AND_EDIT_AND_NOT_CLOUD, statistics.cameraAndEditAndNotCloud);
    prefs->PutInt(CAMERA_AND_NOT_EDIT_AND_CLOUD, statistics.cameraAndNotEditAndCloud);
    prefs->PutInt(CAMERA_AND_EDIT_AND_CLOUD, statistics.cameraAndEditAndCloud);
    prefs->PutInt(NOT_CAMERA_AND_EDIT_AND_CLOUD, statistics.notCameraAndEditAndCloud);
    prefs->PutInt(NOT_CAMERA_AND_EDIT_AND_NOT_CLOUD, statistics.notCameraAndEditAndNotCloud);
    prefs->PutInt(NOT_CAMERA_AND_NOT_EDIT_AND_CLOUD, statistics.notCameraAndNotEditAndCloud);
    prefs->PutInt(NOT_CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD, statistics.notCameraAndNotEditAndNotCloud);
}

int32_t DfxMovingPhoto::AbnormalMovingPhotoStatistics()
{
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
    CHECK_AND_RETURN_RET_WARN_LOG(
        lock.try_lock(), E_OK, "Abnormal moving photo statistics has started, skipping this operation");
    CHECK_AND_RETURN_RET_INFO_LOG(!statisticsFinished_, E_OK, "Abnormal moving photo statistics is finished");

    int32_t errCode = E_OK;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DFX_MOVING_PHOTO_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs, E_ERR, "get preferences error: %{public}d", errCode);

    bool statisticsFinished = prefs->GetBool(STATISTICS_FINISHED, false);
    statisticsFinished_ = statisticsFinished;
    CHECK_AND_RETURN_RET_INFO_LOG(!statisticsFinished, E_OK, "Abnormal moving photo statistics is finished");

    int32_t curFileId = prefs->GetInt(CURRENT_FILE_ID, 0);
    MEDIA_INFO_LOG("Abnormal moving photo statistics start file id: %{public}d", curFileId);
    MovingPhotoStatistics statistics;
    do {
        MEDIA_INFO_LOG("Abnormal moving photo statistics curFileId: %{public}d", curFileId);
        std::vector<MovingPhotoInfo> movingPhotos = QueryMovingPhotos(curFileId);
        CHECK_AND_BREAK_INFO_LOG(!movingPhotos.empty(), "has no moving photo to statistics");
        StatisticsMovingPhotos(movingPhotos, statistics, curFileId);
    } while (MedialibrarySubscriber::IsCurrentStatusOn());

    UpdateStatisticsFromXml(prefs, statistics);
    if (QueryMovingPhotoCount(curFileId) == 0) {
        if (Report(statistics) == 0) {
            prefs->PutBool(STATISTICS_FINISHED, true);
            statisticsFinished_ = true;
            MEDIA_INFO_LOG("Statistics of abnormal moving photos have been reported");
        }
    }
    prefs->PutInt(CURRENT_FILE_ID, curFileId);
    WriteStatisticsToXml(prefs, statistics);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Abnormal moving photo statistics end file id: %{public}d", curFileId);
    return E_OK;
}

int32_t DfxMovingPhoto::Report(const MovingPhotoStatistics &statistics)
{
    CHECK_AND_RETURN_RET_INFO_LOG(
        statistics.dirtyMovingPhotoTotalCount > 0, 0, "No abnormal moving photos, no need to report");

    int32_t ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "MEDIALIB_ABNORMAL_MOV_PIC_STAT",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        MOVING_PHOTO_TOTAL_COUNT, statistics.movingPhotoTotalCount,
        DIRTY_MOVING_PHOTO_TOTAL_COUNT, statistics.dirtyMovingPhotoTotalCount,
        MOVING_PHOTO_NOT_LOCAL, statistics.movingPhotoNotLocal,
        CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD, statistics.cameraAndNotEditAndNotCloud,
        CAMERA_AND_EDIT_AND_NOT_CLOUD, statistics.cameraAndEditAndNotCloud,
        CAMERA_AND_NOT_EDIT_AND_CLOUD, statistics.cameraAndNotEditAndCloud,
        CAMERA_AND_EDIT_AND_CLOUD, statistics.cameraAndEditAndCloud,
        NOT_CAMERA_AND_EDIT_AND_CLOUD, statistics.notCameraAndEditAndCloud,
        NOT_CAMERA_AND_EDIT_AND_NOT_CLOUD, statistics.notCameraAndEditAndNotCloud,
        NOT_CAMERA_AND_NOT_EDIT_AND_CLOUD, statistics.notCameraAndNotEditAndCloud,
        NOT_CAMERA_AND_NOT_EDIT_AND_NOT_CLOUD, statistics.notCameraAndNotEditAndNotCloud);

    CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "Abnormal moving photo statistics report error: %{public}d", ret);
    MEDIA_INFO_LOG("Abnormal moving photo statistics report success");
    return ret;
}
}  // namespace Media
}  // namespace OHOS