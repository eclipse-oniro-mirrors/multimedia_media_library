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

#include "media_music_master_mode_task.h"

#include <memory>
#include <string>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
const std::int32_t MUSIC_MASTER_MODE_SCAN_BATCH_SIZE = 100;
static const int32_t PREFS_NULL_ERR_CODE = -1;
const std::string MUSIC_MASTER_MODE_PROGRESS = "music_master_mode_progress";
const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";

const std::string SQL_PHOTOS_TABLE_QUERY_MUSIC_MASTER_ASSETS =
    "SELECT"
    " file_id,"
    " data,"
    " file_source_type,"
    " storage_path "
    "FROM"
    " Photos "
    "WHERE"
    " music_master_mode = 0"
    " AND sync_status = 0"
    " AND clean_flag = 0"
    " AND time_pending = 0"
    " AND is_temp = 0"
    " AND media_type = " + std::to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) + ""
    " AND (position = 1 OR position = 3)"
    " AND file_id > ?"
    " AND file_id <= ?"
    " ORDER BY file_id ASC ;";

bool MediaMusicMasterModeTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaMusicMasterModeTask::Execute()
{
    this->HandleMusicMasterMode();
}

static int32_t QueryMaxFileId()
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

static std::string GetAssetRealPathForMusicMaster(const MusicMasterAssetInfo &assetInfo)
{
    if (assetInfo.fileSourceType == static_cast<int32_t>(FileSourceType::FILE_MANAGER) ||
        assetInfo.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE)) {
        return assetInfo.storagePath;
    }
    return assetInfo.path;
}

static int32_t ExtractMusicMasterModeFromPath(const std::string &path, int32_t mediaType)
{
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath(path);
    data->SetFileName(MediaFileUtils::GetFileName(path));
    data->SetFileMediaType(mediaType);
    int32_t ret = MetadataExtractor::ExtractAVMetadata(data);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Extract music master metadata failed, path=%{private}s, ret=%{public}d",
            MediaFileUtils::DesensitizePath(path).c_str(), ret);
        return -1;
    }
    return data->GetMusicMasterMode();
}

static int32_t BatchUpdateMusicMasterMode(
    const std::vector<std::pair<int32_t, int32_t>> &updateItems)
{
    if (updateItems.empty()) {
        return E_OK;
    }
    auto rawRdbStore = MediaLibraryRdbStore::GetRaw();
    CHECK_AND_RETURN_RET_LOG(rawRdbStore != nullptr, E_DB_FAIL, "rawRdbStore is nullptr");

    std::map<int32_t, std::vector<int32_t>> modeToFileIds;
    for (const auto &item : updateItems) {
        modeToFileIds[item.second].push_back(item.first);
    }

    const std::string &table = PhotoColumn::PHOTOS_TABLE;
    const std::string &modeCol = PhotoColumn::MUSIC_MASTER_MODE;
    const std::string &idCol = MediaColumn::MEDIA_ID;
    for (const auto &bucket : modeToFileIds) {
        const int32_t targetMode = bucket.first;
        const std::vector<int32_t> &fileIds = bucket.second;
        std::string updateSql = "UPDATE " + table + " SET " + modeCol + " = ? WHERE " + idCol + " IN (";
        std::vector<ValueObject> bindArgs;
        bindArgs.emplace_back(targetMode);
        for (size_t i = 0; i < fileIds.size(); i++) {
            updateSql += (i == 0 ? "?" : ", ?");
            bindArgs.emplace_back(fileIds[i]);
        }
        updateSql += ")";

        auto [errCode, transaction] = rawRdbStore->CreateTransaction(NativeRdb::Transaction::DEFERRED);
        CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK && transaction != nullptr, E_DB_FAIL,
            "Create transaction failed, err=%{public}d", errCode);
        auto res = transaction->Execute(updateSql, bindArgs);
        if (res.first != NativeRdb::E_OK) {
            transaction->Rollback();
            MEDIA_ERR_LOG("Batch update music_master_mode=%{public}d failed, err=%{public}d",
                targetMode, res.first);
            return E_DB_FAIL;
        }
        errCode = transaction->Commit();
        CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, E_DB_FAIL,
            "Commit transaction failed, err=%{public}d", errCode);
    }
    return E_OK;
}

