/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "AlbumDao"

#include "album_dao.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "photo_album_column.h"
#include "values_bucket.h"

using namespace OHOS::NativeRdb;
namespace OHOS::Media {
AlbumDao::AlbumDao(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore) : rdbStore_(rdbStore) {}

AlbumDao::~AlbumDao() = default;

int32_t AlbumDao::QueryAlbumIdByLPath(const std::string &lowerLPath, int32_t &albumId, bool &found)
{
    found = false;
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");
    std::string querySql = "SELECT ALBUM_ID FROM PhotoAlbum WHERE LOWER(lpath) = ?";
    std::vector<std::string> bindArgs = { lowerLPath };
    auto resultSet = rdbStore_->QuerySql(querySql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        int32_t id = 0;
        if (resultSet->GetInt(0, id) == NativeRdb::E_OK) {
            albumId = id;
            found = true;
        }
    }
    resultSet->Close();
    return E_OK;
}

int32_t AlbumDao::InsertAlbum(NativeRdb::ValuesBucket &values, int64_t &rowId)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore_ != nullptr, E_HAS_DB_ERROR, "RdbStore is nullptr");
    return rdbStore_->Insert(rowId, PhotoAlbumColumns::TABLE, values);
}
} // namespace OHOS::Media
