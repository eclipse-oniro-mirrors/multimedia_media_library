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

#include <thread>
#include "datashare_predicates.h"
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
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const std::string URI_CREATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_CREATE;
const std::string URI_UPDATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_UPDATE;
const std::string URI_ORDER_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM;
constexpr int32_t SHOOTING_MODE_ALBUM_MIN_NUM = 9;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

const string PORTRAIT_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::PORTRAIT));
const string WIDE_APERTURE_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::WIDE_APERTURE));
const string NIGHT_SHOT_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::NIGHT_SHOT));
const string MOVING_PICTURE_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::MOVING_PICTURE));
const string PRO_PHOTO_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::PRO_PHOTO));
const string SLOW_MOTION_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::SLOW_MOTION));
const string LIGHT_PAINTING_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::LIGHT_PAINTING));
const string HIGH_PIXEL_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::HIGH_PIXEL));
const string SUPER_MACRO_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::SUPER_MACRO));
const string TIME_LAPSE_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::TIME_LAPSE));
const string QUICK_CAPTURE_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::QUICK_CAPTURE_ALBUM));

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
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(g_rdbStore, {std::to_string(albumId)});
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
    const string& albumName, const shared_ptr<MediaLibraryRdbStore>& store)
{
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(SMARTALBUM_DB_ALBUM_TYPE, SHOOTING_MODE_TYPE);
    valuesBucket.PutInt(COMPAT_ALBUM_SUBTYPE, SHOOTING_MODE_SUB_TYPE);
    valuesBucket.PutString(MEDIA_DATA_DB_ALBUM_NAME, albumName);
    valuesBucket.PutInt(MEDIA_DATA_DB_IS_LOCAL, 1); // local album is 1.
    int64_t outRowId = -1;
    int32_t insertResult = store->Insert(outRowId, ANALYSIS_ALBUM_TABLE, valuesBucket);
    return insertResult;
}

static int32_t QueryExistingShootingModeAlbumNames(const shared_ptr<MediaLibraryRdbStore>& store,
    vector<string>& existingAlbumNames)
{
    string queryRowSql = "SELECT " + PhotoAlbumColumns::ALBUM_NAME + " FROM " + ANALYSIS_ALBUM_TABLE +
        " WHERE " + PhotoAlbumColumns::ALBUM_SUBTYPE + " = " + to_string(PhotoAlbumSubType::SHOOTING_MODE);
    auto resultSet = store->QuerySql(queryRowSql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL,
        "Can not get shootingMode album names, resultSet is nullptr");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
        if (!albumName.empty()) {
            existingAlbumNames.push_back(albumName);
        }
    }
    return E_SUCCESS;
}

static int32_t PrepareShootingModeAlbum()
{
    vector<string> existingAlbumNames;
    if (QueryExistingShootingModeAlbumNames(g_rdbStore, existingAlbumNames) != E_SUCCESS) {
        MEDIA_ERR_LOG("Query existing shootingMode album names failed");
        return NativeRdb::E_ERROR;
    }
    for (int i = static_cast<int>(ShootingModeAlbumType::START);
        i <= static_cast<int>(ShootingModeAlbumType::END); ++i) {
        string albumName = to_string(i);
        if (find(existingAlbumNames.begin(), existingAlbumNames.end(), albumName) != existingAlbumNames.end()) {
            continue;
        }
        if (InsertShootingModeAlbumValues(albumName, g_rdbStore) != NativeRdb::E_OK) {
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
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ClearAnalysisAlbums();
    ClearTable(ANALYSIS_PHOTO_MAP_TABLE);
}

void ShootingModeAlbumTest::TearDownTestCase()
{
    ClearAnalysisAlbums();
    ClearTable(ANALYSIS_PHOTO_MAP_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
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
HWTEST_F(ShootingModeAlbumTest, photoalbum_create_ShootingMode_album_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_album_001 enter");
    auto ret = PrepareShootingModeAlbum();
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
    DoCheckShootingAlbumData(TIME_LAPSE_ALBUM);
    DoCheckShootingAlbumData(QUICK_CAPTURE_ALBUM);
    MEDIA_INFO_LOG("photoalbum_create_album_001 exit");
}

/**
 * @tc.name: query_shooting_mode_album_001
 * @tc.desc: query shooting mode albums and check if number matches
 * @tc.type: FUNC
 */
HWTEST_F(ShootingModeAlbumTest, query_shooting_mode_album_001, TestSize.Level1)
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

HWTEST_F(ShootingModeAlbumTest, query_shooting_mode_album_index_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("query_shooting_mode_album_index_001 enter");
    ShootingModeAlbumType type = ShootingModeAlbumType::PORTRAIT;
    string index = ShootingModeAlbum::GetQueryAssetsIndex(type);
    EXPECT_EQ(index, PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX);
    MEDIA_INFO_LOG("query_shooting_mode_album_index_001 end");
}

HWTEST_F(ShootingModeAlbumTest, GetShootingModeAlbumPredicates_Test_001, TestSize.Level1)
{
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::MOVING_PICTURE, rdbPredicates, false);
    vector<string> args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::MP4_3DGS_ALBUM, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::BURST_MODE_ALBUM, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::FRONT_CAMERA_ALBUM, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::RAW_IMAGE_ALBUM, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::PORTRAIT, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::QUICK_CAPTURE_ALBUM, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::TIME_LAPSE, rdbPredicates, false);
    args = rdbPredicates.GetWhereArgs();
    EXPECT_GT(args.size(), 0);
    rdbPredicates.Clear();
}

HWTEST_F(ShootingModeAlbumTest, GetShootingModeAlbumPredicates_Test_002, TestSize.Level1)
{
    DataShare::DataSharePredicates dataSharePredicates1;
    DataShare::DataSharePredicates dataSharePredicates2;
    DataShare::DataSharePredicates dataSharePredicates3;
    DataShare::DataSharePredicates dataSharePredicates4;
    DataShare::DataSharePredicates dataSharePredicates5;
    DataShare::DataSharePredicates dataSharePredicates6;
    DataShare::DataSharePredicates dataSharePredicates7;
    DataShare::DataSharePredicates dataSharePredicates8;

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::MOVING_PICTURE, dataSharePredicates1, false);
    auto args = dataSharePredicates1.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::BURST_MODE_ALBUM, dataSharePredicates2, false);
    args = dataSharePredicates2.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::FRONT_CAMERA_ALBUM, dataSharePredicates3, false);
    args = dataSharePredicates3.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::RAW_IMAGE_ALBUM, dataSharePredicates4, false);
    args = dataSharePredicates4.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::PORTRAIT, dataSharePredicates5, false);
    args = dataSharePredicates5.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::MP4_3DGS_ALBUM, dataSharePredicates6, false);
    args = dataSharePredicates6.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::TIME_LAPSE, dataSharePredicates7, false);
    args = dataSharePredicates7.GetOperationList();
    EXPECT_GT(args.size(), 0);

    ShootingModeAlbum::GetShootingModeAlbumPredicates(
        ShootingModeAlbumType::QUICK_CAPTURE_ALBUM, dataSharePredicates8, false);
    args = dataSharePredicates8.GetOperationList();
    EXPECT_GT(args.size(), 0);
}

