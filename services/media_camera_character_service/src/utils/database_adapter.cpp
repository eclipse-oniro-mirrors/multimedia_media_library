/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "DatabaseAdapter"

#include "database_adapter.h"

#include "medialibrary_unistore_manager.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
std::shared_ptr<NativeRdb::ResultSet> DatabaseAdapter::Query(MediaLibraryCommand &cmd,
    const std::vector<std::string> &columns)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return nullptr;
    }

    return uniStore->Query(cmd, columns);
}

int32_t DatabaseAdapter::Update(MediaLibraryCommand &cmd)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return E_HAS_DB_ERROR;
    }

    int32_t changedRows = E_ERR;
    auto ret = uniStore->Update(cmd, changedRows);
    if (ret != NativeRdb::E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update DB failed. ret: %{public}d", ret);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}
} // namespace Media
} // namespace OHOS