/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "medialibrary_smartalbum_map_db.h"
#include "media_log.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
int64_t MediaLibrarySmartAlbumMapDb::InsertSmartAlbumMapInfo(const DataShareValuesBucket &values,
                                                             const shared_ptr<RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, ALBUM_OPERATION_ERR, "Invalid RDB store");
    int64_t outRowId(0);
    int32_t insertResult = rdbStore->Insert(outRowId, SMARTALBUM_MAP_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(insertResult == E_OK, ALBUM_OPERATION_ERR, "Insert failed");
    return outRowId;
}
int32_t MediaLibrarySmartAlbumMapDb::DeleteSmartAlbumMapInfo(const int32_t albumId,
                                                             const int32_t assetId,
                                                             const shared_ptr<RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG((rdbStore != nullptr) && (albumId > 0) && (assetId > 0),
        ALBUM_OPERATION_ERR, "Invalid input");
    int32_t deletedRows(ALBUM_OPERATION_ERR);
    vector<string> whereArgs = { std::to_string(albumId), std::to_string(assetId)};
    int32_t deleteResult = rdbStore->Delete(deletedRows, SMARTALBUM_MAP_TABLE, SMARTALBUM_MAP_DB_COND, whereArgs);
    CHECK_AND_RETURN_RET_LOG(deleteResult == E_OK, ALBUM_OPERATION_ERR, "Delete failed");
    return (deletedRows > 0) ? DATA_ABILITY_SUCCESS : DATA_ABILITY_FAIL;
}
int32_t MediaLibrarySmartAlbumMapDb::DeleteAllSmartAlbumMapInfo(const int32_t albumId,
                                                                const shared_ptr<RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG((rdbStore != nullptr) && (albumId > 0), ALBUM_OPERATION_ERR, "Invalid input");
    int32_t deletedRows(ALBUM_OPERATION_ERR);
    vector<string> whereArgs = { std::to_string(albumId)};
    int32_t deleteResult = rdbStore->Delete(deletedRows,
        SMARTALBUM_MAP_TABLE, SMARTALBUM_MAP_DE_SMARTALBUM_COND, whereArgs);
    CHECK_AND_RETURN_RET_LOG(deleteResult == E_OK, ALBUM_OPERATION_ERR, "Delete failed");
    return (deletedRows > 0) ? DATA_ABILITY_SUCCESS : DATA_ABILITY_FAIL;
}
int32_t MediaLibrarySmartAlbumMapDb::DeleteAllAssetsMapInfo(const int32_t assetId,
                                                            const shared_ptr<RdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG((rdbStore != nullptr) && (assetId > 0), ALBUM_OPERATION_ERR, "Invalid input");
    int32_t deletedRows(ALBUM_OPERATION_ERR);
    vector<string> whereArgs = { std::to_string(assetId)};
    int32_t deleteResult = rdbStore->Delete(deletedRows, SMARTALBUM_MAP_TABLE,
        SMARTALBUM_MAP_DE_ASSETS_COND, whereArgs);
    CHECK_AND_RETURN_RET_LOG(deleteResult == E_OK, ALBUM_OPERATION_ERR, "Delete failed");
    return (deletedRows > 0) ? DATA_ABILITY_SUCCESS : DATA_ABILITY_FAIL;
}
}  // namespace Media
}  // namespace OHOS
