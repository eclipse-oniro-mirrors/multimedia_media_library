/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryRestoreTest"
#include "medialibrary_restore_test.h"
#include "media_pure_file_utils.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_restore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"

namespace OHOS {
namespace Media {

namespace {
    constexpr int INSERT_ROWS             = 10;
    constexpr int INCREASE_FORMULA        = 17;
    constexpr int RDB_VERSION             = 1;
    constexpr int32_t SLEEP_1             = 1;
    constexpr int32_t SLEEP_2             = 2;
    const std::string TEST_BACKUP_FOLDER  = "/data/test/media_library_restore/rdb";
    const std::string TEST_BACKUP_FILE    = TEST_BACKUP_FOLDER + "/media_library";
    const std::string SLAVE_DB            = "_slave";
    const std::string EXT_DB              = ".db";
    const std::string EXT_DB_WAL          = ".db-wal";
    const std::string DB_PATH             = TEST_BACKUP_FILE + EXT_DB;
    const std::string SELECT_SQL          = "SELECT album_subtype FROM PhotoAlbum WHERE album_type = 0";
    const std::string INSERT_SQL          = std::string("INSERT INTO PhotoAlbum")
        .append("(album_type, album_subtype, album_name, cover_uri, cloud_id, relative_path) VALUES ")
        .append("(0, 1, 'test101010101010101010101010',")
        .append("'file://media/Photo/2/IMG_1501932657_001/IMG_201785_192917.jpg',")
        .append("'file://media/Photo/2/IMG_1501932657_001/IMG_201785_192917.jpg',")
        .append("'file://media/Photo/2/IMG_1501932657_001/IMG_201785_192917.jpg'")
        .append(");");
    const std::string UPDATE_SQL          = std::string("UPDATE PhotoAlbum set album_subtype = 2")
        .append(" where album_type = 0 and album_subtype = 1");
    const std::string INCREASE_SQL        = std::string("INSERT INTO PhotoAlbum")
        .append(" (album_type, album_subtype, album_name, cover_uri, cloud_id, relative_path)")
        .append(" select album_type, album_subtype, album_name, cover_uri, cloud_id, relative_path from PhotoAlbum");
    const std::string CREATE_PHOTOS_ALBUM = std::string("CREATE TABLE IF NOT EXISTS PhotoAlbum ")
        .append("(album_id INTEGER PRIMARY KEY AUTOINCREMENT, album_type INT, ")
        .append("album_subtype INT, album_name TEXT COLLATE NOCASE, cover_uri TEXT, ")
        .append("count INT DEFAULT 0, date_modified BIGINT DEFAULT 0, dirty INT DEFAULT 1, ")
        .append("cloud_id TEXT, ")
        .append("relative_path TEXT, contains_hidden INT DEFAULT 0, hidden_count INT DEFAULT 0)");
} // namespace

int RestoreDataCallBack::OnCreate(NativeRdb::RdbStore &store)
{
    int errCode = store.ExecuteSql(CREATE_PHOTOS_ALBUM);
    return errCode;
}

int RestoreDataCallBack::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return E_OK;
}

void MediaLibraryRestoreTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::SetUpTestCase");
}

void MediaLibraryRestoreTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::TearDownTestCase");
}

int CorruptDb(bool isSlave)
{
    std::string ext = "";
    if (isSlave) {
        ext = SLAVE_DB;
    }
    std::string filePath = TEST_BACKUP_FILE + ext + EXT_DB;
    std::string cmd = std::string("dd if=/dev/zero of=").append(filePath)
        .append(" bs=4k seek=0 conv=notrunc count=1");
    int ret = std::system(cmd.c_str());
    if (!isSlave) {
        cmd = std::string("dd if=/dev/zero of=").append(filePath).append(" bs=4k seek=1 conv=notrunc count=1");
        ret = std::system(cmd.c_str());
        cmd = std::string("dd if=/dev/zero of=").append(filePath).append(" bs=4k seek=3 conv=notrunc count=1");
        ret = std::system(cmd.c_str());
    }

    filePath = TEST_BACKUP_FILE + ext + EXT_DB_WAL;
    if (!isSlave) {
        cmd = std::string("dd if=/dev/zero of=").append(filePath).append(" bs=4k seek=5 conv=notrunc count=1");
        ret = std::system(cmd.c_str());
    }
    return ret;
}

