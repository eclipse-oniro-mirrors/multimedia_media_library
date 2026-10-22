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

#define MLOG_TAG "KvStoreSwapOperator"

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
    if (oldDirs[0] == newDirs[0] && oldDirs[1] == newDirs[1]) {
        hashPairs_.push_back({oldDirs[0], newDirs[0]});
        hashPairs_.push_back({oldDirs[1], newDirs[1]});
        return true;
    }
    if (oldDirs[0] == newDirs[1] && oldDirs[1] == newDirs[0]) {
        hashPairs_.push_back({oldDirs[0], newDirs[1]});
        hashPairs_.push_back({oldDirs[1], newDirs[0]});
        return true;
    }
    return false;
}

bool KvStoreSwapOperator::MatchHashDirectoriesByKnownHashes(const std::vector<std::string> &oldDirs,
    const std::vector<std::string> &newDirs)
{
    for (const auto &dir : oldDirs) {
        if (dir == OLD_MACHINE_YEAR_HASH || dir == OLD_MACHINE_MONTH_HASH) {
            for (const auto &newDir : newDirs) {
                if (dir == OLD_MACHINE_YEAR_HASH && newDir == NEW_MACHINE_YEAR_HASH) {
                    hashPairs_.push_back({dir, newDir});
                } else if (dir == OLD_MACHINE_MONTH_HASH && newDir == NEW_MACHINE_MONTH_HASH) {
                    hashPairs_.push_back({dir, newDir});
                }
            }
        }
    }
    return hashPairs_.size() == EXPECTED_HASH_PAIR_COUNT;
}

