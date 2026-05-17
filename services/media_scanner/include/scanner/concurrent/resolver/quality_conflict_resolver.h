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

#ifndef QUALITY_CONFLICT_RESOLVER_H
#define QUALITY_CONFLICT_RESOLVER_H

#include <memory>

#include "i_conflict_resolver.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT QualityConflictResolver : public IConflictResolver {
public:
    QualityConflictResolver();
    ~QualityConflictResolver() override;

    ScanSubmitResult Resolve(const std::shared_ptr<ScanTaskContext>& newTask,
        const std::shared_ptr<ScanTaskContext>& executingTask) override;

    bool IsStrategyEnabled(const std::shared_ptr<ScanTaskContext>& newTask) const override;

private:
    bool IsValidQualityIndex(const std::shared_ptr<ScanTaskContext>& task, int32_t& rowIndex) const;
};
} // namespace Media
} // namespace OHOS
#endif // QUALITY_CONFLICT_RESOLVER_H