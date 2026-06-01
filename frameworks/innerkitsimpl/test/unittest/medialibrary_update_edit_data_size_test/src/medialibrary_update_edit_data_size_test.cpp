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

#define MLOG_TAG "MediaLibraryUpdateEditDataSizeTest"

#include "medialibrary_update_edit_data_size_test.h"

#include <fstream>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "attachment_size_update_operation.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "map_operation_flag.h"
#include "media_column_type.h"
#include "photo_album_column.h"
#include "fetch_result.h"
#include "result_set_utils.h"

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
namespace {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

const int64_t SIZE_DEFAULT = 0;
const int64_t SIZE_VALUE = 100;

void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        PhotoExtColumn::PHOTOS_EXT_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
    }
}

void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        PhotoExtUpgrade::CREATE_PHOTO_EXT_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
    }
}

struct InsertPhotoParams {
    int64_t size;
    int64_t localAssetSize;
    int32_t position;
    bool isTemp;
    std::string path;
    int32_t movingPhotoEffectMode;
    int64_t attachmentSize = -1;
    int32_t syncStatus = 0;
    int32_t cleanFlag = 0;
    int32_t timePending = 0;
};

int64_t InsertPhoto(const InsertPhotoParams &params)
{
    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_SIZE, params.size);
    values.Put(PhotoColumn::LOCAL_ASSET_SIZE, params.localAssetSize);
    values.Put(PhotoColumn::PHOTO_POSITION, params.position);
    values.Put(PhotoColumn::PHOTO_IS_TEMP, std::to_string(static_cast<int32_t>(params.isTemp)));
    values.Put(MediaColumn::MEDIA_FILE_PATH, params.path);
    values.Put(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, params.movingPhotoEffectMode);
    values.Put(PhotoColumn::ATTACHMENT_SIZE, params.attachmentSize);
    values.Put(PhotoColumn::PHOTO_SYNC_STATUS, params.syncStatus);
    values.Put(PhotoColumn::PHOTO_CLEAN_FLAG, params.cleanFlag);
    values.Put(MediaColumn::MEDIA_TIME_PENDING, params.timePending);

    int64_t outRowId = -1;
    int32_t insertResult = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    if (insertResult != NativeRdb::E_OK) {
        return -1;
    }
    return outRowId;
}

bool InsertPhotoExt(int64_t photoId, int64_t editDataSize)
{
    ValuesBucket values;
    values.PutLong(PhotoExtColumn::PHOTO_ID, photoId);
    values.PutLong(PhotoExtColumn::EDITDATA_SIZE, editDataSize);

    int64_t outRowId = -1;
    int32_t insertResult = g_rdbStore->Insert(outRowId, PhotoExtColumn::PHOTOS_EXT_TABLE, values);
    return insertResult == NativeRdb::E_OK;
}

int64_t QueryAttachmentSizeByPhotoId(int64_t fileId)
{
    static const std::vector<std::string> columns = { PhotoColumn::ATTACHMENT_SIZE };
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = g_rdbStore->Query(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    return GetInt64Val(PhotoColumn::ATTACHMENT_SIZE, resultSet);
}

int64_t QueryEditDataSizeByPhotoId(int64_t fileId)
{
    static const std::vector<std::string> columns = { PhotoExtColumn::EDITDATA_SIZE };
    NativeRdb::AbsRdbPredicates predicates(PhotoExtColumn::PHOTOS_EXT_TABLE);
    predicates.EqualTo(PhotoExtColumn::PHOTO_ID, fileId);
    auto resultSet = g_rdbStore->Query(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    return GetInt64Val(PhotoExtColumn::EDITDATA_SIZE, resultSet);
}

std::shared_ptr<NativeRdb::ResultSet> QueryPhotoResultSetById(int64_t fileId, const std::vector<std::string> &columns)
{
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    auto resultSet = g_rdbStore->Query(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return nullptr;
    }
    return resultSet;
}

std::unique_ptr<FileAsset> QueryFileAssetById(int64_t fileId)
{
    std::vector<std::string> columns;
    auto resultSet = QueryPhotoResultSetById(fileId, columns);
    if (resultSet == nullptr) {
        return nullptr;
    }
    FetchResult<FileAsset> fetchResult;
    return fetchResult.GetObjectFromRdb(resultSet, 0);
}

bool CreateFileWithSize(const std::string &path, size_t size)
{
    std::ofstream outFile(path, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }
    std::string payload(size, 'A');
    outFile.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    outFile.close();
    return outFile.good();
}
} // namespace

void MediaLibraryUpdateEditDataSizeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryUpdateEditDataSizeTest failed, can not get g_rdbStore");
        exit(1);
    }
    system("rm -rf /data/test/*");
    CleanTestTables();
    SetTables();
}

void MediaLibraryUpdateEditDataSizeTest::TearDownTestCase(void)
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryUpdateEditDataSizeTest::SetUp()
{
    CleanTestTables();
    SetTables();
}

