/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef TDD_PHOTOS_DAO_H
#define TDD_PHOTOS_DAO_H

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "media_log.h"
#include "media_library_database.h"
#include "photos_po.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"

namespace OHOS::Media::TestUtils {
using namespace OHOS::Media::ORM;
class PhotosDao {
public:
    PhotosDao()
    {
        // Get RdbStore
        int32_t errorCode = 0;
        this->rdbStore_ = MediaLibraryDatabase().GetRdbStore(errorCode);
    }

public:
    std::vector<PhotosPo> QueryPhotosByCloudId(const std::string &cloudId)
    {
        std::vector<PhotosPo> result;
        std::string sql = SQL_PHOTOS_QUERY_BY_CLOUD_ID;
        std::vector<NativeRdb::ValueObject> params = {cloudId};
        auto resultSet = this->rdbStore_->QuerySql(sql, params);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotoByCloudId Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        resultSet->Close();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotoByCloudId no photo found");
        }
        return result;
    }

    std::vector<PhotosPo> QueryPhotosDownloadThms()
    {
        std::vector<PhotosPo> result;
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
        predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
        predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
        predicates.NotEqualTo(PhotoColumn::PHOTO_THUMB_STATUS, 0);
        const std::vector<std::string> columns = {
            MediaColumn::MEDIA_FILE_PATH, MediaColumn::MEDIA_SIZE,         MediaColumn::MEDIA_TYPE,
            PhotoColumn::PHOTO_CLOUD_ID,  PhotoColumn::PHOTO_THUMB_STATUS, PhotoColumn::PHOTO_ORIENTATION,
        };

        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosDownloadThms Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosDownloadThms no photo found");
        }
        return result;
    }

    std::vector<PhotosPo> QueryPhotosByCloudIds(const std::vector<std::string> &cloudIds)
    {
        std::vector<PhotosPo> result;
        std::vector<std::string> columns = {DAO_PHOTO_QUERY_COLUMNS};
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotoByCloudIds Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        resultSet->Close();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotoByCloudIds no photo found");
        }
        return result;
    }

    std::string BuildUriByPhoto(PhotosPo &photo)
    {
        std::string uri = "";
        std::string uri_prefix = "file://media/Photo/";
        int32_t fileId = photo.fileId.value_or(-1);
        MEDIA_INFO_LOG("GetDownloadAsset fileId: %{public}d", fileId);
        std::string path = photo.data.value_or("");
        size_t lastSlash = path.find_last_of('/');
        std::string filename;
        if (lastSlash != std::string::npos) {
            filename = path.substr(lastSlash + 1);
            size_t lastDot = filename.find_last_of('.');
            if (lastDot != std::string::npos) {
                filename = filename.substr(0, lastDot);
            }
        }
        std::string displayName = photo.displayName.value_or("");
        uri = uri_prefix + to_string(fileId) + "/" + filename + "/" + displayName;
        MEDIA_INFO_LOG("GetDownloadAsset uri: %{public}s", uri.c_str());
        return uri;
    }

    std::vector<PhotosPo> QueryPhotosByFilePaths(const std::vector<std::string> &paths)
    {
        std::vector<PhotosPo> result;
        std::vector<std::string> columns = {DAO_PHOTO_QUERY_COLUMNS};
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(MediaColumn::MEDIA_FILE_PATH, paths);
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosByFilePaths Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        resultSet->Close();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosByFilePaths no photo found");
        }
        return result;
    }

    std::vector<PhotosPo> QueryPhotosByFileId(const int32_t &fileId)
    {
        std::vector<PhotosPo> result;
        std::vector<std::string> columns = {DAO_PHOTO_QUERY_COLUMNS};
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosByFileId Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        resultSet->Close();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosByFileId no photo found");
        }
        return result;
    }

    std::vector<PhotosPo> QueryAllPhotos()
    {
        std::vector<PhotosPo> result;
        std::vector<std::string> columns = {DAO_PHOTO_QUERY_COLUMNS};
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryAllPhotos Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        resultSet->Close();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryAllPhotos no photo found");
        }
        return result;
    }

    int32_t GetPhotoByCloudId(std::vector<PhotosPo> &photosList, const std::string &cloudId, PhotosPo &photo)
    {
        if (photosList.empty()) {
            return -1;
        }
        for (auto &node : photosList) {
            if (node.cloudId.value_or("") == cloudId) {
                photo = node;
                return 0;
            }
        }
        return -1;
    }

    std::vector<PhotosPo> QueryPhotosByDisplayNames(const std::vector<std::string> &displayNames)
    {
        std::vector<PhotosPo> result;
        std::vector<std::string> columns = {" * "};
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(MediaColumn::MEDIA_NAME, displayNames);
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosByDisplayNames Query failed");
            return result;
        }
        result = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
        resultSet->Close();
        if (result.empty()) {
            MEDIA_ERR_LOG("PhotosDao::QueryPhotosByDisplayNames no photo found");
        }
        return result;
    }

    int32_t GetPhotoByDisplayName(std::vector<PhotosPo> &photosList, const std::string &displayName, PhotosPo &photo)
    {
        if (photosList.empty()) {
            return -1;
        }
        for (auto &node : photosList) {
            if (node.displayName.value_or("") == displayName) {
                photo = node;
                return 0;
            }
        }
        return -1;
    }

    void UpdatePhotoDirtyByCloudId(const std::string &CloudId, int32_t dirty)
    {
        if (dirty < 0) {
            return;
        }
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, CloudId);
        int32_t rowCount;
        NativeRdb::ValuesBucket bucket;
        bucket.PutInt(PhotoColumn::PHOTO_DIRTY, dirty);
        this->rdbStore_->Update(rowCount, bucket, predicates);
    }

    void UpdatePhotoPositionByCloudId(const std::string &CloudId, int32_t position)
    {
        if (position < 0) {
            return;
        }
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, CloudId);
        int32_t rowCount;
        NativeRdb::ValuesBucket bucket;
        bucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
        this->rdbStore_->Update(rowCount, bucket, predicates);
    }

    int32_t GetPhotoDirtyByCloudId(const std::string &CloudId)
    {
        int32_t result = -1;
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, CloudId);
        std::vector<std::string> columns = {PhotoColumn::PHOTO_DIRTY};
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            GTEST_LOG_(ERROR) << "PhotoDao::GetPhotoDirtyByCloudId Query failed:" << CloudId;
            return result;
        }
        int32_t rowCount;
        int32_t ret = resultSet->GetRowCount(rowCount);
        if (ret != NativeRdb::E_OK || rowCount != 1) {
            GTEST_LOG_(ERROR) << "PhotoDao::GetPhotoDirtyByCloudId GetRowCount failed:" << CloudId;
            resultSet->Close();
            return result;
        }
        if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            result = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
        }
        resultSet->Close();
        return result;
    }

    int32_t GetDirtyTypeNum(const int32_t dirtyType)
    {
        int32_t rowCount = -1;
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, dirtyType);
        std::vector<std::string> columns = {PhotoColumn::PHOTO_DIRTY};
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            GTEST_LOG_(ERROR) << "PhotoDao::GetDirtyTypeNum Query failed:" << dirtyType;
            return rowCount;
        }
        int32_t ret = resultSet->GetRowCount(rowCount);
        if (ret != NativeRdb::E_OK || rowCount < 0) {
            GTEST_LOG_(ERROR) << "PhotoDao::GetDirtyTypeNum GetRowCount failed:" << dirtyType;
            resultSet->Close();
            return rowCount;
        }
        resultSet->Close();
        return rowCount;
    }

    int32_t GetCloudThmStatNum(const int32_t ThmStat)
    {
        int32_t rowCount = -1;
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, ThmStat)
            ->And()
            ->BeginWrap()
            ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD))
            ->Or()
            ->EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD))
            ->EndWrap();
        std::vector<std::string> columns = {PhotoColumn::PHOTO_THUMB_STATUS};
        auto resultSet = this->rdbStore_->Query(predicates, columns);
        if (resultSet == nullptr) {
            GTEST_LOG_(ERROR) << "PhotoDao::GetCloudThmStatNum Query failed:" << ThmStat;
            return rowCount;
        }
        int32_t ret = resultSet->GetRowCount(rowCount);
        if (ret != NativeRdb::E_OK || rowCount < 0) {
            GTEST_LOG_(ERROR) << "PhotoDao::GetCloudThmStatNum GetRowCount failed:" << ThmStat;
            resultSet->Close();
            return rowCount;
        }
        resultSet->Close();
        return rowCount;
    }

    int32_t UpdatePhotoFavorite(const std::vector<std::string> &cloudIds, const int32_t &isFavorite)
    {
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(PhotoColumn::MEDIA_IS_FAV, isFavorite);

        int32_t changedRows = -1;

        this->rdbStore_->Update(changedRows, valuesBucket, predicates);
        return changedRows;
    }

    int32_t UpdatePhotoDateTrashed(const std::vector<std::string> &cloudIds, const int32_t &dateTrashed)
    {
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, dateTrashed);

        int32_t changedRows = -1;

        this->rdbStore_->Update(changedRows, valuesBucket, predicates);
        return changedRows;
    }

    int32_t UpdatePhotoHidden(const std::vector<std::string> &cloudIds, const int32_t &isHidden)
    {
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutInt(PhotoColumn::MEDIA_HIDDEN, isHidden);

        int32_t changedRows = -1;

        this->rdbStore_->Update(changedRows, valuesBucket, predicates);
        return changedRows;
    }

    int32_t UpdatePhotoName(const std::vector<std::string> &cloudIds, const std::string &newName)
    {
        NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);

        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.PutString(PhotoColumn::MEDIA_NAME, newName);

        int32_t changedRows = -1;

        this->rdbStore_->Update(changedRows, valuesBucket, predicates);
        return changedRows;
    }

    int32_t RenameTablePhotos()
    {
        std::string createTempTableSql =
            "CREATE TABLE IF NOT EXISTS PhotosTemp AS SELECT * FROM " + PhotoColumn::PHOTOS_TABLE + ";";
        int32_t ret = this->rdbStore_->ExecuteSql(createTempTableSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << " RenameTablePhotos create temp table failed";
            return E_ERR;
        }
        std::string dropOldTable = "DROP TABLE " + PhotoColumn::PHOTOS_TABLE + ";";
        ret = this->rdbStore_->ExecuteSql(createTempTableSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << " RenameTablePhotos drop old table failed";
            return E_ERR;
        }
        return NativeRdb::E_OK;
    }

    int32_t RestoreTablePhotos()
    {
        std::string createTempTableSql =
            "CREATE TABLE IF NOT EXISTS " + PhotoColumn::PHOTOS_TABLE + " AS SELECT * FROM PhotosTemp;";
        int32_t ret = this->rdbStore_->ExecuteSql(createTempTableSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << " RestoreTablePhotos create temp table failed";
            return E_ERR;
        }
        std::string dropTempTable = "DROP TABLE " + PhotoColumn::PHOTOS_TABLE + "Temp;";
        ret = this->rdbStore_->ExecuteSql(dropTempTable);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << " RestoreTablePhotos drop TempTable failed";
            return E_ERR;
        }
        return E_OK;
    }

