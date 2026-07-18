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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESOURCE_INHERIT_SERVICE_H
#define OHOS_MEDIA_REVERSE_CLONE_RESOURCE_INHERIT_SERVICE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rdb_store.h"
#include "reverse_clone_resource_plan.h"
#include "backup_const.h"

namespace OHOS::Media {
class ReverseCloneResourceInheritService {
public:
    ReverseCloneResourcePlan BuildDuplicatePlanByFileId(const FileInfo &absorbedFile, int32_t duplicateFileId,
        const std::shared_ptr<NativeRdb::RdbStore> &donorRdb,
        const std::unordered_set<int32_t> &originalPureCloudFileIds) const;
    ReverseCloneResourcePlan BuildSourcePlanAfterPrepare(const FileInfo &sourceFile) const;
    void MergeDuplicatePlansWithSourceFallback(const std::vector<ReverseCloneResourcePlan> &duplicatePlans,
        std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans) const;
    int32_t ExecuteAfterInsert(const ReverseCloneResourcePlan &plan,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
        ReverseRestoreReportInfo &reportInfo) const;

private:
    std::string GetSourceResourceRoot() const;
    std::string GetSourceOriginPath(const FileInfo &sourceFile) const;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_RESOURCE_INHERIT_SERVICE_H
