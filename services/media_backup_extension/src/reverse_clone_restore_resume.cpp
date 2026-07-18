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
#define MLOG_TAG "Media_Reverse_Restore_Resume"

#include "reverse_clone_restore_resume.h"
#include "reverse_clone_reliability_marker.h"
#include "reverse_clone_restore.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "application_context.h"
#include "medialibrary_data_manager.h"
#include "media_file_utils.h"
#include "rdb_helper.h"
#include "preferences.h"
#include "preferences_helper.h"
#include <thread>

namespace OHOS {
namespace Media {

const std::string REVERSE_RESTORE_RESOURCE_ROOT = "/storage/media/local/files/reverse_restore";

// 初始化原子变量
std::atomic<bool> ReverseCloneRestoreResume::isResuming_(false);
static constexpr const char* ASSET_MOVES_XML = "/data/storage/el2/base/preferences/asset_moves.xml";
static constexpr const char* ASSET_MOVE_KEY_PREFIX = "asset_move_";
static constexpr const char* ASSET_MOVE_COUNT_KEY = "asset_move_count";

// AssetMoveState 字段索引常量
static constexpr int ASSET_MOVE_PARTS_COUNT = 7;  // 期望的字段数量
static constexpr int INDEX_SRC = 0;               // 源路径索引
static constexpr int INDEX_DST = 1;               // 目标路径索引
static constexpr int INDEX_BACKUP = 2;            // 备份路径索引
static constexpr int INDEX_HAD_SRC = 3;           // hadSrc 字段索引
static constexpr int INDEX_HAD_DST = 4;           // hadDst 字段索引
static constexpr int INDEX_MOVED_SRC = 5;         // movedSrc 字段索引
static constexpr int INDEX_BACKED_UP_DST = 6;     // backedUpDst 字段索引

std::vector<ReverseCloneRestore::AssetMoveState> LoadCompletedAssetMovesFromXml()
{
    MEDIA_INFO_LOG("LoadCompletedAssetMovesFromXml: start loading");
    std::vector<ReverseCloneRestore::AssetMoveState> moves;

    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(ASSET_MOVES_XML, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("LoadCompletedAssetMovesFromXml: get preferences failed, errCode=%{public}d", errCode);
        return moves;
    }
    int count = prefs->GetInt(ASSET_MOVE_COUNT_KEY, 0);
    for (int i = 0; i < count; i++) {
        std::string key = ASSET_MOVE_KEY_PREFIX + std::to_string(i);
        std::string value = prefs->GetString(key, "");
        if (value.empty()) {
            continue;
        }
        std::vector<std::string> parts;
        size_t start = 0;
        size_t pos = 0;
        while ((pos = value.find('|', start)) != std::string::npos) {
            parts.push_back(value.substr(start, pos - start));
            start = pos + 1;
        }
        parts.push_back(value.substr(start));

        if (parts.size() == ASSET_MOVE_PARTS_COUNT) {
            ReverseCloneRestore::AssetMoveState move;
            move.src = parts[INDEX_SRC];
            move.dst = parts[INDEX_DST];
            move.backup = parts[INDEX_BACKUP];
            move.hadSrc = parts[INDEX_HAD_SRC] == "1";
            move.hadDst = parts[INDEX_HAD_DST] == "1";
            move.movedSrc = parts[INDEX_MOVED_SRC] == "1";
            move.backedUpDst = parts[INDEX_BACKED_UP_DST] == "1";
            moves.push_back(move);
        } else {
            MEDIA_WARN_LOG("LoadCompletedAssetMovesFromXml: invalid parts size %{public}zu, expected %{public}d",
                           parts.size(), ASSET_MOVE_PARTS_COUNT);
        }
    }

    MEDIA_INFO_LOG("LoadCompletedAssetMovesFromXml: loaded %{public}zu moves", moves.size());
    return moves;
}

void ReverseCloneRestoreResume::CheckAndStartResumeImpl()
{
    MEDIA_INFO_LOG("CheckAndStartResume: start checking");
    // 检查是否正在进行接续克隆
    if (isResuming_.load()) {
        MEDIA_INFO_LOG("Reverse clone restore is already in progress, ignore this call");
        return;
    }
    // 检查标记位是否存在
    if (!ReverseCloneReliabilityMarker::Exists()) {
        MEDIA_INFO_LOG("Reverse clone restore marker does not exist");
        return;
    }
    ReverseCloneRestoreStage stage;
    if (!ReverseCloneReliabilityMarker::GetStage(stage)) {
        MEDIA_WARN_LOG("Failed to read reverse clone restore marker");
        return;
    }
    if (stage == ReverseCloneRestoreStage::COMPLETED) {
        MEDIA_INFO_LOG("Reverse clone restore already completed");
        ReverseCloneReliabilityMarker::Delete();
        return;
    }
    MEDIA_INFO_LOG("Reverse clone restore needs resume, stage=%{public}d",
        static_cast<int>(stage));
    // 设置正在恢复标志
    isResuming_.store(true);

    ResumeWorker(stage);
}


void ReverseCloneRestoreResume::ResumeWorker(ReverseCloneRestoreStage stage)
{
    MEDIA_INFO_LOG("ResumeWorker: start resume, stage=%{public}d",
        static_cast<int>(stage));
    // 执行恢复
    bool success = DoResume(stage);
    if (success) {
        MEDIA_INFO_LOG("Reverse clone resume completed successfully");
    } else {
        MEDIA_ERR_LOG("Reverse clone resume failed");
        // 恢复失败，删除标记位
        ReverseCloneReliabilityMarker::Delete();
    }
    // 重置正在恢复标志
    isResuming_.store(false);
    SettingsDataManager::ClearReverseRestoreStatus();
}

bool ReverseCloneRestoreResume::DoResume(ReverseCloneRestoreStage stage)
{
    switch (stage) {
        case ReverseCloneRestoreStage::EARLY_STAGE:
        case ReverseCloneRestoreStage::DB_SWITCHED:
            // 清理临时文件，忽略该次反向克隆，回滚
            return RollbackAndIgnore();

        case ReverseCloneRestoreStage::ABSORBING_DATA:
            // 重新执行吸收流程
            return ResumeAbsorbData();

        case ReverseCloneRestoreStage::ANALYSIS_RESTORE:
        case ReverseCloneRestoreStage::FINISHING:
            // 重新执行收尾
            return ResumeFinish();

        default:
            MEDIA_WARN_LOG("Unknown stage: %{public}d", static_cast<int>(stage));
            return false;
    }
}

bool ReverseCloneRestoreResume::RollbackAndIgnore()
{
    MEDIA_INFO_LOG("Rollback and ignore reverse clone");
    // 读取当前阶段
    ReverseCloneRestoreStage currentStage;
    if (!ReverseCloneReliabilityMarker::GetStage(currentStage)) {
        MEDIA_ERR_LOG("Failed to get current stage for rollback");
        return false;
    }
    // 判断是否为早期阶段
    bool isEarlyStage = (currentStage == ReverseCloneRestoreStage::EARLY_STAGE);

    // 调用 ReverseCloneRestore 方法执行回滚
    auto reverseRestore = std::make_unique<ReverseCloneRestore>();
    if (reverseRestore == nullptr) {
        MEDIA_ERR_LOG("Failed to create ReverseCloneRestore instance");
        return false;
    }
    if (!reverseRestore->RollbackReverseRestore(isEarlyStage)) {
        MEDIA_ERR_LOG("Rollback reverse restore failed");
        return false;
    }
    // 非早期阶段：从XML文件加载已完成的资产移动状态并回滚
    if (!isEarlyStage) {
        MEDIA_INFO_LOG("RollbackAndIgnore: restoring directories from XML");
        std::vector<ReverseCloneRestore::AssetMoveState> moves = LoadCompletedAssetMovesFromXml();
        if (!reverseRestore->SetCompletedAssetMovesAndRollback(moves)) {
            MEDIA_ERR_LOG("RollbackAndIgnore: restore directories failed");
            return false;
        }
    }
    // 删除标记位
    ReverseCloneReliabilityMarker::Delete();
    return true;
}

bool ReverseCloneRestoreResume::ResumeAbsorbData()
{
    MEDIA_INFO_LOG("Resume absorb new device data");

    // 创建 ReverseCloneRestore 实例
    auto reverseRestore = std::make_unique<ReverseCloneRestore>();
    if (reverseRestore == nullptr) {
        MEDIA_ERR_LOG("Failed to create ReverseCloneRestore instance");
        return false;
    }
    if (reverseRestore->Init("", "", false) != E_OK) {
        MEDIA_ERR_LOG("Failed to init ReverseCloneRestore");
        return false;
    }
    // 准备断点续传所需的所有状态
    if (!reverseRestore->PrepareForResume()) {
        MEDIA_ERR_LOG("Failed to prepare for resume");
        return false;
    }
    reverseRestore->SetCloneParameterAndStopSyncForResume();
    // 执行吸收新机数据
    std::vector<ReverseCloneKvStoreTask> retainedOldPhotoKvStoreTasks;
    reverseRestore->AbsorbNewDeviceData("", retainedOldPhotoKvStoreTasks);
    reverseRestore->FinishReverseRestore();
    return true;
}

bool ReverseCloneRestoreResume::ResumeFinish()
{
    MEDIA_INFO_LOG("Resume finish reverse clone restore");
    // 创建 ReverseCloneRestore 实例
    auto reverseRestore = std::make_unique<ReverseCloneRestore>();
    if (reverseRestore == nullptr) {
        MEDIA_ERR_LOG("Failed to create ReverseCloneRestore instance");
        return false;
    }
    // 初始化
    if (reverseRestore->Init("", "", false) != E_OK) {
        MEDIA_ERR_LOG("Failed to init ReverseCloneRestore");
        return false;
    }
    // 初始化 sourceRdb_ 和 destRdb_
    if (!reverseRestore->InitDatabasesForResume()) {
        MEDIA_ERR_LOG("Failed to init databases for resume");
        return false;
    }
    // 执行收尾
    reverseRestore->FinishReverseRestore();
    // 更新标记位为已完成
    ReverseCloneReliabilityMarker::SetStage(ReverseCloneRestoreStage::COMPLETED);
    ReverseCloneReliabilityMarker::Delete();
    return true;
}

void CheckAndStartResume()
{
    ReverseCloneRestoreResume::CheckAndStartResumeImpl();
}
} // namespace Media
} // namespace OHOS