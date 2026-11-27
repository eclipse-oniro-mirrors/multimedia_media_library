/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "media_live_photo_4d_status_task.h"

#include "preferences.h"
#include "preferences_helper.h"

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "moving_photo_file_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::NativePreferences;

namespace OHOS::Media::Background {

static const std::string LIVE_PHOTO_4D_STATUS_EVENT =
    "/data/storage/el2/base/preferences/live_photo_4d_status_events.xml";
static const int32_t prefsNullErrCode = -1;

bool MediaLivePhoto4dStatusTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaLivePhoto4dStatusTask::Execute()
{
    HandleLivePhoto4dStatus();
}

void MediaLivePhoto4dStatusTask::SetBatchStatus(int32_t startFileId)
{
    MEDIA_INFO_LOG("livePhoto4dStatusTask::SetBatchStatus start, startFileId: %{public}d", startFileId);
    int32_t errCode = 0;
    shared_ptr<Preferences> prefs = PreferencesHelper::GetPreferences(
        MediaPathUtils::AdaptPreferencesPathForAllDevice(LIVE_PHOTO_4D_STATUS_EVENT, true), errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "livePhoto4dStatusTask:Get preferences error: %{public}d", errCode);
    prefs->PutInt("startFileId", startFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("livePhoto4dStatusTask:SetBatchStatus: %{public}d", startFileId);
}

int32_t MediaLivePhoto4dStatusTask::GetBatchStatus()
{
    MEDIA_INFO_LOG("livePhoto4dStatusTask::GetBatchStatus start");
    int32_t errCode = 0;
    shared_ptr<Preferences> prefs = PreferencesHelper::GetPreferences(
        MediaPathUtils::AdaptPreferencesPathForAllDevice(LIVE_PHOTO_4D_STATUS_EVENT, true), errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, prefsNullErrCode,
        "livePhoto4dStatusTask:Get preferences error: %{public}d", errCode);
    int32_t defaultVal = 0;
    int32_t currStartFileId = prefs->GetInt("startFileId", defaultVal);
    MEDIA_INFO_LOG("livePhoto4dStatusTask: currStartFileId is %{public}d", currStartFileId);
    return currStartFileId;
}

void MediaLivePhoto4dStatusTask::HandleLivePhoto4dStatus()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "livePhoto4dStatusTask:RdbStore is null");

    int32_t currStartFileId = GetBatchStatus();
    CHECK_AND_RETURN_LOG(currStartFileId != prefsNullErrCode, "livePhoto4dStatusTask:GetBatchStatus failed");

    int32_t startFileId = (currStartFileId == 0) ? 0 : currStartFileId;

    while (true) {
        if (!Accept()) {
            MEDIA_INFO_LOG("livePhoto4dStatusTask:Accept check failed, save current position and return");
            SetBatchStatus(startFileId);
            return;
        }

        auto resultSet = QueryLivePhoto4d(startFileId);
        CHECK_AND_RETURN_LOG(resultSet != nullptr, "livePhoto4dStatusTask:Failed to query live photo 4d");

        std::vector<LivePhoto4dData> dataList;
        bool ret = ParseLivePhoto4dData(resultSet, dataList);
        CHECK_AND_RETURN_LOG(ret, "livePhoto4dStatusTask:Failed to parse live photo 4d data");

        if (dataList.empty()) {
            MEDIA_INFO_LOG("livePhoto4dStatusTask:No more data, reset scan id");
            SetBatchStatus(startFileId);
            return;
        }

        ProcessLivePhoto4d(dataList);

        int32_t lastFileId = dataList.back().fileId;
        startFileId = lastFileId + 1;
    }
}

std::shared_ptr<NativeRdb::ResultSet> MediaLivePhoto4dStatusTask::QueryLivePhoto4d(int32_t startFileId)
{
    std::vector<std::string> columns = {PhotoColumn::MEDIA_ID, PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
        PhotoColumn::MEDIA_FILE_PATH};

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("livePhoto4dStatusTask:Start query live photo 4d, startFileId: %{public}d", startFileId);

    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO))
        ->And()
        ->EqualTo(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
        static_cast<int32_t>(LivePhoto4dStatusType::TYPE_UNIDENTIFIED))
        ->And()
        ->BeginWrap()
            ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL))
            ->Or()
            ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))
        ->EndWrap();

    if (startFileId > 0) {
        predicates.And()->GreaterThanOrEqualTo(PhotoColumn::MEDIA_ID, startFileId);
    }

    predicates.OrderByAsc(PhotoColumn::MEDIA_ID)->Limit(TASK_MAX_QUERY_NUM);

    return MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
}

