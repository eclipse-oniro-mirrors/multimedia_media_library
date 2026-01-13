/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryAnalysisAlbumOperationTest"

#include "medialibrary_analysis_album_operation_test.h"
#include "datashare_result_set.h"
#include "photo_album_column.h"
#include "get_self_permissions.h"
#include "location_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_analysis_album_operations.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "story_album_column.h"
#include "uri.h"
#include "vision_db_sqls_more.h"
#include "rdb_utils.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;
using namespace testing::ext;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataSharePredicates;
using OHOS::DataShare::DataShareValuesBucket;

constexpr int32_t TRUE_ALBUM_ID = 1;
constexpr int32_t ALBUM_TARGET_COUNT = 10;
constexpr int32_t IS_ME_VALUE = 1;
constexpr int32_t WAIT_TIME = 3;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> number(0);

struct AlbumColumn {
    string coverUri;
    int count;
    string tagId;
    string groupTag;
    string rank;
    int userOperation;
    int displayLevel;
    int isMe;
    int renameOperation;
    int isCoverSatisfied;
    string albumName;
};

struct PortraitData {
    int64_t fileId;
    string title;
    string path;
    vector<string> tag;
};

struct PortraitAlbumData {
    int64_t albumId;
    string tag;
};

struct GroupPhotoAlbumData {
    int64_t albumId;
    string groupTag;
    vector<string> tags;
};

int GetNumber()
{
    return ++number;
}

int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = g_rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

string GetTitle(int64_t &timestamp)
{
    return "IMG_" + to_string(timestamp) + "_" + to_string(GetNumber());
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    return seconds.count() + GetNumber();
}

string GetCoverUri(PortraitData data)
{
    string extrUri = MediaFileUtils::GetExtraUri(data.title + ".jpg", data.path, true);
    return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(data.fileId), extrUri);
}

vector<PortraitAlbumData> CreatePortraitAlbum()
{
    const int albumCount = 3;
    vector<string> tag = {"ser_1711000000000000000", "ser_1711000000000000001", "ser_1711000000000000002"};
    vector<PortraitAlbumData> portraitAlbumData;
    for (size_t index = 0; index < albumCount; index++) {
        PortraitAlbumData data;
        int64_t albumId = -1;
        ValuesBucket valuesBucket;
        valuesBucket.PutInt(ALBUM_TYPE, PhotoAlbumType::SMART);
        valuesBucket.PutInt(ALBUM_SUBTYPE, PhotoAlbumSubType::PORTRAIT);
        valuesBucket.PutString(ALBUM_NAME, tag[index]);
        valuesBucket.PutString(TAG_ID, tag[index]);
        valuesBucket.PutString(GROUP_TAG, tag[index]);
        valuesBucket.PutInt(ALBUM_SUBTYPE, PhotoAlbumSubType::PORTRAIT);
        EXPECT_NE(g_rdbStore, nullptr);
        int32_t ret = g_rdbStore->Insert(albumId, ANALYSIS_ALBUM_TABLE, valuesBucket);
        EXPECT_EQ(ret, E_OK);
        MEDIA_INFO_LOG("InsertAlbum albumId is %{public}s", to_string(albumId).c_str());
        data.albumId = albumId;
        data.tag = tag[index];
        portraitAlbumData.push_back(data);
    }
    return portraitAlbumData;
}

PortraitData InsertPortraitToPhotos()
{
    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    string data = "/storage/cloud/files/photo/1/" + title + ".jpg";
    ValuesBucket valuesBucket;
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, data);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestamp);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    EXPECT_NE((g_rdbStore == nullptr), true);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    PortraitData portraitData;
    portraitData.fileId = fileId;
    portraitData.title = title;
    portraitData.path = data;
    return portraitData;
}

