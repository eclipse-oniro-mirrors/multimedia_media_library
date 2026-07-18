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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RELIABILITY_MARKER_H
#define OHOS_MEDIA_REVERSE_CLONE_RELIABILITY_MARKER_H

#include <string>
#include <cstdint>

namespace OHOS {
namespace Media {

/**
 * @brief 反向克隆恢复阶段标记
 * | 1 | PrepareOldDb | 准备旧机DB临时副本
 * | 2 | DoDataBaseUpgrade | 对旧机DB进行升级
 * | 3 | ClearRedundantData | 清除新机DB中的冗余数据
 * | 4 | CleanInvalidPhotos | 清理旧机DB中的无效照片记录
 * | 5 | PerformInitialMigration | 第一次ID偏移（旧机DB → 新机DB）
 * | 6 | BackupAndRenameNewDb | 备份新DB并重命名为source
 * | 7 | MoveAssets | 交换媒体文件目录
 * | 8 | FinalizeDatabaseSwap | 数据库转正（处理后的旧DB接管主库）
 * | 9 | PerformSecondaryMigration | 第二次ID偏移（检查并迁移新增数据
 * | 10 | AbsorbNewData | 吸收新机数据（相册、照片、云图
 * | 11 | ReverseRestoreAnalysisData | 智慧数据恢复
 * | 12 | HandleRestData | 清理收尾（关库、设置参数、清理临时文件）
 */
enum class ReverseCloneRestoreStage {
    NOT_STARTED = 0,           // 未开始
    EARLY_STAGE = 1,           // 早期阶段（步骤1-5）
    DB_SWITCHED = 2,           // 数据库已切换（步骤6-9完成）
    ABSORBING_DATA = 3,        // 正在吸收新机数据（步骤10）
    ANALYSIS_RESTORE = 4,      // 智慧数据恢复（步骤11）
    FINISHING = 5,             // 收尾阶段（步骤12）
    COMPLETED = 6              // 已完成
};

/**
 * @brief 反向克隆可靠性恢复标记位管理类
 *        用于管理反向克隆可靠性恢复的进度标记，支持断点续传
 */
class ReverseCloneReliabilityMarker {
public:
    /**
     * @brief 删除标记位
     * @return true表示成功，false表示失败
     */
    static bool Delete();

    /**
     * @brief 检查标记位文件是否存在
     * @return true表示存在，false表示不存在
     */
    static bool Exists();

    /**
     * @brief 设置阶段
     * @param stage 新的恢复阶段
     * @return true表示成功，false表示失败
     */
    static bool SetStage(ReverseCloneRestoreStage stage);

    /**
     * @brief 获取阶段
     * @param stage 输出参数，当前恢复阶段
     * @return true表示成功，false表示失败
     */
    static bool GetStage(ReverseCloneRestoreStage &stage);

private:
    static const std::string MARKER_XML;
    static const std::string KEY_STAGE;
    static const std::string KEY_TIMESTAMP;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_REVERSE_CLONE_RELIABILITY_MARKER_H