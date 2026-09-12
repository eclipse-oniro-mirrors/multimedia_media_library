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

#ifndef MEDIALIBRARY_BACKUP_CLONE_TEST_H
#define MEDIALIBRARY_BACKUP_CLONE_TEST_H

#include "gtest/gtest.h"
#include "rdb_helper.h"
#include "result_set_utils.h"
#include "backup_const.h"
#include "media_upgrade.h"
#include "clone_restore.h"
#include "clone_source.h"
#include "medialibrary_unittest_utils.h"
#include "tab_old_albums_clone.h"
#include "vision_db_sqls_more.h"

namespace OHOS {
namespace Media {
enum class TestAlbumType {
    SOURCE = 0,
    USER,
    SHOOTING_MODE_PORTRAIT,
};
const string TEST_BACKUP_PATH = "/data/test/backup/db";
const string TEST_DB_PATH = "/data/storage/el2/database/rdb/media_library.db";
const string TEST_BACKUP_DB_PATH = TEST_BACKUP_PATH + TEST_DB_PATH;
const string TEST_FAKE_FILE_DIR = "/fake/fake/fake.fake";
const string TEST_PATH = "/data/test";
const string SHOOTING_MODE_PORTRAIT_ALBUM_NAME = "1";
const string WHERE_CLAUSE_SHOOTING_MODE = "shooting_mode = '1'";
const string WHERE_CLAUSE_TRASHED = "date_trashed > 0";
const string WHERE_CLAUSE_IS_FAVORITE = "is_favorite > 0";
const string WHERE_CLAUSE_HIDDEN = "hidden > 0";
const string WHERE_CLAUSE_EDIT = "edit_time > 0";
const string INVALID_STR = "-1";
const string EMPTY_STR = "";
const string TEST_FILE_PATH_PHOTO = "test_file_path_photo";
const string TEST_FILE_PATH_VIDEO = "test_file_path_video";
const string TEST_FILE_PATH_AUDIO = "test_file_path_audio";
const string TASK_ID = "1";
const string TEST_DIR_PATH = "test";
const string SEGMENTATION_ANALYSIS_TABLE = "tab_analysis_segmentation";
const string SEGMENTATION_TYPE = "segmentation";
const int32_t SEGMENTATION_TABLE_SIZE = 7;
const int32_t TEST_ORIENTATION_ZERO = 0;
const int32_t TEST_ORIENTATION_NINETY = 90;
const int32_t I_PHONE_DYNAMIC_VIDEO_TYPE = 13;
const int32_t PAGE_SIZE = 200;
const int32_t GROUP_PHOTO_COUNT = 250;

static std::vector<std::string> createTableSqlLists = {
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoMap::CREATE_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
    AudioColumn::CREATE_AUDIO_TABLE,
    CREATE_GEO_DICTIONARY_TABLE,
    CREATE_SEGMENTATION_ANALYSIS_TABLE,
    CREATE_TAB_ANALYSIS_LABEL,
    CREATE_TAB_ANALYSIS_VIDEO_LABEL,
    CREATE_GEO_KNOWLEDGE_TABLE,
    CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
    CREATE_SEARCH_INDEX_TBL,
    CREATE_VIDEO_FACE_TBL,
    CREATE_AESTHETICS_SCORE_TBL,
    ConfigInfoColumn::CREATE_CONFIG_INFO_TABLE,
    CREATE_TAB_ANALYSIS_VIDEO_TOTAL,
};

static std::vector<std::string> testTables = {
    PhotoColumn::PHOTOS_TABLE,
    PhotoAlbumColumns::TABLE,
    PhotoMap::TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    AudioColumn::AUDIOS_TABLE,
    GEO_DICTIONARY_TABLE,
    SEGMENTATION_ANALYSIS_TABLE,
    VISION_LABEL_TABLE,
    VISION_VIDEO_LABEL_TABLE,
    GEO_KNOWLEDGE_TABLE,
    VISION_TOTAL_TABLE,
    ANALYSIS_SEARCH_INDEX_TABLE,
    ANALYSIS_VIDEO_FACE_TABLE,
    ANALYSIS_BEAUTY_SCORE_TABLE,
    ConfigInfoColumn::MEDIA_CONFIG_INFO_TABLE_NAME,
    TAB_OLD_ALBUMS,
    ANALYSIS_VIDEO_TOTAL_TABLE,
};

struct WaterMarkExpectInfo {
    int32_t fileId = -1;
    int32_t status = -1;
    int32_t type = -1;
    double validRegionX = -1.0;
    double validRegionY = -1.0;
    double validRegionWidth = -1.0;
    double validRegionHeight = -1.0;
    std::string algoVersion = "";

