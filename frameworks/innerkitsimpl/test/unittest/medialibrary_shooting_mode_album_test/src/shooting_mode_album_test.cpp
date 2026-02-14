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

#include <sstream>
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

struct TestAlbumInfo {
    int32_t albumId {0};
    int32_t count {0};
    string coverUri;
    string ToString() const
    {
        stringstream infoStr;
        infoStr << "albumId: " << albumId << ", count: " << count << ", coverUri: " << coverUri;
        return infoStr.str();
    }
};

struct TestFileInfo {
    int64_t fileId {0};
    int64_t dateTaken {0};
    string displayName;
    string data;
};

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const std::string URI_CREATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + CONST_PHOTO_ALBUM_OPRN + "/" + OPRN_CREATE;
const std::string URI_UPDATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + CONST_PHOTO_ALBUM_OPRN + "/" + OPRN_UPDATE;
const std::string URI_ORDER_ALBUM = MEDIALIBRARY_DATA_URI + "/" + CONST_PHOTO_ALBUM_OPRN + "/" + CONST_OPRN_ORDER_ALBUM;
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
const string CINEMATIC_VIDEO_ALBUM = to_string(static_cast<int32_t>(ShootingModeAlbumType::CINEMATIC_VIDEO_ALBUM));

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
    ClearTable(PhotoColumn::PHOTOS_TABLE);
}

void ShootingModeAlbumTest::TearDownTestCase()
{
    ClearAnalysisAlbums();
    ClearTable(ANALYSIS_PHOTO_MAP_TABLE);
    ClearTable(PhotoColumn::PHOTOS_TABLE);
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
    ASSERT_NE(g_rdbStore, nullptr);
    auto ret = MediaLibraryRdbStore::PrepareShootingModeAlbum(*g_rdbStore->GetRaw().get());
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
    DoCheckShootingAlbumData(CINEMATIC_VIDEO_ALBUM);
    MEDIA_INFO_LOG("photoalbum_create_album_001 exit");
}

void GetShootingModeAlbumInfo(const string &albumName, TestAlbumInfo &albumInfo, int32_t &rowCount)
{
    RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::SHOOTING_MODE);
    const vector<string> columns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_COVER_URI };
    shared_ptr<OHOS::NativeRdb::ResultSet> resultSet = g_rdbStore->Query(predicates, columns);
    EXPECT_NE(resultSet, nullptr);
    resultSet->GetRowCount(rowCount);
    if (resultSet->GoToFirstRow() == E_OK) {
        albumInfo.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        albumInfo.count = GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
        albumInfo.coverUri = GetStringVal(PhotoAlbumColumns::ALBUM_COVER_URI, resultSet);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("Query albumInfo of %{public}s: %{public}s", albumName.c_str(), albumInfo.ToString().c_str());
}

HWTEST_F(ShootingModeAlbumTest, photoalbum_create_ShootingMode_album_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("photoalbum_create_ShootingMode_album_002 enter");
    ASSERT_NE(g_rdbStore, nullptr);
    auto ret = MediaLibraryRdbStore::PrepareShootingModeAlbum(*g_rdbStore->GetRaw().get());
    EXPECT_EQ(ret, E_OK);
    TestAlbumInfo albumInfoBefore;
    int32_t rowCountBefore = -1;
    GetShootingModeAlbumInfo(CINEMATIC_VIDEO_ALBUM, albumInfoBefore, rowCountBefore);
    EXPECT_GT(albumInfoBefore.albumId, 0);
    EXPECT_GT(rowCountBefore, 0);
    ret = MediaLibraryRdbStore::PrepareShootingModeAlbum(*g_rdbStore->GetRaw().get());
    EXPECT_EQ(ret, E_OK);
    TestAlbumInfo albumInfoAfter;
    int32_t rowCountAfter = -1;
    GetShootingModeAlbumInfo(CINEMATIC_VIDEO_ALBUM, albumInfoAfter, rowCountAfter);
    EXPECT_EQ(albumInfoBefore.albumId, albumInfoAfter.albumId);
    EXPECT_EQ(rowCountBefore, rowCountAfter); // no duplicate insertation
    MEDIA_INFO_LOG("photoalbum_create_ShootingMode_album_002 exit");
}

/**
 * @tc.name: query_shooting_mode_album_001
 * @tc.desc: query shooting mode albums and check if number matches
 * @tc.type: FUNC
 */