void InsertPortraitToImageFace(int64_t fileId, int totalFaces, const vector<string> &tag)
{
    ASSERT_NE(g_rdbStore, nullptr);
    int64_t rowId = -1;
    for (size_t faceId = 0; faceId < totalFaces; faceId++) {
        ValuesBucket valuesBucket;
        valuesBucket.PutInt(MediaColumn::MEDIA_ID, fileId);
        valuesBucket.PutInt(FACE_ID, faceId);
        valuesBucket.PutInt(TOTAL_FACES, totalFaces);
        valuesBucket.PutString(TAG_ID, tag[faceId]);
        EXPECT_NE((g_rdbStore == nullptr), true);
        int32_t ret = g_rdbStore->Insert(rowId, VISION_IMAGE_FACE_TABLE, valuesBucket);
        EXPECT_EQ(ret, E_OK);
    }
}

void InsertPortraitsToAlbum(const vector<PortraitData> &portraitData,
    const vector<PortraitAlbumData> &portraitAlbumData)
{
    ASSERT_NE(g_rdbStore, nullptr);
    for (auto albumData : portraitAlbumData) {
        string coverUri;
        int64_t rowId = -1;
        for (auto data : portraitData) {
            if (std::find(data.tag.begin(), data.tag.end(), albumData.tag) != data.tag.end()) {
                ValuesBucket valuesBucket;
                valuesBucket.PutInt(MAP_ALBUM, albumData.albumId);
                valuesBucket.PutInt(MAP_ASSET, data.fileId);
                EXPECT_NE((g_rdbStore == nullptr), true);
                int32_t ret = g_rdbStore->Insert(rowId, ANALYSIS_PHOTO_MAP_TABLE, valuesBucket);
                EXPECT_EQ(ret, E_OK);
                coverUri = GetCoverUri(data);
            }
        }
        int32_t changedRows = -1;
        MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::UPDATE);
        ValuesBucket updateValues;
        updateValues.PutString(COVER_URI, coverUri);
        updateValues.PutInt(IS_COVER_SATISFIED, static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING));
        cmd.SetValueBucket(updateValues);
        cmd.GetAbsRdbPredicates()->EqualTo(ALBUM_ID, to_string(albumData.albumId));
        int32_t ret = g_rdbStore->Update(cmd, changedRows);
        EXPECT_EQ(ret, E_OK);
    }
}

void InsertAlbumTestData(AlbumColumn &column, const PhotoAlbumSubType &subtype = PhotoAlbumSubType::PORTRAIT)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, subtype);
    valuesBucket.Put(COVER_URI, column.coverUri);
    valuesBucket.Put(COUNT, column.count);
    valuesBucket.Put(TAG_ID, column.tagId);
    valuesBucket.Put(GROUP_TAG, column.tagId);
    valuesBucket.Put(RANK, column.rank);
    valuesBucket.Put(USER_OPERATION, column.userOperation);
    valuesBucket.Put(USER_DISPLAY_LEVEL, column.displayLevel);
    valuesBucket.Put(IS_ME, column.isMe);
    valuesBucket.Put(RENAME_OPERATION, column.renameOperation);
    valuesBucket.Put(IS_COVER_SATISFIED, column.isCoverSatisfied);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

