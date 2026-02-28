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
#ifndef MEDIA_LAKE_CHECK_MANAGER_H
#define MEDIA_LAKE_CHECK_MANAGER_H

#include <memory>
#include <mutex>

#include "media_enable_shared_create.h"

namespace OHOS::Media {
class MediaInLakeCheckManager : protected EnableSharedCreate<MediaInLakeCheckManager> {
public:
    static std::shared_ptr<MediaInLakeCheckManager> GetInstance();
    void Start();
    void Stop();

protected:
    MediaInLakeCheckManager() = default;
    ~MediaInLakeCheckManager() = default;
    MediaInLakeCheckManager(const MediaInLakeCheckManager&) = delete;
    const MediaInLakeCheckManager& operator=(const MediaInLakeCheckManager&) = delete;
};
}
#endif // MEDIA_LAKE_CHECK_MANAGER_H
