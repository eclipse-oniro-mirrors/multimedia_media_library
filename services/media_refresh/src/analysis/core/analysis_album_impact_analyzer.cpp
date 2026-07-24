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

#define MLOG_TAG "AccurateRefresh::AnalysisAlbumImpactAnalyzer"

#include "analysis_album_impact_analyzer.h"

#include "media_log.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

AnalysisAlbumImpactAnalyzer::AnalysisAlbumImpactAnalyzer(AnalysisStrategyPolicy policy,
    AnalysisAlbumPipelineEngine pipeline) : policy_(std::move(policy)), pipeline_(std::move(pipeline))
{
    CHECK_AND_RETURN_LOG(policy_.isValidPolicy(), "Invalid StrategyPolicy");
}

void AnalysisAlbumImpactAnalyzer::Analyze(AnalysisAnalyzerContext &ctx, PipelineFlow flow)
{
    pipeline_.Run(ctx, flow);
}

/** CalculateDataChangePhase */
int32_t AnalysisAlbumImpactAnalyzer::CalcDataChange(const PhotoAssetChangeData &data,
    const UpdateAlbumData &baseInfo, AnalysisAlbumRefreshInfo &info)
{
    int32_t invalidDelta = 0;
    CHECK_AND_RETURN_RET_DEBUG_LOG(policy_.effective != nullptr && policy_.effective->IsEffective(data, baseInfo),
        invalidDelta, "CalcDataChange failed");

    auto ctxOpt = AnalysisAnalyzerContextBuilder()
        .SetBaseInfo(baseInfo)
        .SetAssetChangeData(data)
        .SetRefreshInfo(info)
        .Build();

    CHECK_AND_RETURN_RET_LOG(ctxOpt.has_value(), invalidDelta, "Build ctx failed");

    Analyze(*ctxOpt, PipelineFlow::CalculateDataChangePhase());
    return ctxOpt->GetLastDelta();
}

/** ApplyCoverChangePhase */
bool AnalysisAlbumImpactAnalyzer::ApplyCoverChange(
    const UpdateAlbumData &base, AnalysisAlbumRefreshInfo &info)
{
    std::string oldCover = base.albumCoverUri;

    auto ctxOpt = AnalysisAnalyzerContextBuilder()
        .SetBaseInfo(base)
        .SetRefreshInfo(info)
        .Build();

    CHECK_AND_RETURN_RET_LOG(ctxOpt.has_value(), false, "Build ctx failed");

    Analyze(*ctxOpt, PipelineFlow::ApplyCoverChangePhase());
    return info.refreshCover_ != oldCover;
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
