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

#ifndef OHOS_MEDIA_I_MEDIA_RDB_OPEN_EVENT_H
#define OHOS_MEDIA_I_MEDIA_RDB_OPEN_EVENT_H

#include <string>
#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
class IMediaRdbOpenEvent {
public:
    virtual int32_t OnCreate(std::shared_ptr<MediaLibraryRdbStore> store) = 0;
    virtual int32_t OnUpgrade(std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion) = 0;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_I_MEDIA_RDB_OPEN_EVENT_H
