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

#define MLOG_TAG "ScanStrategyManager"

#include "scan_strategy_manager.h"

#include "default_scan_strategy.h"
#include "media_log.h"

namespace OHOS {
namespace Media {

ScanStrategyManager& ScanStrategyManager::GetInstance()
{
    static ScanStrategyManager instance;
    return instance;
}

ScanStrategyManager::ScanStrategyManager()
{
    RegisterDefaultStrategies();
    MEDIA_INFO_LOG("ScanStrategyManager created");
}

ScanStrategyManager::~ScanStrategyManager()
{
    ClearAllStrategies();
    MEDIA_INFO_LOG("ScanStrategyManager destroyed");
}

void ScanStrategyManager::RegisterDefaultStrategies()
{
    auto defaultStrategy = std::make_shared<DefaultScanStrategy>();
    RegisterStrategy(defaultStrategy);
    
    MEDIA_INFO_LOG("DefaultScanStrategy registered");
}

void ScanStrategyManager::RegisterStrategy(const std::shared_ptr<IScanStrategy>& strategy)
{
    if (strategy == nullptr) {
        MEDIA_ERR_LOG("RegisterStrategy: strategy is nullptr");
        return;
    }

    ScanStrategyType strategyType = strategy->GetStrategyType();

    std::lock_guard<std::mutex> lock(mutex_);

    strategies_[strategyType] = strategy;

    MEDIA_INFO_LOG("strategy type %{public}d registered (total %{public}zu)",
        static_cast<int>(strategyType), strategies_.size());
}

std::shared_ptr<IScanStrategy> ScanStrategyManager::SelectStrategy(ScanStrategyType strategyType)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = strategies_.find(strategyType);
    if (it == strategies_.end()) {
        MEDIA_WARN_LOG("SelectStrategy: strategy type %{public}d not found", static_cast<int>(strategyType));
        return nullptr;
    }

    MEDIA_DEBUG_LOG("SelectStrategy: strategy type %{public}d selected", static_cast<int>(strategyType));
    return it->second;
}

void ScanStrategyManager::ClearAllStrategies()
{
    std::lock_guard<std::mutex> lock(mutex_);

    strategies_.clear();

    MEDIA_INFO_LOG("all strategies cleared");
}

} // namespace Media
} // namespace OHOS