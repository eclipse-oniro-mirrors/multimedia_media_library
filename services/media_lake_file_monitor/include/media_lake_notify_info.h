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
#ifndef MEDIA_LAKE_NOTIFY_INFO_H
#define MEDIA_LAKE_NOTIFY_INFO_H

namespace OHOS {
namespace Media {
enum class FileNotifyObjectType {
    UNDEFINED = -1,
    FILE = 0,
    DIRECTORY
};

enum class FileNotifyOperationType {
    UNDEFINED = -1,
    ADD = 0,
    MOD = 1,
    DEL = 2
};

struct MediaLakeNotifyInfo {
    std::string beforePath;
    std::string afterPath;
    FileNotifyObjectType objType { FileNotifyObjectType::UNDEFINED };
    FileNotifyOperationType optType { FileNotifyOperationType::UNDEFINED };
};
}
}

#endif // MEDIA_LAKE_NOTIFY_INFO_H