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

#define MLOG_TAG "RomLowSpaceGuard"

#include "rom_low_space_guard.h"

#include <sys/statvfs.h>

#include <cerrno>

#include "backup_const.h"
#include "backup_const_column.h"
#include "backup_database_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "vision_column.h"

namespace OHOS {
namespace Media {
std::atomic<RomCheckMode> RomLowSpaceGuard::mode_{RomCheckMode::NORMAL};
std::mutex AnalysisDataDropper::dropBufferMutex_;
std::unordered_map<std::string, std::vector<int32_t>> AnalysisDataDropper::dropBuffers_;

void RomLowSpaceGuard::Reset()
{
    mode_.store(RomCheckMode::NORMAL);
}

int64_t RomLowSpaceGuard::GetAvailableBytes(const std::string &path)
{
    if (path.empty()) {
        MEDIA_ERR_LOG("Path is empty");
        return -1;
    }
    struct statvfs statInfo;
    if (statvfs(path.c_str(), &statInfo) != 0) {
        MEDIA_ERR_LOG("Statvfs failed, path: %{public}s, errno: %{public}d",
            path.c_str(), errno);
        return -1;
    }
    return static_cast<int64_t>(statInfo.f_bsize) * static_cast<int64_t>(statInfo.f_bfree);
}

RomCheckMode RomLowSpaceGuard::EvaluateCheckpoint(const std::string &checkPath)
{
    // already latched: idempotent, no more polling
    if (mode_.load() == RomCheckMode::DROP_LATCHED) {
        return RomCheckMode::DROP_LATCHED;
    }
    int64_t avail = GetAvailableBytes(checkPath);
    if (avail < 0) {
        // statvfs failure: keep current mode rather than blocking restore
        MEDIA_WARN_LOG("EvaluateCheckpoint: get available bytes failed, keep mode %{public}d",
            static_cast<int32_t>(mode_.load()));
        return mode_.load();
    }
    if (avail < ROM_LOW_SPACE_THRESHOLD) {
        mode_.store(RomCheckMode::DROP_LATCHED); // avail < 3G: fuse-then-drop directly
    } else {
        mode_.store(RomCheckMode::MONITORING);   // avail >= 3G: fuse only + per-batch ROM check
    }
    MEDIA_INFO_LOG("EvaluateCheckpoint: avail=%{public}lld, mode=%{public}d",
        static_cast<long long>(avail), static_cast<int32_t>(mode_.load()));
    return mode_.load();
}

RomCheckMode RomLowSpaceGuard::GetMode()
{
    return mode_.load();
}

bool AnalysisDataDropper::IsDropTable(const std::string &table)
{
    for (const auto &dropTable : ROM_DROP_ANALYSIS_TABLES) {
        if (dropTable == table) {
            return true;
        }
    }
    return false;
}

std::string AnalysisDataDropper::BuildFileIdClause(const std::vector<int32_t> &fileIds)
{
    std::string clause = "(";
    bool isFirst = true;
    for (int32_t fileId : fileIds) {
        if (!isFirst) {
            clause += ",";
        }
        clause += std::to_string(fileId);
        isFirst = false;
    }
    clause += ")";
    return clause;
}

int32_t AnalysisDataDropper::DropBatchRows(const std::shared_ptr<NativeRdb::RdbStore> &sourceRdb,
    const std::string &table, const std::vector<int32_t> &fileIds)
{
    if (sourceRdb == nullptr || table.empty() || fileIds.empty()) {
        MEDIA_WARN_LOG("DropBatchRows: invalid args, table: %{public}s, fileId count: %{public}zu",
            table.c_str(), fileIds.size());
        return E_ERR;
    }
    std::string fileIdClause = BuildFileIdClause(fileIds);
    std::string deleteSql = "DELETE FROM " + table + " WHERE " + IMAGE_FACE_COL_FILE_ID + " IN " + fileIdClause;
    int32_t errCode = BackupDatabaseUtils::ExecuteSQL(sourceRdb, deleteSql);
    CHECK_AND_PRINT_LOG(errCode >= 0, "DropBatchRows: table: %{public}s, count: %{public}zu, ret: %{public}d",
        table.c_str(), fileIds.size(), errCode);
    return errCode >= 0 ? E_OK : errCode;
}
int32_t AnalysisDataDropper::DropRowsBuffered(const std::shared_ptr<NativeRdb::RdbStore> &sourceRdb,
    const std::string &table, const std::vector<int32_t> &fileIds)
{
    if (sourceRdb == nullptr || table.empty()) {
        MEDIA_WARN_LOG("DropRowsBuffered: invalid args, table: %{public}s, fileId count: %{public}zu",
            table.c_str(), fileIds.size());
        return E_ERR;
    }
    std::vector<int32_t> toDrop;
    {
        std::lock_guard<std::mutex> lock(dropBufferMutex_);
        std::vector<int32_t> &buffer = dropBuffers_[table];
        buffer.insert(buffer.end(), fileIds.begin(), fileIds.end());
        if (static_cast<int32_t>(buffer.size()) < ROM_DROP_ANALYSIS_BATCH_ROWS) {
            return E_OK;
        }
        toDrop.swap(buffer);
    }
    return DropBatchRows(sourceRdb, table, toDrop);
}

int32_t AnalysisDataDropper::FlushRows(const std::shared_ptr<NativeRdb::RdbStore> &sourceRdb,
    const std::string &table)
{
    std::vector<int32_t> toDrop;
    {
        std::lock_guard<std::mutex> lock(dropBufferMutex_);
        auto it = dropBuffers_.find(table);
        if (it == dropBuffers_.end() || it->second.empty()) {
            return E_OK;
        }
        toDrop.swap(it->second);
        dropBuffers_.erase(it);
    }
    return DropBatchRows(sourceRdb, table, toDrop);
}

void AnalysisDataDropper::ResetBuffers()
{
    std::lock_guard<std::mutex> lock(dropBufferMutex_);
    dropBuffers_.clear();
}
} // namespace Media
} // namespace OHOS
