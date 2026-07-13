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

#ifndef TDD_DATABASE_DATA_MOCK_H
#define TDD_DATABASE_DATA_MOCK_H

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "rdb_store.h"
#include "result_set_utils.h"

#include "medialibrary_mock_tocken.h"
#include "media_library_database.h"
#include "csv_file_reader.h"
#include "table_writer.h"
#include "medialibrary_errno.h"
#include "photo_album_column.h"
#include "medialibrary_db_const.h"

namespace OHOS::Media::TestUtils {
struct TableMockInfo {
    std::string tableName;
    std::string csvFilePath;
    std::vector<std::string> columnNames;
};
class DatabaseDataMock {
public:
    enum {
        E_OK,
        E_RDB_STORE_NULL,
        E_PHOTOS_CHECKPOINT_FAIL,
        E_PHOTOALBUM_CHECKPOINT_FAIL,
        E_PHOTOS_MOCKDATA_FAIL,
        E_PHOTOALBUM_MOCKDATA_FAIL,
        E_PHOTOS_ROLLBACK_FAIL,
        E_PHOTOALBUM_ROLLBACK_FAIL,
        E_PHOTOS_MOCKDATA_CSV_FAIL,
        E_PHOTOALBUM_MOCKDATA_CSV_FAIL,
        E_ANALYSIS_ID_CHECKPOINT_FAIL,
        E_ANALYSIS_FILE_ID_CHECKPOINT_FAIL,
        E_ANALYSIS_SEARCH_CHECKPOINT_FAIL,
        E_ANALYSIS_ID_FILE_ID_CHECKPOINT_FAIL,
    };

private:
    int64_t maxFileId_;
    int64_t maxAnalysisFileId_;
    int64_t maxAlbumId_;
    int64_t maxAnalysisId_;
    int64_t tabAnalysisSearchIndexMaxId_;
    std::shared_ptr<NativeRdb::RdbStore> rdbStore_;
    const std::string BACK_ALBUM_TABLE_NAME = PhotoAlbumColumns::TABLE + "Temp";
    const std::string BACK_PHOTO_TABLE_NAME = PhotoColumn::PHOTOS_TABLE + "Temp";

public:
    DatabaseDataMock &SetRdbStore(std::shared_ptr<NativeRdb::RdbStore> rdbStore)
    {
        this->rdbStore_ = rdbStore;
        return *this;
    }

public:  // Getter & Setter
    int64_t GetMaxFileId() const
    {
        return maxFileId_;
    }
    int64_t GetMaxAlbumId() const
    {
        return maxAlbumId_;
    }
    int64_t GetMaxAnalysisId() const
    {
        return maxAnalysisId_;
    }
    int64_t GetMaxAnalysisFileId() const
    {
        return maxAnalysisFileId_;
    }
    int64_t GetTabAnalysisSearchIndexMaxId() const
    {
        return tabAnalysisSearchIndexMaxId_;
    }

public:
    int32_t BackupDatabase()
    {
        // backup album
        std::string dropBackupAlbumTable = "DROP TABLE IF EXISTS " + BACK_ALBUM_TABLE_NAME + ";";
        int32_t ret = this->rdbStore_->ExecuteSql(dropBackupAlbumTable);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Drop PhotoAlbumTemp Failed:" << dropBackupAlbumTable;
            return E_ERR;
        }
        std::string backupAlbumSql = "CREATE TABLE IF NOT EXISTS " + BACK_ALBUM_TABLE_NAME + " AS SELECT * FROM " +
                                     PhotoAlbumColumns::TABLE + ";";
        ret = this->rdbStore_->ExecuteSql(backupAlbumSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Backup PhotoAlbum Failed:" << backupAlbumSql;
            return E_ERR;
        }

