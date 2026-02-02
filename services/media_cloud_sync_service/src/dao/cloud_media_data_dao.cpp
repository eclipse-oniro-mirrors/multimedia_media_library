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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_data_dao.h"

#include "media_file_utils.h"
#include "cloud_media_file_utils.h"
#include "cloud_media_dao_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "photo_album_po_writer.h"
#include "asset_accurate_refresh.h"
#include "refresh_business_name.h"
#include "rdb_utils.h"

using namespace OHOS::RdbDataShareAdapter;

namespace OHOS::Media::CloudSync {
int32_t CloudMediaDataDao::UpdateDirty(const std::string &cloudId, int32_t dirtyType)
{
    MEDIA_INFO_LOG("enter UpdateDirty, cloudId: %{public}s, dirtyType: %{public}d", cloudId.c_str(), dirtyType);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateDirty Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, dirtyType);

    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateDirty, ret: %{public}d", ret);
    CHECK_AND_PRINT_LOG(changedRows > 0, "UpdateDirty Check updateRows: %{public}d.", changedRows);
    return ret;
}

int32_t CloudMediaDataDao::UpdatePosition(const std::vector<std::string> &cloudIds, int32_t position)
{
    MEDIA_INFO_LOG("enter UpdatePosition, cloudIds size: %{public}zu, position: %{public}d", cloudIds.size(), position);

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, position);
    if (position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    }

    int32_t changedRows = DEFAULT_VALUE;
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_POSITION_BUSSINESS_NAME);
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret,
        "Failed to UpdatePosition, ret: %{public}d.", ret);
    CHECK_AND_PRINT_LOG(changedRows > 0, "UpdatePosition Check updateRows: %{public}d.", changedRows);
    assetRefresh.Notify();
    return ret;
}

int32_t CloudMediaDataDao::UpdatePosWithType(const std::vector<std::string> &cloudIds,
    int32_t position, int32_t fileSourceType)
{
    MEDIA_INFO_LOG("enter UpdatePosWithType, cloudIds size: %{public}zu, position: %{public}d,"
        "fileSourceType: %{public}d",
        cloudIds.size(), position, fileSourceType);

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, position);
    if (position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
        values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    }
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, fileSourceType);

    int32_t changedRows = DEFAULT_VALUE;
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_POSITION_BUSSINESS_NAME);
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret,
        "Failed to UpdatePosWithType, ret: %{public}d.", ret);
    CHECK_AND_PRINT_LOG(changedRows > 0, "UpdatePosWithType Check updateRows: %{public}d.", changedRows);
    assetRefresh.Notify();
    return ret;
}

int32_t CloudMediaDataDao::UpdateFileSourceType(const std::vector<std::string> &cloudIds,
    int32_t fileSourceType)
{
    MEDIA_INFO_LOG("enter UpdatePosWithType, cloudIds size: %{public}zu, fileSourceType: %{public}d",
        cloudIds.size(), fileSourceType);

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, fileSourceType);

    int32_t changedRows = DEFAULT_VALUE;
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_POSITION_BUSSINESS_NAME);
    int32_t ret = assetRefresh.Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret,
        "Failed to UpdateFileSourceType, ret: %{public}d.", ret);
    CHECK_AND_PRINT_LOG(changedRows > 0, "UpdateFileSourceType Check updateRows: %{public}d.", changedRows);
    assetRefresh.Notify();
    return ret;
}

int32_t CloudMediaDataDao::UpdateSyncStatus(const std::string &cloudId, int32_t syncStatus)
{
    MEDIA_INFO_LOG("enter UpdateSyncStatus, cloudId: %{public}s, syncStatus: %{public}d", cloudId.c_str(), syncStatus);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateSyncStatus Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, syncStatus);

    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateSyncStatus, ret: %{public}d.", ret);
    CHECK_AND_PRINT_LOG(changedRows > 0, "UpdateSyncStatus Check updateRows: %{public}d.", changedRows);
    return ret;
}

int32_t CloudMediaDataDao::UpdateThmStatus(const std::string &cloudId, int32_t thmStatus)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, thmStatus);

    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateThmStatusForCloudCheck, ret: %{public}d", ret);
    CHECK_AND_PRINT_LOG(changedRows > 0, "Check updateRows: %{public}d.", changedRows);
    return ret;
}

int32_t CloudMediaDataDao::QueryFilePosStat(const int32_t position, int &num)
{
    MEDIA_INFO_LOG("enter QueryFilePosStat, position: %{public}d", position);
    num = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryFilePosStat Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_POSITION, std::to_string(position));
    std::vector<std::string> queryColums = {"COUNT(1) AS count"};
    auto resultSet = rdbStore->Query(queryPredicates, queryColums);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "resultSet is null");
    bool isValid = resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_EXECUTE(isValid, resultSet->Close());
    CHECK_AND_RETURN_RET_LOG(isValid, E_RDB, "failed to get row");
    num = GetInt32Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("QueryFilePosStat end %{public}d", num);
    return E_OK;
}

int32_t CloudMediaDataDao::QueryCloudThmStat(const int32_t cloudThmStat, int &num)
{
    MEDIA_INFO_LOG("enter QueryCloudThmStat, cloudThmStat: %{public}d", cloudThmStat);
    num = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "QueryCloudThmStat Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, cloudThmStat)
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))
        ->EndWrap();
    std::vector<std::string> queryColums = {"COUNT(1) AS count"};
    auto resultSet = rdbStore->Query(queryPredicates, queryColums);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RDB, "resultSet is null");
    bool isValid = resultSet->GoToFirstRow() == NativeRdb::E_OK;
    CHECK_AND_EXECUTE(isValid, resultSet->Close());
    CHECK_AND_RETURN_RET_LOG(isValid, E_RDB, "failed to get row");
    num = GetInt32Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("QueryCloudThmStat end %{public}d", num);
    return E_OK;
}

int32_t CloudMediaDataDao::QueryDirtyTypeStat(const int32_t dirtyType, int64_t &num)
{
    num = 0;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_DIRTY, std::to_string(dirtyType));
    vector<string> queryColums = {"COUNT(1) AS count"};
    auto resultSet = rdbStore->Query(queryPredicates, queryColums);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("resultSet is null or failed to get row");
        return E_RDB;
    }
    num = GetInt64Val("count", resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("QueryDirtyTypeStat, dirtyType: %{public}d, num: %{public}" PRIu64, dirtyType, num);
    return E_OK;
}

void CloudMediaDataDao::InitDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat)
{
    dirtyTypeStat.clear();
    for (int32_t i = 0; i < DIRTY_TYPE_STAT_SIZE; i++) {
        dirtyTypeStat.push_back(0);
    }
    return;
}

int32_t CloudMediaDataDao::GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat, const int32_t dirtyType)
{
    CHECK_AND_RETURN_RET_LOG(0 <= dirtyType && dirtyType < DIRTY_TYPE_STAT_SIZE,
        E_INVALID_VALUES,
        "dirtyType is invalid, dirtyType: %{public}d",
        dirtyType);
    CHECK_AND_EXECUTE(
        static_cast<int32_t>(dirtyTypeStat.size()) == DIRTY_TYPE_STAT_SIZE, InitDirtyTypeStat(dirtyTypeStat));
    int64_t num = 0;
    int32_t ret = this->QueryDirtyTypeStat(dirtyType, num);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "Failed to GetDirtyTypeStat, dirtyType: %{public}d, ret: %{public}d", dirtyType, ret);
    dirtyTypeStat[dirtyType] = static_cast<uint64_t>(num);
    return E_OK;
}

int32_t CloudMediaDataDao::GetDirtyTypeStat(std::vector<uint64_t> &dirtyTypeStat)
{
    int32_t ret = E_OK;
    for (int32_t i = 0; i < DIRTY_TYPE_STAT_SIZE; i++) {
        ret = this->GetDirtyTypeStat(dirtyTypeStat, i);
        CHECK_AND_RETURN_RET_LOG(
            ret == E_OK, ret, "Failed to GetDirtyTypeStat, ret: %{public}d, dirtyType: %{public}d", ret, i);
    }
    MEDIA_INFO_LOG(
        "GetDirtyTypeStat, dirtyTypeStat: %{public}s", CloudMediaDaoUtils::VectorToString(dirtyTypeStat).c_str());
    return E_OK;
}

int32_t CloudMediaDataDao::GetVideoToCache(std::vector<PhotosPo> &photosPos)
{
    MEDIA_INFO_LOG("GetVideoToCache begin");
    std::vector<PhotosDto> photosDtoVec;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_TYPE, static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO));
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    predicates.OrderByDesc(PhotoColumn::MEDIA_DATE_TAKEN);
    predicates.Limit(CACHE_VIDEO_NUM);
    auto resultSet = rdbStore->Query(predicates, this->COLUMNS_VIDEO_CACHE_QUERY);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");

    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetVideoToCache Failed to query, ret: %{public}d", ret);
    return E_OK;
}

