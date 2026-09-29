/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaPermissionCheck"

#include "media_analysis_permission_check.h"
#include "permission_utils.h"

namespace OHOS::Media {
int32_t AnalysisPermissionCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("AnalysisPermissionCheck enter, API code=%{public}d", businessCode);
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::CheckCallerPermission("ohos.permission.CONTROL_IMAGEVIDEO_ANALYSIS"),
        E_PERMISSION_DENIED, "AnalysisPermissionCheck failed: lack of CONTROL_IMAGEVIDEO_ANALYSIS permission");
    return E_SUCCESS;
}
} // namespace OHOS::Media
