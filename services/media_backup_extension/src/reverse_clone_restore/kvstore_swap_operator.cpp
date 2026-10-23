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

#include "reverse_clone_restore/kvstore_swap_operator.h"

#include "clone_restore.h"

#include <chrono>
#include <thread>
#include <fstream>

#include "backup_const.h"
#include "base_restore.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "settings_data_manager.h"
#include "reverse_clone_restore_marker.h"

namespace OHOS {
namespace Media {
static constexpr int64_t POLL_INTERVAL_MS = 100;
static constexpr int64_t POLL_TIMEOUT_MS = 150000;

static constexpr int32_t KVDB_CLOSE = 5;
static constexpr int32_t KVDB_OPEN = 6;

// year + month, the number of KVDB stores to match for swap
static constexpr size_t EXPECTED_HASH_PAIR_COUNT = 2;

static const std::string NEW_MACHINE_YEAR_HASH = "d1fceec62f61bcc5d3b48e9b01c80384ba4ab9c91eb68b13a573ccea03803e43";
static const std::string NEW_MACHINE_MONTH_HASH = "3ddb6fb8b2fcb38d2f431e86bfb806dab771637860d6e86bb9430fa15df04248";
static const std::string OLD_MACHINE_YEAR_HASH = "5f796bfa83fcd52d829a2304cb7194119a6e6e84b8efdde115b26fbd424b8541";
static const std::string OLD_MACHINE_MONTH_HASH = "1bd52423fd08d9b97e0c93366e3f9747b17e5dc321a3f0e328273cbd3dca611a";

bool KvStoreSwapOperator::NotifyMainProcessClose()
{
    failedStep_ = KvStoreSwapStep::NOTIFY_MAIN_CLOSE;
    BaseRestore::NotifyKvDbStatusForClone(KVDB_CLOSE);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(POLL_TIMEOUT_MS);
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
        std::string value;
        int32_t ret = SettingsDataManager::GetKvDbCloseStatus(value);
        const std::string CLOSE_READY = "0";
        if (ret == E_OK && value == CLOSE_READY) {
            MEDIA_INFO_LOG("NotifyMainProcessClose success, kvdb closed");
            return true;
        }
    }
    MEDIA_ERR_LOG("NotifyMainProcessClose timeout");
    return false;
}

bool KvStoreSwapOperator::NotifyMainProcessReopen()
{
    failedStep_ = KvStoreSwapStep::NOTIFY_MAIN_REOPEN;
    BaseRestore::NotifyKvDbStatusForClone(KVDB_OPEN);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(POLL_TIMEOUT_MS);
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
        std::string value;
        int32_t ret = SettingsDataManager::GetKvDbCloseStatus(value);
        const std::string REOPEN_READY = "1";
        if (ret == E_OK && value == REOPEN_READY) {
            MEDIA_INFO_LOG("NotifyMainProcessReopen success, kvdb reopened");
            return true;
        }
    }
    MEDIA_ERR_LOG("NotifyMainProcessReopen timeout");
    return false;
}

bool KvStoreSwapOperator::InitBaseDirs(const std::string &backupRestorePath)
{
    oldBaseDir_ = backupRestorePath + CLONE_KVDB_BACKUP_DIR + "/kvdb";
    newBaseDir_ = MEDIA_KVDB_DIR + "/kvdb";
    tempBaseDir_ = REVERSE_KVDB_TEMP_DIR + "/kvdb";
    MEDIA_INFO_LOG("InitBaseDirs old=%{public}s, new=%{public}s, temp=%{public}s",
        MediaFileUtils::DesensitizePath(oldBaseDir_).c_str(),
        MediaFileUtils::DesensitizePath(newBaseDir_).c_str(),
        MediaFileUtils::DesensitizePath(tempBaseDir_).c_str());
    return true;
}