int32_t CloudMediaDataDao::GetAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosPo> &photosPos)
{
    int64_t time = queryDto.time;
    int32_t mediaType = queryDto.mediaType;
    int32_t sizeLimit = queryDto.sizeLimit;
    auto currentTime = MediaFileUtils::UTCTimeSeconds();
    MEDIA_INFO_LOG("enter GetAgingFile, queryDto: %{public}s", queryDto.ToString().c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    if (mediaType != -1) {
        predicates.EqualTo(MediaColumn::MEDIA_TYPE, mediaType);
    }
    std::string timeRangeCon = std::to_string(TO_MILLISECONDS * (currentTime - time));
    predicates.LessThanOrEqualTo(PhotoColumn::PHOTO_LAST_VISIT_TIME, timeRangeCon);
    predicates.And()->LessThanOrEqualTo(MediaColumn::MEDIA_DATE_TAKEN, timeRangeCon);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    predicates.OrderByAsc(PhotoColumn::PHOTO_LAST_VISIT_TIME);
    predicates.Limit(sizeLimit);
    const std::vector<std::string> columns = {};
    auto resultSet = rdbStore->Query(predicates, columns);
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetAgingFile Failed to query, ret: %{public}d", ret);
    return E_OK;
}

int32_t CloudMediaDataDao::GetActiveAgingFile(const AgingFileQueryDto &queryDto, std::vector<PhotosPo> &photosPos)
{
    int64_t time = queryDto.time;
    int32_t mediaType = queryDto.mediaType;
    int32_t sizeLimit = queryDto.sizeLimit;
    auto currentTime = MediaFileUtils::UTCTimeSeconds();
    MEDIA_INFO_LOG("enter GetActiveAgingFile, queryDto: %{public}s", queryDto.ToString().c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    if (mediaType != -1) {
        predicates.EqualTo(MediaColumn::MEDIA_TYPE, mediaType);
    }
    std::string timeRangeCon = std::to_string(TO_MILLISECONDS * (currentTime - time));
    predicates.LessThanOrEqualTo(PhotoColumn::PHOTO_LAST_VISIT_TIME, timeRangeCon);
    predicates.And()->LessThanOrEqualTo(MediaColumn::MEDIA_DATE_TAKEN, timeRangeCon);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    predicates.And()->EqualTo(PhotoColumn::MEDIA_HIDDEN, 0);  // 非隐藏文件
    predicates.OrderByAsc(PhotoColumn::PHOTO_LAST_VISIT_TIME);
    predicates.Limit(sizeLimit);
    const std::vector<std::string> columns = {};
    auto resultSet = rdbStore->Query(predicates, columns);
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetActiveAgingFile Failed to query, ret: %{public}d", ret);
    return E_OK;
}

int32_t CloudMediaDataDao::UpdateLocalFileDirty(std::string &cloudId)
{
    MEDIA_INFO_LOG("enter UpdateLocalFileDirty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateLocalFileDirty get store failed.");
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    int32_t changedRows;
    int32_t ret = rdbStore->Update(
        changedRows, PhotoColumn::PHOTOS_TABLE, valuesBucket, PhotoColumn::PHOTO_CLOUD_ID + " = ?", {cloudId});
    MEDIA_INFO_LOG("UpdateLocalFileDirty changedRows: %{public}d, ret: %{public}d", changedRows, ret);
    return ret;
}

int32_t CloudMediaDataDao::CheckAndDeleteAlbum()
{
    MEDIA_INFO_LOG("enter CheckAndDeleteAlbum");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "CheckAndDeleteAlbum get store failed.");
    int32_t ret = rdbStore->ExecuteSql(SQL_CHECK_AND_DELETE_ALBUM);
    MEDIA_INFO_LOG("CheckAndDeleteAlbum: Delete albums, ret: %{public}d", ret);
    return ret;
}

int32_t CloudMediaDataDao::CheckAndUpdateAlbum()
{
    MEDIA_INFO_LOG("enter CheckAndUpdateAlbum");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "CheckAndUpdateAlbum get store failed.");
    int32_t ret = rdbStore->ExecuteSql(SQL_CHECK_AND_UPDATE_ALBUM);
    MEDIA_INFO_LOG("CheckAndUpdateAlbum: Update albums, ret: %{public}d", ret);
    return ret;
}

int32_t CloudMediaDataDao::QueryDataFromPhotos(const DataShare::DataSharePredicates &predicates,
    const std::vector<std::string> &columnNames, std::vector<PhotosPo> &photoInfos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "QueryData Failed to get rdbStore.");
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    auto resultSet = rdbStore->Query(rdbPredicates, columnNames);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "QueryData resultset is null");
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photoInfos);
    MEDIA_INFO_LOG("QueryData, ret: %{public}d, size: %{public}zu", ret, photoInfos.size());
    return ret;
}

int32_t CloudMediaDataDao::QueryDataFromPhotoAlbums(const DataShare::DataSharePredicates &predicates,
    const std::vector<std::string> &columnNames, std::vector<PhotoAlbumPo> &photoAlbumInfos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "QueryData Failed to get rdbStore.");
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, PhotoAlbumColumns::TABLE);
    auto resultSet = rdbStore->Query(rdbPredicates, columnNames);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "QueryData resultset is null");
    int32_t ret = ResultSetReader<PhotoAlbumPoWriter, PhotoAlbumPo>(resultSet).ReadRecords(photoAlbumInfos);
    MEDIA_INFO_LOG("QueryData, ret: %{public}d, size: %{public}zu", ret, photoAlbumInfos.size());
    return ret;
}

int32_t CloudMediaDataDao::UpdateDataInPhotos(const DataShare::DataSharePredicates &predicates,
    const DataShare::DataShareValuesBucket &value)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "UpdateData Failed to get rdbStore.");
    NativeRdb::RdbPredicates rdbPredicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    NativeRdb::ValuesBucket values = RdbDataShareAdapter::RdbUtils::ToValuesBucket(value);
    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, rdbPredicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateDataInPhotos, ret: %{public}d", ret);
    MEDIA_INFO_LOG("UpdateDataInPhotos Check updateRows: %{public}d", changedRows);
    return ret;
}
}  // namespace OHOS::Media::CloudSync