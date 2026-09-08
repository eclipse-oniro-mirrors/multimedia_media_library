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

#define MLOG_TAG "MediaLibraryMusicMasterModeTest"

#include "medialibrary_music_master_mode_test.h"

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
using namespace OHOS::Media::Background;

namespace OHOS::Media {
namespace {
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;

const int64_t SIZE_DEFAULT = 0;
const int64_t SIZE_VALUE = 100;

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

} // namespace

void MediaLibraryMusicMasterModeTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    ClearDirectoryContents("/data/test");
    CleanTestTables();
    SetTables();
}

void MediaLibraryMusicMasterModeTest::TearDownTestCase(void)
{
    if (g_rdbStore != nullptr) {
        CleanTestTables();
    }
    ClearDirectoryContents("/data/test");
}

HWTEST_F(MediaLibraryMusicMasterModeTest, MediaMusicMasterModeTask_Accept_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MediaMusicMasterModeTask_Accept_test_001 start");
    MediaMusicMasterModeTask task;
    EXPECT_TRUE(task.Accept());
    MEDIA_INFO_LOG("MediaMusicMasterModeTask_Accept_test_001 end");
}

HWTEST_F(MediaLibraryMusicMasterModeTest, QueryMusicMasterAssets_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("QueryMusicMasterAssets_test_002 start");
    int64_t matchId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/mmm_test_002_match",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        0, 0, 0, 0,
        static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO), 0 });
    int64_t skipId = InsertPhoto(InsertPhotoParams { SIZE_VALUE, SIZE_DEFAULT,
        static_cast<int32_t>(PhotoPositionType::LOCAL), false,
        "/data/test/mmm_test_002_skip",
        static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT),
        0, 0, 0, 0,
        static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO), 1 });
    ASSERT_GT(matchId, 0);
    ASSERT_GT(skipId, 0);

    MediaMusicMasterModeTask task;
    std::vector<MusicMasterAssetInfo> assetInfos;
    task.QueryMusicMasterAssets(0, static_cast<int32_t>(skipId), assetInfos);
    ASSERT_EQ(assetInfos.size(), 1);
    EXPECT_EQ(assetInfos[0].fileId, matchId);
    EXPECT_EQ(assetInfos[0].path, "/data/test/mmm_test_002_match");
    MEDIA_INFO_LOG("QueryMusicMasterAssets_test_002 end");
}

} // namespace OHOS::Media
