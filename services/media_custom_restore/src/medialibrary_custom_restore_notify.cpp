/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "CustomRestoreNotify"

#include "medialibrary_custom_restore_notify.h"

#include "dataobs_mgr_client.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
const std::string CustomRestoreNotify::NOTIFY_URI_PREFIX = "file://media/custom_restore/";
int32_t CustomRestoreNotify::Notify(std::string keyPath, const InnerRestoreResult &restoreResult)
{
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        MEDIA_ERR_LOG("obsMgrClient is nullptr");
        return E_DATA_OBS_MGR_CLIENT_IS_NULL;
    }
    AAFwk::ChangeInfo::VBucket vBucket;
    vBucket["stage"] = restoreResult.stage;
    vBucket["errCode"] = restoreResult.errCode;
    vBucket["progress"] = restoreResult.progress;
    vBucket["uriType"] = restoreResult.uriType;
    vBucket["uri"] = restoreResult.uri;
    vBucket["totalNum"] = restoreResult.totalNum;
    vBucket["successNum"] = restoreResult.successNum;
    vBucket["failedNum"] = restoreResult.failedNum;
    vBucket["sameNum"] = restoreResult.sameNum;
    vBucket["cancelNum"] = restoreResult.cancelNum;
    // notify callback
    Uri customRestoreUri(NOTIFY_URI_PREFIX + keyPath);
    AAFwk::ChangeInfo changeInfo = {
        AAFwk::ChangeInfo::ChangeType::INSERT, { customRestoreUri }, nullptr, 0, { vBucket }
    };
    int result = obsMgrClient->NotifyChangeExt(changeInfo);
    MEDIA_DEBUG_LOG("NotifyChangeExt result: %{public}d", result);
    return result;
}

} // namespace OHOS::Media
