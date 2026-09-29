/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_MEDIA_CUSTOM_RESTORE_DAO_H
#define OHOS_MEDIA_MEDIA_CUSTOM_RESTORE_DAO_H

#include <string>

namespace OHOS::Media::Restore {
class MediaCustomRestoreDao {
public:
    MediaCustomRestoreDao() = default;
    ~MediaCustomRestoreDao() = default;

public:
    int32_t UpdatePhotos(const std::string &filePath, const int32_t livePhoto4dStatus);
};
}  // namespace OHOS::Media::Restore
#endif  // OHOS_MEDIA_MEDIA_CUSTOM_RESTORE_DAO_H