    WaterMarkExpectInfo() = default;
};

const vector<string> WHERE_CLAUSE_LIST_PHOTO = { WHERE_CLAUSE_SHOOTING_MODE, WHERE_CLAUSE_TRASHED,
    WHERE_CLAUSE_IS_FAVORITE, WHERE_CLAUSE_HIDDEN, WHERE_CLAUSE_EDIT,
};
const vector<string> WHERE_CLAUSE_LIST_AUDIO = { WHERE_CLAUSE_TRASHED, WHERE_CLAUSE_IS_FAVORITE };
const unordered_map<TestAlbumType, pair<string, string>> ALBUM_TABLE_MAP = {
    { TestAlbumType::SOURCE, { PhotoAlbumColumns::TABLE, PhotoMap::TABLE } },
    { TestAlbumType::USER, { PhotoAlbumColumns::TABLE, PhotoMap::TABLE } },
    { TestAlbumType::SHOOTING_MODE_PORTRAIT, { ANALYSIS_ALBUM_TABLE, ANALYSIS_PHOTO_MAP_TABLE } },
};
const unordered_map<TestAlbumType, pair<PhotoAlbumSubType, string>> ALBUM_CONDITION_MAP = {
    { TestAlbumType::SOURCE, { PhotoAlbumSubType::SOURCE_GENERIC, "" } },
    { TestAlbumType::USER, { PhotoAlbumSubType::USER_GENERIC, "" } },
    { TestAlbumType::SHOOTING_MODE_PORTRAIT, { PhotoAlbumSubType::SHOOTING_MODE, SHOOTING_MODE_PORTRAIT_ALBUM_NAME } },
};
const int32_t INVALID_COUNT = -1;
const int32_t EXPECTED_SOURCE_ALBUM_COUNT = 2;
const int32_t EXPECTED_USER_ALBUM_COUNT = 1;
const int32_t EXPECTED_SHOOTING_MODE_ALBUM_COUNT = 1;
const int32_t EXPECTED_PHOTO_COUNT = 5;
const int32_t EXPECTED_PHOTO_MAP_COUNT = 7;
const int32_t EXPECTED_ANALYSIS_PHOTO_MAP_COUNT = 1;
const int32_t EXPECTED_COUNT_1 = 1;
const int32_t EXPECTED_COUNT_0 = 0;
const int32_t EXPECTED_ALBUM_TOTAL_COUNT = 4;
const int32_t EXPECTED_AUDIO_COUNT = 3;
const int32_t INVALID_ERROR_CODE = -1;
const std::string DIRTY_FILE_ID = "1000";
const std::string DIRTY_FILE_PATH = "/storage/cloud/files/Documents/1.jpg";

const int PHONE_FIRST_NUMBER = 105;
const int PHONE_SECOND_NUMBER = 80;
const int PHONE_THIRD_NUMBER = 104;
const int PHONE_FOURTH_NUMBER = 111;
const int PHONE_FIFTH_NUMBER = 110;
const int PHONE_SIXTH_NUMBER = 101;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

const std::string DEFAULT_DEVICE_ID = "device_id";
const std::string CONFIG_INFO_INVALID_KEY = "invalid_key";
const std::string CONFIG_INFO_INVALID_VALUE = "invalid_value";
const std::string CONFIG_DB_DIRECTORY = "/storage/media/local/files/.backup/restore/storage/emulated/0";

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static unique_ptr<CloneRestore> restoreService = nullptr;

static const std::string CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status";
static constexpr int64_t RESTORE_OR_BACKUP_WAIT_FORCE_RETAIN_CLOUD_MEDIA_TIMEOUT_MILLISECOND = 60 * 60 * 1000;
static constexpr int64_t TIMEOUT_DELTA = 1;
static const std::string BACKUP_FLAG = "multimedia.medialibrary.backupFlag";
static const int64_t DEFAULT_TIME_STAMP = 0;
static const int RELEASE_SCENE_RESTORE = 2;
static const int RELEASE_SCENE_BACKUP = 1;

static void ModifyAlbumMapTbl(AlbumMapTbl& albumMapTbl, std::optional<int32_t> albumId,
    std::optional<int32_t> albumType, std::optional<int32_t> albumSubtype, std::optional<int32_t> oldAlbumId,
    std::optional<int32_t> cloneSequence);
    

static void ExecuteSqls(shared_ptr<NativeRdb::RdbStore> store, const vector<string> &sqls)
{
    for (const auto &sql : sqls) {
        int32_t errCode = store->ExecuteSql(sql);
        if (errCode == E_OK) {
            continue;
        }
        MEDIA_ERR_LOG("Execute %{public}s failed: %{public}d", sql.c_str(), errCode);
    }
}

static void ClearData()
{
    MEDIA_INFO_LOG("Start clear data");
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MediaLibraryRdbUtils::UpdateAllAlbums(g_rdbStore);
    MEDIA_INFO_LOG("End clear data");
}

static void ClearCloneSource(CloneSource &cloneSource, const string &dbPath)
{
    cloneSource.cloneStorePtr_ = nullptr;
    NativeRdb::RdbHelper::DeleteRdbStore(dbPath);
}

static void Init(CloneSource &cloneSource, const string &path, const vector<string> &tableList)
{
    MEDIA_INFO_LOG("Start init clone source database");
    NativeRdb::RdbHelper::DeleteRdbStore(path);
    cloneSource.Init(path, tableList);
}

static shared_ptr<NativeRdb::ResultSet> GetResultSet(shared_ptr<NativeRdb::RdbStore> rdbStore, const string &querySql)
{
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return nullptr;
    }
    return rdbStore->QuerySql(querySql);
}

static void QueryInt(shared_ptr<NativeRdb::RdbStore> rdbStore, const string &querySql, const string &columnName,
    int32_t &result)
{
    ASSERT_NE(rdbStore, nullptr);
    auto resultSet = rdbStore->QuerySql(querySql);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_EQ(resultSet->GoToFirstRow(), E_OK);
    result = GetInt32Val(columnName, resultSet);
    MEDIA_INFO_LOG("Query %{public}s result: %{public}d", querySql.c_str(), result);
}

class MediaLibraryBackupCloneTest : public testing::Test {
public:
    static constexpr int32_t FILE_INFO_NEW_ID = 101;
    static constexpr int32_t PORTRAIT_SUBTYPE = 4102;