vector<GroupPhotoAlbumData> CreateGroupPhotoAlbum(const vector<PortraitAlbumData> &portraitData)
{
    vector<GroupPhotoAlbumData> groupPhotoData;
    for (size_t index = 0; index < portraitData.size(); index++) {
        GroupPhotoAlbumData data;
        int64_t albumId = -1;
        string groupTag;
        if (index != portraitData.size() - 1) {
            groupTag = portraitData[index].tag + "," + portraitData[index + 1].tag;
            data.tags.push_back(portraitData[index].tag);
            data.tags.push_back(portraitData[index + 1].tag);
        } else {
            groupTag = portraitData[0].tag + "," + portraitData[index].tag;
            data.tags.push_back(portraitData[0].tag);
            data.tags.push_back(portraitData[index].tag);
        }
        ValuesBucket valuesBucket;
        valuesBucket.PutInt(ALBUM_TYPE, PhotoAlbumType::SMART);
        valuesBucket.PutInt(ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
        valuesBucket.PutString(ALBUM_NAME, portraitData[index].tag);
        valuesBucket.PutString(TAG_ID, groupTag);
        valuesBucket.PutString(GROUP_TAG, groupTag);
        EXPECT_NE(g_rdbStore, nullptr);
        int32_t ret = g_rdbStore->Insert(albumId, ANALYSIS_ALBUM_TABLE, valuesBucket);
        EXPECT_EQ(ret, E_OK);
        MEDIA_INFO_LOG("InsertAlbum albumId is %{public}s", to_string(albumId).c_str());
        data.albumId = albumId;
        data.groupTag = groupTag;
        groupPhotoData.push_back(data);
    }
    return groupPhotoData;
}

void InsertAnalysisPhotoMap(int64_t albumId, int64_t fileId)
{
    int defaultOrderPostion = -1;
    ValuesBucket valuesBucket;
    valuesBucket.PutInt(MAP_ALBUM, albumId);
    valuesBucket.PutInt(MAP_ASSET, fileId);
    valuesBucket.PutInt(ORDER_POSITION, defaultOrderPostion);
    EXPECT_NE((g_rdbStore == nullptr), true);
    int64_t rowId;
    int32_t ret = g_rdbStore->Insert(rowId, ANALYSIS_PHOTO_MAP_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
}

void PrepareGroupPhotoAlbumData(vector<PortraitAlbumData> &portraitData, vector<GroupPhotoAlbumData> &groupAlbumData)
{
    portraitData = CreatePortraitAlbum();
    groupAlbumData = CreateGroupPhotoAlbum(portraitData);
    size_t groupAlbumCount = groupAlbumData.size();
    for (size_t index = 0; index < groupAlbumCount; index++) {
        PortraitData data = InsertPortraitToPhotos();
        InsertPortraitToImageFace(data.fileId, groupAlbumData[index].tags.size(), groupAlbumData[index].tags);
        InsertAnalysisPhotoMap(groupAlbumData[index].albumId, data.fileId);
    }
}

void ClearTables()
{
    string clearPhoto = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    string clearAnalysisAlbum = "DELETE FROM " + ANALYSIS_ALBUM_TABLE;
    string clearAnalysisPhotoMap = "DELETE FROM " + ANALYSIS_PHOTO_MAP_TABLE;
    string clearImageFace = "DELETE FROM " + VISION_IMAGE_FACE_TABLE;
    vector<string> executeSqlStrs = {clearPhoto, clearAnalysisAlbum, clearAnalysisPhotoMap, clearImageFace};
    MEDIA_INFO_LOG("start clear data");
    ExecSqls(executeSqlStrs);
    number = 0;
}

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        ANALYSIS_ALBUM_TABLE,
        ANALYSIS_PHOTO_MAP_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
        CREATE_ANALYSIS_ALBUM_MAP,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

void ClearAnalysisAlbum()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.NotEqualTo(ALBUM_SUBTYPE, PhotoAlbumSubType::SHOOTING_MODE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearAnalysisAlbum Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void MediaLibraryAnalysisAlbumOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Vision_Test::Start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    vector<string> perms = { "ohos.permission.MEDIA_LOCATION" };
    uint64_t tokenId = 0;
    
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryAnalysisAlbumOperationTest", perms, tokenId);
    ASSERT_TRUE(tokenId != 0);
    ClearAnalysisAlbum();
    ClearAndRestart();
}

void MediaLibraryAnalysisAlbumOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("Vision_Test::End");
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
}

void MediaLibraryAnalysisAlbumOperationTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    MediaLibraryUnitTestUtils::Init();
    ClearAnalysisAlbum();
    ClearAndRestart();
}

void MediaLibraryAnalysisAlbumOperationTest::TearDown(void) {}

shared_ptr<NativeRdb::ResultSet> QueryAnalysisAlbumInfo(int32_t albumId)
{
    EXPECT_GT(albumId, 0);
    vector<string> columns = {COUNT, COVER_URI, IS_COVER_SATISFIED};
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(ALBUM_ID, to_string(albumId));
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->Query(cmd, columns);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    return resultSet;
}

