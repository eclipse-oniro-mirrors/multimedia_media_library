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

#define MLOG_TAG "TabOldPhotosRestoreTest"

#include "tab_old_photos_restore_test.h"

#include <string>

#define private public
#define protected public
#include "database_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_log.h"
#include "tab_old_photos_restore.h"
#undef protected
#undef private

using namespace testing::ext;

namespace OHOS::Media {
const int32_t TEST_NEW_FILE_ID = 1;
const int32_t TEST_OLD_LOCAL_MEDIA_ID = 10;
const std::string TEST_NEW_DATA = "/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg";
const std::string TEST_OLD_DATA = "/storage/emulated/0/Pictures/test.jpg";

static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

void TabOldPhotosRestoreTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MEDIA_INFO_LOG("Start Init");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    TabOldPhotosRestoreTestUtils::ClearAllData();
    MEDIA_INFO_LOG("Start InsertPhoto");
    TabOldPhotosRestoreTestUtils::InsertPhoto();
}

void TabOldPhotosRestoreTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("TearDownTestCase");
    TabOldPhotosRestoreTestUtils::ClearAllData();
}

// SetUp:Execute before each test case
void TabOldPhotosRestoreTest::SetUp()
{
    TabOldPhotosRestoreTestUtils::ClearTabOldPhotosData();
}

void TabOldPhotosRestoreTest::TearDown() {}

HWTEST_F(TabOldPhotosRestoreTest, tab_old_photos_restore_empty_ptr_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start tab_old_photos_restore_empty_ptr_test_001");
    TabOldPhotosRestore tabOldPhotosRestore;
    std::shared_ptr<NativeRdb::RdbStore> rdbStorePtr;
    std::vector<FileInfo> fileInfos;
    int32_t ret = tabOldPhotosRestore.Restore(rdbStorePtr, fileInfos);
    EXPECT_EQ(ret, NativeRdb::E_DB_NOT_EXIST);
    MEDIA_INFO_LOG("End tab_old_photos_restore_empty_ptr_test_001");
}

HWTEST_F(TabOldPhotosRestoreTest, tab_old_photos_restore_empty_fileInfos_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start tab_old_photos_restore_empty_fileInfos_test_001");
    TabOldPhotosRestore tabOldPhotosRestore;
    std::vector<FileInfo> fileInfos;
    int32_t ret = tabOldPhotosRestore.Restore(g_rdbStore->GetRaw(), fileInfos);
    EXPECT_EQ(ret, NativeRdb::E_OK);
    MEDIA_INFO_LOG("End tab_old_photos_restore_empty_fileInfos_test_001");
}

HWTEST_F(TabOldPhotosRestoreTest, tab_old_photos_restore_duplicate_fileInfos_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start tab_old_photos_restore_duplicate_fileInfos_test_001");
    TabOldPhotosRestore tabOldPhotosRestore;
    FileInfo fileInfo;
    fileInfo.localMediaId = TEST_OLD_LOCAL_MEDIA_ID;
    fileInfo.oldPath = TEST_OLD_DATA;
    fileInfo.cloudPath = TEST_NEW_DATA;
    fileInfo.needMove = false;

    std::vector<FileInfo> fileInfos = { fileInfo };
    int32_t ret = tabOldPhotosRestore.Restore(g_rdbStore->GetRaw(), fileInfos);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    int32_t count = TabOldPhotosRestoreTestUtils::QueryTabOldPhotosCount();
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("End tab_old_photos_restore_duplicate_fileInfos_test_001");
}

HWTEST_F(TabOldPhotosRestoreTest, tab_old_photos_restore_normal_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start tab_old_photos_restore_normal_test_001");
    TabOldPhotosRestore tabOldPhotosRestore;
    FileInfo fileInfo;
    fileInfo.localMediaId = TEST_OLD_LOCAL_MEDIA_ID;
    fileInfo.oldPath = TEST_OLD_DATA;
    fileInfo.cloudPath = TEST_NEW_DATA;
    
    std::vector<FileInfo> fileInfos = { fileInfo };
    int32_t ret = tabOldPhotosRestore.Restore(g_rdbStore->GetRaw(), fileInfos);
    EXPECT_EQ(ret, NativeRdb::E_OK);

    int32_t count = TabOldPhotosRestoreTestUtils::QueryTabOldPhotosCount();
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("End tab_old_photos_restore_normal_test_001");
}

HWTEST_F(TabOldPhotosRestoreTest, tab_old_photos_helper_insert_into_table_failed_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start tab_old_photos_helper_insert_into_table_failed_test_001");
    FileInfo fileInfo;
    fileInfo.localMediaId = TEST_OLD_LOCAL_MEDIA_ID;
    fileInfo.oldPath = TEST_OLD_DATA;
    fileInfo.cloudPath = TEST_NEW_DATA;
    TabOldPhotosRestoreHelper restoreHelper;
    restoreHelper.AddBindArgs(fileInfo); // insert sql & args incompatible

    int32_t ret = restoreHelper.InsertIntoTable(g_rdbStore->GetRaw());
    EXPECT_NE(ret, NativeRdb::E_OK);
    MEDIA_INFO_LOG("End tab_old_photos_helper_insert_into_table_failed_test_001");
}

void TabOldPhotosRestoreTestUtils::ClearAllData()
{
    ClearPhotosData();
    ClearTabOldPhotosData();
}

void TabOldPhotosRestoreTestUtils::ClearPhotosData()
{
    const std::string CLEAR_PHOTOS_SQL = "DELETE FROM Photos";
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), CLEAR_PHOTOS_SQL);
}

void TabOldPhotosRestoreTestUtils::ClearTabOldPhotosData()
{
    const std::string CLEAR_TAB_OLD_PHOTOS_SQL = "DELETE FROM tab_old_photos";
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), CLEAR_TAB_OLD_PHOTOS_SQL);
}

void TabOldPhotosRestoreTestUtils::InsertPhoto()
{
    const std::string INSERT_SQL = "INSERT INTO Photos (file_id, data) VALUES (?, ?)";
    const std::vector<NativeRdb::ValueObject> BIND_ARGS = { TEST_NEW_FILE_ID, TEST_NEW_DATA };
    DatabaseUtils::ExecuteSql(g_rdbStore->GetRaw(), INSERT_SQL, BIND_ARGS);
}

int32_t TabOldPhotosRestoreTestUtils::QueryTabOldPhotosCount()
{
    const std::string QUERY_SQL = "SELECT count(1) FROM tab_old_photos";
    return DatabaseUtils::QueryInt(g_rdbStore->GetRaw(), "count(1)", QUERY_SQL);
}
}  // namespace OHOS::Media