private:
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;

private:
    const std::string SQL_PHOTOS_QUERY_BY_CLOUD_ID = "SELECT * FROM Photos WHERE cloud_id = ?";

    const std::vector<std::string> DAO_PHOTO_QUERY_COLUMNS = {
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TITLE,
        PhotoColumn::MEDIA_SIZE,
        PhotoColumn::MEDIA_NAME,
        PhotoColumn::MEDIA_TYPE,
        PhotoColumn::MEDIA_MIME_TYPE,
        PhotoColumn::MEDIA_DEVICE_NAME,
        PhotoColumn::MEDIA_DATE_ADDED,
        PhotoColumn::MEDIA_DATE_MODIFIED,
        PhotoColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::MEDIA_DURATION,
        PhotoColumn::MEDIA_IS_FAV,
        PhotoColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTO_HIDDEN_TIME,
        PhotoColumn::MEDIA_RELATIVE_PATH,
        PhotoColumn::MEDIA_VIRTUAL_PATH,
        PhotoColumn::PHOTO_META_DATE_MODIFIED,
        PhotoColumn::PHOTO_ORIENTATION,
        PhotoColumn::PHOTO_LATITUDE,
        PhotoColumn::PHOTO_LONGITUDE,
        PhotoColumn::PHOTO_HEIGHT,
        PhotoColumn::PHOTO_WIDTH,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        PhotoColumn::PHOTO_BURST_KEY,
        PhotoColumn::PHOTO_DATE_YEAR,
        PhotoColumn::PHOTO_DATE_MONTH,
        PhotoColumn::PHOTO_DATE_DAY,
        PhotoColumn::PHOTO_USER_COMMENT,
        PhotoColumn::PHOTO_THUMB_STATUS,
        PhotoColumn::PHOTO_SYNC_STATUS,
        PhotoColumn::PHOTO_SHOOTING_MODE,
        PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
        PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
        PhotoColumn::PHOTO_FRONT_CAMERA,
        PhotoColumn::PHOTO_DETAIL_TIME,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_COVER_POSITION,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_OWNER_ALBUM_ID,
        PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID,
        PhotoColumn::PHOTO_SOURCE_PATH,
        PhotoColumn::SUPPORTED_WATERMARK_TYPE,
        PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_CLOUD_ID,
        PhotoColumn::PHOTO_DIRTY,
        PhotoColumn::PHOTO_POSITION,
        PhotoColumn::PHOTO_CLOUD_VERSION,
    };
};
}  // namespace OHOS::Media::TestUtils
#endif  // TDD_CSV_FILE_READER_H