        // backup photo
        std::string dropBackupPhotoTable = "DROP TABLE IF EXISTS " + BACK_PHOTO_TABLE_NAME + ";";
        ret = this->rdbStore_->ExecuteSql(dropBackupPhotoTable);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Drop PhotosTemp Failed:" << dropBackupPhotoTable;
            return E_ERR;
        }
        std::string backupPhotosSql = "CREATE TABLE IF NOT EXISTS " + BACK_PHOTO_TABLE_NAME + " AS SELECT * FROM " +
                                      PhotoColumn::PHOTOS_TABLE + ";";
        ret = this->rdbStore_->ExecuteSql(backupPhotosSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Backup Photos Failed:" << backupPhotosSql;
            return E_ERR;
        }
        return DeleteDatabaseData();
    }

    int32_t DeleteDatabaseData()
    {
        // delete photo
        std::string deletePhotoData = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
        int32_t ret = this->rdbStore_->ExecuteSql(deletePhotoData);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Delete Photos Data Failed:" << deletePhotoData;
            return E_ERR;
        }

        // delete album
        std::string deleteAlbumData =
            "DELETE FROM " + PhotoAlbumColumns::TABLE + " WHERE " + PhotoAlbumColumns::ALBUM_TYPE + "<>" +
            std::to_string(static_cast<int32_t>(PhotoAlbumType::SYSTEM)) + " AND (" +
            PhotoAlbumColumns::ALBUM_BUNDLE_NAME +
            " NOT IN ('com.other.album','com.hidden.album','com.huawei.hmos.camera') OR " +
            PhotoAlbumColumns::ALBUM_BUNDLE_NAME + " IS NULL OR " + PhotoAlbumColumns::ALBUM_BUNDLE_NAME + "='');";
        ret = this->rdbStore_->ExecuteSql(deleteAlbumData);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Delete PhotoAlbum Data Failed:" << deleteAlbumData;
            return E_ERR;
        }

        // tab_analysis_total
        std::string deleteAnalysisTotalSql = "DELETE FROM tab_analysis_total;";
        ret = this->rdbStore_->ExecuteSql(deleteAnalysisTotalSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Delete tab_analysis_total Data Failed:" << deleteAnalysisTotalSql;
            return E_ERR;
        }

        // tab_analysis_search_index
        std::string deleteAnalysisSearchSql = "DELETE FROM tab_analysis_search_index;";
        ret = this->rdbStore_->ExecuteSql(deleteAnalysisSearchSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Delete tab_analysis_search_index Data Failed:" << deleteAnalysisSearchSql;
            return E_ERR;
        }
        return E_OK;
    }

    int32_t CleanDatabaseData()
    {
        // delete photo
        std::string deletePhotoData = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
        int32_t ret = this->rdbStore_->ExecuteSql(deletePhotoData);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "CleanDatabaseData Photos Data Failed:" << deletePhotoData;
            return E_ERR;
        }

        // delete album
        std::string deleteAlbumData = "DELETE FROM " + PhotoAlbumColumns::TABLE + ";";
        ret = this->rdbStore_->ExecuteSql(deleteAlbumData);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "CleanDatabaseData PhotoAlbum Data Failed:" << deleteAlbumData;
            return E_ERR;
        }

        // tab_analysis_total
        std::string deleteAnalysisTotalSql = "DELETE FROM tab_analysis_total;";
        ret = this->rdbStore_->ExecuteSql(deleteAnalysisTotalSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "CleanDatabaseData tab_analysis_total Data Failed:" << deleteAnalysisTotalSql;
            return E_ERR;
        }

        // tab_analysis_search_index
        std::string deleteAnalysisSearchSql = "DELETE FROM tab_analysis_search_index;";
        ret = this->rdbStore_->ExecuteSql(deleteAnalysisSearchSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "CleanDatabaseData tab_analysis_search_index Data Failed:" << deleteAnalysisSearchSql;
            return E_ERR;
        }
        return E_OK;
    }

    int32_t RestoreDatabase()
    {
        CleanDatabaseData();

        // insert into album from temp
        std::string albumColumns = "album_id, album_type, album_subtype, album_name, cover_uri, count, \
            date_modified, dirty, cloud_id, relative_path, contains_hidden, hidden_count, hidden_cover, \
            album_order, image_count, video_count, bundle_name, local_language, is_local, date_added, \
            lpath, priority, metadata_flags, check_flag";
        std::string insertAlbumSql = "INSERT INTO " + PhotoAlbumColumns::TABLE + "(" + albumColumns + ") " + "SELECT " +
                                     albumColumns + " FROM " + BACK_ALBUM_TABLE_NAME + ";";
        int32_t ret = this->rdbStore_->ExecuteSql(insertAlbumSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "RestoreDatabase INSERT INTO PhotoColumn Failed:" << insertAlbumSql;
            return E_ERR;
        }

        // insert into photo from temp
        std::string photoColumns = "file_id, data, size, title, display_name, media_type, mime_type, \
            owner_package, owner_appid, package_name, device_name, date_added, date_modified, date_taken, \
            duration, time_pending, is_favorite, date_trashed, date_deleted, hidden,parent, relative_path, \
            virtual_path, dirty, cloud_id, meta_date_modified, sync_status, cloud_version, orientation, latitude, \
            longitude, height, width, edit_time, lcd_visit_time, position, subtype, original_subtype, \
            camera_shot_key, user_comment, all_exif, date_year, date_month, date_day, shooting_mode, \
            shooting_mode_tag, last_visit_time, hidden_time, thumb_status, clean_flag, photo_id, photo_quality, \
            first_visit_time, deferred_proc_type, dynamic_range_type, moving_photo_effect_mode, cover_position, \
            thumbnail_ready, lcd_size, thumb_size, front_camera, is_temp, burst_cover_level, burst_key,\
            ce_available, ce_status_code, strong_association, associate_file_id, has_cloud_watermark, detail_time, \
            owner_album_id, original_asset_cloud_id, thumbnail_visible, source_path, metadata_flags, \
            supported_watermark_type";

        std::string insertPhotoSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + photoColumns + ") " +
                                     "SELECT " + photoColumns + " FROM " + BACK_PHOTO_TABLE_NAME + ";";
        ret = this->rdbStore_->ExecuteSql(insertPhotoSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "RestoreDatabase INSERT INTO Photos Failed:" << insertPhotoSql;
            return E_ERR;
        }

        // drop temp tables
        std::string dropBackupAlbumTable = "DROP TABLE IF EXISTS " + BACK_ALBUM_TABLE_NAME + ";";
        ret = this->rdbStore_->ExecuteSql(dropBackupAlbumTable);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "RestoreDatabase Drop PhotoAlbumTemp Failed:" << dropBackupAlbumTable;
            return E_ERR;
        }
        std::string dropBackupPhotoTable = "DROP TABLE IF EXISTS " + BACK_PHOTO_TABLE_NAME + ";";
        ret = this->rdbStore_->ExecuteSql(dropBackupPhotoTable);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "RestoreDatabase Drop PhotosTemp Failed:" << dropBackupPhotoTable;
            return E_ERR;
        }
        return E_OK;
    }

    int32_t CheckPoint()
    {
        if (this->rdbStore_ == nullptr) {
            return E_RDB_STORE_NULL;
        }
        if (!this->GetMaxFileId(this->maxFileId_)) {
            return E_PHOTOS_CHECKPOINT_FAIL;
        }
        if (!this->GetMaxAlbumId(this->maxAlbumId_)) {
            return E_PHOTOALBUM_CHECKPOINT_FAIL;
        }
        if (!this->GetMaxAnalysisId(this->maxAnalysisId_)) {
            return E_ANALYSIS_ID_CHECKPOINT_FAIL;
        }
        if (!this->GetMaxAnalysisFileId(this->maxAnalysisFileId_)) {
            return E_ANALYSIS_FILE_ID_CHECKPOINT_FAIL;
        }
        if (!this->GetTabAnalysisSearchIndexMaxId(this->tabAnalysisSearchIndexMaxId_)) {
            return E_ANALYSIS_SEARCH_CHECKPOINT_FAIL;
        }
        if (this->maxAnalysisFileId_ > this->maxFileId_) {
            this->DeleteAnalysisRecords();
            if (!this->GetMaxAnalysisFileId(this->maxAnalysisFileId_) ||
                !this->GetMaxAnalysisId(this->maxAnalysisId_)) {
                return E_ANALYSIS_ID_FILE_ID_CHECKPOINT_FAIL;
            }
        }
        return E_OK;
    }

    int32_t MockData(const std::vector<TableMockInfo> &tableMockInfos)
    {
        if (this->rdbStore_ == nullptr) {
            return E_RDB_STORE_NULL;
        }
        for (const auto &tableMockInfo : tableMockInfos) {
            int32_t ret = this->MockData(tableMockInfo);
            if (ret != E_OK) {
                return ret;
            }
        }
        return E_OK;
    }

    int32_t Rollback()
    {
        if (this->rdbStore_ == nullptr) {
            return E_RDB_STORE_NULL;
        }
        if (!this->RollbackPhotosToCheckpoint()) {
            GTEST_LOG_(INFO) << "RollBack Photos Error, MaxFileID:" << this->maxFileId_;
        }
        if (!this->RollbackAnalysisToCheckpoint()) {
            GTEST_LOG_(INFO) << "RollBack Analysis Error, maxAnalysisId_:" << this->maxAnalysisId_;
        }
        if (!this->RollbackTabAnalysisSearchIndexToCheckpoint()) {
            GTEST_LOG_(INFO) << "RollBack AnalysisSearchIndex Error, Id:" << this->tabAnalysisSearchIndexMaxId_;
        }
        if (!this->RollbackAlbumsToCheckpoint()) {
            GTEST_LOG_(INFO) << "RollBack PhotoAlbum Error, MaxAlbumId:" << this->maxAlbumId_;
        }
        return E_OK;
    }

