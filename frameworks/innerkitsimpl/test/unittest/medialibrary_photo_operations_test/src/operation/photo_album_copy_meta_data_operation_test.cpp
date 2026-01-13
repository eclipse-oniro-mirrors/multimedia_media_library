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

#define MLOG_TAG "PhotoAlbumCopyMetaDataOperationTest"

#include "ability_context_impl.h"
#include "photo_album_copy_meta_data_operation_test.h"
#include "medialibrary_db_const_sqls.h"

#include <string>
#include <vector>

#include "photo_album_copy_meta_data_operation.h"
#include "media_log.h"
#include "media_upgrade.h"

using namespace testing::ext;

namespace OHOS::Media {
std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr = nullptr;

void CleanTestTable()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        MEDIALIBRARY_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = rdbStorePtr->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void SetTable()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_MEDIA_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (rdbStorePtr == nullptr) {
            MEDIA_ERR_LOG("can not get rdbStorePtr");
            return;
        }
        int32_t ret = rdbStorePtr->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void PhotoAlbumCopyMetaDataOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    rdbStorePtr = std::make_shared<MediaLibraryRdbStore>(abilityContextImpl);
    int32_t ret = rdbStorePtr->Init();
    CleanTestTable();
    SetTable();
    MEDIA_INFO_LOG("PhotoAlbumCopyMetaDataOperationTest rdbstore start ret = %{public}d", ret);
}

void PhotoAlbumCopyMetaDataOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
}

void PhotoAlbumCopyMetaDataOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void PhotoAlbumCopyMetaDataOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, CopyAlbumMetaData_Test, TestSize.Level1)
{
    NativeRdb::ValuesBucket values;
    int32_t newAlbumId = PhotoAlbumCopyMetaDataOperation()
                                .SetRdbStore(nullptr)
                                .CopyAlbumMetaData(values);
    EXPECT_EQ(newAlbumId, -1);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, ReadAlbumValue_EmptyValues_Test, TestSize.Level0)
{
    NativeRdb::ValuesBucket values;
    PhotoAlbumCopyMetaDataOperation::AlbumInfo albumInfo;
    PhotoAlbumCopyMetaDataOperation().SetRdbStore(nullptr).ReadAlbumValue(albumInfo, values);

    EXPECT_EQ(albumInfo.albumType, -1);
    EXPECT_EQ(albumInfo.albumName, "");
    EXPECT_EQ(albumInfo.bundleName, "");
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, FindAlbumInfo_AlbumTypeSource_Test, TestSize.Level0)
{
    PhotoAlbumCopyMetaDataOperation::AlbumInfo albumInfo;
    albumInfo.albumId = 1;
    albumInfo.albumType = OHOS::Media::PhotoAlbumType::SOURCE;
    albumInfo.albumName = "MyAlbum";
    albumInfo.bundleName = "com.example.album";
    albumInfo.lPath = "";

    PhotoAlbumCopyMetaDataOperation operation;
    operation.FindAlbumInfo(albumInfo);

    EXPECT_EQ(albumInfo.lPath, "");
    EXPECT_EQ(albumInfo.albumName, "MyAlbum");
    EXPECT_EQ(albumInfo.bundleName, "com.example.album");
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, FindAlbumInfo_AlbumTypeNotSource_Test, TestSize.Level0)
{
    PhotoAlbumCopyMetaDataOperation::AlbumInfo albumInfo;
    albumInfo.albumId = 2;
    albumInfo.albumType = 1;
    albumInfo.albumName = "AnotherAlbum";
    albumInfo.bundleName = "com.example.anotheralbum";
    albumInfo.lPath = "/initial/path";

    PhotoAlbumCopyMetaDataOperation operation;
    operation.FindAlbumInfo(albumInfo);

    EXPECT_EQ(albumInfo.lPath, "/Pictures/Users/AnotherAlbum");
    EXPECT_EQ(albumInfo.albumName, "AnotherAlbum");
    EXPECT_EQ(albumInfo.bundleName, "com.example.anotheralbum");
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, GetOrCreateAlbum_MediaRdbStoreIsNull_Test, TestSize.Level0)
{
    std::string lPath = "/path/to/album";
    NativeRdb::ValuesBucket values;
    PhotoAlbumCopyMetaDataOperation operation;
    int64_t albumId = operation.GetOrCreateAlbum(lPath, values);
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, GetOrCreateAlbum_AlbumExistsNotDeleted_Test, TestSize.Level0)
{
    std::string lPath = "/path/to/existing/album";
    NativeRdb::ValuesBucket values;
    
    NativeRdb::ValuesBucket insertValues;
    insertValues.PutString(PhotoAlbumColumns::ALBUM_LPATH, lPath);
    insertValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED));
    int64_t rowId = -1;
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Insert(rowId, PhotoAlbumColumns::TABLE, insertValues);

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int64_t albumId = operation.GetOrCreateAlbum(lPath, values);
    EXPECT_GT(albumId, 0);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, GetOrCreateAlbum_AlbumExistsButDeleted_Test, TestSize.Level0)
{
    std::string lPath = "/path/to/deleted/album";
    NativeRdb::ValuesBucket values;

    NativeRdb::ValuesBucket insertValues;
    insertValues.PutString(PhotoAlbumColumns::ALBUM_LPATH, lPath);
    insertValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
    int64_t rowId = -1;
    ASSERT_NE(rdbStorePtr, nullptr);
    int32_t ret = rdbStorePtr->Insert(rowId, PhotoAlbumColumns::TABLE, insertValues);
    ASSERT_EQ(ret, E_SUCCESS) << "Failed to insert test data.";

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int64_t albumId = operation.GetOrCreateAlbum(lPath, values);
    EXPECT_GT(albumId, 0);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, GetLatestAlbumIdBylPath_NullOrEmptyInput_Test, TestSize.Level0)
{
    std::string emptyLpath = "";
    int32_t dirty = 0;
    
    int64_t albumId = PhotoAlbumCopyMetaDataOperation().GetLatestAlbumIdBylPath(emptyLpath, dirty);
    EXPECT_EQ(albumId, -1);

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(nullptr);
    albumId = operation.GetLatestAlbumIdBylPath("some/path", dirty);
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, GetLatestAlbumIdBylPath_NoMatchFound_Test, TestSize.Level0)
{
    std::string nonExistingLpath = "/non/existing/album";
    int32_t dirty = 0;

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int64_t albumId = operation.GetLatestAlbumIdBylPath(nonExistingLpath, dirty);
    EXPECT_EQ(albumId, -1);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, GetLatestAlbumIdBylPath_Success_Test, TestSize.Level0)
{
    std::string existingLpath = "/existing/album";
    NativeRdb::ValuesBucket insertValues;
    insertValues.PutString(PhotoAlbumColumns::ALBUM_LPATH, existingLpath);
    insertValues.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED));
    insertValues.PutInt(PhotoAlbumColumns::ALBUM_ID, 123);
    int64_t rowId = -1;
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Insert(rowId, PhotoAlbumColumns::TABLE, insertValues);

    int32_t dirty = 0;
    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int64_t albumId = operation.GetLatestAlbumIdBylPath(existingLpath, dirty);
    EXPECT_GT(albumId, 0);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, QueryAlbumPluginInfo_MediaRdbStoreIsNull_Test, TestSize.Level0)
{
    std::string lPath;
    std::string bundleName;
    std::string albumName = "testAlbum";

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(nullptr);
    int32_t result = operation.QueryAlbumPluginInfo(lPath, bundleName, albumName);

    EXPECT_EQ(result, E_INVALID_ARGUMENTS);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, QueryAlbumPluginInfo_NoMatchFound_Test, TestSize.Level0)
{
    std::string lPath;
    std::string bundleName;
    std::string albumName = "nonExistingAlbum";

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int32_t result = operation.QueryAlbumPluginInfo(lPath, bundleName, albumName);

    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(lPath, "/Pictures/nonExistingAlbum");
    EXPECT_TRUE(bundleName.empty());
    EXPECT_EQ(albumName, "nonExistingAlbum");
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, QueryAlbumPluginInfo_QueryByName_Success_Test, TestSize.Level0)
{
    std::string existingLpath = "/existing/album";
    std::string existingBundleName = "com.example.bundle";
    std::string existingAlbumName = "existingAlbum";

    NativeRdb::ValuesBucket insertValues;
    insertValues.PutString(PhotoAlbumColumns::ALBUM_LPATH, existingLpath);
    insertValues.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, existingBundleName);
    insertValues.PutString(PhotoAlbumColumns::ALBUM_NAME, existingAlbumName);
    int64_t rowId = -1;
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Insert(rowId, PhotoAlbumColumns::TABLE, insertValues);

    std::string lPath;
    std::string bundleName;
    std::string albumName = existingAlbumName;

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int32_t result = operation.QueryAlbumPluginInfo(lPath, bundleName, albumName);

    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(lPath, "/Pictures/existingAlbum");
    EXPECT_EQ(bundleName, "");
    EXPECT_EQ(albumName, existingAlbumName);
}

HWTEST_F(PhotoAlbumCopyMetaDataOperationTest, QueryAlbumPluginInfo_QueryByBundleAndName_Success_Test, TestSize.Level0)
{
    std::string existingLpath = "/existing/bundleSpecificAlbum";
    std::string existingBundleName = "com.example.specificBundle";
    std::string existingAlbumName = "specificAlbum";

    NativeRdb::ValuesBucket insertValues;
    insertValues.PutString(PhotoAlbumColumns::ALBUM_LPATH, existingLpath);
    insertValues.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, existingBundleName);
    insertValues.PutString(PhotoAlbumColumns::ALBUM_NAME, existingAlbumName);
    int64_t rowId = -1;
    ASSERT_NE(rdbStorePtr, nullptr);
    rdbStorePtr->Insert(rowId, PhotoAlbumColumns::TABLE, insertValues);

    std::string lPath;
    std::string bundleName = existingBundleName;
    std::string albumName = existingAlbumName;

    PhotoAlbumCopyMetaDataOperation operation;
    operation.SetRdbStore(rdbStorePtr);
    int32_t result = operation.QueryAlbumPluginInfo(lPath, bundleName, albumName);

    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(lPath, "/Pictures/specificAlbum");
    EXPECT_EQ(bundleName, existingBundleName);
    EXPECT_EQ(albumName, existingAlbumName);
}
} // namespace OHOS::Media