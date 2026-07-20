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

#define MLOG_TAG "PortraitFriendIdService"

#include "portrait_friend_id_service.h"

#include <unordered_set>

#include "analysis_album_attribute_const.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "portrait_friend_id_repository.h"

namespace OHOS::Media {
int32_t PortraitFriendIdService::SetOperate(const std::string &albumId,
    const std::vector<std::string> &friendIds, const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    PortraitFriendIdRepository repository(rdbStore);
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is nullptr");
    CHECK_AND_RETURN_RET_LOG(repository.Exists(albumId), E_HAS_DB_ERROR, "portrait album not found");
    CHECK_AND_RETURN_RET_LOG(!friendIds.empty(), E_INVALID_VALUES, "friendIds is empty");
    const std::string friendId = friendIds[0];
    return repository.UpdateFriendId(albumId, friendId);
}

int32_t PortraitFriendIdService::GetOperate(const int32_t &albumId, std::string &friendId,
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    PortraitFriendIdRepository repository(rdbStore);
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "rdbStore is nullptr");
    CHECK_AND_RETURN_RET_LOG(repository.Exists(std::to_string(albumId)), E_INNER_FAIL, "portrait album not found");
    return repository.GetFriendId(albumId, friendId);
}
} // namespace OHOS::Media
