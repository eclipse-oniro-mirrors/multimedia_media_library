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

#include "lcd_download_task.h"

#include "lcd_download_operation.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "sys_utils.h"
#include "cloud_sync_utils.h"
#include "result_set_reader.h"
#include "medialibrary_related_system_state_manager.h"
#include <sstream>
#include "media_analysis_data_service.h"
#include "lcd_aging_manager.h"

namespace OHOS::Media::Background {

using namespace OHOS::Media::CloudSync;

std::mutex LcdDownloadTask::LcdDownloadMutex_;
static constexpr int32_t WAIT_FOR_START_SYNC = 150000;
const int32_t BATCH_SIZE = 50;

const std::string SQL_QUERY_DOWNLOAD_FILE = "\
    WITH PortraitCoverFileId AS ( \
        SELECT DISTINCT CAST(SUBSTR(cover_uri, 20, INSTR(SUBSTR(cover_uri, 20), '/') - 1) AS INTEGER) AS file_id \
        FROM AnalysisAlbum \
        WHERE (album_subtype = 4102 OR album_subtype = 4103) AND cover_uri IS NOT NULL AND cover_uri <> '' \
    ), \
    HighlightCoverFileId AS ( \
        SELECT DISTINCT CAST( \
            SUBSTR( \
                cover_key, \
                INSTR(cover_key, 'file://media/Photo/') + LENGTH('file://media/Photo/'), \
                INSTR(SUBSTR(cover_key, \
                    INSTR(cover_key, 'file://media/Photo/') + LENGTH('file://media/Photo/')), '/') - 1 \
            ) AS INTEGER \
        ) AS file_id \
        FROM tab_highlight_cover_info \
        WHERE cover_key IS NOT NULL AND cover_key <> '' AND cover_key LIKE '%file://media/Photo/%' \
    ), \
    FavoriteHighlightFileId AS ( \
        SELECT DISTINCT map_asset AS file_id \
        FROM AnalysisPhotoMap \
        WHERE map_album IN ( \
            SELECT album_id FROM tab_highlight_album WHERE is_favorite <> 0 AND highlight_status = 1 \
        ) \
    ), \
    FavoriteFileId AS ( \
        SELECT file_id \
        FROM Photos \
        WHERE is_favorite = 1 \
    ) \
    SELECT P.file_id, P.data, P.display_name \
    FROM Photos P \
    INNER JOIN ( \
        SELECT file_id FROM PortraitCoverFileId \
        UNION \
        SELECT file_id FROM HighlightCoverFileId \
        UNION \
        SELECT file_id FROM FavoriteHighlightFileId \
        UNION \
        SELECT file_id FROM FavoriteFileId \
    ) AS T ON T.file_id = P.file_id \
    WHERE P.sync_status = 0 \
      AND P.clean_flag = 0 \
      AND P.time_pending = 0 \
      AND P.is_temp = 0 \
      AND P.thumb_status IN (1, 3) \
      AND P.position IN (2, 3) ";

std::vector<int64_t> LcdDownloadTask::QueryNeedDownloadFiles()
{
    std::vector<int64_t> fileIds;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, fileIds,
        "QueryNeedDownloadFiles rdbStore is nullptr!");

    auto resultSet = rdbStore->QuerySql(SQL_QUERY_DOWNLOAD_FILE);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, fileIds,
        "QueryNeedDownloadFiles Failed to query LCD images!");

    int32_t count = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        count ++;
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        fileIds.push_back(static_cast<int64_t>(fileId));
    }
    MEDIA_INFO_LOG("QueryNeedDownloadFiles: count = %{public}d", count);
    resultSet->Close();
    return fileIds;
}

bool LcdDownloadTask::IsNetworkConditionMet()
{
    return MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnected() ||
           (MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnectedAtRealTime() &&
            CloudSyncUtils::IsUnlimitedTrafficStatusOn());
}

int32_t LcdDownloadTask::HandleLcdDownload()
{
    std::unique_lock<std::mutex> lock(LcdDownloadMutex_, std::defer_lock);
    CHECK_AND_RETURN_RET_LOG(lock.try_lock(), E_OK, "Smart Data LCD download has started, skipping this operation");
    // 执行过主动老化，才进行下载LCD
    bool isActiveLcdAging = LcdAgingManager::GetInstance().GetIsActiveLcdAging();
    CHECK_AND_RETURN_RET_INFO_LOG(isActiveLcdAging, E_OK, "isActiveLcdAging is false");
    if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
        MEDIA_INFO_LOG("TryStartPendingDownload: conditions not met, wait for next time");
        return E_OK;
    }
    if (!IsNetworkConditionMet()) {
        MEDIA_INFO_LOG("TryStartPendingDownload: network conditions not met");
        return E_OK;
    }
    std::vector<int64_t> fileIds = QueryNeedDownloadFiles();
    if (fileIds.empty()) {
        MEDIA_INFO_LOG("TryStartPendingDownload: no lcd need to download");
        return E_OK;
    }
    MEDIA_INFO_LOG("TryStartPendingDownload: check for pending LCD download tasks (%{public}zu files)",
                   fileIds.size());
    std::thread([fileIds]() {
        #ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
            SysUtils::SlowDown();
        #endif
        MEDIA_INFO_LOG("TryStartPendingDownload thread begin");
        this_thread::sleep_for(chrono::milliseconds(WAIT_FOR_START_SYNC));
        uint32_t netBearerBitmap = 0xFFFFFFFF;
        uint32_t BEARER_WIFI = 2;
        if (!CloudSyncUtils::IsUnlimitedTrafficStatusOn()) {
            netBearerBitmap = BEARER_WIFI;
        }
        int32_t totalBatches = (fileIds.size() + BATCH_SIZE - 1) / BATCH_SIZE;
        MEDIA_INFO_LOG("TryStartPendingDownload: total files=%{public}zu, batches=%{public}d",
                       fileIds.size(), totalBatches);
        for (int32_t i = 0; i < totalBatches; ++i) {
            if (!MedialibrarySubscriber::IsCurrentStatusOn() || !IsNetworkConditionMet()) {
                MEDIA_INFO_LOG("TryStartPendingDownload: conditions not met, cancel remaining batches");
                break;
            }
            auto startIt = fileIds.begin() + i * BATCH_SIZE;
            auto endIt = (i == totalBatches - 1) ? fileIds.end() : fileIds.begin() + (i + 1) * BATCH_SIZE;
            std::vector<int64_t> batchFileIds(startIt, endIt);
            LcdDownloadOperation::GetInstance()->StartDownload(batchFileIds, netBearerBitmap);
        }
        MEDIA_INFO_LOG("TryStartPendingDownload thread end");
    }).detach();
    return E_OK;
}
}  // namespace OHOS::Media::Background