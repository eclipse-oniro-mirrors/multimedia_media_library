/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "analysis_strategy_registry.h"

#include "default_strategies.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

std::unordered_map<int32_t, std::unique_ptr<AnalysisAlbumImpactAnalyzer>>& AnalysisStrategyRegistry::Impl()
{
    static std::unordered_map<int32_t, std::unique_ptr<AnalysisAlbumImpactAnalyzer>> map;
    return map;
}

std::unique_ptr<AnalysisAlbumImpactAnalyzer> AnalysisStrategyRegistry::CreateDefaultAnalyzer()
{
    AnalysisStrategyPolicy policy;
    policy.effective = std::make_shared<DefaultAlbumEffectiveStrategy>();
    policy.count = std::make_shared<DefaultCountStrategy>();
    policy.cover = std::make_shared<DefaultCoverStrategy>();
    policy.picker = std::make_shared<DefaultCoverPickerStrategy>();

    AnalysisAlbumPipelineEngine pipeline;
    pipeline.AddStep(std::make_unique<CountStep>(policy.count));
    pipeline.AddStep(std::make_unique<CoverRecordStep>(policy.cover));
    pipeline.AddStep(std::make_unique<CoverDecisionStep>(policy.cover));
    pipeline.AddStep(std::make_unique<PickerStep>(policy.picker));

    return std::make_unique<AnalysisAlbumImpactAnalyzer>(std::move(policy), std::move(pipeline));
}

AnalysisStrategyRegistry::AnalyzerBuilder::AnalyzerBuilder(int32_t subtype) : subtype_(subtype) {}

AnalysisStrategyRegistry::AnalyzerBuilder& AnalysisStrategyRegistry::AnalyzerBuilder::UseDefaultPipeline()
{
    CHECK_AND_RETURN_RET_LOG(policy_.isValidPolicy(), *this, "Invalid StrategyPolicy");
    pipeline_.AddStep(std::make_unique<CountStep>(policy_.count));
    pipeline_.AddStep(std::make_unique<CoverRecordStep>(policy_.cover));
    pipeline_.AddStep(std::make_unique<CoverDecisionStep>(policy_.cover));
    pipeline_.AddStep(std::make_unique<PickerStep>(policy_.picker));
    return *this;
}

bool AnalysisStrategyRegistry::AnalyzerBuilder::Build()
{
    CHECK_AND_RETURN_RET_LOG(policy_.isValidPolicy(), false, "Invalid StrategyPolicy");
    Impl()[subtype_] = std::make_unique<AnalysisAlbumImpactAnalyzer>(
        std::move(policy_), std::move(pipeline_));
    MEDIA_INFO_LOG("Register analyzer subtype=%{public}d", subtype_);
    return true;
}

AnalysisAlbumImpactAnalyzer* AnalysisStrategyRegistry::GetAnalyzer(int32_t subtype)
{
    auto& analyzerMap = Impl();
    auto it = analyzerMap.find(subtype);
    CHECK_AND_RETURN_RET(it == analyzerMap.end(), it->second.get());
    MEDIA_DEBUG_LOG("Subtype: %{public}d not registered, fallback to ANY", subtype);
    it = analyzerMap.find(DEFAULT_SUBTYPE);
    CHECK_AND_RETURN_RET(it == analyzerMap.end(), it->second.get());
    analyzerMap.emplace(DEFAULT_SUBTYPE, CreateDefaultAnalyzer());
    return analyzerMap[DEFAULT_SUBTYPE].get();
}

AnalysisStrategyRegistry::AnalyzerBuilder AnalysisStrategyRegistry::Register(int32_t subtype)
{
    return AnalyzerBuilder(subtype);
}

AnalysisStrategyRegistry::AnalyzerBuilder AnalysisStrategyRegistry::RegisterDefault()
{
    return AnalyzerBuilder(DEFAULT_SUBTYPE);
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
