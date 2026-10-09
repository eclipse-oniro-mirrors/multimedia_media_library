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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESTORE_RESUME_H
#define OHOS_MEDIA_REVERSE_CLONE_RESTORE_RESUME_H

#include "reverse_clone_reliability_marker.h"

#include <atomic>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

extern "C" {
    /**
     * @brief 检查并启动反向克隆恢复
     *        媒体库启动时调用，检查标记位，如果需要恢复则立即启动异步线程
     */
    EXPORT void CheckAndStartResume();
}

class ReverseCloneRestoreResume {
public:
    static void CheckAndStartResumeImpl();
private:
    /**
     * @brief 原子变量，表示当前是否正在进行接续克隆
     *        true表示正在进行，false表示没有进行
     */
    static std::atomic<bool> isResuming_;
    /**
     * @brief 反向克隆恢复工作线程
     * @param stage 当前恢复阶段
     */
    static void ResumeWorker(ReverseCloneRestoreStage stage);

    /**
     * @brief 执行恢复
     * @param stage 当前恢复阶段
     * @return true表示成功，false表示失败
     */
    static bool DoResume(ReverseCloneRestoreStage stage);

    /**
     * @brief 回滚并忽略反向克隆
     *        适用于早期阶段和数据库切换阶段
     * @return true表示成功，false表示失败
     */
    static bool RollbackAndIgnore();

    /**
     * @brief 恢复吸收新机数据
     *        适用于吸收数据阶段
     * @return true表示成功，false表示失败
     */
    static bool ResumeAbsorbData();

    /**
     * @brief 恢复收尾
     *        适用于智慧数据恢复阶段和收尾阶段
     * @return true表示成功，false表示失败
     */
    static bool ResumeFinish();
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_REVERSE_CLONE_RESTORE_RESUME_H