void MediaLibraryUpdateEditDataSizeTest::TearDown()
{
    system("rm -rf /data/test/*");
    MAP_OPERATION_FLAG = false;
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, UpdateEditDataSize_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateEditDataSize_test_001 start");

    std::string editDataDir = "/data/test/UpdateEditDataSize_test_001";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(editDataDir), true);

    EXPECT_EQ(CreateFileWithSize(editDataDir + "/editdata", 10), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/editdata_camera", 20), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/source.jpg", 30), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/source.mp4", 40), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/source_back.heic", 50), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/transcode.jpg", 60), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/extraData", 70), true);

    int64_t fileId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/UpdateEditDataSize_test_001/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) });
    EXPECT_EQ(fileId > 0, true);
    EXPECT_EQ(InsertPhotoExt(fileId, 0), true);

    int32_t ret = MediaLibraryRdbStore::UpdateEditDataSize(g_rdbStore, std::to_string(fileId), editDataDir);
    EXPECT_EQ(ret, E_OK);

    const int64_t expectedEditDataSize = 10 + 20 + 30 + 40 + 50 + 60 + 70;
    const int64_t expectedAttachmentSize = 1024 + 10 + 20 + 30 + 40 + 50;
    EXPECT_EQ(QueryEditDataSizeByPhotoId(fileId), expectedEditDataSize);
    EXPECT_EQ(QueryAttachmentSizeByPhotoId(fileId), expectedAttachmentSize);
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, UpdateEditDataSize_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateEditDataSize_test_002 start");

    std::string editDataDir = "/data/test/UpdateEditDataSize_test_002";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(editDataDir), true);

    int64_t fileId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/UpdateEditDataSize_test_002/photo.jpg", static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) });
    EXPECT_EQ(fileId > 0, true);
    EXPECT_EQ(InsertPhotoExt(fileId, 999), true);

    int32_t ret = MediaLibraryRdbStore::UpdateEditDataSize(g_rdbStore, std::to_string(fileId), editDataDir);
    EXPECT_EQ(ret, E_OK);

    const int64_t expectedEditDataSize = 0;
    const int64_t expectedAttachmentSize = 1024;
    EXPECT_EQ(QueryEditDataSizeByPhotoId(fileId), expectedEditDataSize);
    EXPECT_EQ(QueryAttachmentSizeByPhotoId(fileId), expectedAttachmentSize);
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, AttachmentSize_FileAssetSetterGetter_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("AttachmentSize_FileAssetSetterGetter_test_003 start");

    std::string editDataDir = "/data/test/AttachmentSize_FileAssetSetterGetter_test_003";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(editDataDir), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/editdata", 64), true);

    int64_t fileId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/AttachmentSize_FileAssetSetterGetter_test_003/photo.jpg",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) });
    EXPECT_EQ(fileId > 0, true);
    EXPECT_EQ(InsertPhotoExt(fileId, 0), true);
    EXPECT_EQ(MediaLibraryRdbStore::UpdateEditDataSize(g_rdbStore, std::to_string(fileId), editDataDir), E_OK);

    std::vector<std::string> columns = { PhotoColumn::MEDIA_ID, PhotoColumn::ATTACHMENT_SIZE };
    auto queriedAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, std::to_string(fileId), OperationObject::FILESYSTEM_PHOTO, columns);
    ASSERT_NE(queriedAsset, nullptr);
    const int64_t expectedAttachmentSize = 1024 + 64;
    EXPECT_EQ(queriedAsset->GetAttachmentSize(), expectedAttachmentSize);

    FileAsset fileAsset;
    fileAsset.SetAttachmentSize(4096);
    EXPECT_EQ(fileAsset.GetAttachmentSize(), 4096);

    MEDIA_INFO_LOG("AttachmentSize_FileAssetSetterGetter_test_003 end");
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, AttachmentSize_ResultSetUtilsRead_test_004, TestSize.Level1)
{
    std::string editDataDir = "/data/test/AttachmentSize_ResultSetUtilsRead_test_004";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(editDataDir), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/editdata", 100), true);

    int64_t fileId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/AttachmentSize_ResultSetUtilsRead_test_004/photo.jpg",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) });
    EXPECT_EQ(fileId > 0, true);
    EXPECT_EQ(InsertPhotoExt(fileId, 0), true);
    EXPECT_EQ(MediaLibraryRdbStore::UpdateEditDataSize(g_rdbStore, std::to_string(fileId), editDataDir), E_OK);

    auto resultSet = QueryPhotoResultSetById(fileId, { PhotoColumn::ATTACHMENT_SIZE });
    ASSERT_NE(resultSet, nullptr);
    int64_t attachmentSize = get<int64_t>(
        ResultSetUtils::GetValFromColumn(PhotoColumn::ATTACHMENT_SIZE, resultSet, TYPE_INT64));
    const int64_t expectedAttachmentSize = 1024 + 100;
    EXPECT_EQ(attachmentSize, expectedAttachmentSize);
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, AttachmentSize_FetchResultMapToFileAsset_test_005, TestSize.Level1)
{
    std::string editDataDir = "/data/test/AttachmentSize_FetchResultMapToFileAsset_test_005";
    EXPECT_EQ(MediaFileUtils::CreateDirectory(editDataDir), true);
    EXPECT_EQ(CreateFileWithSize(editDataDir + "/editdata_camera", 88), true);

    int64_t fileId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/AttachmentSize_FetchResultMapToFileAsset_test_005/photo.jpg",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT) });
    EXPECT_EQ(fileId > 0, true);
    EXPECT_EQ(InsertPhotoExt(fileId, 0), true);
    EXPECT_EQ(MediaLibraryRdbStore::UpdateEditDataSize(g_rdbStore, std::to_string(fileId), editDataDir), E_OK);

    auto fileAsset = QueryFileAssetById(fileId);
    ASSERT_NE(fileAsset, nullptr);
    const int64_t expectedAttachmentSize = 1024 + 88;
    EXPECT_EQ(fileAsset->GetAttachmentSize(), expectedAttachmentSize);
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, AttachmentSize_FileAssetMemberTypeMap_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("AttachmentSize_FileAssetMemberTypeMap_test_006 start");
    const auto &memberMap = GetFileAssetMemberMap();
    auto iter = memberMap.find(PhotoColumn::ATTACHMENT_SIZE);
    ASSERT_NE(iter, memberMap.end());
    MEDIA_INFO_LOG("AttachmentSize member type: %{public}d", iter->second);
    EXPECT_EQ(iter->second, MEMBER_TYPE_INT64);
    MEDIA_INFO_LOG("AttachmentSize_FileAssetMemberTypeMap_test_006 end");
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, AttachmentSize_OrmPhotoColumnTypeMap_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("AttachmentSize_OrmPhotoColumnTypeMap_test_007 start");
    auto iter = ORM::MediaColumnType::PHOTOS_COLUMNS.find(PhotoColumn::ATTACHMENT_SIZE);
    ASSERT_NE(iter, ORM::MediaColumnType::PHOTOS_COLUMNS.end());
    MEDIA_INFO_LOG("AttachmentSize ORM data type: %{public}d", static_cast<int32_t>(iter->second));
    EXPECT_EQ(iter->second, ORM::MediaColumnType::DataType::LONG);
    MEDIA_INFO_LOG("AttachmentSize_OrmPhotoColumnTypeMap_test_007 end");
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, AttachmentSizeUpdateOperation_Stop_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("AttachmentSizeUpdateOperation_Stop_test_008 start");
    AttachmentSizeUpdateOperation::Stop();
    EXPECT_FALSE(AttachmentSizeUpdateOperation::isContinue_.load());
    MEDIA_INFO_LOG("AttachmentSizeUpdateOperation_Stop_test_008 end");
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, QueryAttachmentSizeAssets_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryAttachmentSizeAssets_test_009 start");
    int64_t matchId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/QueryAttachmentSizeAssets_test_009/match",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        -1, 0, 0, 0 });
    int64_t skipId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/QueryAttachmentSizeAssets_test_009/skip",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        -1, 1, 0, 0 });
    ASSERT_GT(matchId, 0);
    ASSERT_GT(skipId, 0);

    int32_t batchSize = -1;
    auto assetInfos = AttachmentSizeUpdateOperation::QueryAttachmentSizeAssets(0, static_cast<int32_t>(skipId),
        batchSize);
    ASSERT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, matchId);
    EXPECT_EQ(assetInfos[0].attachmentSize, -1);
    EXPECT_EQ(batchSize, matchId);
    MEDIA_INFO_LOG("QueryAttachmentSizeAssets_test_009 end");
}