    static constexpr int32_t COLUMN_INDEX_ZERO = 0;
    static constexpr int32_t COLUMN_INDEX_ONE = 1;
    static constexpr int32_t COLUMN_INDEX_TWO = 2;
    static constexpr int32_t COLUMN_INDEX_THREE = 3;
    static constexpr int32_t COLUMN_INDEX_FOUR = 4;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static void VerifyGeoDictionaryRestore(const std::shared_ptr<NativeRdb::RdbStore>& db);
    static void VerifyClassifyRestore(const std::shared_ptr<NativeRdb::RdbStore>& db);
    static void VerifyClassifyVideoRestore(const std::shared_ptr<NativeRdb::RdbStore>& db);
    static void InsertSampleSearchIndexData(const std::shared_ptr<NativeRdb::RdbStore>& db,
        int32_t fileId, const std::string& data, const std::string& displayName, double latitude, double longitude,
        int64_t dateModified, int32_t photoStatus, int32_t cvStatus, int32_t geoStatus, int32_t version,
        const std::string& systemLanguage);
    static void VerifySearchIndexRestore(const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap);
    static void VerifyBeautyScoreRestore(const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, OHOS::Media::PhotoInfo>& photoInfoMap);
    static void VerifyAssetMapRestore(const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        const std::unordered_map<int32_t, OHOS::Media::PhotoInfo>& photoInfoMap);
    static void InsertTestAlbumData(const std::shared_ptr<NativeRdb::RdbStore>& rdbStore,
        const std::string& tableName, int32_t albumId, int32_t albumType, int32_t albumSubtype);
    static int32_t CountAlbumsInSourceTable(const std::shared_ptr<NativeRdb::RdbStore>& rdbStore,
        const std::string& tableName);
    static void VerifyTabOldAlbumsRecord(const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
        int32_t expectedOldAlbumId, int32_t expectedNewAlbumId,
        int32_t exptectedAlbumType, int32_t exptectedAlbumSubType);
};
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_BACKUP_CLONE_TEST_H
