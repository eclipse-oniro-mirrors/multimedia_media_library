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

#define MLOG_TAG "MediaLibraryRdbOperations"

#include "medialibrary_rdb_operations.h"

#include <sys/stat.h>

#include "dfx_utils.h"
#include "media_edit_utils.h"
#include "media_file_access_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "media_time_utils.h"
#include "medialibrary_restore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_unistore_manager.h"
#include "moving_photo_file_utils.h"
#include "rdb_sql_utils.h"
#include "result_set_utils.h"

using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
constexpr ssize_t RDB_CHECK_WAL_SIZE = 50 * 1024 * 1024;   /* check wal file size : 50MB */

std::mutex MediaLibraryRdbOperations::walCheckPointMutex_;

// LCOV_EXCL_START
std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbOperations::GetIndexOfUri(const AbsRdbPredicates &predicates,
    const std::vector<std::string> &columns, const std::string &id)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetIndexOfUri");
    std::string sql;
    sql.append("SELECT ").append(CONST_PHOTO_INDEX).append(" From (");
    sql.append(RdbSqlUtils::BuildQueryString(predicates, columns));
    sql.append(") where "+ MediaColumn::MEDIA_ID + " = ").append(id);
    MEDIA_DEBUG_LOG("sql = %{private}s", sql.c_str());
    const std::vector<std::string> &args = predicates.GetWhereArgs();
    for (const auto &arg : args) {
        MEDIA_DEBUG_LOG("arg = %{private}s", arg.c_str());
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "rdbStore is nullptr");

    auto resultSet = rdbStore->QuerySql(sql, args);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    return resultSet;
}

std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbOperations::GetIndexOfUriForPhotos(
    const AbsRdbPredicates &predicates, const std::vector<std::string> &columns, const std::string &id)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetIndexOfUriForPhotos");
    std::string sql;
    sql.append(RdbSqlUtils::BuildQueryString(predicates, columns));
    MEDIA_DEBUG_LOG("sql = %{private}s", sql.c_str());
    const std::vector<std::string> &args = predicates.GetWhereArgs();
    for (const auto &arg : args) {
        MEDIA_DEBUG_LOG("arg = %{private}s", arg.c_str());
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "rdbStore is nullptr");

    auto resultSet = rdbStore->QuerySql(sql, args);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    return resultSet;
}
// LCOV_EXCL_STOP

int32_t MediaLibraryRdbOperations::UpdateLastVisitTime(const std::string &id)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLastVisitTime");
    ValuesBucket values;
    int32_t changedRows = 0;
    values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaTimeUtils::UTCTimeMilliSeconds());
    std::string whereClause = MediaColumn::MEDIA_ID + " = ?";
    std::vector<std::string> whereArgs = {id};

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is nullptr");

    int32_t ret = rdbStore->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    if (ret != NativeRdb::E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("rdbStore_->UpdateLastVisitTime failed, changedRows = %{public}d, ret = %{public}d",
            changedRows, ret);
        MediaLibraryRestore::GetInstance().CheckRestore(ret);
    }
    return changedRows;
}

int32_t MediaLibraryRdbOperations::QueryPragma(const std::string &key, int64_t &value)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    std::shared_ptr<ResultSet> resultSet = rdbStore->QuerySql("PRAGMA " + key);
    MediaLibraryRestore::GetInstance().CheckResultSet(resultSet);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("rdbStore_->QuerySql failed");
        return E_HAS_DB_ERROR;
    }
    resultSet->GetLong(0, value);
    resultSet->Close();
    return E_OK;
}

// LCOV_EXCL_START
std::shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbOperations::QueryEditDataExists(
    const NativeRdb::AbsRdbPredicates &predicates)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr,
        "rdbStore_ is nullptr. Maybe it didn't init successfully.");

    std::vector<std::string> columns = { MediaColumn::MEDIA_FILE_PATH };
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query edit data err");
        return nullptr;
    }

    std::string photoPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    if (MediaPureFileUtils::IsFileExists(MediaEditUtils::GetEditDataPath(photoPath)) ||
        MediaPureFileUtils::IsFileExists(MediaEditUtils::GetEditDataCameraPath(photoPath))) {
        return rdbStore->QuerySql("SELECT 1 AS hasEditData");
    }
    return rdbStore->QuerySql("SELECT 0 AS hasEditData");
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryRdbOperations::QueryMovingPhotoVideoReady(
    const NativeRdb::AbsRdbPredicates &predicates)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr,
        "Pointer rdbStore_ is nullptr. Maybe it didn't init successfully.");

    std::vector<std::string> columns = { MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_POSITION };
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, resultSet, "query moving photo video ready err");

    std::string photoPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);

    std::string realPath = MediaFileAccessUtils::GetAssetRealPath(photoPath);
    if (position != static_cast<int32_t>(PhotoPositionType::CLOUD) && !realPath.empty() &&
        MovingPhotoFileUtils::IsLivePhotoAsset(realPath)) {
        MEDIA_DEBUG_LOG("photoPath:%{public}s is livePhoto, video is ready",
            DfxUtils::GetSafePath(photoPath).c_str());
        return rdbStore->QuerySql("SELECT 1 AS movingPhotoVideoReady");
    }
    size_t fileSize;
    auto videoPath = MediaFileUtils::GetMovingPhotoVideoPath(photoPath);
    cond = MediaFileUtils::GetFileSize(videoPath, fileSize) && (fileSize > 0);
    MEDIA_DEBUG_LOG("photoPath:%{public}s, videoPath:%{public}s, video size:%zu",
        DfxUtils::GetSafePath(photoPath).c_str(), DfxUtils::GetSafePath(videoPath).c_str(), fileSize);
    CHECK_AND_RETURN_RET(!cond, rdbStore->QuerySql("SELECT 1 AS movingPhotoVideoReady"));
    return rdbStore->QuerySql("SELECT 0 AS movingPhotoVideoReady");
}

void MediaLibraryRdbOperations::WalCheckPoint()
{
    std::unique_lock<std::mutex> lock(walCheckPointMutex_, std::defer_lock);
    if (!lock.try_lock()) {
        MEDIA_WARN_LOG("wal_checkpoint in progress, skip this operation");
        return;
    }

    struct stat fileStat;
    const std::string walFile = std::string(CONST_MEDIA_DB_DIR) + "/rdb/media_library.db-wal";
    if (stat(walFile.c_str(), &fileStat) < 0) {
        CHECK_AND_PRINT_LOG(errno == ENOENT, "wal_checkpoint stat failed, errno: %{public}d", errno);
        return;
    }
    ssize_t size = fileStat.st_size;
    if (size < 0) {
        MEDIA_ERR_LOG("Invalid size for wal_checkpoint, size: %{public}zd", size);
        return;
    }
    if (size <= RDB_CHECK_WAL_SIZE) {
        return;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "wal_checkpoint rdbStore is nullptr!");

    auto errCode = rdbStore->ExecuteSql("PRAGMA wal_checkpoint(TRUNCATE)");
    CHECK_AND_PRINT_LOG(errCode == NativeRdb::E_OK, "wal_checkpoint ExecuteSql failed, errCode: %{public}d", errCode);
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS