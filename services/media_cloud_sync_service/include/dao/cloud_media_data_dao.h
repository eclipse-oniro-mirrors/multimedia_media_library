/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_DAO_H

#include <string>
#include <vector>
#include <unordered_map>

#include "media_column.h"
#include "photos_dto.h"
#include "photos_vo.h"
#include "photos_po.h"
#include "photo_album_po.h"
#include "rdb_store.h"
#include "safe_map.h"
#include "result_set.h"
#include "medialibrary_db_const.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_file_utils.h"
#include "photo_album_dto.h"
#include "medialibrary_rdbstore.h"
#include "safe_vector.h"
#include "cloud_media_dao_const.h"
#include "aging_file_query_dto.h"
#include "datashare_values_bucket.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;

class CloudMediaDataDao {
public:
    CloudMediaDataDao() = default;
    ~CloudMediaDataDao() = default;

public:
    int32_t UpdateDirty(const std::string &cloudId, int32_t dirtyType);
    int32_t UpdatePosition(const std::vector<std::string> &cloudIds, int32_t position);
    int32_t UpdatePosWithType(const std::vector<std::string> &cloudIds,
        int32_t position, int32_t fileSourceType);
    int32_t UpdateFileSourceType(const std::vector<std::string> &cloudIds,
        int32_t fileSourceType);
    int32_t UpdateLocalFileDirty(std::string &cloudId);
    int32_t UpdateSyncStatus(const std::string &cloudId, int32_t syncStatus);
    int32_t UpdateThmStatus(const std::string &cloudId, int32_t thmStatus);
    int32_t GetAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosPo> &photosPos);
    int32_t GetActiveAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosPo> &photosPos);
    int32_t GetVideoToCache(std::vector<PhotosPo> &photosPos);
    int32_t QueryFilePosStat(const int32_t position, int &num);
    int32_t QueryCloudThmStat(const int32_t cloudThmStat, int &num);
    int32_t GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat);
    int32_t CheckAndDeleteAlbum();
    int32_t CheckAndUpdateAlbum();
    int32_t QueryDataFromPhotos(const DataShare::DataSharePredicates &predicates,
                                const std::vector<std::string> &columnNames, std::vector<PhotosPo> &photoInfos);
    int32_t QueryDataFromPhotoAlbums(const DataShare::DataSharePredicates &predicates,
                                     const std::vector<std::string> &columnNames,
                                     std::vector<PhotoAlbumPo> &photoAlbumInfos);
    int32_t UpdateDataInPhotos(const DataShare::DataSharePredicates &predicates,
                               const DataShare::DataShareValuesBucket &value);

private:
    int32_t QueryDirtyTypeStat(const int32_t dirtyType, int64_t &num);
    void InitDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat);
    int32_t GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat, const int32_t dirtyType);

private:
    const std::vector<std::string> COLUMNS_VIDEO_CACHE_QUERY = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::PHOTO_CLOUD_ID,
        PhotoColumn::MEDIA_SIZE,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_ORIENTATION,
    };
    const int32_t DIRTY_TYPE_STAT_SIZE = 9;

private:
    const std::string SQL_CHECK_AND_DELETE_ALBUM = "\
        DELETE FROM PhotoAlbum \
        WHERE COALESCE(cloud_id, '') = '' AND \
            dirty = 4;";
    const std::string SQL_CHECK_AND_UPDATE_ALBUM = "\
        UPDATE PhotoAlbum \
        SET dirty = 1 \
        WHERE \
            COALESCE(cloud_id, '') = '' AND \
            dirty IN (0, 2);";
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_DAO_H