vector<PortraitData> PreparePortraitData()
{
    const int portraitCount = 4;
    const vector<int> faceCount = {2, 2, 2, 3};
    string tagA = "ser_1711000000000000000";
    string tagB = "ser_1711000000000000001";
    string tagC = "ser_1711000000000000002";
    vector<vector<string>> tag = {{tagA, tagB}, {tagA, tagC}, {tagB, tagC}, {tagA, tagB, tagC}};
    vector<PortraitData> portraits;
    for (size_t index = 0; index < portraitCount; index++) {
        PortraitData portrait = InsertPortraitToPhotos();
        portrait.tag = tag[index];
        portraits.push_back(portrait);
        sleep(1);
        InsertPortraitToImageFace(portrait.fileId, faceCount[index], tag[index]);
    }
    return portraits;
}

int32_t GetCount(const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t count = GetInt32Val(COUNT, resultSet);
    EXPECT_GE(count, 0);
    if (count < 0) {
        MEDIA_ERR_LOG("Failed to get count");
        return count;
    }
    return count;
}

string GetPhotoId(const string &uri)
{
    if (uri.compare(0, PhotoColumn::PHOTO_URI_PREFIX.size(), PhotoColumn::PHOTO_URI_PREFIX) != 0) {
        return "";
    }
    string tmp = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());
    return tmp.substr(0, tmp.find_first_of('/'));
}

string GetCoverId(const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    string coverUri = GetStringVal(COVER_URI, resultSet);
    return GetPhotoId(coverUri);
}

bool GetIsCoverSatisfied(const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t isCoverSatisfied = GetInt32Val(IS_COVER_SATISFIED, resultSet);
    EXPECT_GE(isCoverSatisfied, 0);
    return isCoverSatisfied == 1;
}

