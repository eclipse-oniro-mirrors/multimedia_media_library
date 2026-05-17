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

#ifndef I_CONFLICT_RESOLVER_H
#define I_CONFLICT_RESOLVER_H

#include <memory>

#include "scan_config.h"
#include "scan_task_context.h"

namespace OHOS {
namespace Media {
enum class ScanSubmitResult {
    EXECUTING = 0,  // 任务已提交，正在执行
    WAITING = 1,    // 任务已加入等待队列（同 fileId 正在执行）
    REJECTED = 2    // 任务被拒绝（如 LOW 被 FULL 拒绝）
};

class IConflictResolver {
public:
    virtual ~IConflictResolver() = default;

    virtual ScanSubmitResult Resolve(
        const std::shared_ptr<ScanTaskContext>& newTask,
        const std::shared_ptr<ScanTaskContext>& executingTask) = 0;

    virtual bool IsStrategyEnabled(const std::shared_ptr<ScanTaskContext>& newTask) const = 0;
};

} // namespace Media
} // namespace OHOS

#endif // I_CONFLICT_RESOLVER_H