bool KvStoreSwapOperator::MatchHashDirectories()
{
    std::vector<std::string> oldDirs;
    std::vector<std::string> newDirs;
    MediaFileUtils::GetFolderListUnderPath(oldBaseDir_, oldDirs);
    MediaFileUtils::GetFolderListUnderPath(newBaseDir_, newDirs);

    if (oldDirs.empty() || newDirs.empty()) {
        MEDIA_ERR_LOG("MatchHashDirectories failed, oldDirs or newDirs is empty");
        return false;
    }
    if (MatchHashDirectoriesByEqualSet(oldDirs, newDirs)) {
        return true;
    }
    if (MatchHashDirectoriesByKnownHashes(oldDirs, newDirs)) {
        return true;
    }
    MEDIA_WARN_LOG("MatchHashDirectories failed, hash directories not matched");
    hashPairs_.clear();
    
    return false;
}

bool KvStoreSwapOperator::MatchHashDirectoriesByEqualSet(const std::vector<std::string> &oldDirs,
    const std::vector<std::string> &newDirs)
{
    if (oldDirs.size() != EXPECTED_HASH_PAIR_COUNT || newDirs.size() != EXPECTED_HASH_PAIR_COUNT) {
        return false;
    }
    std::string oldYearHash = oldDirs[0];
    std::string oldMonthHash = oldDirs[1];
    std::string newYearHash = newDirs[0];
    std::string newMonthHash = newDirs[1];

    if (oldYearHash == newYearHash && oldMonthHash == newMonthHash) {
        hashPairs_.push_back({oldYearHash, newYearHash});
        hashPairs_.push_back({oldMonthHash, newMonthHash});
        return true;
    }
    if (oldYearHash == newMonthHash && oldMonthHash == newYearHash) {
        hashPairs_.push_back({oldYearHash, newMonthHash});
        hashPairs_.push_back({oldMonthHash, newYearHash});
        return true;
    }
    return false;
}

bool KvStoreSwapOperator::MatchHashDirectoriesByKnownHashes(const std::vector<std::string> &oldDirs,
    const std::vector<std::string> &newDirs)
{
    std::string oldYearHash;
    std::string oldMonthHash;
    std::string newYearHash;
    std::string newMonthHash;
    for (const auto &dir : oldDirs) {
        if (dir == OLD_MACHINE_YEAR_HASH) {
            oldYearHash = dir;
        } else if (dir == OLD_MACHINE_MONTH_HASH) {
            oldMonthHash = dir;
        }
    }
    for (const auto &dir : newDirs) {
        if (dir == NEW_MACHINE_YEAR_HASH) {
            newYearHash = dir;
        } else if (dir == NEW_MACHINE_MONTH_HASH) {
            newMonthHash = dir;
        }
    }
    if (!oldYearHash.empty() && !newYearHash.empty()) {
        hashPairs_.push_back({oldYearHash, newYearHash});
    }
    if (!oldMonthHash.empty() && !newMonthHash.empty()) {
        hashPairs_.push_back({oldMonthHash, newMonthHash});
    }

    if (hashPairs_.size() != EXPECTED_HASH_PAIR_COUNT) {
        return false;
    }
    return true;
}

