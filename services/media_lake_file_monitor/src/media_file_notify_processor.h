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

#ifndef MEDIA_FILE_NOTIFY_PROCESSOR_H
#define MEDIA_FILE_NOTIFY_PROCESSOR_H

#include <cstdint>
#include <string>

#include "media_enable_shared_create.h"
#include "media_lake_notify_info.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore.h"

namespace OHOS::Media {
class MediaFileNotifyProcessor : protected EnableSharedCreateInit<MediaFileNotifyProcessor> {
public:
    static std::shared_ptr<MediaFileNotifyProcessor> GetInstance();

    int32_t Initialize()
    {
        InitializeRdb();
        RegisterAllProcessorsOnce();
        return 0;
    }
    int32_t ProcessNotification(const MediaLakeNotifyInfo &notifyInfo);

protected:
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;

    void RegisterAllProcessorsOnce();
    void InitializeRdb();

    MediaFileNotifyProcessor() = default;
    ~MediaFileNotifyProcessor() = default;

    MediaFileNotifyProcessor(const MediaFileNotifyProcessor&) = delete;
    MediaFileNotifyProcessor& operator=(const MediaFileNotifyProcessor&) = delete;
};
}
#endif // MEDIA_FILE_NOTIFY_PROCESSOR_H
