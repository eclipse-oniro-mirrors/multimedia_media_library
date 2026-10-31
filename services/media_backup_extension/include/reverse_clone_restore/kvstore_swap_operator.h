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

#ifndef OHOS_MEDIA_KVSTORE_SWAP_OPERATOR_H
#define OHOS_MEDIA_KVSTORE_SWAP_OPERATOR_H

#include <string>
#include <vector>

namespace OHOS {
namespace Media {

class CloneRestore;

enum class KvStoreSwapStep {
    NONE = 0,
    NOTIFY_MAIN_CLOSE,
    RENAME_NEW_TO_TEMP,
    MOVE_OLD_TO_MASTER,
    NOTIFY_MAIN_REOPEN,
    WRITE_MARKER,
};

class KvStoreSwapOperator {
public:
    KvStoreSwapOperator() = default;
    ~KvStoreSwapOperator() = default;
    bool Swap(const std::string &backupRestorePath);
    bool Rollback();
    bool RestoreOriginalPositions();
    bool RestoreOriginalPositionsForResume(const std::string &backupRestorePath);
    bool CleanupTempDir();

private:
    bool NotifyMainProcessClose();
    bool NotifyMainProcessReopen();
    bool RenameNewKvDbToTemp();
    bool MoveOldKvDbToMaster();
    bool WriteSwapTimestamp();
    bool InitBaseDirs(const std::string &backupRestorePath);
    bool MatchHashDirectories();
    bool MatchHashDirectoriesByEqualSet(const std::vector<std::string> &oldDirs,
        const std::vector<std::string> &newDirs);
    bool MatchHashDirectoriesByKnownHashes(const std::vector<std::string> &oldDirs,
        const std::vector<std::string> &newDirs);
    bool RestoreKvDbFromTempToMaster();
    bool RestoreKvDbToInitialPosition();
    bool MoveKvDbDirectory(const std::string &srcPath, const std::string &dstPath);

    struct HashPair {
        std::string oldHash;
        std::string newHash;
    };
    std::vector<HashPair> hashPairs_;
    std::string oldBaseDir_;
    std::string newBaseDir_;
    std::string tempBaseDir_;
    KvStoreSwapStep failedStep_ = KvStoreSwapStep::NONE;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_KVSTORE_SWAP_OPERATOR_H
