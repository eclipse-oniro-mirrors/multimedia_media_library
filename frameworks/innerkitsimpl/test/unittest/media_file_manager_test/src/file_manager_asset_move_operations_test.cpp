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

#define MLOG_TAG "FileManagerAssetMoveOperationsTest"

#include "file_manager_asset_operations_test.h"
#include "file_manager_asset_move_operations_test.h"
#include "file_manager_asset_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_errno.h"
#include "medialibrary_unittest_utils.h"
#include "asset_accurate_refresh.h"
#include "medialibrary_unistore_manager.h"
#include "refresh_business_name.h"
#include "media_log.h"
#include "file_asset.h"
#include "medialibrary_command.h"
#include <fstream>

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
    string sql = "DELETE FROM " + table;
    int32_t err = g_rdbStore->ExecuteSql(sql);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to clear table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    sql = "UPDATE sqlite_sequence SET seq = 0 WHERE name = '" + table + "'";
    err = g_rdbStore->ExecuteSql(sql);
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to clear table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    system("rm -rf /storage/cloud/files/Photo/16/");
    return E_OK;
}

void FileManagerAssetMoveOperationsTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start FileManagerAssetMoveOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("FileManagerAssetMoveOperationsTest::SetUpTestCase");
}

void FileManagerAssetMoveOperationsTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("FileManagerAssetMoveOperationsTest::TearDownTestCase");
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void FileManagerAssetMoveOperationsTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUp");
}

void FileManagerAssetMoveOperationsTest::TearDown()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, MoveFileManagerAsset_4arg_LocalRename_SrcNotExist, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveFileManagerAsset_4arg_LocalRename_SrcNotExist start");
    std::string src = "/data/local/tmp/fm_4arg_lr_nonexist_src.jpg";
    std::string dest = "/data/local/tmp/fm_4arg_lr_nonexist_dest.jpg";
    MediaFileUtils::DeleteFile(src);
    MediaFileUtils::DeleteFile(dest);
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(src, dest, false, true);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MoveFileManagerAsset_4arg_LocalRename_SrcNotExist end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, MoveFileManagerAsset_4arg_Copy_SrcNotExist, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveFileManagerAsset_4arg_Copy_SrcNotExist start");
    std::string src = "/data/local/tmp/fm_4arg_cp_nonexist_src.jpg";
    std::string dest = "/data/local/tmp/fm_4arg_cp_nonexist_dest.jpg";
    MediaFileUtils::DeleteFile(src);
    MediaFileUtils::DeleteFile(dest);
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(src, dest, false, false);
    EXPECT_EQ(ret, E_ERR);
    MEDIA_INFO_LOG("MoveFileManagerAsset_4arg_Copy_SrcNotExist end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, MoveFileManagerAsset_4arg_LocalRename_Success, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveFileManagerAsset_4arg_LocalRename_Success start");
    std::string src = "/data/local/tmp/fm_4arg_src.jpg";
    std::string dest = "/data/local/tmp/fm_4arg_dest.jpg";
    MediaFileUtils::DeleteFile(src);
    MediaFileUtils::DeleteFile(dest);
    std::ofstream(src).put('x');
    int32_t ret = FileManagerAssetOperations::MoveFileManagerAsset(src, dest, false, true);
    EXPECT_EQ(ret, E_OK);
    MediaFileUtils::DeleteFile(src);
    MediaFileUtils::DeleteFile(dest);
    MEDIA_INFO_LOG("MoveFileManagerAsset_4arg_LocalRename_Success end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, MoveAssetsToFileManager_EmptyIds, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveAssetsToFileManager_EmptyIds start");
    AccurateRefresh::AssetAccurateRefresh refresh(AccurateRefresh::UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME);
    std::vector<std::string> ids;
    int32_t ret = FileManagerAssetOperations::MoveAssetsToFileManager(refresh, ids);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("MoveAssetsToFileManager_EmptyIds end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, MoveAssetsFromFileManager_EmptyIds, TestSize.Level1)
{
    MEDIA_INFO_LOG("MoveAssetsFromFileManager_EmptyIds start");
    AccurateRefresh::AssetAccurateRefresh refresh(AccurateRefresh::UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME);
    std::vector<std::string> ids;
    int32_t ret = FileManagerAssetOperations::MoveAssetsFromFileManager(refresh, ids, true);
    EXPECT_EQ(ret, E_INVALID_ARGUMENTS);
    MEDIA_INFO_LOG("MoveAssetsFromFileManager_EmptyIds end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, CheckAndRename_NonFileManager_Skip, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAndRename_NonFileManager_Skip start");
    AccurateRefresh::AssetAccurateRefresh refresh(AccurateRefresh::UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME);
    MediaLibraryCommand cmd((NativeRdb::ValuesBucket()));
    auto fileAsset = make_shared<FileAsset>();
    fileAsset->SetFileSourceType(0);
    fileAsset->SetStoragePath("/storage/media/local/files/Docs/test.jpg");
    EXPECT_EQ(FileManagerAssetOperations::CheckAndRenameFileManagerAsset(refresh, cmd, fileAsset), E_OK);
    MEDIA_INFO_LOG("CheckAndRename_NonFileManager_Skip end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, CheckAndRename_NoDocsPrefix_Skip, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAndRename_NoDocsPrefix_Skip start");
    AccurateRefresh::AssetAccurateRefresh refresh(AccurateRefresh::UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME);
    MediaLibraryCommand cmd((NativeRdb::ValuesBucket()));
    auto fileAsset = make_shared<FileAsset>();
    fileAsset->SetFileSourceType(1);
    fileAsset->SetStoragePath("/data/local/tmp/other.jpg");
    EXPECT_EQ(FileManagerAssetOperations::CheckAndRenameFileManagerAsset(refresh, cmd, fileAsset), E_OK);
    MEDIA_INFO_LOG("CheckAndRename_NoDocsPrefix_Skip end");
}

HWTEST_F(FileManagerAssetMoveOperationsTest, CheckAndRename_EmptyStoragePath_Skip, TestSize.Level1)
{
    MEDIA_INFO_LOG("CheckAndRename_EmptyStoragePath_Skip start");
    AccurateRefresh::AssetAccurateRefresh refresh(AccurateRefresh::UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME);
    MediaLibraryCommand cmd((NativeRdb::ValuesBucket()));
    auto fileAsset = make_shared<FileAsset>();
    fileAsset->SetFileSourceType(1);
    fileAsset->SetStoragePath("");
    EXPECT_EQ(FileManagerAssetOperations::CheckAndRenameFileManagerAsset(refresh, cmd, fileAsset), E_OK);
    MEDIA_INFO_LOG("CheckAndRename_EmptyStoragePath_Skip end");
}
} // namespace Media
} // namespace OHOS