HWTEST_F(ShootingModeAlbumTest, query_shooting_mode_album_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("query_shooting_mode_album_001 enter");
    Uri analysisAlbumUri(CONST_PAH_INSERT_ANA_PHOTO_ALBUM);
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

HWTEST_F(ShootingModeAlbumTest, query_shooting_mode_album_index_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("query_shooting_mode_album_index_002 enter");
    ShootingModeAlbumType type = ShootingModeAlbumType::CINEMATIC_VIDEO_ALBUM;
    string index = ShootingModeAlbum::GetQueryAssetsIndex(type);
    EXPECT_EQ(index, PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX);
    MEDIA_INFO_LOG("query_shooting_mode_album_index_002 end");
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

bool IsPrediatesContainColumn(const DataShare::DataSharePredicates &predicates, DataShare::OperationType type,
    const string &column)
{
    const size_t SINGLE_PARAM_MIN_SIZE = 1;
    const vector<OperationItem> &operationList = predicates.GetOperationList();
    for (const auto &operationItem : operationList) {
        if (operationItem.operation != type) {
            continue;
        }
        if (operationItem.singleParams.size() < SINGLE_PARAM_MIN_SIZE) {
            continue;
        }
        string field = static_cast<string>(operationItem.GetSingle(0));
        if (field == column) {
            return true;
        }
    }
    return false;
}

HWTEST_F(ShootingModeAlbumTest, GetShootingModeAlbumPredicates_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShootingModeAlbumPredicates_Test_003 enter");
    DataShare::DataSharePredicates predicates;
    ShootingModeAlbum::GetShootingModeAlbumPredicates(ShootingModeAlbumType::CINEMATIC_VIDEO_ALBUM, predicates, false);
    MEDIA_INFO_LOG("Get whereClause: %{public}s", predicates.GetWhereClause().c_str());
    EXPECT_TRUE(IsPrediatesContainColumn(predicates, DataShare::OperationType::EQUAL_TO, PhotoColumn::PHOTO_SUBTYPE));
    MEDIA_INFO_LOG("GetShootingModeAlbumPredicates_Test_003 exit");
}

string GetColumnValueOfPredicates(const NativeRdb::RdbPredicates &predicates, const string &column)
{
    const string &whereClause = predicates.GetWhereClause();
    const vector<string> &whereArgs = predicates.GetWhereArgs();
    size_t pos = whereClause.find(column);
    if (pos == string::npos) {
        MEDIA_ERR_LOG("whereClause is invalid");
        return "";
    }
    size_t argsIndex = 0;
    for (size_t index = 0; index < pos; index++) {
        if (whereClause[index] == '?') {
            argsIndex++;
        }
    }
    if (argsIndex >= whereArgs.size()) {
        MEDIA_ERR_LOG("whereArgs is invalid");
        return "";
    }
    return whereArgs[argsIndex];
}

HWTEST_F(ShootingModeAlbumTest, GetShootingModeAlbumPredicates_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShootingModeAlbumPredicates_Test_004 enter");
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    ShootingModeAlbum::GetShootingModeAlbumPredicates(ShootingModeAlbumType::CINEMATIC_VIDEO_ALBUM, predicates, true);
    MEDIA_INFO_LOG("Get whereClause: %{public}s", predicates.GetWhereClause().c_str());
    string hiddenValue = GetColumnValueOfPredicates(predicates, MediaColumn::MEDIA_HIDDEN);
    EXPECT_EQ(hiddenValue, "1");
    MEDIA_INFO_LOG("GetShootingModeAlbumPredicates_Test_004 exit");
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

HWTEST_F(ShootingModeAlbumTest, GetShootingModeAlbumOfAsset_Test_001, TestSize.Level1)
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

HWTEST_F(ShootingModeAlbumTest, GetShootingModeAlbumOfAsset_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetShootingModeAlbumOfAsset_Test_002 enter");
    vector<ShootingModeAlbumType> result = ShootingModeAlbum::GetShootingModeAlbumOfAsset(
        static_cast<int>(PhotoSubType::CINEMATIC_VIDEO), "video/avi", 0, "", "");
    EXPECT_EQ(result.size(), 1); // only CINEMATIC_VIDEO
    EXPECT_NE(find(result.begin(), result.end(), ShootingModeAlbumType::CINEMATIC_VIDEO_ALBUM), result.end());
    MEDIA_INFO_LOG("GetShootingModeAlbumOfAsset_Test_002 exit");
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

int32_t InsertPhotoBySubtype(PhotoSubType subtype, TestFileInfo &fileInfo)
{
    static int32_t uniqueId = 1;
    NativeRdb::ValuesBucket values;
    fileInfo.data = "/storage/cloud/files/Photo/1/IMG_" + to_string(uniqueId++) + ".jpg";
    values.Put(MediaColumn::MEDIA_FILE_PATH, fileInfo.data);
    values.Put(MediaColumn::MEDIA_DATE_TAKEN, fileInfo.dateTaken);
    values.Put(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(subtype));
    return g_rdbStore->Insert(fileInfo.fileId, PhotoColumn::PHOTOS_TABLE, values);
}

HWTEST_F(ShootingModeAlbumTest, UpdateAnalysisAlbumInternal_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdateAnalysisAlbumInternal_Test_001 enter");
    ASSERT_NE(g_rdbStore, nullptr);
    int32_t ret = MediaLibraryRdbStore::PrepareShootingModeAlbum(*g_rdbStore->GetRaw().get());
    ASSERT_EQ(ret, E_OK);
    TestFileInfo fileInfoWithLargeDateTaken = { .dateTaken = 1744362716123, .displayName = "large_datetaken.jpg" };
    TestFileInfo fileInfoWithSmallDateTaken = { .dateTaken = 1744362716000, .displayName = "small_datetaken.jpg" };
    ret = InsertPhotoBySubtype(PhotoSubType::CINEMATIC_VIDEO, fileInfoWithLargeDateTaken);
    ASSERT_EQ(ret, E_OK);
    ret = InsertPhotoBySubtype(PhotoSubType::CINEMATIC_VIDEO, fileInfoWithSmallDateTaken);
    ASSERT_EQ(ret, E_OK);

    TestAlbumInfo albumInfo;
    int32_t rowCount = -1;
    int32_t expectedCount = 2;
    GetShootingModeAlbumInfo(CINEMATIC_VIDEO_ALBUM, albumInfo, rowCount);
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(g_rdbStore, { to_string(albumInfo.albumId) });

    GetShootingModeAlbumInfo(CINEMATIC_VIDEO_ALBUM, albumInfo, rowCount);
    EXPECT_TRUE(albumInfo.coverUri.find(fileInfoWithLargeDateTaken.displayName) != string::npos);
    EXPECT_EQ(albumInfo.count, expectedCount);
}
} // namespace OHOS::Media
