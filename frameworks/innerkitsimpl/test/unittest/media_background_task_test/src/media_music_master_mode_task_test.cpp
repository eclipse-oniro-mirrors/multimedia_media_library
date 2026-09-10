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

#define MLOG_TAG "MediaMusicMasterModeTaskTest"

#include "medialibrary_subscriber.h"
#include "media_music_master_mode_task_test.h"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

#include "media_column.h"
#include "media_column_type.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_upgrade.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_music_master_mode_task.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
namespace {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const int64_t SIZE_DEFAULT = 0;
const int64_t SIZE_VALUE = 100;
const std::string TEST_MISSING_VIDEO_PATH = "/data/test/mmm_no_such_video.mp4";

void ClearDirectoryContents(const std::string &path)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string fullPath = path + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                ClearDirectoryContents(fullPath);
                rmdir(fullPath.c_str());
            } else {
                remove(fullPath.c_str());
            }
        }
    }
    closedir(dir);
}

void CleanTestTables()
{
    if (g_rdbStore == nullptr) {
        return;
    }
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
        }
    }
}

void SetTables()
{
    if (g_rdbStore == nullptr) {
        return;
    }
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
        }
    }
}

struct InsertPhotoParams {
    int64_t size = SIZE_VALUE;
    int64_t localAssetSize = 0;
    int32_t position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    bool isTemp = false;
    std::string path;
    int32_t movingPhotoEffectMode = static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT);
    int64_t attachmentSize = 0;
    int32_t syncStatus = 0;
    int32_t cleanFlag = 0;
    int32_t timePending = 0;
    int32_t mediaType = static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO);
    int32_t musicMasterMode = 0;
};

int64_t InsertPhoto(const InsertPhotoParams &params)
{
    if (g_rdbStore == nullptr) {
        return -1;
    }
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
    values.Put(MediaColumn::MEDIA_TYPE, params.mediaType);
    values.Put(PhotoColumn::MUSIC_MASTER_MODE, params.musicMasterMode);

    int64_t outRowId = -1;
    int32_t insertResult = g_rdbStore->Insert(outRowId, PhotoColumn::PHOTOS_TABLE, values);
    if (insertResult != NativeRdb::E_OK) {
        return -1;
    }
    return outRowId;
}

int32_t QueryMusicMasterMode(int32_t fileId)
{
    if (g_rdbStore == nullptr) {
        return -1;
    }
    std::string sql = "SELECT " + PhotoColumn::MUSIC_MASTER_MODE + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_ID + " = " + std::to_string(fileId);
    auto resultSet = g_rdbStore->QuerySql(sql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return -1;
    }
    int32_t mode = GetInt32Val(PhotoColumn::MUSIC_MASTER_MODE, resultSet);
    resultSet->Close();
    return mode;
}

int32_t QueryMaxFileIdLocal()
{
    if (g_rdbStore == nullptr) {
        return -1;
    }
    std::string sql = "SELECT Max(file_id) FROM " + PhotoColumn::PHOTOS_TABLE;
    auto resultSet = g_rdbStore->QuerySql(sql);
    if (resultSet == nullptr) {
        return -1;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return -1;
    }
    int32_t maxFileId = GetInt32Val("Max(file_id)", resultSet);
    resultSet->Close();
    return maxFileId;
}

int64_t InsertVideoWithPath(const std::string &path)
{
    return InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        path,
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        0, 0, 0, 0,
        static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO), 0 });
}

} // namespace

void MediaMusicMasterModeTaskTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("MediaMusicMasterModeTaskTest SetUpTestCase start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaMusicMasterModeTaskTest failed, can not get g_rdbStore");
        return;
    }
    ClearDirectoryContents("/data/test");
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaMusicMasterModeTaskTest SetUpTestCase end");
}

void MediaMusicMasterModeTaskTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("MediaMusicMasterModeTaskTest TearDownTestCase start");
    CleanTestTables();
    ClearDirectoryContents("/data/test");
    MEDIA_INFO_LOG("MediaMusicMasterModeTaskTest TearDownTestCase end");
}

void MediaMusicMasterModeTaskTest::SetUp(void)
{
    CleanTestTables();
    SetTables();
    MedialibrarySubscriber::currentStatus_ = true;
    MEDIA_INFO_LOG("MediaMusicMasterModeTaskTest SetUp");
}

void MediaMusicMasterModeTaskTest::TearDown(void)
{
    MEDIA_INFO_LOG("MediaMusicMasterModeTaskTest TearDown");
}

HWTEST_F(MediaMusicMasterModeTaskTest, BatchStatus_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("BatchStatus_test_001 start");
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    task->SetBatchStatus(100);
    EXPECT_EQ(task->GetBatchStatus(), 100);
    task->SetBatchStatus(0);
    EXPECT_EQ(task->GetBatchStatus(), 0);
    task->SetBatchStatus(12345);
    EXPECT_EQ(task->GetBatchStatus(), 12345);
}

HWTEST_F(MediaMusicMasterModeTaskTest, QueryMusicMasterAssets_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryMusicMasterAssets_test_001 start");
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    std::vector<MusicMasterAssetInfo> assetInfos;
    task->QueryMusicMasterAssets(0, 1000, assetInfos);
    EXPECT_TRUE(assetInfos.empty());
}

