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
#ifndef MEDIA_LIBRARY_I_PROCESSOR_H
#define MEDIA_LIBRARY_I_PROCESSOR_H

#include <string>
#include <vector>

#include "media_lake_notify_info.h"

namespace OHOS {
namespace Media {
class IProcessor {
public:
    virtual ~IProcessor() = default;
    virtual void Process(const MediaLakeNotifyInfo &notifyInfo) = 0;
    virtual void Process(const std::vector<MediaLakeNotifyInfo> &notifyInfos) {}
    virtual bool IsComposite() const
    {
        return false;
    }
};
}
}

#endif // MEDIA_LIBRARY_I_PROCESSOR_H