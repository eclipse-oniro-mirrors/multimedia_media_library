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

#include "medialibrary_smartalbum_operations.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
constexpr int32_t DEFAULT_SMARTID = -1;
int32_t MediaLibrarySmartAlbumOperations::CreateSmartAlbumOperation(MediaLibraryCommand &cmd)
{
    ValueObject valueObject;
    if (!cmd.GetValueBucket().GetObject(SMARTALBUMMAP_DB_ALBUM_ID, valueObject)) {
        MEDIA_ERR_LOG("Failed to get parentAlbumId");
        return E_HAS_DB_ERROR;
    }
    int32_t parentAlbumId = DEFAULT_SMARTID;
    valueObject.GetInt(parentAlbumId);
    if (parentAlbumId > 0 && !MediaLibraryObjectUtils::IsSmartAlbumExistInDb(parentAlbumId)) {
        MEDIA_ERR_LOG("Failed to get parent smart album, parentAlbumId = %{public}d", parentAlbumId);
        return E_PARENT_SMARTALBUM_IS_NOT_EXISTED;
    }
    if (!cmd.GetValueBucket().GetObject(SMARTALBUM_DB_NAME, valueObject)) {
        MEDIA_ERR_LOG("Failed to get albumName");
        return E_HAS_DB_ERROR;
    }
    string albumName;
    valueObject.GetString(albumName);
    if (MediaFileUtils::CheckAlbumName(albumName) < 0) {
        MEDIA_ERR_LOG("Smart album invalid, albumName = %{private}s", albumName.c_str());
        return E_INVALID_VALUES;
    }
    ValuesBucket valuebucket;
    valuebucket.PutString(SMARTALBUM_DB_NAME, albumName);
    cmd.SetValueBucket(valuebucket);
    return MediaLibraryObjectUtils::InsertInDb(cmd);
}

int32_t MediaLibrarySmartAlbumOperations::DeleteSmartAlbumOperation(MediaLibraryCommand &cmd)
{
    ValueObject valueObject;
    if (!cmd.GetValueBucket().GetObject(SMARTALBUM_DB_ID, valueObject)) {
        MEDIA_ERR_LOG("Failed to get albumId");
        return E_HAS_DB_ERROR;
    }
    int32_t albumId = DEFAULT_SMARTID;
    valueObject.GetInt(albumId);
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_GET_VALUEBUCKET_FAIL, "Failed to get smartAlbumId");
    if (MediaLibraryObjectUtils::IsParentSmartAlbum(albumId)) {
        return E_PARENT_SMARTALBUM_CAN_NOT_DELETE;
    }
    MediaLibraryCommand smartAlbumMapCmd(OperationObject::SMART_ALBUM_MAP, OperationType::DELETE);
    smartAlbumMapCmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUMMAP_DB_ALBUM_ID, to_string(albumId))->Or()->
        EqualTo(SMARTALBUMMAP_DB_CHILD_ALBUM_ID, to_string(albumId));
    int32_t errCode = MediaLibraryObjectUtils::DeleteInfoByIdInDb(smartAlbumMapCmd);
    CHECK_AND_RETURN_RET_LOG(errCode > 0, E_DELETE_SMARTALBUM_MAP_FAIL, "Failed to delete smartAlbumMap");
    MediaLibraryCommand smartAlbumCmd(OperationObject::SMART_ALBUM, OperationType::DELETE);
    smartAlbumCmd.GetAbsRdbPredicates()->EqualTo(SMARTALBUM_DB_ID, to_string(albumId));
    return MediaLibraryObjectUtils::DeleteInfoByIdInDb(smartAlbumCmd);
}

int32_t MediaLibrarySmartAlbumOperations::HandleSmartAlbumOperation(MediaLibraryCommand &cmd)
{
    int32_t errCode = E_ERR;
    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            errCode = CreateSmartAlbumOperation(cmd);
            break;
        case OperationType::DELETE:
            errCode = DeleteSmartAlbumOperation(cmd);
            break;
        default:
            MEDIA_WARN_LOG("Unknown operation type %{private}d", cmd.GetOprnType());
            break;
        }
    return errCode;
}
} // namespace Media
} // namespace OHOS