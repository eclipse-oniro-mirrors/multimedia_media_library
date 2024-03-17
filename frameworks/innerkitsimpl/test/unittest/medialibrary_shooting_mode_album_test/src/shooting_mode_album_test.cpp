/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "medialibrary_command.h"
#define MLOG_TAG "ShootingModeAlbumTest"

#include "shooting_mode_album_test.h"

#include "fetch_result.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_smart_album_column.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unittest_utils.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "shooting_mode_column.h"
#include "vision_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<RdbStore> g_rdbStore;
const std::string URI_CREATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_CREATE;
const std::string URI_UPDATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_UPDATE;
const std::string URI_ORDER_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM;
constexpr int32_t SHOOTING_MODE_ALBUM_MIN_NUM = 9;
int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t ClearAnalysisAlbums()
{
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SMART));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::SHOOTING_MODE));
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear AnalysisAlbum table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

inline void CheckColumn(shared_ptr<OHOS::NativeRdb::ResultSet> &resultSet, const string &column,
    ResultSetDataType type, const variant<int32_t, string, int64_t, double> &expected)
{
    EXPECT_EQ(ResultSetUtils::GetValFromColumn(column, resultSet, type), expected);
}

void DoCheckShootingAlbumData(const string &name)
{
    string albumName = name;
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SHOOTING_MODE);
    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_TYPE,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_COVER_URI,
        PhotoAlbumColumns::ALBUM_COUNT,
    };
    shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t count = -1;
    int32_t ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to get count! err: %{public}d", ret);
    MEDIA_INFO_LOG("Query count: %{public}d", count);
    ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to GoToFirstRow! err: %{public}d", ret);

    int32_t albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
        TYPE_INT32));
    std::unordered_map<int32_t, int32_t> updateResult;
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(g_rdbStore, updateResult, {std::to_string(albumId)});
    EXPECT_GT(albumId, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_TYPE, TYPE_INT32, PhotoAlbumType::SMART);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SUBTYPE, TYPE_INT32, PhotoAlbumSubType::SHOOTING_MODE);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_NAME, TYPE_STRING, albumName);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_COVER_URI, TYPE_STRING, "");
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_COUNT, TYPE_INT32, 0);
}

struct ShootingModeValueBucket {
    int32_t albumType;
    int32_t albumSubType;
    std::string albumName;
};

static int32_t InsertShootingModeAlbumValues(
    const ShootingModeValueBucket &shootingModeAlbum, shared_ptr<RdbStore> store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUM_DB_ALBUM_TYPE, shootingModeAlbum.albumType);
    valuesBucket.PutInt(COMPAT_ALBUM_SUBTYPE, shootingModeAlbum.albumSubType);
    valuesBucket.PutString(MEDIA_DATA_DB_ALBUM_NAME, shootingModeAlbum.albumName);
    int64_t outRowId = -1;
    int32_t insertResult = store->Insert(outRowId, ANALYSIS_ALBUM_TABLE, valuesBucket);
    return insertResult;
}

static int32_t CreateShootingModeAlbum()
{
    ShootingModeValueBucket portraitAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, PORTRAIT_ALBUM
    };
    ShootingModeValueBucket wideApertureAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, WIDE_APERTURE_ALBUM
    };
    ShootingModeValueBucket nightShotAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, NIGHT_SHOT_ALBUM
    };
    ShootingModeValueBucket movingPictureAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, MOVING_PICTURE_ALBUM
    };
    ShootingModeValueBucket proPhotoAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, PRO_PHOTO_ALBUM
    };
    ShootingModeValueBucket slowMotionAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, SLOW_MOTION_ALBUM
    };
    ShootingModeValueBucket lightPaintingAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, LIGHT_PAINTING_ALBUM
    };
    ShootingModeValueBucket highPixelAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, HIGH_PIXEL_ALBUM
    };
    ShootingModeValueBucket superMicroAlbum = {
        SHOOTING_MODE_TYPE, SHOOTING_MODE_SUB_TYPE, SUPER_MACRO_ALBUM
    };

    vector<ShootingModeValueBucket> shootingModeValuesBucket = {
        portraitAlbum, wideApertureAlbum, nightShotAlbum, movingPictureAlbum,
        proPhotoAlbum, lightPaintingAlbum, highPixelAlbum, superMicroAlbum, slowMotionAlbum
    };
    for (const auto& shootingModeAlbum : shootingModeValuesBucket) {
        if (InsertShootingModeAlbumValues(shootingModeAlbum, g_rdbStore) != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Prepare shootingMode album failed");
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}

inline int32_t DeletePhotoAlbum(DataSharePredicates &predicates)
{
    Uri uri(URI_CREATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri, OperationType::DELETE);
    return MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

void ShootingModeAlbumTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    ClearAnalysisAlbums();
    ClearTable(ANALYSIS_PHOTO_MAP_TABLE);
}

void ShootingModeAlbumTest::TearDownTestCase()
{
    ClearAnalysisAlbums();
    ClearTable(ANALYSIS_PHOTO_MAP_TABLE);
}

// SetUp:Execute before each test case
void ShootingModeAlbumTest::SetUp() {}

void ShootingModeAlbumTest::TearDown() {}

/**
 * @tc.name: photoalbum_create_album_001
 * @tc.desc: Create ShootingMode albums test
 * @tc.type: FUNC
 * @tc.require: issueI6B1SE
 */
HWTEST_F(ShootingModeAlbumTest, photoalbum_create_ShootingMode_album_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("photoalbum_create_album_001 enter");
    auto ret = CreateShootingModeAlbum();
    EXPECT_EQ(ret, E_OK);
    DoCheckShootingAlbumData(PORTRAIT_ALBUM);
    DoCheckShootingAlbumData(WIDE_APERTURE_ALBUM);
    DoCheckShootingAlbumData(NIGHT_SHOT_ALBUM);
    DoCheckShootingAlbumData(MOVING_PICTURE_ALBUM);
    DoCheckShootingAlbumData(PRO_PHOTO_ALBUM);
    DoCheckShootingAlbumData(SLOW_MOTION_ALBUM);
    DoCheckShootingAlbumData(LIGHT_PAINTING_ALBUM);
    DoCheckShootingAlbumData(HIGH_PIXEL_ALBUM);
    DoCheckShootingAlbumData(SUPER_MACRO_ALBUM);
    MEDIA_INFO_LOG("photoalbum_create_album_001 exit");
}

/**
 * @tc.name: query_shooting_mode_album_001
 * @tc.desc: query shooting mode albums and check if number matches
 * @tc.type: FUNC
 */
HWTEST_F(ShootingModeAlbumTest, query_shooting_mode_album_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("query_shooting_mode_album_001 enter");
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, SHOOTING_MODE_TYPE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, SHOOTING_MODE_SUB_TYPE);
    int errCode = 0;
    shared_ptr<DataShare::ResultSetBridge> queryResultSet =
        MediaLibraryDataManager::GetInstance()->Query(cmd, {}, predicates, errCode);
    shared_ptr<DataShareResultSet> resultSet = make_shared<DataShareResultSet>(queryResultSet);
    int32_t albumCount = 0;
    resultSet->GetRowCount(albumCount);
    EXPECT_EQ((albumCount >= SHOOTING_MODE_ALBUM_MIN_NUM), true);
}
} // namespace OHOS::Media