HWTEST_F(MediaLibraryUpdateEditDataSizeTest, HandleAttachmentSizeAssets_test10, TestSize.Level1)
{
    MEDIA_INFO_LOG("HandleAttachmentSizeAssets_test10 start");
    std::string editDataDir = "/data/test/HandleAttachmentSizeAssets_test10";
    EXPECT_TRUE(MediaFileUtils::CreateDirectory(editDataDir));
    EXPECT_TRUE(CreateFileWithSize(editDataDir + "/editdata", 64));

    int64_t fileId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        editDataDir,
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        -1, 0, 0, 0 });
    ASSERT_GT(fileId, 0);
    ASSERT_TRUE(InsertPhotoExt(fileId, 0));
    ASSERT_EQ(QueryAttachmentSizeByPhotoId(fileId), -1);

    MAP_OPERATION_FLAG = true;
    std::vector<AttachmentSizeAssetInfo> assetInfos = {
        { static_cast<int32_t>(fileId), editDataDir, -1 }
    };
    AttachmentSizeUpdateOperation::HandleAttachmentSizeAssets(assetInfos);

    const int64_t expectedAttachmentSize = 1024 + 64;
    EXPECT_EQ(QueryAttachmentSizeByPhotoId(fileId), expectedAttachmentSize);
    MEDIA_INFO_LOG("HandleAttachmentSizeAssets_test10 end");
}
} // namespace OHOS::Media
