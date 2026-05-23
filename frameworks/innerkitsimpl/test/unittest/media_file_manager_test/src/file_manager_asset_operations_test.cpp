/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "FileManagerAssetOperationsTest"

#include "file_manager_asset_operations_test.h"
#include "file_manager_asset_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_errno.h"
#include "medialibrary_unittest_utils.h"
#include "asset_accurate_refresh.h"
#include "medialibrary_unistore_manager.h"
#include "refresh_business_name.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const string SQL_INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
    MediaColumn::MEDIA_FILE_PATH + ", " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + ", " +
    PhotoColumn::PHOTO_STORAGE_PATH + ")";

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    system("rm -rf /storage/cloud/files/Photo/16/");
    return E_OK;
}

void FileManagerAssetOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start FileManagerAssetOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("FileManagerAssetOperationsTest::SetUpTestCase");
}

void FileManagerAssetOperationsTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("FileManagerAssetOperationsTest::TearDownTestCase");
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void FileManagerAssetOperationsTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("FileManagerAssetOperationsTest::SetUp");
}

void FileManagerAssetOperationsTest::TearDown()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("FileManagerAssetOperationsTest::TearDown");
}

HWTEST_F(FileManagerAssetOperationsTest, MoveAssetsFromFileManager_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveAssetsFromFileManager_test_001 start");

    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + " VALUES('/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 0, "
        "'/storage/media/local/files/Docs/Download/mediatool/1.jpg')");
    std::vector<std::string> ids = {"1", "2", "3", "4", "5", "6", "7", "8"};
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME);
    int32_t ret = FileManagerAssetOperations::MoveAssetsFromFileManager(assetRefresh, ids, true);
    EXPECT_EQ(ret, E_OK);

    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + " VALUES('/storage/cloud/files/Photo/1/IMG_1501924305_001.jpg', 1, "
        "'/storage/media/local/files/Docs/Download/mediatool/2.jpg')");
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + " VALUES('', 1, "
        "'/storage/media/local/files/Docs/Download/mediatool/3.jpg')");
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + " VALUES('/storage/cloud/files/Photo/2/IMG_1501924305_002.jpg', 1, '')");
    ret = FileManagerAssetOperations::MoveAssetsFromFileManager(assetRefresh, ids, true);
    EXPECT_NE(ret, E_OK);

    MEDIA_INFO_LOG("MoveAssetsFromFileManager_test_001 end");
}

HWTEST_F(FileManagerAssetOperationsTest, MoveFileManagerAsset_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveFileManagerAsset_test_002 start");

    std::string srcPath = "/storage/media/local/files/Docs/Download/mediatool/7.jpg";
    std::string destPath = "/storage/cloud/files/Photo/2/IMG_1501924305_007.jpg";
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(srcPath, destPath);
    EXPECT_EQ(ret, E_ERR);

    MEDIA_INFO_LOG("MoveFileManagerAsset_test_002 end");
}
} // namespace Media
} // namespace OHOS