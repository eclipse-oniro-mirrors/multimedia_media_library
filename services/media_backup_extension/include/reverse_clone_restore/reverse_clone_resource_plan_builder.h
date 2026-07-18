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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESOURCE_PLAN_BUILDER_H
#define OHOS_MEDIA_REVERSE_CLONE_RESOURCE_PLAN_BUILDER_H

#include "reverse_clone_resource_plan.h"

namespace OHOS::Media {
class ReverseCloneResourcePlanBuilder {
public:
    ReverseCloneResourcePlan Build(const FileInfo &absorbedFile, const ReverseCloneCandidate &candidate,
        int32_t absorbedFileId) const;
    ReverseCloneResourcePlan BuildFromSource(const FileInfo &sourceFile, const std::string &sourceRoot,
        const std::string &sourceOriginPath, int32_t absorbedFileId) const;

private:
    ReverseCloneResourcePlan BuildInheritPlan(const ReverseCloneAssetResource &absorbed,
        const ReverseCloneCandidate &candidate) const;
    ReverseCloneResourcePlan BuildInheritPlan(const ReverseCloneAssetResource &absorbed,
        const ReverseCloneAssetResource &donor, ReverseCloneMatchType matchType) const;
    ReverseCloneAssetResource ToResource(const FileInfo &fileInfo, int32_t absorbedFileId) const;
    bool HasLcdThumbnail(const ReverseCloneAssetResource &asset) const;
    bool HasThumbnail(const ReverseCloneAssetResource &asset) const;
    void FillCommonPlan(ReverseCloneResourcePlan &plan, const ReverseCloneAssetResource &absorbed,
        const ReverseCloneCandidate &candidate) const;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_RESOURCE_PLAN_BUILDER_H
