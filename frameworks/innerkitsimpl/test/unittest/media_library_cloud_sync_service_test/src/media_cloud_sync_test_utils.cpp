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

#define MLOG_TAG "MediaCloudSync"

#include "media_cloud_sync_test_utils.h"

#include <vector>
#include <unordered_map>
#include "gtest/gtest.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "media_unique_number_column.h"
#include "medialibrary_db_const_sqls.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "medialibrary_asset_operations.h"
#include "media_audio_column.h"
#include "media_upgrade.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::CloudSync {
static unordered_map<string, ResultSetDataType> albumColumnTypeMap = {
    {PhotoAlbumColumns::ALBUM_ID, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_TYPE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_SUBTYPE, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_NAME, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_COVER_URI, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_DATE_MODIFIED, ResultSetDataType::TYPE_INT64},
    {PhotoAlbumColumns::CONTAINS_HIDDEN, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::HIDDEN_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::HIDDEN_COVER, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_ORDER, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_IMAGE_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_VIDEO_COUNT, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_BUNDLE_NAME, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_IS_LOCAL, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_CLOUD_ID, ResultSetDataType::TYPE_STRING},
    {PhotoAlbumColumns::ALBUM_DIRTY, ResultSetDataType::TYPE_INT32},
    {PhotoAlbumColumns::ALBUM_LPATH, ResultSetDataType::TYPE_STRING},
};

void CleanTestTables(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE, AudioColumn::AUDIOS_TABLE,        MEDIALIBRARY_TABLE,
        ASSET_UNIQUE_NUMBER_TABLE, PhotoExtColumn::PHOTOS_EXT_TABLE, PhotoAlbumColumns::TABLE};
    for (const std::string &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "Drop table: " << dropTable << " failed";
            continue;
        }
    }
}

void PrepareUniqueNumberTable(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    if (rdbStore == nullptr) {
        GTEST_LOG_(ERROR) << "can not get g_rdbstore";
        return;
    }

    string queryRowSql = "SELECT COUNT(*) as count FROM " + ASSET_UNIQUE_NUMBER_TABLE;
    auto resultSet = rdbStore->QuerySql(queryRowSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        GTEST_LOG_(ERROR) << "Can not get AssetUniqueNumberTable count";
        return;
    }
    if (GetInt32Val("count", resultSet) != 0) {
        GTEST_LOG_(INFO) << "AssetUniqueNumberTable is already inited";
        return;
    }

    UniqueMemberValuesBucket imageBucket = {IMAGE_ASSET_TYPE, 1};
    UniqueMemberValuesBucket videoBucket = {VIDEO_ASSET_TYPE, 1};
    UniqueMemberValuesBucket audioBucket = {AUDIO_ASSET_TYPE, 1};

    vector<UniqueMemberValuesBucket> uniqueNumberValueBuckets = {imageBucket, videoBucket, audioBucket};

    for (const auto &uniqueNumberValueBucket : uniqueNumberValueBuckets) {
        ValuesBucket valuesBucket;
        valuesBucket.PutString(ASSET_MEDIA_TYPE, uniqueNumberValueBucket.assetMediaType);
        valuesBucket.PutInt(UNIQUE_NUMBER, uniqueNumberValueBucket.startNumber);
        int64_t outRowId = -1;
        int32_t insertResult = rdbStore->Insert(outRowId, ASSET_UNIQUE_NUMBER_TABLE, valuesBucket);
        if (insertResult != NativeRdb::E_OK || outRowId <= 0) {
            GTEST_LOG_(ERROR) << "Prepare smartAlbum failed";
        }
    }
}

void SetTestTables(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    if (rdbStore == nullptr) {
        GTEST_LOG_(ERROR) << "can not get g_rdbstore";
        return;
    }

    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        AudioColumn::CREATE_AUDIO_TABLE,
        CREATE_MEDIA_TABLE,
        CREATE_ASSET_UNIQUE_NUMBER_TABLE,
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
        PhotoAlbumColumns::CREATE_TABLE
        // todo: album tables
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            GTEST_LOG_(ERROR) << "create table: " << createTableSql << " failed";
            return;
        }
    }
    PrepareUniqueNumberTable(rdbStore);
}

void InitTestTables(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    CleanTestTables(rdbStore);
    SetTestTables(rdbStore);
}

void ClearAndRestart(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
    }
    CleanTestTables(rdbStore);
    SetTestTables(rdbStore);
}

