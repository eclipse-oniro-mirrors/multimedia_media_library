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

#ifndef SCAN_STRATEGY_MANAGER_H
#define SCAN_STRATEGY_MANAGER_H

#include <memory>
#include <mutex>
#include <unordered_map>

#include "i_scan_strategy.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT ScanStrategyManager {
public:
    static ScanStrategyManager& GetInstance();

    void RegisterStrategy(const std::shared_ptr<IScanStrategy>& strategy);
    std::shared_ptr<IScanStrategy> SelectStrategy(ScanStrategyType strategyType);

    void ClearAllStrategies();

private:
    ScanStrategyManager();
    ~ScanStrategyManager();

    void RegisterDefaultStrategies();

    std::unordered_map<ScanStrategyType, std::shared_ptr<IScanStrategy>> strategies_;

    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif // SCAN_STRATEGY_MANAGER_H