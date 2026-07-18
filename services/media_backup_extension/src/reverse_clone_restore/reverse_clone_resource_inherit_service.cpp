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

#include "reverse_clone_resource_inherit_service.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "reverse_clone_candidate_resolver.h"
#include "reverse_clone_resource_executor.h"
#include "reverse_clone_resource_plan_builder.h"

namespace OHOS::Media {
namespace {
const std::string REVERSE_RESTORE_RESOURCE_ROOT = "/storage/media/local/files/reverse_restore";

// LCOV_EXCL_START
std::string GetReverseRestoreOriginPath(const FileInfo &sourceFile)
{
    CHECK_AND_RETURN_RET_LOG(!sourceFile.relativePath.empty(), "", "Reverse restore relativePath is empty");
    return REVERSE_RESTORE_RESOURCE_ROOT + sourceFile.relativePath;
}
} // namespace

ReverseCloneResourcePlan ReverseCloneResourceInheritService::BuildDuplicatePlanByFileId(
    const FileInfo &absorbedFile, int32_t duplicateFileId, const std::shared_ptr<NativeRdb::RdbStore> &donorRdb,
    const std::unordered_set<int32_t> &originalPureCloudFileIds) const
{
    ReverseCloneCandidateResolver candidateResolver(originalPureCloudFileIds);
    ReverseCloneResourcePlanBuilder planBuilder;
    ReverseCloneCandidate candidate = candidateResolver.ResolveByFileId(absorbedFile, donorRdb, duplicateFileId);
    ReverseCloneResourcePlan plan = planBuilder.Build(absorbedFile, candidate, absorbedFile.fileIdOld);
    MEDIA_INFO_LOG("RevRes duplicate resource plan built, absorbedFileId=%{public}d, donorFileId=%{public}d, "
        "matchType=%{public}d, decision=%{public}d, absorbedPureCloud=%{public}d, donorFileSourceType=%{public}d, "
        "donorStoragePath=%{public}s, donorCloudId=%{public}s",
        plan.absorbed.fileId, plan.donor.fileId, static_cast<int32_t>(plan.matchType),
        static_cast<int32_t>(plan.decision), plan.absorbed.isPureCloud, plan.donor.fileSourceType,
        MediaFileUtils::DesensitizePath(plan.donor.storagePath).c_str(), plan.donor.fingerprint.cloudId.c_str());
    return plan;
}

ReverseCloneResourcePlan ReverseCloneResourceInheritService::BuildSourcePlanAfterPrepare(
    const FileInfo &sourceFile) const
{
    ReverseCloneResourcePlanBuilder planBuilder;
    return planBuilder.BuildFromSource(sourceFile, GetSourceResourceRoot(), GetSourceOriginPath(sourceFile),
        sourceFile.fileIdOld);
}

void ReverseCloneResourceInheritService::MergeDuplicatePlansWithSourceFallback(
    const std::vector<ReverseCloneResourcePlan> &duplicatePlans,
    std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans) const
{
    for (const auto &plan : duplicatePlans) {
        CHECK_AND_CONTINUE(plan.decision == ReverseCloneResourceDecision::INHERIT);
        int32_t fileId = plan.absorbed.fileId;
        CHECK_AND_CONTINUE(fileId > 0);

        ReverseCloneResourcePlan mergedPlan = plan;
        auto sourcePlan = resourcePlans.find(fileId);
        if (sourcePlan != resourcePlans.end() &&
            sourcePlan->second.decision == ReverseCloneResourceDecision::INHERIT &&
            sourcePlan->second.donor.HasResourcePath()) {
            mergedPlan.fallbackSource = sourcePlan->second.donor;
            mergedPlan.hasFallbackSource = true;
            mergedPlan.inheritOrigin = mergedPlan.inheritOrigin || sourcePlan->second.inheritOrigin;
            mergedPlan.inheritLcdThumbnail =
                mergedPlan.inheritLcdThumbnail || sourcePlan->second.inheritLcdThumbnail;
            mergedPlan.inheritThumbnail = mergedPlan.inheritThumbnail || sourcePlan->second.inheritThumbnail;
        }
        resourcePlans[fileId] = mergedPlan;
    }
}

int32_t ReverseCloneResourceInheritService::ExecuteAfterInsert(const ReverseCloneResourcePlan &plan,
    const std::shared_ptr<NativeRdb::RdbStore> &targetRdb, ReverseRestoreReportInfo &reportInfo) const
{
    if (plan.decision != ReverseCloneResourceDecision::INHERIT || !plan.HasResourceAction()) {
        MEDIA_INFO_LOG("RevRes ExecuteAfterInsert skipped, fileId=%{public}d, donorFileId=%{public}d, "
            "decision=%{public}d, matchType=%{public}d, hasAction=%{public}d",
            plan.absorbed.fileId, plan.donor.fileId, static_cast<int32_t>(plan.decision),
            static_cast<int32_t>(plan.matchType), plan.HasResourceAction());
        return E_OK;
    }
    ReverseCloneResourceExecutor executor;
    return executor.Execute(plan, targetRdb, reportInfo);
}

std::string ReverseCloneResourceInheritService::GetSourceResourceRoot() const
{
    return REVERSE_RESTORE_RESOURCE_ROOT;
}

std::string ReverseCloneResourceInheritService::GetSourceOriginPath(const FileInfo &sourceFile) const
{
    return GetReverseRestoreOriginPath(sourceFile);
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
