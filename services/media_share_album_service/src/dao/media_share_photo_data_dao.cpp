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

#define MLOG_TAG "Media_Dao"

#include "media_share_photo_data_dao.h"

#include <vector>

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"

namespace OHOS::Media::ShareAlbum {

int32_t MediaSharePhotoDataDao::GetShareAlbumOwnerId(const std::string &data, std::vector<PhotosPo> &photosPos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "GetShareAlbumOwnerId Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, data);
    std::vector<std::string> columns = { PhotoColumn::PHOTO_SHARE_ALBUM_OWNER,
        MediaColumn::MEDIA_HIDDEN, MediaColumn::MEDIA_DATE_TRASHED, PhotoColumn::PHOTO_IS_SHARED };

    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "GetShareAlbumOwnerId Failed to query.");

    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetShareAlbumOwnerId ReadRecords failed, ret = %{public}d", ret);

    MEDIA_INFO_LOG("GetShareAlbumOwnerId data:%{public}s, count:%{public}zu",
        MediaFileUtils::DesensitizePath(data).c_str(), photosPos.size());
    return E_OK;
}

}  // namespace OHOS::Media::ShareAlbum
