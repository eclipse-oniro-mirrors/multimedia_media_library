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

#ifndef OHOS_MEDIA_PHOTO_MIMETYPE_OPERATION_H
#define OHOS_MEDIA_PHOTO_MIMETYPE_OPERATION_H

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
class PhotoMimetypeOperation {
public:
    static int32_t UpdateInvalidMimeType();

private:
    static int32_t HandleUpdateInvalidMimeType(const std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::unordered_map<std::string, std::vector<std::string>> &invalidMimeTypeMap);
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_MIMETYPE_OPERATION_H