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

#define MLOG_TAG "MediaAssetsControllerServiceTest"

#include "get_cloud_enhancement_pair_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "get_cloud_enhancement_pair_vo.h"

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "media_file_uri.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";
static const string VALUES_END = ") ";

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear photos table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

void GetCloudEnhancementPairTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void GetCloudEnhancementPairTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void GetCloudEnhancementPairTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("SetUp");
}

void GetCloudEnhancementPairTest::TearDown(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("TearDown");
}

static int32_t GetCloudEnhancementPair(string &photoUri, std::shared_ptr<DataShare::DataShareResultSet> &resultSet)
{
    GetCloudEnhancementPairReqBody reqBody;
    reqBody.photoUri = photoUri;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->GetCloudEnhancementPair(data, reply);

    IPC::MediaRespVo<GetCloudEnhancementPairRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return errCode;
    }
    resultSet = respVo.GetBody().resultSet;
    return 0;
}

static void InsertAsset(string displayName)
{
    // data, size,
    // title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    std::string insertSql = SQL_INSERT_PHOTO + " VALUES (" + "'/storage/cloud/files/Photo/16/" + displayName +
                            ".jpg', 175258, '" + displayName + "', '" + displayName + ".jpg', 1, " +
                            "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
                            "1280, 960, 0, '1'" + VALUES_END;
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static void UpdateAssetAssociateFileId(int32_t fileId, const std::string &displayName)
{
    std::string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_ASSOCIATE_FILE_ID +
                            " = " + std::to_string(fileId) + " WHERE " + MediaColumn::MEDIA_NAME + " = '" +
                            displayName + "'";
    int32_t ret = g_rdbStore->ExecuteSql(updateSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", updateSql.c_str());
    }
}

static int32_t QueryPhotoIdByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get fileId");
        return -1;
    }
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    return fileId;
}

HWTEST_F(GetCloudEnhancementPairTest, GetCloudEnhancementPair_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetCloudEnhancementPair_Test_001 Begin");
    // invalid uri prefix
    string uri = "file://media/xxx/20000/";
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
    int32_t result = GetCloudEnhancementPair(uri, resultSet);
    ASSERT_EQ(result, 0);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_NE(resultSet->GoToFirstRow(), E_OK);
}

HWTEST_F(GetCloudEnhancementPairTest, GetCloudEnhancementPair_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetCloudEnhancementPair_Test_002 Begin");
    // invalid fileId
    string uri = "file://media/Photo/200000/";
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
    int32_t result = GetCloudEnhancementPair(uri, resultSet);
    ASSERT_EQ(result, 0);
    ASSERT_NE(resultSet, nullptr);
    ASSERT_NE(resultSet->GoToFirstRow(), E_OK);
}
}  // namespace OHOS::Media