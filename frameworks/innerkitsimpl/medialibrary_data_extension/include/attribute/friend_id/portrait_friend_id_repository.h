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

#ifndef OHOS_MEDIA_PORTRAIT_FRIEND_ID_REPOSITORY_H
#define OHOS_MEDIA_PORTRAIT_FRIEND_ID_REPOSITORY_H

#include <memory>
#include <string>
#include <vector>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
class PortraitFriendIdRepository {
public:
    explicit PortraitFriendIdRepository(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore);

    bool Exists(const std::string &albumId) const;
    int32_t GetFriendId(const int32_t &albumId, std::string &friendId) const;
    int32_t UpdateFriendId(const std::string &albumId, const std::string &friendId) const;
private:
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_PORTRAIT_FRIEND_ID_REPOSITORY_H
