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

#ifndef MEDIA_FILE_CHANGE_MANAGER_H
#define MEDIA_FILE_CHANGE_MANAGER_H

#include <stdint.h>
#include <memory>

#include "media_enable_shared_create.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaFileChangeManager : protected EnableSharedCreateInit<MediaFileChangeManager> {
public:
    EXPORT static std::shared_ptr<MediaFileChangeManager> GetInstance();
    int32_t Initialize();
    EXPORT void StartProcessChangeData();
    EXPORT void StopProcessChangeData();

protected:
    MediaFileChangeManager();
    ~MediaFileChangeManager();
};

}

#endif // MEDIA_FILE_CHANGE_MANAGER_H
