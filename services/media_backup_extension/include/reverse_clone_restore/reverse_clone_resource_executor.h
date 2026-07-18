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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESOURCE_EXECUTOR_H
#define OHOS_MEDIA_REVERSE_CLONE_RESOURCE_EXECUTOR_H

#include <memory>

#include "rdb_store.h"
#include "reverse_clone_resource_plan.h"
#include "backup_const.h"

namespace OHOS::Media {
class ReverseCloneResourceExecutor {
public:
    int32_t Execute(const ReverseCloneResourcePlan &plan,
        const std::shared_ptr<NativeRdb::RdbStore> &targetRdb,
        ReverseRestoreReportInfo &reportInfo) const;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_RESOURCE_EXECUTOR_H