private:
    bool GetMaxFileId(int64_t &maxFileId)
    {
        // Use COALSECE to avoid NULL result when the table is empty.
        std::string querySql = "SELECT COALESCE(MAX(file_id), 0) AS file_id FROM Photos;";
        auto resultSet = this->rdbStore_->QuerySql(querySql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            GTEST_LOG_(INFO) << "resultSet is null, querySql: " << querySql;
            return false;
        }
        maxFileId = GetInt64Val("file_id", resultSet);
        return true;
    }
    bool GetMaxAlbumId(int64_t &maxAlbumId)
    {
        // Use COALSECE to avoid NULL result when the table is empty.
        std::string querySql = "SELECT COALESCE(MAX(album_id), 0) AS album_id FROM PhotoAlbum;";
        auto resultSet = this->rdbStore_->QuerySql(querySql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            GTEST_LOG_(INFO) << "resultSet is null, querySql: " << querySql;
            return false;
        }
        maxAlbumId = GetInt64Val("album_id", resultSet);
        return true;
    }
    bool GetMaxAnalysisId(int64_t &maxAnalysisId)
    {
        // Use COALSECE to avoid NULL result when the table is empty.
        std::string querySql = "SELECT COALESCE(MAX(id), 0) AS id FROM tab_analysis_total;";
        auto resultSet = this->rdbStore_->QuerySql(querySql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            GTEST_LOG_(INFO) << "resultSet is null, querySql: " << querySql;
            return false;
        }
        maxAnalysisId = GetInt64Val("id", resultSet);
        return true;
    }
    bool GetMaxAnalysisFileId(int64_t &maxAnalysisFileId)
    {
        // Use COALSECE to avoid NULL result when the table is empty.
        std::string querySql = "SELECT COALESCE(MAX(file_id), 0) AS file_id FROM tab_analysis_total;";
        auto resultSet = this->rdbStore_->QuerySql(querySql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            GTEST_LOG_(INFO) << "resultSet is null, querySql: " << querySql;
            return false;
        }
        maxAnalysisFileId = GetInt64Val("file_id", resultSet);
        return true;
    }
    bool GetTabAnalysisSearchIndexMaxId(int64_t &tabAnalysisSearchIndexMaxId)
    {
        // Use COALSECE to avoid NULL result when the table is empty.
        std::string querySql = "SELECT COALESCE(MAX(id), 0) AS id FROM tab_analysis_search_index;";
        auto resultSet = this->rdbStore_->QuerySql(querySql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            GTEST_LOG_(INFO) << "resultSet is null, querySql: " << querySql;
            return false;
        }
        tabAnalysisSearchIndexMaxId = GetInt64Val("id", resultSet);
        return true;
    }
    bool RollbackPhotosToCheckpoint()
    {
        // Accept 0 as maxFileId_ means no photo data.
        if (this->maxFileId_ < 0) {
            GTEST_LOG_(INFO) << "RollBack Photos Error";
            return false;
        }
        GTEST_LOG_(INFO) << "RollBack Photos file_id:" << this->maxFileId_;
        std::string execSql = "DELETE FROM Photos WHERE file_id > ?;";
        std::vector<NativeRdb::ValueObject> bindArgs = {this->maxFileId_};
        int32_t ret = this->rdbStore_->ExecuteSql(execSql, bindArgs);
        return ret == NativeRdb::E_OK;
    }
    bool RollbackAlbumsToCheckpoint()
    {
        // Reject 0 as maxAlbumId_ means album data never be empty.
        if (this->maxAlbumId_ <= 0) {
            GTEST_LOG_(INFO) << "RollBack PhotoAlbum Error";
            return false;
        }
        GTEST_LOG_(INFO) << "RollBack PhotoAlbum album_id:" << this->maxAlbumId_;
        std::string execSql = "DELETE FROM PhotoAlbum WHERE album_id > ?;";
        std::vector<NativeRdb::ValueObject> bindArgs = {this->maxAlbumId_};
        int32_t ret = this->rdbStore_->ExecuteSql(execSql, bindArgs);
        return ret == NativeRdb::E_OK;
    }
    bool RollbackAnalysisToCheckpoint()
    {
        // Reject 0 as maxAnalysisId_ means album data never be empty.
        if (this->maxAnalysisId_ < 0) {
            GTEST_LOG_(INFO) << "RollBack tab_analysis_total to check point Error";
            return false;
        }
        GTEST_LOG_(INFO) << "RollBack tab_analysis_total id:" << this->maxAnalysisId_;
        std::string execSql = "DELETE FROM tab_analysis_total WHERE id > ?;";
        std::vector<NativeRdb::ValueObject> bindArgs = {this->maxAnalysisId_};
        int32_t ret = this->rdbStore_->ExecuteSql(execSql, bindArgs);
        return ret == NativeRdb::E_OK;
    }
    bool RollbackTabAnalysisSearchIndexToCheckpoint()
    {
        // Reject 0 as maxAnalysisId_ means album data never be empty.
        if (this->maxAnalysisId_ < 0) {
            GTEST_LOG_(INFO) << "RollBack tab_analysis_search_index to check point Error";
            return false;
        }
        GTEST_LOG_(INFO) << "RollBack tab_analysis_search_index id:" << this->tabAnalysisSearchIndexMaxId_;
        std::string execSql = "DELETE FROM tab_analysis_search_index WHERE id > ?;";
        std::vector<NativeRdb::ValueObject> bindArgs = {this->tabAnalysisSearchIndexMaxId_};
        int32_t ret = this->rdbStore_->ExecuteSql(execSql, bindArgs);
        return ret == NativeRdb::E_OK;
    }
    bool DeleteAnalysisRecords()
    {
        if (this->maxAnalysisFileId_ <= 0) {
            GTEST_LOG_(INFO) << "delete tab_analysis_total Error";
            return false;
        }
        GTEST_LOG_(INFO) << "DeleteAnalysisRecords tab_analysis_total file_id:" << this->maxFileId_;
        std::string execSql = "DELETE FROM tab_analysis_total WHERE file_id > ?;";
        std::vector<NativeRdb::ValueObject> bindArgs = {this->maxFileId_};
        int32_t ret = this->rdbStore_->ExecuteSql(execSql, bindArgs);
        return ret == NativeRdb::E_OK;
    }
    int32_t MockData(const TableMockInfo &tableMockInfo)
    {
        const std::string &tableName = tableMockInfo.tableName;
        const std::string &csvFilePath = tableMockInfo.csvFilePath;
        const std::vector<std::string> &columnNames = tableMockInfo.columnNames;
        CSVFileReader csvFileReader(csvFilePath);
        GTEST_LOG_(INFO) << "MockData table:" << tableName << ", csv:" << csvFilePath;
        int32_t ret = csvFileReader.ReadCSVFile();
        if (ret != E_OK) {
            return E_PHOTOALBUM_MOCKDATA_CSV_FAIL;
        }
        std::shared_ptr<NativeRdb::RdbStore> rdbStore = MediaLibraryDatabase().GetRdbStore();
        if (rdbStore == nullptr) {
            return E_RDB_STORE_NULL;
        }
        TableWriter tableWriter(rdbStore);
        return tableWriter.SetTableName(tableName).SetColumnNames(columnNames).SetReader(csvFileReader).Write();
    }
};
}  // namespace OHOS::Media::TestUtils
#endif  // TDD_DATABASE_DATA_MOCK_H