int InsertRdbData(const std::shared_ptr<NativeRdb::RdbStore> &rdb)
{
    int errCode = 0;
    for (int i = 0; i < INSERT_ROWS; i++) {
        errCode = rdb->ExecuteSql(INSERT_SQL);
    }
    return errCode;
}

int IncreaseRdbData(const std::shared_ptr<NativeRdb::RdbStore> &rdb)
{
    int errCode = rdb->ExecuteSql(INSERT_SQL);
    for (int i = 0; i < INCREASE_FORMULA; i++) {
        errCode = rdb->ExecuteSql(INCREASE_SQL);
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_1));
        MEDIA_INFO_LOG("MediaLibraryRestoreTest::IncreaseRdbData i = [%{public}d]", i);
    }
    return errCode;
}

void WaitForBackup()
{
    while (MediaLibraryRestore::GetInstance().IsBackuping()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_2));
    }
}

const NativeRdb::RdbStoreConfig GetConfig()
{
    NativeRdb::RdbStoreConfig config(DB_PATH);
    config.SetHaMode(NativeRdb::HAMode::MANUAL_TRIGGER);
    config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
    config.SetAllowRebuild(true);
    return config;
}

// SetUp:Execute before each test case
void MediaLibraryRestoreTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::SetUp");
    if (MediaPureFileUtils::IsDirectory(TEST_BACKUP_FOLDER)) {
        MediaFileUtils::DeleteDir(TEST_BACKUP_FOLDER);
    }
    MediaPureFileUtils::CreateDirectory(TEST_BACKUP_FOLDER);
}

void MediaLibraryRestoreTest::TearDown(void)
{
    MEDIA_INFO_LOG("MediaLibraryRestoreTest::TearDown");
}

HWTEST_F(MediaLibraryRestoreTest, medialib_restore_test_restore_001, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_restore_test_restore_001 start");
    auto config = GetConfig();
    RestoreDataCallBack callBack;
    int errCode = 0;
    auto rdb = NativeRdb::RdbHelper::GetRdbStore(config, RDB_VERSION, callBack, errCode);
    ASSERT_TRUE(rdb != nullptr);

    errCode = InsertRdbData(rdb);
    EXPECT_EQ(errCode, E_OK);

    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, RDB_VERSION, callBack);
    EXPECT_EQ(ret, E_OK);

    MediaLibraryUnitTestUtils::StopUnistore();
    NativeRdb::RdbHelper::DeleteRdbStore(config);
    MEDIA_INFO_LOG("medialib_restore_test_restore_001 end");
}

HWTEST_F(MediaLibraryRestoreTest, medialib_restore_test_restore_002, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_restore_test_restore_002 start");
    auto config = GetConfig();
    RestoreDataCallBack callBack;
    int errCode = 0;
    auto rdb = NativeRdb::RdbHelper::GetRdbStore(config, RDB_VERSION, callBack, errCode);
    ASSERT_TRUE(rdb != nullptr);

    auto result = IncreaseRdbData(rdb);
    EXPECT_EQ(result, NativeRdb::E_OK);

    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, RDB_VERSION, callBack);
    EXPECT_EQ(ret, E_OK);

    MediaLibraryUnitTestUtils::StopUnistore();
    NativeRdb::RdbHelper::DeleteRdbStore(config);

    MEDIA_INFO_LOG("medialib_restore_test_restore_002 end");
}

HWTEST_F(MediaLibraryRestoreTest, medialib_restore_test_restore_003, testing::ext::TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_restore_test_restore_003 start");
    auto config = GetConfig();
    RestoreDataCallBack callBack;
    int errCode = 0;
    auto rdb = NativeRdb::RdbHelper::GetRdbStore(config, RDB_VERSION, callBack, errCode);
    ASSERT_TRUE(rdb != nullptr);

    int32_t ret = MediaLibraryUnitTestUtils::InitUnistore(config, RDB_VERSION, callBack);
    EXPECT_EQ(ret, E_OK);

    MediaLibraryUnitTestUtils::StopUnistore();
    NativeRdb::RdbHelper::DeleteRdbStore(config);
    MEDIA_INFO_LOG("medialib_restore_test_restore_003 end");
}

} // namespace Media
} // namespace OHOS
