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

#ifndef OHOS_MEDIA_TABLE_EVENT_HANDLER_H
#define OHOS_MEDIA_TABLE_EVENT_HANDLER_H

#include <string>

#include "rdb_store.h"
#include "i_media_rdb_open_event.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT TableEventHandler : public IMediaRdbOpenEvent {
public:  // constructors
    TableEventHandler();
    virtual ~TableEventHandler() = default;

public:
    int32_t OnCreate(std::shared_ptr<MediaLibraryRdbStore> store) override;
    int32_t OnUpgrade(std::shared_ptr<MediaLibraryRdbStore> store, int32_t oldVersion, int32_t newVersion) override;

private:  // private members
    std::vector<std::shared_ptr<IMediaRdbOpenEvent>> handlers_;

private:  // private definitions
    enum {
        E_OK = 0,
    };
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_TABLE_EVENT_HANDLER_H