bool MediaLivePhoto4dStatusTask::ParseLivePhoto4dData(shared_ptr<NativeRdb::ResultSet>& resultSet,
    std::vector<LivePhoto4dData>& dataList)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "livePhoto4dStatusTask:ResultSet is nullptr");

    int rowCount = 0;
    int32_t err = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(err == E_OK && rowCount >= 0, false,
        "livePhoto4dStatusTask:GetRowCount failed, err:%{public}d", err);
    CHECK_AND_RETURN_RET_LOG(rowCount > 0, true, "livePhoto4dStatusTask:Rdb has no data");

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "livePhoto4dStatusTask:Failed to GoToFirstRow %{public}d", err);

    int32_t index = -1;
    do {
        LivePhoto4dData data;

        err = resultSet->GetColumnIndex(PhotoColumn::MEDIA_ID, index);
        CHECK_AND_CONTINUE_ERR_LOG(err == E_OK, "livePhoto4dStatusTask:Failed to GetColumnIndex MEDIA_ID");
        err = resultSet->GetInt(index, data.fileId);
        CHECK_AND_CONTINUE_ERR_LOG(err == E_OK, "livePhoto4dStatusTask:Failed to GetInt fileId");

        err = resultSet->GetColumnIndex(PhotoColumn::MEDIA_FILE_PATH, index);
        CHECK_AND_CONTINUE_ERR_LOG(err == E_OK, "livePhoto4dStatusTask:Failed to GetColumnIndex MEDIA_FILE_PATH");
        err = resultSet->GetString(index, data.path);
        CHECK_AND_CONTINUE_ERR_LOG(err == E_OK, "livePhoto4dStatusTask:Failed to GetString path");

        data.extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(data.path);
        dataList.push_back(data);
    } while (resultSet->GoToNextRow() == E_OK);

    return true;
}

int32_t MediaLivePhoto4dStatusTask::UpdateLivePhoto4dStatus(int32_t fileId, int32_t status)
{
    ValuesBucket values;
    string whereClause = PhotoColumn::MEDIA_ID + " = ?";
    vector<string> whereArgs = { to_string(fileId) };
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "livePhoto4dStatusTask:rdbStore is null");

    values.PutInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, status);

    int32_t updateCount = 0;
    int32_t result = rdbStore->Update(updateCount, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK && updateCount > 0, E_ERR,
        "livePhoto4dStatusTask:Update status failed, result: %{public}d, updateCount: %{public}d", result, updateCount);
    return E_OK;
}

void MediaLivePhoto4dStatusTask::ProcessLivePhoto4d(const std::vector<LivePhoto4dData>& dataList)
{
    MEDIA_INFO_LOG("livePhoto4dStatusTask:Start processing %{public}zu live photo 4d", dataList.size());
    int32_t count = 0;
    int32_t lastFileId = 0;
    for (const auto& livePhoto : dataList) {
        CHECK_AND_BREAK_INFO_LOG(Accept(), "livePhoto4dStatusTask:The conditions for task are not met");
        lastFileId = livePhoto.fileId;

        string extraDataPath = livePhoto.extraDataPath;
        if (extraDataPath.empty() || !MediaPureFileUtils::IsFileExists(extraDataPath)) {
            MEDIA_WARN_LOG("livePhoto4dStatusTask:Extra data path not exists, file_id: %{public}d", livePhoto.fileId);
            continue;
        }

        uint32_t version = 0;
        int32_t ret = MovingPhotoFileUtils::GetExtraDataVersion(extraDataPath, version);
        if (ret != E_OK) {
            MEDIA_WARN_LOG("livePhoto4dStatusTask:Failed to get extra data version, file_id: %{public}d",
                livePhoto.fileId);
            continue;
        }

        MEDIA_DEBUG_LOG("livePhoto4dStatusTask:Live photo 4d version: %{public}d, file_id: %{public}d",
            version, livePhoto.fileId);
        if (version == LIVE_PHOTO_4D_VERSION) {
            ret = UpdateLivePhoto4dStatus(livePhoto.fileId,
                static_cast<int32_t>(LivePhoto4dStatusType::TYPE_LIVEPHOTO_4D));
            if (ret == E_OK) {
                count++;
                MEDIA_DEBUG_LOG("livePhoto4dStatusTask:Update live photo 4d status to LIVE, file_id: %{public}d",
                    livePhoto.fileId);
            }
        }
    }

    if (lastFileId > 0) {
        SetBatchStatus(lastFileId);
    }
    MEDIA_INFO_LOG("livePhoto4dStatusTask:Finish processing, updated %{public}d, last file_id: %{public}d",
        count, lastFileId);
}
} // namespace OHOS::Media::Background