bool KvStoreSwapOperator::RenameNewKvDbToTemp()
{
    failedStep_ = KvStoreSwapStep::RENAME_NEW_TO_TEMP;

    if (!MediaFileUtils::DeleteDir(tempBaseDir_) && !MediaPureFileUtils::IsFileExists(tempBaseDir_)) {
        MEDIA_WARN_LOG("DeleteDir tempBaseDir failed or not exist, continue");
    }

    if (!MediaFileUtils::CreateDirectory(tempBaseDir_)) {
        MEDIA_ERR_LOG("Failed to create tempBaseDir_: %{public}s",
            MediaFileUtils::DesensitizePath(tempBaseDir_).c_str());
        return false;
    }
    for (const auto &pair : hashPairs_) {
        std::string srcSingleVer = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string dstSingleVer = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        if (!MoveKvDbDirectory(srcSingleVer, dstSingleVer)) {
            MEDIA_ERR_LOG("RenameNewKvDbToTemp failed for pair (old=%{public}s, new=%{public}s)",
                pair.oldHash.c_str(), pair.newHash.c_str());
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::MoveOldKvDbToMaster()
{
    failedStep_ = KvStoreSwapStep::MOVE_OLD_TO_MASTER;

    for (const auto &pair : hashPairs_) {
        std::string srcSingleVer = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string dstSingleVer = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        if (!MoveKvDbDirectory(srcSingleVer, dstSingleVer)) {
            MEDIA_ERR_LOG("MoveOldKvDbToMaster move kvdb directory failed, src=%{public}s, dst=%{public}s",
                MediaFileUtils::DesensitizePath(srcSingleVer).c_str(),
                MediaFileUtils::DesensitizePath(dstSingleVer).c_str());
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::RestoreKvDbFromTempToMaster()
{
    for (const auto &pair : hashPairs_) {
        std::string srcSingleVer = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string dstSingleVer = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string oldSingleVer = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        if (MediaPureFileUtils::IsFileExists(dstSingleVer) &&
            !MediaPureFileUtils::IsFileExists(oldSingleVer)) {
            if (!MoveKvDbDirectory(dstSingleVer, oldSingleVer)) {
                MEDIA_ERR_LOG("RestoreKvDbFromTempToMaster move master old kvdb back to backup failed "
                    "(old=%{public}s, new=%{public}s)", pair.oldHash.c_str(), pair.newHash.c_str());
                return false;
            }
        }
        if (!MoveKvDbDirectory(srcSingleVer, dstSingleVer)) {
            MEDIA_ERR_LOG("RestoreKvDbFromTempToMaster failed for pair (old=%{public}s, new=%{public}s)",
                pair.oldHash.c_str(), pair.newHash.c_str());
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::MoveKvDbDirectory(const std::string &srcPath, const std::string &dstPath)
{
    MEDIA_INFO_LOG("MoveKvDbDirectory: %{public}s -> %{public}s",
        MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(dstPath).c_str());
    if (!MediaPureFileUtils::IsFileExists(srcPath)) {
        MEDIA_ERR_LOG("MoveKvDbDirectory src not exist, src=%{public}s",
            MediaFileUtils::DesensitizePath(srcPath).c_str());
        return false;
    }
    size_t lastSlash = dstPath.find_last_of('/');
    std::string dstParentDir = (lastSlash != std::string::npos) ? dstPath.substr(0, lastSlash) : "";
    if (!dstParentDir.empty() && !MediaFileUtils::CreateDirectory(dstParentDir)) {
        MEDIA_ERR_LOG("MoveKvDbDirectory: failed to create destination parent dir: %{public}s",
            MediaFileUtils::DesensitizePath(dstParentDir).c_str());
        return false;
    }

    if (MediaFileUtils::RenameDir(srcPath, dstPath)) {
        MEDIA_INFO_LOG("MoveKvDbDirectory: rename succeeded");
        return true;
    }
    if (MediaPureFileUtils::IsFileExists(dstPath)) {
        if (!MediaFileUtils::DeleteFileOrFolder(dstPath, false)) {
            MEDIA_ERR_LOG("MoveKvDbDirectory delete existing dst failed, dst=%{public}s",
                MediaFileUtils::DesensitizePath(dstPath).c_str());
            return false;
        }
    }
    if (MediaFileUtils::CopyDirectory(srcPath, dstPath) != E_OK) {
        MEDIA_ERR_LOG("MoveKvDbDirectory copy failed");
        return false;
    }
    if (!MediaFileUtils::DeleteFileOrFolder(srcPath, false)) {
        MEDIA_ERR_LOG("MoveKvDbDirectory delete src after copy failed, src=%{public}s",
            MediaFileUtils::DesensitizePath(srcPath).c_str());
        return false;
    }
    return true;
}

bool KvStoreSwapOperator::RestoreKvDbToInitialPosition()
{
    for (const auto &pair : hashPairs_) {
        std::string currentOldPath = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string oldTargetPath = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        if (!MoveKvDbDirectory(currentOldPath, oldTargetPath)) {
            MEDIA_ERR_LOG("RestoreKvDbToInitialPosition restore old failed, (old=%{public}s, new=%{public}s)",
                pair.oldHash.c_str(), pair.newHash.c_str());
            return false;
        }

        std::string currentNewPath = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string newTargetPath = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        if (!MoveKvDbDirectory(currentNewPath, newTargetPath)) {
            MEDIA_ERR_LOG("RestoreKvDbToInitialPosition restore new failed, (old=%{public}s, new=%{public}s)",
                pair.oldHash.c_str(), pair.newHash.c_str());
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::WriteSwapTimestamp()
{
    failedStep_ = KvStoreSwapStep::WRITE_MARKER;
    CHECK_AND_RETURN_RET_LOG(ReverseCloneRestoreMarker::Recreate(ReverseCloneMarkerType::KVDB), false,
        "kvStoreSwapOperator: Recreate kvdb reverse clone restore marker failed");
    return true;
}

bool KvStoreSwapOperator::Swap(const std::string &backupRestorePath)
{
    MEDIA_INFO_LOG("KvStoreSwapOperator Swap start, backupRestorePath: %{public}s",
        MediaFileUtils::DesensitizePath(backupRestorePath).c_str());

    InitBaseDirs(backupRestorePath);

    if (!MatchHashDirectories()) {
        MEDIA_ERR_LOG("KvStoreSwapOperator, MatchHashDirectories failed");
        return false;
    }
    if (!NotifyMainProcessClose()) {
        MEDIA_ERR_LOG("KvStoreSwapOperator, NotifyMainProcessClose failed");
        return false;
    }
    if (!RenameNewKvDbToTemp()) {
        MEDIA_ERR_LOG("KvStoreSwapOperator, RenameNewKvDbToTemp failed");
        Rollback();
        return false;
    }
    if (!MoveOldKvDbToMaster()) {
        MEDIA_ERR_LOG("KvStoreSwapOperator, MoveOldKvDbToMaster failed");
        Rollback();
        return false;
    }
    if (!NotifyMainProcessReopen()) {
        MEDIA_WARN_LOG("KvStoreSwapOperator, NotifyMainProcessReopen failed");
    }
    if (!WriteSwapTimestamp()) {
        MEDIA_WARN_LOG("KvStoreSwapOperator, WriteSwapTimestamp failed, non-fatal");
    }
    MEDIA_INFO_LOG("KvStoreSwapOperator::Swap success");
    failedStep_ = KvStoreSwapStep::NONE;
    return true;
}

bool KvStoreSwapOperator::RestoreOriginalPositions()
{
    MEDIA_INFO_LOG("KvStoreSwapOperator::RestoreOriginalPositions start");
    // 还原新旧库到初始位置：master位置的旧库回备份目录，temp位置的新库回master目录
    bool ret = RestoreKvDbToInitialPosition();
    if (!NotifyMainProcessReopen()) {
        MEDIA_WARN_LOG("KvStoreSwapOperator::RestoreOriginalPositions NotifyMainProcessReopen failed");
    }
    // 目录已动，通知主进程重开句柄；失败仅告警（主进程下次打开时自然恢复）
    CleanupTempDir();
    MEDIA_INFO_LOG("KvStoreSwapOperator::RestoreOriginalPositions finished, ret=%{public}d", ret);
    return ret;
}

bool KvStoreSwapOperator::RestoreOriginalPositionsForResume(const std::string &backupRestorePath)
{
    MEDIA_INFO_LOG("KvStoreSwapOperator::RestoreOriginalPositionsForResume start");
    InitBaseDirs(backupRestorePath);
    hashPairs_.clear();
    hashPairs_.push_back({OLD_MACHINE_YEAR_HASH, NEW_MACHINE_YEAR_HASH});
    hashPairs_.push_back({OLD_MACHINE_MONTH_HASH, NEW_MACHINE_MONTH_HASH});

    // 对 year/month 各自按目录状态分 3 种场景处理（不通知主进程重开句柄）：
    // 场景1: master 有数据 且 old 有数据 ->  不处理（master 即新机库，temp 若有数据视为残留）
    // 场景2: master 空 且 temp 有数据 -> temp 新机库移动到 master 位置（保护新机库）
    // 场景3: master 有数据 且 old空 且 temp 有数据 -> master 旧机库搬回 old， temp 新机库搬回 master
    // 其余 -> 不处理 （无数据可还原， 靠 createIfMissing 重建空库）
    bool ret = true;
    for (const auto &pair : hashPairs_) {
        std::string masterPath = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string oldPath = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string tempPath = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        bool masterHas = MediaPureFileUtils::IsFileExists(masterPath);
        bool oldHas = MediaPureFileUtils::IsFileExists(oldPath);
        bool tempHas = MediaPureFileUtils::IsFileExists(tempPath);
        MEDIA_INFO_LOG("RestoreOriginalPositionsForResume, masterHas=%{public}d, oldHas=%{public}d, tempHas=%{public}d",
            masterHas, oldHas, tempHas);
        if (masterHas && oldHas) {
            MEDIA_INFO_LOG("RestoreOriginalPositionsForResume: master and old both exist, skip pair");
            continue;
        }
        if (!masterHas && tempHas) {
            MEDIA_INFO_LOG("RestoreOriginalPositionsForResume: master empty, temp has data, move temp to master");
            if (!MoveKvDbDirectory(tempPath, masterPath)) {
                MEDIA_ERR_LOG("RestoreOriginalPositionsForResume: move temp to master failed");
                ret = false;
            }
            continue;
        }
        if (masterHas && !oldHas && tempHas) {
            MEDIA_INFO_LOG("RestoreOriginalPositionsForResume: full restore, master to old and temp to master");
            if (!MoveKvDbDirectory(masterPath, oldPath)) {
                MEDIA_ERR_LOG("RestoreOriginalPositionsForResume: move master to old failed");
                ret = false;
            }
            if (!MoveKvDbDirectory(tempPath, masterPath)) {
                MEDIA_ERR_LOG("RestoreOriginalPositionsForResume: move temp to master failed");
                ret = false;
            }
            continue;
        }
        MEDIA_INFO_LOG("RestoreOriginalPositionsForResume: no restore needed for pair");
    }
    CleanupTempDir();
    MEDIA_INFO_LOG("KvStoreSwapOperator::RestoreOriginalPositionsForResume finished, ret=%{public}d", ret);
    return ret;
}

bool KvStoreSwapOperator::CleanupTempDir()
{
    MEDIA_INFO_LOG("KvStoreSwapOperator::CleanupTempDir start");
    if (!MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR) && MediaPureFileUtils::IsFileExists(REVERSE_KVDB_TEMP_DIR)) {
        MEDIA_WARN_LOG("KvStoreSwapOperator::CleanupTempDir failed, dir still exists");
        return false;
    }
    MEDIA_INFO_LOG("KvStoreSwapOperator::CleanupTempDir finished");
    return true;
}

bool KvStoreSwapOperator::Rollback()
{
    MEDIA_INFO_LOG("KvStoreSwapOperator::Rollback start, failedStep=%{public}d", static_cast<int>(failedStep_));
    switch (failedStep_) {
        case KvStoreSwapStep::MOVE_OLD_TO_MASTER:
            RestoreKvDbFromTempToMaster();
            break;
        case KvStoreSwapStep::RENAME_NEW_TO_TEMP:
            RestoreKvDbFromTempToMaster();
            break;
        case KvStoreSwapStep::NONE:
            RestoreKvDbToInitialPosition();
            break;
        default:
            break;
    }
    if (static_cast<int>(failedStep_) >= static_cast<int>(KvStoreSwapStep::NOTIFY_MAIN_CLOSE)) {
        NotifyMainProcessReopen();
    }
    CleanupTempDir();
    failedStep_ = KvStoreSwapStep::NONE;
    MEDIA_INFO_LOG("KvStoreSwapOperator::Rollback finished");
    return true;
}
} // namespace Media
} // namespace OHOS