void CheckAlbum(int64_t albumId, int count, int64_t coverId, bool isCoverSatisfied)
{
    shared_ptr<NativeRdb::ResultSet> resultSet = QueryAnalysisAlbumInfo(albumId);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(GetCoverId(resultSet), to_string(coverId));
    EXPECT_EQ(GetCount(resultSet), count);
    EXPECT_EQ(GetIsCoverSatisfied(resultSet), isCoverSatisfied);
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoAlbumName_no_album_id, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_no_album_id::Start");
    OperationType operationType = OperationType::GROUP_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_no_album_id End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoAlbumName_no_album_name, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_no_album_name::Start");
    OperationType operationType = OperationType::GROUP_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_no_album_name End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoAlbumName_album_name_empty, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_album_name_empty::Start");
    OperationType operationType = OperationType::GROUP_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(ALBUM_NAME, "");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_album_name_empty End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoAlbumName_update_err, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_update_err::Start");
    OperationType operationType = OperationType::GROUP_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(ALBUM_NAME, "dd");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    CleanTestTables();
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_update_err End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoAlbumName_update_succ, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_update_succ::Start");
    OperationType operationType = OperationType::GROUP_ALBUM_NAME;
    NativeRdb::ValuesBucket values;
    values.Put(ALBUM_ID, TRUE_ALBUM_ID);
    values.Put(ALBUM_NAME, "dd");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/10/10.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "eww,wwe";
    targetColumn.isMe = IS_ME_VALUE;
    InsertAlbumTestData(targetColumn);
    EXPECT_EQ(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_update_succ End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoCoverUri_no_coverUri_id, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_no_coverUri_id::Start");
    OperationType operationType = OperationType::GROUP_COVER_URI;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_no_coverUri_id End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoCoverUri_no_coverUri, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_no_coverUri::Start");
    OperationType operationType = OperationType::GROUP_COVER_URI;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_no_coverUri End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoCoverUri_coverUri_empty, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_coverUri_empty::Start");
    OperationType operationType = OperationType::GROUP_COVER_URI;
    NativeRdb::ValuesBucket values;
    values.Put(COVER_URI, "");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_coverUri_empty End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoCoverUri_update_err, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_update_err::Start");
    OperationType operationType = OperationType::GROUP_COVER_URI;
    NativeRdb::ValuesBucket values;
    values.Put(COVER_URI, "eee");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    CleanTestTables();
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_update_err End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoCoverUri_update_succ, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_update_succ::Start");
    OperationType operationType = OperationType::GROUP_COVER_URI;
    NativeRdb::ValuesBucket values;
    values.Put(COVER_URI, "dd");
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/10/10.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "eww,wwe";
    InsertAlbumTestData(targetColumn);
    EXPECT_EQ(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("SetGroupPhotoCoverUri_update_succ End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, DismissGroupPhotoAlbum_no_album_id, TestSize.Level1)
{
    MEDIA_INFO_LOG("DismissGroupPhotoAlbum_no_album_id::Start");
    OperationType operationType = OperationType::DISMISS;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("DismissGroupPhotoAlbum_no_album_id End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, DismissGroupPhotoAlbum_update_err, TestSize.Level1)
{
    MEDIA_INFO_LOG("DismissGroupPhotoAlbum_update_err::Start");
    OperationType operationType = OperationType::DISMISS;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    CleanTestTables();
    EXPECT_NE(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("DismissGroupPhotoAlbum_update_err End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, DismissGroupPhotoAlbum_update_succ, TestSize.Level1)
{
    MEDIA_INFO_LOG("DismissGroupPhotoAlbum_update_succ::Start");
    OperationType operationType = OperationType::DISMISS;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    dataPredicates.EqualTo(ALBUM_ID, TRUE_ALBUM_ID);
    AlbumColumn targetColumn;
    targetColumn.coverUri = "file://media/Photo/1/10/10.jpg";
    targetColumn.count = ALBUM_TARGET_COUNT;
    targetColumn.tagId = "eww,wwe";
    InsertAlbumTestData(targetColumn);
    EXPECT_EQ(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_OK);
    MEDIA_INFO_LOG("DismissGroupPhotoAlbum_update_succ End");
}

void InsertPortraitAlbumCoverSatisfiedTestData(int fileId, CoverSatisfiedType coverSatisfiedType)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    string coverUri =
        PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId) + "/" + to_string(fileId) + "/" + to_string(fileId) + ".jpg";
    valuesBucket.Put(COVER_URI, coverUri);
    int count = 10 + fileId;
    valuesBucket.Put(COUNT, count);
    string tagId = "ser_171100000000000000" + to_string(fileId);
    valuesBucket.Put(TAG_ID, tagId);
    valuesBucket.Put(GROUP_TAG, tagId);
    valuesBucket.Put(USER_DISPLAY_LEVEL, 1);
    valuesBucket.Put(IS_COVER_SATISFIED, static_cast<uint8_t>(coverSatisfiedType));
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int32_t GetCoverSatisfied(const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t isCoverSatisfied = GetInt32Val(IS_COVER_SATISFIED, resultSet);
    EXPECT_GE(isCoverSatisfied, 0);
    return isCoverSatisfied;
}

shared_ptr<NativeRdb::ResultSet> QueryAnalysisAlbumInfoByFileId(int32_t fileId)
{
    EXPECT_GT(fileId, 0);
    vector<string> columns = { COUNT, COVER_URI, IS_COVER_SATISFIED };
    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_ALBUM, OperationType::QUERY, MediaLibraryApi::API_10);
    const string coverUriPrefix = PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId) + "/%";
    cmd.GetAbsRdbPredicates()->Like(COVER_URI, coverUriPrefix);
    EXPECT_NE(g_rdbStore, nullptr);
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->Query(cmd, columns);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    return resultSet;
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, UpdatePortraitAlbumCoverSatisfied, TestSize.Level1)
{
    MEDIA_INFO_LOG("UpdatePortraitAlbumCoverSatisfied Start");
    ClearTables();

    int fileId = 1;
    InsertPortraitAlbumCoverSatisfiedTestData(fileId, CoverSatisfiedType::NO_SETTING);
    MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(fileId);
    shared_ptr<NativeRdb::ResultSet> resultSet = QueryAnalysisAlbumInfoByFileId(fileId);
    EXPECT_EQ(GetCoverSatisfied(resultSet), 1);

    fileId = 2;
    InsertPortraitAlbumCoverSatisfiedTestData(fileId, CoverSatisfiedType::DEFAULT_SETTING);
    MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(fileId);
    resultSet = QueryAnalysisAlbumInfoByFileId(fileId);
    EXPECT_EQ(GetCoverSatisfied(resultSet), 1);

    fileId = 3;
    InsertPortraitAlbumCoverSatisfiedTestData(fileId, CoverSatisfiedType::USER_SETTING);
    MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(fileId);
    resultSet = QueryAnalysisAlbumInfoByFileId(fileId);
    EXPECT_EQ(GetCoverSatisfied(resultSet), 3);

    fileId = 4;
    InsertPortraitAlbumCoverSatisfiedTestData(fileId, CoverSatisfiedType::BEAUTY_SETTING);
    MediaLibraryAnalysisAlbumOperations::UpdatePortraitAlbumCoverSatisfied(fileId);
    resultSet = QueryAnalysisAlbumInfoByFileId(fileId);
    EXPECT_EQ(GetCoverSatisfied(resultSet), 5);

    ClearTables();
    MEDIA_INFO_LOG("UpdatePortraitAlbumCoverSatisfied End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, QueryGroupPhotoAlbum_test_001, TestSize.Level0)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    std::string whereClause = "ALBUM_TYPE =? AND ALBUM_SUBTYPE =?";
    std::vector<std::string> whereArgs = {"1", "2"};
    cmd.GetAbsRdbPredicates()->SetWhereClause(whereClause);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    std::vector<std::string> columns = {
        "ALBUM_ID",
        "ALBUM_TYPE",
        "ALBUM_SUBTYPE",
        "IS_REMOVED",
        "IS_ME",
        "RENAME_OPERATION",
        "TAG_ID",
    };
    auto result = MediaLibraryAnalysisAlbumOperations::QueryGroupPhotoAlbum(cmd, columns);
    EXPECT_NE(result, nullptr);
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, QueryGroupPhotoAlbum_test_002, TestSize.Level0)
{
    Uri uri("");
    MediaLibraryCommand cmd(uri);
    std::string whereClause = "";
    std::vector<std::string> whereArgs = {"1", "2"};
    cmd.GetAbsRdbPredicates()->SetWhereClause(whereClause);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    std::vector<std::string> columns = {
        "ALBUM_ID",
        "ALBUM_TYPE",
        "ALBUM_SUBTYPE",
        "IS_REMOVED",
        "IS_ME",
        "RENAME_OPERATION",
        "TAG_ID",
    };
    auto result = MediaLibraryAnalysisAlbumOperations::QueryGroupPhotoAlbum(cmd, columns);
    EXPECT_NE(result, nullptr);
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, UpdateMergeGroupAlbumsInfo_test_002, TestSize.Level0)
{
    MergeAlbumInfo info = {
        100,
        4096,
        4102,
        "tag1",
        0,
        0,
        "cover100.jpg",
        0,
        0,
        0,
        0,
        "Album1",
        static_cast<uint8_t>(CoverSatisfiedType::NO_SETTING),
        "tag1",
    };
    MergeAlbumInfo info2 = {
        101,
        4096,
        4102,
        "tag2",
        0,
        0,
        "cover101.jpg",
        0,
        0,
        0,
        0,
        "Album2",
        static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING),
        "tag2",
    };
    std::vector<MergeAlbumInfo> mergeAlbumInfo = {info, info2};
    int32_t result = MediaLibraryAnalysisAlbumOperations::UpdateMergeGroupAlbumsInfo(mergeAlbumInfo);
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetGroupPhotoAlbumName_Unknown_Type, TestSize.Level0)
{
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_no_album_id::Start");
    OperationType operationType = OperationType::UNKNOWN_TYPE;
    NativeRdb::ValuesBucket values;
    DataShare::DataSharePredicates dataPredicates;
    EXPECT_EQ(MediaLibraryAnalysisAlbumOperations::HandleGroupPhotoAlbum(operationType, values, dataPredicates), E_ERR);
    MEDIA_INFO_LOG("SetGroupPhotoAlbumName_no_album_id End");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, UpdateGroupPhotoAlbumById_test_001, TestSize.Level0)
{
    int32_t albumId = 0;
    MediaLibraryAnalysisAlbumOperations::UpdateGroupPhotoAlbumById(albumId);
    albumId = -1;
    MediaLibraryAnalysisAlbumOperations::UpdateGroupPhotoAlbumById(albumId);
    albumId = 1;
    MediaLibraryAnalysisAlbumOperations::UpdateGroupPhotoAlbumById(albumId);
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetAnalysisAlbumOrderPosition_test_001, TestSize.Level0)
{
    const std::string ORDER_POSITION = "ORDER_POSITION";
    const std::string ALBUM_ID = "ALBUM_ID";
    NativeRdb::ValuesBucket values;
    values.Put(ORDER_POSITION, "5");
    MediaLibraryCommand cmd(
        OperationObject::ANALYSIS_PHOTO_ALBUM,
        OperationType::UPDATE,
        values
    );
    int32_t result = MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumOrderPosition(cmd);
    EXPECT_NE(result, E_OK);
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, UpdateGroupPhotoAlbumById_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("start UpdateGroupPhotoAlbumById_test");
    ClearTables();
    shared_ptr<FileAsset> parent;
    shared_ptr<FileAsset> album;
    auto ret = MediaLibraryUnitTestUtils::CreateAlbum("testAlbum", parent, album);
    ASSERT_TRUE(ret);
    MediaLibraryAnalysisAlbumOperations::UpdateGroupPhotoAlbumById(album->GetId());
    MEDIA_INFO_LOG("end UpdateGroupPhotoAlbumById_test");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetAnalysisAlbumOrderPosition_test, TestSize.Level0)
{
    MEDIA_INFO_LOG("start SetAnalysisAlbumOrderPosition_test");
    ASSERT_TRUE(g_rdbStore);
    ClearTables();
    string sqlStr = "INSERT INTO AnalysisPhotoMap (map_album, map_asset, order_position) VALUES (1, 2 ,3)";
    auto ret = g_rdbStore->ExecuteSql(sqlStr);
    EXPECT_EQ(ret, E_OK);

    MediaLibraryCommand cmd(OperationObject::ANALYSIS_PHOTO_MAP, OperationType::UPDATE_ORDER, MediaLibraryApi::API_10);
    ret = MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumOrderPosition(cmd);
    EXPECT_NE(ret, E_OK);

    ValuesBucket values;
    values.PutInt(ORDER_POSITION, 1);
    cmd.GetAbsRdbPredicates()->EqualTo(ORDER_POSITION, to_string(3));
    cmd.SetValueBucket(values);
    ret = MediaLibraryAnalysisAlbumOperations::SetAnalysisAlbumOrderPosition(cmd);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end  SetAnalysisAlbumOrderPosition_test");
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, MergeGroupPhotoAlbum_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start MergeGroupPhotoAlbum_test_001");
    ASSERT_TRUE(g_rdbStore);
    ClearTables();
    vector<PortraitAlbumData> portraitData;
    vector<GroupPhotoAlbumData> groupAlbumData;
    PrepareGroupPhotoAlbumData(portraitData, groupAlbumData);
    size_t portraitDataSize = portraitData.size();
    size_t groupAlbumDataSize = groupAlbumData.size();
    EXPECT_GT(portraitDataSize, 2);
    EXPECT_GT(groupAlbumDataSize, 2);

    vector<MergeAlbumInfo> mergeAlbumInfos;
    MergeAlbumInfo sourceAlbum;
    MergeAlbumInfo targetAlbum;
    sourceAlbum.albumId = portraitData[0].albumId;
    sourceAlbum.groupTag = portraitData[0].tag;
    targetAlbum.albumId = portraitData[0].albumId;
    targetAlbum.groupTag = portraitData[0].tag;
    mergeAlbumInfos.push_back(sourceAlbum);
    mergeAlbumInfos.push_back(targetAlbum);

    int32_t ret = MediaLibraryAnalysisAlbumOperations::UpdateMergeGroupAlbumsInfo(mergeAlbumInfos);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end MergeGroupPhotoAlbum_test_001");
}

shared_ptr<NativeRdb::ResultSet> QueryHighlightAlbumInfoByAlbumId(const int32_t &albumId)
{
    EXPECT_GT(albumId, 0);
    std::string querySql = "SELECT is_favorite, is_viewed, notification_time \
        FROM tab_highlight_album WHERE album_id = ?;";
    std::vector<NativeRdb::ValueObject> params = { NativeRdb::ValueObject(std::to_string(albumId)) };
    EXPECT_NE(g_rdbStore, nullptr);
    shared_ptr<NativeRdb::ResultSet> resultSet = g_rdbStore->QuerySql(querySql, params);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_OK);
    return resultSet;
}

int32_t InsertValueHighlightAlbum(const int32_t &albumId)
{
    EXPECT_GT(albumId, 0);
    std::string insertSql = " \
        INSERT INTO tab_highlight_album \
        (album_id, cluster_type, cluster_sub_type, cluster_condition, highlight_version, \
        is_favorite, is_viewed, notification_time) \
        VALUES (?, 'TYPE_LIFE_STAGE', 'Graduate', ?, 0, 0, 0, 0);";
    std::string clusterCondition =
        R"([{"end": "1244081480000", "group_tag": "ser_1755585091890809000", \
        "locationType": "COLLEGE_UNIVERSITY", "start":"1117851080000"}])";
    std::vector<NativeRdb::ValueObject> params = {};
    params.push_back(NativeRdb::ValueObject(std::to_string(albumId)));
    params.push_back(NativeRdb::ValueObject(clusterCondition));
    EXPECT_NE(g_rdbStore, nullptr);
    int32_t ret = g_rdbStore->ExecuteSql(insertSql, params);
    return ret;
}

HWTEST_F(MediaLibraryAnalysisAlbumOperationTest, SetHighlightAttribute_test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("start SetHighlightAttribute_test_001");
    ASSERT_TRUE(g_rdbStore);
    int32_t albumId = 1;
    int32_t ret = InsertValueHighlightAlbum(albumId);
    EXPECT_EQ(ret, E_OK);
    int32_t highlightAlbumChangeAttribute = 0;
    std::string viewedValue = "1";
    ret = MediaLibraryAnalysisAlbumOperations::SetHighlightAttribute(albumId,
        highlightAlbumChangeAttribute, viewedValue);
    EXPECT_EQ(ret, E_OK);
    highlightAlbumChangeAttribute = 1;
    std::string notificationValue = "1756092178000";
    ret = MediaLibraryAnalysisAlbumOperations::SetHighlightAttribute(albumId,
        highlightAlbumChangeAttribute, notificationValue);
    EXPECT_EQ(ret, E_OK);
    highlightAlbumChangeAttribute = 2;
    std::string favoriteValue = "1";
    ret = MediaLibraryAnalysisAlbumOperations::SetHighlightAttribute(albumId,
        highlightAlbumChangeAttribute, favoriteValue);
    EXPECT_EQ(ret, E_OK);
    highlightAlbumChangeAttribute = 3;
    std::string value = "1";
    ret = MediaLibraryAnalysisAlbumOperations::SetHighlightAttribute(albumId,
        highlightAlbumChangeAttribute, value);
    EXPECT_NE(ret, E_OK);
    auto resultSet = QueryHighlightAlbumInfoByAlbumId(albumId);
    int32_t isViewed = GetInt32Val(HIGHLIGHT_IS_VIEWED, resultSet);
    int32_t isFavorite = GetInt32Val(HIGHLIGHT_IS_FAVORITE, resultSet);
    int64_t notificationTime = GetInt64Val(HIGHLIGHT_NOTIFICATION_TIME, resultSet);
    EXPECT_EQ(std::to_string(isViewed), viewedValue);
    EXPECT_EQ(std::to_string(isFavorite), favoriteValue);
    EXPECT_EQ(std::to_string(notificationTime), notificationValue);
    resultSet->Close();
    MEDIA_INFO_LOG("end SetHighlightAttribute_test_001");
}
} // namespace Media
} // namespace OHOS