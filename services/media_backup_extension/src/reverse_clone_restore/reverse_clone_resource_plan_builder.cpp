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

#include "reverse_clone_resource_plan_builder.h"

#include <variant>

#include "media_column.h"
namespace OHOS::Media {
namespace {
bool GetInt64FromValMap(const FileInfo &fileInfo, const std::string &columnName, int64_t &value)
{
    auto iter = fileInfo.valMap.find(columnName);
    if (iter == fileInfo.valMap.end()) {
        return false;
    }
    if (std::holds_alternative<int64_t>(iter->second)) {
        value = std::get<int64_t>(iter->second);
        return true;
    }
    if (std::holds_alternative<int32_t>(iter->second)) {
        value = static_cast<int64_t>(std::get<int32_t>(iter->second));
        return true;
    }
    return false;
}

bool GetStringFromValMap(const FileInfo &fileInfo, const std::string &columnName, std::string &value)
{
    auto iter = fileInfo.valMap.find(columnName);
    if (iter == fileInfo.valMap.end() || !std::holds_alternative<std::string>(iter->second)) {
        return false;
    }
    value = std::get<std::string>(iter->second);
    return true;
}
} // namespace

// LCOV_EXCL_START
ReverseCloneResourcePlan ReverseCloneResourcePlanBuilder::Build(const FileInfo &absorbedFile,
    const ReverseCloneCandidate &candidate, int32_t absorbedFileId) const
{
    ReverseCloneAssetResource absorbed = ToResource(absorbedFile, absorbedFileId);
    if (!candidate.IsFound()) {
        ReverseCloneResourcePlan plan;
        plan.absorbed = absorbed;
        return plan;
    }
    if (candidate.matchType == ReverseCloneMatchType::SAME_CLOUD_CONFLICT) {
        ReverseCloneResourcePlan plan;
        FillCommonPlan(plan, absorbed, candidate);
        plan.decision = ReverseCloneResourceDecision::SKIP_CLOUD_VERSION_CONFLICT;
        return plan;
    }
    if (!candidate.CanInheritResource()) {
        ReverseCloneResourcePlan plan;
        FillCommonPlan(plan, absorbed, candidate);
        return plan;
    }
    if (!candidate.donor.HasResourcePath()) {
        ReverseCloneResourcePlan plan;
        FillCommonPlan(plan, absorbed, candidate);
        plan.decision = ReverseCloneResourceDecision::SKIP_NO_DONOR_RESOURCE;
        return plan;
    }
    return BuildInheritPlan(absorbed, candidate);
}

ReverseCloneResourcePlan ReverseCloneResourcePlanBuilder::BuildFromSource(const FileInfo &sourceFile,
    const std::string &sourceRoot, const std::string &sourceOriginPath, int32_t absorbedFileId) const
{
    ReverseCloneAssetResource absorbed = ToResource(sourceFile, absorbedFileId);
    ReverseCloneAssetResource donor = absorbed;
    donor.localRoot = sourceRoot;
    donor.originPath = sourceOriginPath;
    donor.relativePath = sourceFile.relativePath;
    return BuildInheritPlan(absorbed, donor, ReverseCloneMatchType::SOURCE_ASSET);
}

ReverseCloneResourcePlan ReverseCloneResourcePlanBuilder::BuildInheritPlan(const ReverseCloneAssetResource &absorbed,
    const ReverseCloneCandidate &candidate) const
{
    return BuildInheritPlan(absorbed, candidate.donor, candidate.matchType);
}

ReverseCloneResourcePlan ReverseCloneResourcePlanBuilder::BuildInheritPlan(const ReverseCloneAssetResource &absorbed,
    const ReverseCloneAssetResource &donor, ReverseCloneMatchType matchType) const
{
    ReverseCloneResourcePlan plan;
    plan.absorbed = absorbed;
    plan.donor = donor;
    plan.matchType = matchType;
    plan.inheritOrigin = donor.HasOriginCandidate();
    plan.inheritLcdThumbnail = HasLcdThumbnail(donor);
    plan.inheritThumbnail = HasThumbnail(donor);
    plan.decision = plan.HasResourceAction() ? ReverseCloneResourceDecision::INHERIT :
        ReverseCloneResourceDecision::SKIP_NO_DONOR_RESOURCE;
    return plan;
}

ReverseCloneAssetResource ReverseCloneResourcePlanBuilder::ToResource(const FileInfo &fileInfo,
    int32_t absorbedFileId) const
{
    ReverseCloneAssetResource resource;
    resource.fileId = absorbedFileId;
    resource.cloudPath = fileInfo.cloudPath;
    resource.localRoot = RESTORE_FILES_LOCAL_DIR;
    resource.storagePath = fileInfo.storagePath;
    resource.inode = fileInfo.inode;
    resource.sourcePath = fileInfo.sourcePath;
    resource.fingerprint.cloudId = fileInfo.cloudUniqueId;
    resource.fingerprint.displayName = fileInfo.displayName;
    resource.fingerprint.fileSize = fileInfo.fileSize;
    resource.fingerprint.orientation = fileInfo.orientation;
    resource.fingerprint.fileType = fileInfo.fileType;
    resource.fileSourceType = fileInfo.fileSourceType;
    resource.subtype = fileInfo.subtype;
    resource.effectMode = fileInfo.effectMode;
    resource.dateTrashed = fileInfo.dateTrashed > 0 ? fileInfo.dateTrashed : fileInfo.recycledTime;
    resource.hidden = fileInfo.hidden;
    resource.dateModified = fileInfo.dateModified;
    resource.dateTaken = fileInfo.dateTaken;
    resource.thumbnailReady = fileInfo.thumbnailReady;
    resource.lcdVisitTime = fileInfo.lcdVisitTime;
    resource.position = fileInfo.position;
    int64_t int64Value = 0;
    if (GetInt64FromValMap(fileInfo, PhotoColumn::PHOTO_EDIT_TIME, int64Value)) {
        resource.editTime = int64Value;
    }
    if (GetInt64FromValMap(fileInfo, PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, int64Value)) {
        resource.realLcdVisitTime = int64Value;
    }
    if (GetInt64FromValMap(fileInfo, PhotoColumn::PHOTO_LCD_VISIT_COUNT, int64Value)) {
        resource.lcdVisitCount = static_cast<int32_t>(int64Value);
    }
    if (GetInt64FromValMap(fileInfo, PhotoColumn::PHOTO_LCD_FILE_SIZE, int64Value)) {
        resource.lcdFileSize = int64Value;
    }
    if (GetInt64FromValMap(fileInfo, PhotoColumn::PHOTO_THUMB_STATUS, int64Value)) {
        resource.thumbStatus = static_cast<int32_t>(int64Value);
    }
    GetStringFromValMap(fileInfo, PhotoColumn::PHOTO_LCD_SIZE, resource.lcdSize);
    GetStringFromValMap(fileInfo, PhotoColumn::PHOTO_THUMB_SIZE, resource.thumbSize);
    return resource;
}

bool ReverseCloneResourcePlanBuilder::HasLcdThumbnail(const ReverseCloneAssetResource &asset) const
{
    return asset.HasThumbnailCandidate();
}

bool ReverseCloneResourcePlanBuilder::HasThumbnail(const ReverseCloneAssetResource &asset) const
{
    return asset.HasThumbnailCandidate();
}

void ReverseCloneResourcePlanBuilder::FillCommonPlan(ReverseCloneResourcePlan &plan,
    const ReverseCloneAssetResource &absorbed, const ReverseCloneCandidate &candidate) const
{
    plan.absorbed = absorbed;
    plan.donor = candidate.donor;
    plan.matchType = candidate.matchType;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
