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

bool HasMovingPhotoVideo(const ReverseCloneAssetResource &resource)
{
    return resource.subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        resource.effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY);
}

bool ShouldBlockMovingPhotoOrigin(const ReverseCloneAssetResource &target,
    const ReverseCloneAssetResource &source)
{
    // 旧机提供动态照片，新机目标是湖内资产。
    if (HasMovingPhotoVideo(source) && !target.storagePath.empty()) {
        return true;
    }

    // 旧机提供湖内资源，新机目标是动态照片。
    if (source.IsLakeAsset() && HasMovingPhotoVideo(target)) {
        return true;
    }

    return false;
}
} // namespace

// LCOV_EXCL_START
ReverseCloneResourcePlan ReverseCloneResourcePlanBuilder::Build(const FileInfo &absorbedFile,
    const ReverseCloneCandidate &candidate, int32_t absorbedFileId) const
{
    ReverseCloneResourcePlan plan;
    plan.absorbed = ToResource(absorbedFile, absorbedFileId);
    if (!candidate.IsFound()) {
        return plan;
    }
    plan.donor = candidate.donor;
    plan.matchType = candidate.matchType;
    if (candidate.matchType == ReverseCloneMatchType::SAME_CLOUD_CONFLICT) {
        plan.decision = ReverseCloneResourceDecision::SKIP_CLOUD_VERSION_CONFLICT;
        return plan;
    }
    if (!candidate.CanInheritResource()) {
        return plan;
    }
    if (!candidate.donor.HasResourcePath()) {
        plan.decision = ReverseCloneResourceDecision::SKIP_NO_DONOR_RESOURCE;
        return plan;
    }
    plan.blockOriginInheritance = ShouldBlockMovingPhotoOrigin(plan.absorbed, plan.donor);
    FillResourceActions(plan);
    return plan;
}

ReverseCloneResourcePlan ReverseCloneResourcePlanBuilder::BuildFromSource(const FileInfo &sourceFile,
    const std::string &sourceRoot, const std::string &sourceOriginPath, int32_t absorbedFileId) const
{
    ReverseCloneResourcePlan plan;
    plan.absorbed = ToResource(sourceFile, absorbedFileId);
    plan.donor = plan.absorbed;
    plan.donor.localRoot = sourceRoot;
    plan.donor.originPath = sourceOriginPath;
    plan.donor.relativePath = sourceFile.relativePath;
    plan.matchType = ReverseCloneMatchType::SOURCE_ASSET;
    FillResourceActions(plan);
    return plan;
}

void ReverseCloneResourcePlanBuilder::FillResourceActions(ReverseCloneResourcePlan &plan) const
{
    plan.inheritOrigin = !plan.blockOriginInheritance && plan.donor.HasOriginCandidate();
    plan.inheritLcdThumbnail = plan.donor.HasThumbnailCandidate();
    plan.inheritThumbnail = plan.inheritLcdThumbnail;
    plan.decision = plan.HasResourceAction() ? ReverseCloneResourceDecision::INHERIT :
        ReverseCloneResourceDecision::SKIP_NO_DONOR_RESOURCE;
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
    resource.lcdUsingStatus = fileInfo.lcdUsingStatus;
    resource.compositeDisplayStatus = fileInfo.compositeDisplayStatus;
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
    if (GetInt64FromValMap(fileInfo, PhotoColumn::PHOTO_CE_AVAILABLE, int64Value)) {
        resource.ceAvailable = static_cast<int32_t>(int64Value);
    }
    GetStringFromValMap(fileInfo, PhotoColumn::PHOTO_LCD_SIZE, resource.lcdSize);
    GetStringFromValMap(fileInfo, PhotoColumn::PHOTO_THUMB_SIZE, resource.thumbSize);
    return resource;
}

// LCOV_EXCL_STOP
} // namespace OHOS::Media
