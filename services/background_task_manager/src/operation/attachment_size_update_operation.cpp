/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "attachment_size_update_operation.h"

#include "map_operation_flag.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_type_const.h"
#include "moving_photo_file_utils.h"
#include "photo_album_column.h"
#include "media_column.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "values_bucket.h"
#include "medialibrary_photo_operations.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
const std::string ORIGIN_ATTACHMENT_SIZE_ASSETS_NUMBER = "origin_attachment_size_assets_number";
const std::int32_t ATTACHMENT_SIZE_SCAN_BATCH_SIZE = 200;
const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";

const std::string SQL_PHOTOS_TABLE_QUERY_ATTACHMENT_ASSETS = "SELECT"
                                                             " file_id,"
                                                             " data,"
                                                             " attachment_size "
                                                             "FROM"
                                                             " Photos "
                                                             "WHERE"
                                                             " attachment_size = 0"
                                                             " AND sync_status = 0"
                                                             " AND clean_flag = 0"
                                                             " AND time_pending = 0"
                                                             " AND is_temp = 0"
                                                             " AND (position = 1 OR position = 3)"
                                                             " AND file_id > ?"
                                                             " AND file_id <= ? ;";

std::atomic<bool> AttachmentSizeUpdateOperation::isContinue_{true};

void AttachmentSizeUpdateOperation::Stop()
{
    isContinue_.store(false);
}

static int QueryMaxFileId()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "get rdb store failed");
    string queryMaxSql = "SELECT Max(file_id) FROM " + PhotoColumn::PHOTOS_TABLE;
    auto resultSet = rdbStore->QuerySql(queryMaxSql);
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), E_ERR, "Query max file_id failed");
    int32_t maxFileId = -1;
    maxFileId = GetInt32Val("Max(file_id)", resultSet);
    resultSet->Close();
    return maxFileId;
}

static int32_t UpdateSingleAttachmentSizeIfNeeded(int32_t fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "rdbStore is nullptr");

    if (!MAP_OPERATION_FLAG) {
        return E_OK;
    }

    std::string filePath;
    int32_t ret = MediaLibraryPhotoOperations::GetFilePathById(rdbStore, std::to_string(fileId), filePath);
    if (ret != E_OK) {
        MEDIA_WARN_LOG("Skip invalid file ID: %{public}d (error code: %{public}d)", fileId, ret);
        return ret;
    }

    uint64_t editDataSize = 0;
    uint64_t attachmentSize = 0;
    MediaLibraryRdbStore::StatEditAndAttachmentSize(filePath, editDataSize, attachmentSize);
    if (attachmentSize == 0) {
        MEDIA_DEBUG_LOG("No attachment size for file ID: %{public}d, skip update", fileId);
        return E_OK;
    }

    return MediaLibraryRdbStore::UpdateAttachmentSize(rdbStore, std::to_string(fileId), attachmentSize);
}

void AttachmentSizeUpdateOperation::UpdateAttachmentSize()
{
    isContinue_.store(true);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t errCode = E_OK;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs, "get preferences error: %{public}d", errCode);
    int32_t curFileId = prefs->GetInt(ORIGIN_ATTACHMENT_SIZE_ASSETS_NUMBER, 0);
    int maxFileId = QueryMaxFileId();
    MEDIA_INFO_LOG("UpdateAttachmentSize start. "
        "curFileId: %{public}d, maxFileId: %{public}d", curFileId, maxFileId);
    CHECK_AND_RETURN_LOG(maxFileId > 0, "query max file id failed");
    while (curFileId < maxFileId && MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load()) {
        int32_t endId = std::min(curFileId + ATTACHMENT_SIZE_SCAN_BATCH_SIZE, maxFileId);
        int32_t batchSize = -1;
        std::vector<AttachmentSizeAssetInfo> assetInfos = QueryAttachmentSizeAssets(curFileId, endId, batchSize);
        HandleAttachmentSizeAssets(assetInfos);
        if (batchSize > 0) {
            curFileId = batchSize;
        } else {
            MEDIA_WARN_LOG("No valid batch progress in current range. curFileId: %{public}d, endId: %{public}d, "
                "batchSize: %{public}d", curFileId, endId, batchSize);
            curFileId = endId;
        }
    }
    prefs->PutInt(ORIGIN_ATTACHMENT_SIZE_ASSETS_NUMBER, curFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG(
        "end handle attachment size! curFileId: %{public}d, cost: %{public}" PRId64,
        curFileId, MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return;
}

std::vector<AttachmentSizeAssetInfo> AttachmentSizeUpdateOperation::QueryAttachmentSizeAssets(int32_t startFileId,
    int32_t maxFileId, int32_t &batchSize)
{
    std::vector<AttachmentSizeAssetInfo> assetInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, assetInfos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, maxFileId};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_ATTACHMENT_ASSETS, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, assetInfos, "resultSet is null");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return assetInfos;
    }

    do {
        AttachmentSizeAssetInfo assetInfo;
        assetInfo.path =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        assetInfo.fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        assetInfo.attachmentSize =
            get<int64_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::ATTACHMENT_SIZE, resultSet, TYPE_INT64));
        assetInfos.push_back(assetInfo);
        batchSize = max(batchSize, assetInfo.fileId);
    } while (MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load() &&
        resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return assetInfos;
}

void AttachmentSizeUpdateOperation::HandleAttachmentSizeAssets(const std::vector<AttachmentSizeAssetInfo> &assetInfos)
{
    for (const AttachmentSizeAssetInfo &assetInfo : assetInfos) {
        if (assetInfo.attachmentSize > 0) {
            continue;
        }
        int32_t ret = UpdateSingleAttachmentSizeIfNeeded(assetInfo.fileId);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("UpdateSingleAttachmentSizeIfNeeded failed ID: %{public}d (ret code: %{public}d)",
                assetInfo.fileId, ret);
        }
    }
}
}  // namespace OHOS::Media
