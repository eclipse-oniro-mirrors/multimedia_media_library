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

#ifndef MEDIALIBRARY_NOTIFY_MULTISTAGES_CAPTURE_LOW_QUALITY_MEMORY_NUM_OBSERVER_H
#define MEDIALIBRARY_NOTIFY_MULTISTAGES_CAPTURE_LOW_QUALITY_MEMORY_NUM_OBSERVER_H

#include "low_quality_memory_num_notify_info.h"
#include "medialibrary_notify_user_define_observer.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace Notification;
class LowQualityMemoryNumObserver : public MediaOnNotifyUserDefineObserverBodyBase {
public:
    LowQualityMemoryNumObserver() {}
    virtual ~LowQualityMemoryNumObserver() = default;

    void OnChange(const UserDefineCallbackWrapper &callbackWrapper) override;
    std::string ToString() const override
    {
        return "";
    }

private:
    std::shared_ptr<LowQualityMemoryNumNotifyInfo> ConvertWrapperToNotifyInfo(
        const UserDefineCallbackWrapper &callbackWrapper);
};

} // Media
} // OHOS
#endif // MEDIALIBRARY_NOTIFY_MULTISTAGES_CAPTURE_LOW_QUALITY_MEMORY_NUM_OBSERVER_H