bool KvStoreSwapOperator::RenameNewKvDbToTemp()
{
    failedStep_ = KvStoreSwapStep::RENAME_NEW_TO_TEMP;
    if (!MediaFileUtils::DeleteDir(tempBaseDir_)) {
        if (MediaPureFileUtils::IsFileExists(tempBaseDir_)) {
            MEDIA_WARN_LOG("RenameNewKvDbToTemp delete temp dir failed but dir still exists, temp=%{public}s",
                MediaFileUtils::DesensitizePath(tempBaseDir_).c_str());
        }
    }
    if (!MediaFileUtils::CreateDirectory(tempBaseDir_)) {
        MEDIA_ERR_LOG("RenameNewKvDbToTemp create temp dir failed, temp=%{public}s",
            MediaFileUtils::DesensitizePath(tempBaseDir_).c_str());
        return false;
    }
    for (const auto &pair : hashPairs_) {
        std::string src = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string dst = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        if (!MoveKvDbDirectory(src, dst)) {
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::MoveOldKvDbToMaster()
{
    failedStep_ = KvStoreSwapStep::MOVE_OLD_TO_MASTER;
    for (const auto &pair : hashPairs_) {
        std::string src = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string dst = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        if (!MoveKvDbDirectory(src, dst)) {
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::RestoreKvDbFromTempToMaster()
{
    for (const auto &pair : hashPairs_) {
        std::string src = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string dst = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string old = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        if (MediaPureFileUtils::IsFileExists(dst) && !MediaPureFileUtils::IsFileExists(old)) {
            if (!MoveKvDbDirectory(dst, old)) {
                return false;
            }
        }
        if (!MoveKvDbDirectory(src, dst)) {
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::MoveKvDbDirectory(const std::string &srcPath, const std::string &dstPath)
{
    if (!MediaPureFileUtils::IsFileExists(srcPath)) {
        MEDIA_ERR_LOG("MoveKvDbDirectory src not exist, src=%{public}s",
            MediaFileUtils::DesensitizePath(srcPath).c_str());
        return false;
    }
    size_t pos = dstPath.find_last_of('/');
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("MoveKvDbDirectory invalid dst path, dst=%{public}s",
            MediaFileUtils::DesensitizePath(dstPath).c_str());
        return false;
    }
    std::string dstParent = dstPath.substr(0, pos);
    if (!MediaFileUtils::CreateDirectory(dstParent)) {
        MEDIA_ERR_LOG("MoveKvDbDirectory create dst parent failed, dstParent=%{public}s",
            MediaFileUtils::DesensitizePath(dstParent).c_str());
        return false;
    }
    MEDIA_INFO_LOG("MoveKvDbDirectory src=%{public}s, dst=%{public}s",
        MediaFileUtils::DesensitizePath(srcPath).c_str(),
        MediaFileUtils::DesensitizePath(dstPath).c_str());
    if (MediaFileUtils::RenameDir(srcPath, dstPath)) {
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
        MEDIA_ERR_LOG("MoveKvDbDirectory copy directory failed, src=%{public}s, dst=%{public}s",
            MediaFileUtils::DesensitizePath(srcPath).c_str(),
            MediaFileUtils::DesensitizePath(dstPath).c_str());
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
        std::string currentOld = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string oldTarget = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        if (!MoveKvDbDirectory(currentOld, oldTarget)) {
            return false;
        }
        std::string currentNew = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string newTarget = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        if (!MoveKvDbDirectory(currentNew, newTarget)) {
            return false;
        }
    }
    return true;
}

bool KvStoreSwapOperator::WriteSwapTimestamp()
{
    failedStep_ = KvStoreSwapStep::WRITE_MARKER;
    return ReverseCloneRestoreMarker::Recreate(ReverseCloneMarkerType::KVDB);
}

bool KvStoreSwapOperator::Swap(const std::string &backupRestorePath)
{
    MEDIA_INFO_LOG("KvStoreSwapOperator Swap start");
    if (!InitBaseDirs(backupRestorePath)) {
        return false;
    }
    if (!MatchHashDirectories()) {
        return false;
    }
    if (!NotifyMainProcessClose()) {
        return false;
    }
    if (!RenameNewKvDbToTemp()) {
        Rollback();
        return false;
    }
    if (!MoveOldKvDbToMaster()) {
        Rollback();
        return false;
    }
    if (!NotifyMainProcessReopen()) {
        MEDIA_WARN_LOG("KvStoreSwapOperator Swap reopen failed, continue");
    }
    if (!WriteSwapTimestamp()) {
        MEDIA_WARN_LOG("KvStoreSwapOperator Swap write swap timestamp failed, continue");
    }
    failedStep_ = KvStoreSwapStep::NONE;
    MEDIA_INFO_LOG("KvStoreSwapOperator Swap success");
    return true;
}

bool KvStoreSwapOperator::Rollback()
{
    if (failedStep_ == KvStoreSwapStep::MOVE_OLD_TO_MASTER ||
        failedStep_ == KvStoreSwapStep::RENAME_NEW_TO_TEMP) {
        RestoreKvDbFromTempToMaster();
    } else if (failedStep_ == KvStoreSwapStep::NONE) {
        RestoreKvDbToInitialPosition();
    }
    if (static_cast<int>(failedStep_) >= static_cast<int>(KvStoreSwapStep::NOTIFY_MAIN_CLOSE)) {
        NotifyMainProcessReopen();
    }
    CleanupTempDir();
    failedStep_ = KvStoreSwapStep::NONE;
    return true;
}

bool KvStoreSwapOperator::RestoreOriginalPositions()
{
    bool ret = RestoreKvDbToInitialPosition();
    if (!NotifyMainProcessReopen()) {
        MEDIA_WARN_LOG("RestoreOriginalPositions reopen failed");
    }
    CleanupTempDir();
    return ret;
}

bool KvStoreSwapOperator::RestoreOriginalPositionsForResume(const std::string &backupRestorePath)
{
    InitBaseDirs(backupRestorePath);
    hashPairs_.clear();
    hashPairs_.push_back({OLD_MACHINE_YEAR_HASH, NEW_MACHINE_YEAR_HASH});
    hashPairs_.push_back({OLD_MACHINE_MONTH_HASH, NEW_MACHINE_MONTH_HASH});

    bool ret = true;
    for (const auto &pair : hashPairs_) {
        std::string masterPath = newBaseDir_ + "/" + pair.newHash + "/single_ver";
        std::string oldPath = oldBaseDir_ + "/" + pair.oldHash + "/single_ver";
        std::string tempPath = tempBaseDir_ + "/" + pair.oldHash + "/single_ver";
        bool masterHas = MediaPureFileUtils::IsFileExists(masterPath);
        bool oldHas = MediaPureFileUtils::IsFileExists(oldPath);
        bool tempHas = MediaPureFileUtils::IsFileExists(tempPath);
        if (masterHas && oldHas) {
            MEDIA_INFO_LOG("RestoreOriginalPositionsForResume skip, both master and old exist");
            continue;
        }
        if (!masterHas && tempHas) {
            if (!MoveKvDbDirectory(tempPath, masterPath)) {
                ret = false;
            }
            continue;
        }
        if (masterHas && !oldHas && tempHas) {
            if (!MoveKvDbDirectory(masterPath, oldPath)) {
                ret = false;
            }
            if (!MoveKvDbDirectory(tempPath, masterPath)) {
                ret = false;
            }
            continue;
        }
    }
    CleanupTempDir();
    return ret;
}

bool KvStoreSwapOperator::CleanupTempDir()
{
    if (!MediaFileUtils::DeleteDir(REVERSE_KVDB_TEMP_DIR)) {
        if (MediaPureFileUtils::IsFileExists(REVERSE_KVDB_TEMP_DIR)) {
            MEDIA_WARN_LOG("CleanupTempDir delete temp dir failed, dir still exists");
            return false;
        }
    }
    MEDIA_INFO_LOG("CleanupTempDir success");
    return true;
}
} // namespace Media
} // namespace OHOS