HWTEST_F(MediaMusicMasterModeTaskTest, QueryMusicMasterAssets_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("QueryMusicMasterAssets_test_002 start");
    int64_t matchId = InsertVideoWithPath("/data/test/mmm_match");
    int64_t skipId = InsertVideoWithPath("/data/test/mmm_skip");
    ASSERT_GT(matchId, 0);
    ASSERT_GT(skipId, 0);
    int32_t changedRows = 0;
    ValuesBucket values;
    values.Put(PhotoColumn::MUSIC_MASTER_MODE, 1);
    ASSERT_EQ(g_rdbStore->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values,
        MediaColumn::MEDIA_ID + " = ?", { std::to_string(skipId) }), E_OK);

    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    std::vector<MusicMasterAssetInfo> assetInfos;
    task->QueryMusicMasterAssets(0, static_cast<int32_t>(skipId), assetInfos);
    ASSERT_EQ(assetInfos.size(), 1u);
    EXPECT_EQ(assetInfos[0].fileId, static_cast<int32_t>(matchId));
    EXPECT_EQ(assetInfos[0].path, "/data/test/mmm_match");
}

HWTEST_F(MediaMusicMasterModeTaskTest, HandleMusicMasterAssets_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleMusicMasterAssets_test_001 start");
    int64_t fileId = InsertVideoWithPath("/data/test/mmm_empty_keep");
    ASSERT_GT(fileId, 0);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    std::vector<MusicMasterAssetInfo> assetInfos;
    task->HandleMusicMasterAssets(assetInfos);
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(fileId)), 0);
}

HWTEST_F(MediaMusicMasterModeTaskTest, HandleMusicMasterAssets_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleMusicMasterAssets_test_002 start");
    int64_t f1 = InsertVideoWithPath("/data/test/mmm_fm");
    int64_t f2 = InsertVideoWithPath("/data/test/mmm_ho");
    int64_t f3 = InsertVideoWithPath("/data/test/mmm_pe");
    int64_t f4 = InsertVideoWithPath("/data/test/mmm_md");
    ASSERT_GT(f1, 0);
    ASSERT_GT(f2, 0);
    ASSERT_GT(f3, 0);
    ASSERT_GT(f4, 0);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    std::vector<MusicMasterAssetInfo> assetInfos;
    assetInfos.push_back({static_cast<int32_t>(f1),
        static_cast<int32_t>(FileSourceType::FILE_MANAGER), "", TEST_MISSING_VIDEO_PATH});
    assetInfos.push_back({static_cast<int32_t>(f2),
        static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE), "", TEST_MISSING_VIDEO_PATH});
    assetInfos.push_back({static_cast<int32_t>(f3),
        static_cast<int32_t>(FileSourceType::PERIPHERAL), TEST_MISSING_VIDEO_PATH, ""});
    MusicMasterAssetInfo m4;
    m4.fileId = static_cast<int32_t>(f4);
    m4.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    m4.path = "";
    m4.storagePath = "";
    assetInfos.push_back(m4);
    task->HandleMusicMasterAssets(assetInfos);
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(f1)), 0);
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(f2)), 0);
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(f3)), 0);
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(f4)), 0);
}

HWTEST_F(MediaMusicMasterModeTaskTest, HandleMusicMasterMode_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleMusicMasterMode_test_001 start");
    int64_t fileId = InsertVideoWithPath("/data/test/mmm_mode_1");
    ASSERT_GT(fileId, 0);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    task->SetBatchStatus(0);
    task->HandleMusicMasterMode();
    EXPECT_EQ(task->GetBatchStatus(), static_cast<int32_t>(fileId));
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(fileId)), 0);
}

HWTEST_F(MediaMusicMasterModeTaskTest, HandleMusicMasterMode_test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleMusicMasterMode_test_002 start");
    const int32_t insertNum = 105;
    for (int32_t i = 0; i < insertNum; ++i) {
        ASSERT_GT(InsertVideoWithPath("/data/test/mmm_mode_2_" + std::to_string(i)), 0);
    }
    int32_t maxFileId = QueryMaxFileIdLocal();
    ASSERT_GT(maxFileId, 0);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    task->SetBatchStatus(0);
    task->HandleMusicMasterMode();
    EXPECT_EQ(task->GetBatchStatus(), maxFileId);
}

HWTEST_F(MediaMusicMasterModeTaskTest, HandleMusicMasterMode_test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleMusicMasterMode_test_003 start");
    int64_t fileId = InsertVideoWithPath("/data/test/mmm_mode_3");
    ASSERT_GT(fileId, 0);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    task->SetBatchStatus(0);
    MedialibrarySubscriber::currentStatus_ = false;
    task->HandleMusicMasterMode();
    MedialibrarySubscriber::currentStatus_ = true;
    EXPECT_EQ(task->GetBatchStatus(), 0);
}

HWTEST_F(MediaMusicMasterModeTaskTest, HandleMusicMasterMode_test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("HandleMusicMasterMode_test_004 start");
    int64_t fileId = InsertVideoWithPath("/data/test/mmm_mode_4");
    ASSERT_GT(fileId, 0);
    int32_t changedRows = 0;
    ValuesBucket values;
    values.Put(PhotoColumn::MUSIC_MASTER_MODE, 1);
    ASSERT_EQ(g_rdbStore->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values,
        MediaColumn::MEDIA_ID + " = ?", { std::to_string(fileId) }), E_OK);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    task->SetBatchStatus(0);
    task->HandleMusicMasterMode();
    EXPECT_EQ(task->GetBatchStatus(), static_cast<int32_t>(fileId));
}

HWTEST_F(MediaMusicMasterModeTaskTest, Execute_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Execute_test_001 start");
    int64_t fileId = InsertVideoWithPath("/data/test/mmm_execute");
    ASSERT_GT(fileId, 0);
    auto task = std::make_shared<MediaMusicMasterModeTask>();
    ASSERT_NE(task, nullptr);
    task->SetBatchStatus(0);
    task->Execute();
    EXPECT_EQ(task->GetBatchStatus(), static_cast<int32_t>(fileId));
    EXPECT_EQ(QueryMusicMasterMode(static_cast<int32_t>(fileId)), 0);
}
} // namespace OHOS::Media::Background
