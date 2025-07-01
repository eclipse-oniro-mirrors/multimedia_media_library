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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_DAO_H

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
#include "cloud_media_pull_data_dto.h"
#include "cloud_media_common_dao.h"
#include "report_failure_dto.h"
#include "accurate_common_data.h"
#include "asset_accurate_refresh.h"
#include "album_accurate_refresh.h"

namespace OHOS::Media::CloudSync {
class CloudMediaPhotosDao {
public:
    CloudMediaPhotosDao() = default;
    ~CloudMediaPhotosDao() = default;

public:
    int32_t BatchInsertFile(std::map<std::string, int> &recordAnalysisAlbumMaps,
        std::map<std::string, std::set<int>> &recordAlbumMaps, std::vector<NativeRdb::ValuesBucket> &insertFiles,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t BatchInsertAssetAnalysisMaps(std::map<std::string, int32_t> recordAnalysisAlbumMaps);
    int32_t BatchInsertQuick(int64_t &outRowId, const std::string &table,
        std::vector<NativeRdb::ValuesBucket> &initialBatchValues,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t BatchInsert(
        int64_t &outRowId, const std::string &table, std::vector<NativeRdb::ValuesBucket> &initialBatchValues);
    int32_t BatchInsertAssetMaps(std::map<std::string, std::set<int32_t>> &recordAlbumMaps);
    int32_t UpdateAssetInPhotoMap(const int32_t &fileId, std::set<int> cloudMapIds);
    int32_t UpdateRecordToDatabase(const CloudMediaPullDataDto &pullData, bool isLocal, bool mtimeChanged,
        std::set<std::string> &refreshAlbums, std::vector<int32_t> &stats,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t ConflictDataMerge(const CloudMediaPullDataDto &pullData, const std::string fullPath, const bool cloudStd,
        std::set<int32_t> albumIds, std::set<std::string> &refreshAlbums,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t GetInsertParams(const CloudMediaPullDataDto &pullData, std::map<std::string, int> &recordAnalysisAlbumMaps,
        std::map<std::string, std::set<int>> &recordAlbumMaps, std::set<std::string> &refreshAlbums,
        std::vector<NativeRdb::ValuesBucket> &insertFiles);
    int32_t UpdateFixDB(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values, int32_t &albumId,
        std::set<int32_t> &albumIds, std::set<std::string> &refreshAlbums);
    bool IsNeededFix(const CloudMediaPullDataDto &data);
    void HandleShootingMode(const std::string &cloudId, const NativeRdb::ValuesBucket &valuebucket,
        std::map<std::string, int> &recordAnalysisAlbumMaps);
    int32_t GetLocalKeyData(KeyData &localKeyData, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    bool JudgeConflict(const CloudMediaPullDataDto &pullData, const KeyData &localKeyData, const KeyData &cloudKeyData);
    void UpdateAlbumInternal(std::set<std::string> &refreshAlbums);
    int32_t GetRetryRecords(std::vector<std::string> &cloudIds);
    std::vector<PhotosPo> GetCheckRecords(const std::vector<std::string> cloudIds);
    int32_t GetCreatedRecords(int32_t size, std::vector<PhotosPo> &createdRecords);
    int32_t GetMetaModifiedRecords(int32_t size, std::vector<PhotosPo> &cloudRecordPoList);
    int32_t GetFileModifiedRecords(int32_t size, std::vector<PhotosPo> &cloudRecordPoList);
    int32_t GetDeletedRecordsAsset(int32_t size, std::vector<PhotosPo> &cloudRecordPoList);
    int32_t GetCopyRecords(int32_t size, std::vector<PhotosPo> &copyRecords);
    int32_t GetPhotoLocalInfo(const std::vector<PhotosDto> &records,
        std::unordered_map<std::string, LocalInfo> &infoMap, const std::string &type);
    int32_t UpdateLocalAlbumMap(const std::string &cloudId);
    int32_t OnModifyPhotoRecord(
        const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t UpdateFdirtyVersion(
        const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t OnDeleteRecordsAsset(
        const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t OnCopyPhotoRecord(
        const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t ClearCloudInfo(const std::string &cloudId);
    int32_t DeleteFileNotExistPhoto(
        std::string &path, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t HandleSameNameRename(
        const PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t UpdatePhotoVisible();
    int32_t SetRetry(const std::string &cloudId);
    int32_t DeleteLocalByCloudId(
        const std::string &cloudId, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t UpdateFailRecordsCloudId(
        const PhotosDto &record, const std::unordered_map<std::string, LocalInfo> &localMap,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    void InsertPhotoCreateFailedRecord(int32_t fileId);
    void InsertPhotoModifyFailedRecord(const std::string &cloudId);
    void InsertPhotoCopyFailedRecord(int32_t fileId);
    void RemovePhotoCreateFailedRecord(int32_t fileId);
    void RemovePhotoModifyFailedRecord(const std::string &cloudId);
    void RemovePhotoCopyFailedRecord(int32_t fileId);
    int32_t ClearPhotoFailedRecords();
    int32_t UpdatePhotoCreatedRecord(
        const PhotosDto &record, const std::unordered_map<std::string, LocalInfo> &localMap,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t GetSameNamePhotoCount(const PhotosDto &photo, bool isHide, int32_t count);
    int32_t DeleteSameNamePhoto(const PhotosDto &photo);
    int32_t AddRemoveAlbumCloudId(std::shared_ptr<MediaLibraryRdbStore> rdbStore, const int32_t fileId,
        const int32_t ownerAlbumId, PhotosPo &record);
    std::shared_ptr<NativeRdb::ResultSet> BatchQueryLocal(
        const std::vector<CloudMediaPullDataDto> &datas, const std::vector<std::string> &columns, int32_t &rowCount);
    int32_t DeleteLocalFileNotExistRecord(const PhotosDto &photo);
    int32_t RenewSameCloudResource(const PhotosDto &photo);
    int32_t RepushDuplicatedPhoto(const PhotosDto &photo);

private:
    bool IsTimeChanged(const PhotosDto &record, const std::unordered_map<std::string, LocalInfo> &localMap,
        const std::string &fileId, const std::string &type);
    void UpdateAllAlbumsCountForCloud(const std::vector<std::string> &albums);
    void UpdateAlbumCountInternal(const std::vector<std::string> &subtypes);
    void GetSourceAlbumFromPath(const CloudMediaPullDataDto &pullData, int32_t &albumId, std::set<int32_t> &cloudMapIds,
        SafeMap<std::string, std::pair<int32_t, std::string>> &lpathToIdMap);
    int32_t GetSourceAlbumForMerge(const CloudMediaPullDataDto &pullData, std::vector<std::string> &albumCloudIds,
        SafeMap<std::string, std::pair<int32_t, std::string>> &lpathToIdMap);
    int32_t GetSourceAlbum(const CloudMediaPullDataDto &pullData, int32_t &albumId, std::set<int32_t> &cloudMapIds,
        bool &isHidden, SafeMap<std::string, int32_t> &cloudToLocalMap);
    int32_t UpdateAlbumReplacedSignal(const std::vector<std::string> &albumIdVector);
    std::shared_ptr<NativeRdb::ResultSet> GetAllSysAlbums(
        const std::vector<std::string> &subtypes, const std::vector<std::string> &columns);
    std::shared_ptr<NativeRdb::ResultSet> GetAllSysAlbumsQuery(
        NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns);
    SafeMap<int32_t, std::pair<std::string, std::string>> GetAlbumLocalToCloudMap();
    SafeMap<std::string, int32_t> GetAlbumCloudToLocalMap();
    SafeMap<std::string, std::pair<int32_t, std::string>> GetAlbumLPathToIdMap();
    void PrepareAlbumMap(SafeMap<int32_t, std::pair<std::string, std::string>> &localToCloudMap,
        SafeMap<std::string, int32_t> &cloudToLocalMap,
        SafeMap<std::string, std::pair<int32_t, std::string>> &lpathToIdMap, bool isUpload = true);
    bool IsAlbumCloud(bool isUpload, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    int UpdateProxy(int &changedRows, const NativeRdb::ValuesBucket &row, const NativeRdb::AbsRdbPredicates &predicates,
        const std::string &cloudId, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int UpdateProxy(int &changedRows, const std::string &table, const NativeRdb::ValuesBucket &row,
        const std::string &whereClause, const std::vector<std::string> &args,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh);
    int32_t UpdatePhotosSynced(const NativeRdb::AbsRdbPredicates &predicates, const int32_t &dirtyValue);
    int32_t UpdatePhotosSynced(const std::string &table, const std::string &whereClause,
        const std::vector<std::string> &args, const int32_t &dirtyValue);
    int32_t GetFieldIntValue(
        const NativeRdb::ValuesBucket &values, const std::string &fieldName, const int32_t &defaultFieldValue);
    void GetUpdateRecordValues(const CloudMediaPullDataDto &pullData, NativeRdb::ValuesBucket &values);
    NativeRdb::AbsRdbPredicates GetUpdateRecordCondition(const std::string &cloudId);
    int32_t UpdatePhoto(const std::string &whereClause, const std::vector<std::string> &whereArgs,
        NativeRdb::ValuesBucket &values, int32_t &changeRows);
    int32_t DeletePhoto(const std::string &whereClause, const std::vector<std::string> &whereArgs, int32_t &deleteRows);

private:
    CloudMediaCommonDao commonDao_;

private:
    /* photo failure records */
    SafeVector<std::string> photoModifyFailSet_;
    SafeVector<std::string> photoCreateFailSet_;
    SafeVector<std::string> photoCopyFailSet_;
    int32_t hiddenAlbumId_ = -1;

private:
    const std::string SQL_PHOTOS_GET_CREATE_RECORDS = "\
        WITH DATA AS \
        ( \
            SELECT \
                * \
            FROM Photos \
            WHERE \
                dirty = 1 AND \
                thumbnail_ready >= 3 AND \
                lcd_visit_time >= 2 AND \
                date_trashed = 0 AND \
                time_pending = 0 AND \
                file_id NOT IN ({0}) \
            ORDER BY size ASC \
            LIMIT ?  \
        ) \
        SELECT DATA.*, \
            PhotoAlbum.cloud_id AS album_cloud_id, \
            PhotoAlbum.lpath AS lpath \
        FROM DATA \
            INNER JOIN PhotoAlbum \
            ON DATA.owner_album_id = PhotoAlbum.album_id \
        ;";
    const std::string SQL_PHOTOS_GET_COPY_RECORDS = "\
        WITH DATA AS \
        ( \
            SELECT * \
            FROM Photos \
            WHERE dirty = 7 AND \
                file_id NOT IN ({0}) \
            ORDER BY size ASC \
            LIMIT ? \
        ) \
        SELECT DATA.*, \
            PhotoAlbum.cloud_id AS album_cloud_id, \
            PhotoAlbum.lpath AS lpath \
        FROM DATA \
            INNER JOIN PhotoAlbum \
            ON DATA.owner_album_id = PhotoAlbum.album_id \
        ;";
    const std::string SQL_PHOTOS_GET_META_MODIFIED_RECORDS = "\
        WITH DATA AS \
        ( \
            SELECT * \
            FROM Photos \
            WHERE dirty IN (2, 6) AND \
                cloud_id <> '' AND \
                cloud_id IS NOT NULL AND \
                file_id NOT IN ({0}) \
            ORDER BY size ASC \
            LIMIT ? \
        ) \
        SELECT DATA.*, \
            PhotoAlbum.cloud_id AS album_cloud_id, \
            PhotoAlbum.lpath AS lpath \
        FROM DATA \
            INNER JOIN PhotoAlbum \
            ON DATA.owner_album_id = PhotoAlbum.album_id \
        ;";
    const std::string SQL_PHOTOS_GET_FILE_MODIFIED_RECORDS = "\
        WITH DATA AS \
        ( \
            SELECT \
                * \
            FROM Photos \
            WHERE \
                dirty = 3 AND \
                thumbnail_ready >= 3 AND \
                lcd_visit_time >= 2 AND \
                cloud_id <> '' AND \
                cloud_id IS NOT NULL AND \
                file_id NOT IN ({0}) \
            ORDER BY size ASC \
            LIMIT ? \
        ) \
        SELECT DATA.*, \
            PhotoAlbum.cloud_id AS album_cloud_id, \
            PhotoAlbum.lpath AS lpath \
        FROM DATA \
            INNER JOIN PhotoAlbum \
            ON DATA.owner_album_id = PhotoAlbum.album_id \
        ;";
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_PHOTOS_DAO_H