void MediaMusicMasterModeTask::QueryMusicMasterAssets(int32_t startFileId, int32_t maxFileId,
    std::vector<MusicMasterAssetInfo> &assetInfos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, maxFileId};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_MUSIC_MASTER_ASSETS, bindArgs);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "resultSet is null");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return;
    }

    do {
        MusicMasterAssetInfo assetInfo;
        assetInfo.fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        assetInfo.fileSourceType =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet, TYPE_INT32));
        assetInfo.path =
            get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        assetInfo.storagePath =
            get<std::string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_STORAGE_PATH, resultSet, TYPE_STRING));
        assetInfos.push_back(assetInfo);
    } while (MedialibrarySubscriber::IsCurrentStatusOn() &&
        resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
}

void MediaMusicMasterModeTask::HandleMusicMasterAssets(const std::vector<MusicMasterAssetInfo> &assetInfos)
{
    std::vector<std::pair<int32_t, int32_t>> updateItems;
    updateItems.reserve(assetInfos.size());
    for (const MusicMasterAssetInfo &assetInfo : assetInfos) {
        std::string realPath = GetAssetRealPathForMusicMaster(assetInfo);
        if (realPath.empty()) {
            MEDIA_ERR_LOG("GetAssetRealPathForMusicMaster failed, skip file_id=%{public}d", assetInfo.fileId);
            continue;
        }
        int32_t musicMasterMode =
            ExtractMusicMasterModeFromPath(realPath, MediaType::MEDIA_TYPE_VIDEO);
        if (musicMasterMode < 0) {
            MEDIA_ERR_LOG("ExtractMusicMasterModeFromPath failed, skip file_id=%{public}d", assetInfo.fileId);
            continue;
        }
        updateItems.emplace_back(assetInfo.fileId, musicMasterMode);
    }
    int32_t ret = BatchUpdateMusicMasterMode(updateItems);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BatchUpdateMusicMasterMode failed, ret=%{public}d", ret);
    }
}

int32_t MediaMusicMasterModeTask::GetBatchStatus()
{
    MEDIA_INFO_LOG("MediaMusicMasterModeTask::GetBatchStatus start");
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, PREFS_NULL_ERR_CODE, "get preferences error: %{public}d", errCode);
    int32_t defaultVal = 0;
    int32_t currStartFileId = prefs->GetInt(MUSIC_MASTER_MODE_PROGRESS, defaultVal);
    MEDIA_INFO_LOG("currStartFileId is %{public}d", currStartFileId);
    return currStartFileId;
}

void MediaMusicMasterModeTask::SetBatchStatus(int32_t startFileId)
{
    MEDIA_INFO_LOG("MediaMusicMasterModeTask::SetBatchStatus start");
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutInt(MUSIC_MASTER_MODE_PROGRESS, startFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("startFileId set to: %{public}d", startFileId);
}

void MediaMusicMasterModeTask::HandleMusicMasterMode()
{
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t maxFileId = QueryMaxFileId();
    MEDIA_INFO_LOG("MediaMusicMasterModeTask start. maxFileId: %{public}d", maxFileId);
    CHECK_AND_RETURN_LOG(maxFileId > 0, "query max file id failed");

    int32_t curFileId = GetBatchStatus();
    while (curFileId < maxFileId && MedialibrarySubscriber::IsCurrentStatusOn()) {
        int32_t endId = std::min(curFileId + MUSIC_MASTER_MODE_SCAN_BATCH_SIZE, maxFileId);
        std::vector<MusicMasterAssetInfo> assetInfos;
        QueryMusicMasterAssets(curFileId, endId, assetInfos);
        HandleMusicMasterAssets(assetInfos);
        if (!assetInfos.empty()) {
            curFileId = assetInfos.back().fileId;
        } else {
            MEDIA_WARN_LOG("No valid music master asset in current range. curFileId: %{public}d, endId: %{public}d",
                curFileId, endId);
            curFileId = endId;
        }
        if (!MedialibrarySubscriber::IsCurrentStatusOn()) {
            MEDIA_INFO_LOG("Music master mode status off, save progress and stop. curFileId: %{public}d", curFileId);
            SetBatchStatus(curFileId);
            return;
        }
    }
    SetBatchStatus(curFileId);
    MEDIA_INFO_LOG("MediaMusicMasterModeTask end. curFileId: %{public}d, cost: %{public}" PRId64,
        curFileId, MediaFileUtils::UTCTimeMilliSeconds() - startTime);
}
}  // namespace OHOS::Media::Background