void SetValuesBucketInPhotosTable(const string &columnKey, const string &columnValue, ValuesBucket &values)
{
    if (GetFileAssetMemberMap().find(columnKey) == GetFileAssetMemberMap().end()) {
        GTEST_LOG_(ERROR) << "columnKey: " << columnKey << " not found";
        return;
    }

    int type = GetFileAssetMemberMap().at(columnKey);
    switch (type) {
        case MEMBER_TYPE_INT32:
            values.PutInt(columnKey, stoi(columnValue));
            break;
        case MEMBER_TYPE_INT64:
            values.PutLong(columnKey, stol(columnValue));
            break;
        case MEMBER_TYPE_STRING:
            values.PutString(columnKey, columnValue);
            break;
        case MEMBER_TYPE_DOUBLE:
            values.PutDouble(columnKey, stod(columnValue));
            break;
        default:
            GTEST_LOG_(ERROR) << "this column type: " << columnKey << " not excepted";
            break;
    }
}

void SetValuesBucketInPhotoAlbumTable(const string &columnKey, const string &columnValue, ValuesBucket &values)
{
    if (albumColumnTypeMap.find(columnKey) == albumColumnTypeMap.end()) {
        GTEST_LOG_(ERROR) << "columnKey: " << columnKey << " not found";
        return;
    }

    ResultSetDataType type = albumColumnTypeMap.at(columnKey);
    switch (type) {
        case ResultSetDataType::TYPE_INT32:
            values.PutInt(columnKey, stoi(columnValue));
            break;
        case ResultSetDataType::TYPE_INT64:
            values.PutLong(columnKey, stol(columnValue));
            break;
        case ResultSetDataType::TYPE_STRING:
            values.PutString(columnKey, columnValue);
            break;
        case ResultSetDataType::TYPE_DOUBLE:
            values.PutDouble(columnKey, stod(columnValue));
            break;
        default:
            GTEST_LOG_(ERROR) << "this column type: " << columnKey << " not excepted";
            break;
    }
}

void InitPhotosTable(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    const std::string photoTable = "Photos";
    int64_t rowId = -1;
    int32_t ret = E_OK;

    ValuesBucket values1;
    SetValuesBucketInPhotosTable("file_id", "1", values1);
    SetValuesBucketInPhotosTable("data", "xxxxx", values1);
    SetValuesBucketInPhotosTable("size", "100000", values1);
    SetValuesBucketInPhotosTable("title", "yyyyyy", values1);
    ret = rdbStore->Insert(rowId, photoTable, values1);
    std::cout << "rowId: " << rowId << ", ret: " << ret << std::endl;
    if (ret != E_OK) {
        GTEST_LOG_(ERROR) << "insert photos table failed";
    }

    ValuesBucket values2;
    SetValuesBucketInPhotosTable("file_id", "2", values2);
    SetValuesBucketInPhotosTable("data", "xxxxx", values2);
    SetValuesBucketInPhotosTable("size", "100000", values2);
    SetValuesBucketInPhotosTable("title", "yyyyyy", values2);
    ret = rdbStore->Insert(rowId, photoTable, values2);
    std::cout << "rowId: " << rowId << ", ret: " << ret << std::endl;
    if (ret != E_OK) {
        GTEST_LOG_(ERROR) << "insert photos table failed";
    }
}

void InitPhotoAlbumTable(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    const std::string photoAlbumTable = "PhotoAlbum";
    int64_t rowId = -1;
    int32_t ret = E_OK;

    ValuesBucket values1;
    SetValuesBucketInPhotoAlbumTable("album_id", "1", values1);
    SetValuesBucketInPhotoAlbumTable("album_type", "1024", values1);
    SetValuesBucketInPhotoAlbumTable("album_subtype", "0", values1);
    SetValuesBucketInPhotoAlbumTable("album_name", "local_test", values1);
    ret = rdbStore->Insert(rowId, photoAlbumTable, values1);
    std::cout << "rowId: " << rowId << ", ret: " << ret << std::endl;
    if (ret != E_OK) {
        GTEST_LOG_(ERROR) << "insert photoAlbum table failed";
    }

    ValuesBucket values2;
    SetValuesBucketInPhotoAlbumTable("album_id", "2", values2);
    SetValuesBucketInPhotoAlbumTable("album_type", "1024", values2);
    SetValuesBucketInPhotoAlbumTable("album_subtype", "0", values2);
    SetValuesBucketInPhotoAlbumTable("album_name", "local_test", values2);
    ret = rdbStore->Insert(rowId, photoAlbumTable, values2);
    std::cout << "rowId: " << rowId << ", ret: " << ret << std::endl;
    if (ret != E_OK) {
        GTEST_LOG_(ERROR) << "insert photoAlbum table failed";
    }
}

int32_t InsertTable(std::shared_ptr<MediaLibraryRdbStore> rdbStore, const std::string &tableName,
                    OHOS::NativeRdb::ValuesBucket &values)
{
    int64_t rowId = -1;
    int32_t ret = rdbStore->Insert(rowId, tableName, values);
    if (ret != E_OK) {
        GTEST_LOG_(ERROR) << "insert table: " << tableName << ", failed, ret: " << ret;
    }
    return ret;
}
}