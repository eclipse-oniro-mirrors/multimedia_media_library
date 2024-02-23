/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "dfx_timer.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "dfx_manager.h"
#include "medialibrary_bundle_manager.h"

namespace OHOS {
namespace Media {
DfxTimer::DfxTimer(const std::string &type, const std::string &object, int64_t timeOut, bool isReport)
{
    type_ = type;
    object_ = object;
    start_ = MediaFileUtils::UTCTimeMilliSeconds();
    timeOut_ = timeOut;
    isReport_ = isReport;
    isEnd_ = false;
}

DfxTimer::~DfxTimer()
{
    if (isEnd_) {
        return;
    }
    end_ = MediaFileUtils::UTCTimeMilliSeconds();
    if (end_ - start_ > timeOut_) {
        if (isReport_) {
            std::string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
            MEDIA_WARN_LOG("timeout! bundleName: %{public}s, type: %{public}s, object: %{public}s, cost %{public}lld",
                bundleName.c_str(), type_.c_str(), object_.c_str(), (long long) (end_ - start_));
            DfxManager::GetInstance()->HandleTimeOutOperation(bundleName, type_, object_, end_ - start_);
        } else {
            MEDIA_WARN_LOG("timeout! type: %{public}s, object: %{public}s, cost %{public}lld ms", type_.c_str(),
                object_.c_str(), (long long) (end_ - start_));
        }
    }
}

void DfxTimer::End()
{
    end_ = MediaFileUtils::UTCTimeMilliSeconds();
    if (end_ - start_ > timeOut_) {
        MEDIA_WARN_LOG("timeout! type: %{public}s, object: %{public}s, cost %{public}lld ms", type_.c_str(),
            object_.c_str(), (long long) (end_ - start_));
    }
}

} // namespace Media
} // namespace OHOS