HWTEST_F(ShootingModeAlbumTest, AlbumNameToShootingModeAlbumType_Test_001, TestSize.Level1)
{
    ShootingModeAlbumType result;
    bool ret = ShootingModeAlbum::AlbumNameToShootingModeAlbumType("1", result);
    EXPECT_TRUE(ret);
    EXPECT_EQ(result, ShootingModeAlbumType::PORTRAIT);

    ret = ShootingModeAlbum::AlbumNameToShootingModeAlbumType("-1", result);
    EXPECT_FALSE(ret);
}

HWTEST_F(ShootingModeAlbumTest, AlbumNameToShootingModeAlbumType_Test_002, TestSize.Level1)
{
    vector<ShootingModeAlbumType> result = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        static_cast<int>(PhotoSubType::BURST), "image/x-adobe-dng", 0, "1", "1");
    EXPECT_EQ(result.size(), 4);

    result = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        static_cast<int>(PhotoSubType::MOVING_PHOTO), "image/x-adobe-dng", 0, "1", "1");
    EXPECT_EQ(result.size(), 4);

    result = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        static_cast<int>(PhotoSubType::SPATIAL_3DGS), "image/x-adobe-dng", 0, "1", "1");
    EXPECT_EQ(result.size(), 4);
}

HWTEST_F(ShootingModeAlbumTest, MapShootingModeTagToShootingMode_Test_001, TestSize.Level1)
{
    string shootingMode = ShootingModeAlbum::MapShootingModeTagToShootingMode(PORTRAIT_ALBUM_TAG);
    EXPECT_EQ(shootingMode, "1");

    shootingMode = ShootingModeAlbum::MapShootingModeTagToShootingMode("-1");
    EXPECT_EQ(shootingMode, "");
}

HWTEST_F(ShootingModeAlbumTest, LookUpShootingMode_Test_001, TestSize.Level1)
{
    ShootingModeValue value = static_cast<ShootingModeValue>(0);
    EXPECT_EQ(ShootingModeAlbum::LookUpShootingModeAlbumType(value), "");
    value = ShootingModeValue::PORTRAIT_SHOOTING_MODE;
    EXPECT_EQ(ShootingModeAlbum::LookUpShootingModeAlbumType(value),
        std::to_string(static_cast<int32_t>(ShootingModeAlbumType::PORTRAIT)));

    std::string albumType = "0";
    EXPECT_EQ(ShootingModeAlbum::LookUpShootingModeValues(albumType), "");
    albumType = "1";
    EXPECT_EQ(ShootingModeAlbum::LookUpShootingModeValues(albumType),
        std::to_string(static_cast<int32_t>(ShootingModeValue::PORTRAIT_SHOOTING_MODE)));
}
} // namespace OHOS::Media
