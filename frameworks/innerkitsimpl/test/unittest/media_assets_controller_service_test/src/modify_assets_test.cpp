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

#define MLOG_TAG "ModifyAssetsTest"

#include "modify_assets_test.h"

#include <string>
#include <vector>

#include "media_assets_controller_service.h"

#include "modify_assets_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_asset_operations.h"
#include "media_file_utils.h"
#include "mimetype_utils.h"
#include "parameter_utils.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

static std::string Quote(const std::string &str)
{
    return "'" + str + "'";
}

static void ClearTable(const string &table)
{
    int32_t rows = 0;
    RdbPredicates predicates(table);
    int32_t errCode = g_rdbStore->Delete(rows, predicates);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "g_rdbStore->Delete errCode:%{public}d", errCode);
}

static void ClearAssetsFile()
{
    std::string assetPath;
    vector<string> columns = {MediaColumn::MEDIA_FILE_PATH};
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "MediaLibraryRdbStore::Query failed");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetPath = MediaLibraryRdbStore::GetString(resultSet, columns.front());
        MEDIA_INFO_LOG("DeleteFile assetPath:%{public}s", assetPath.c_str());
        MediaFileUtils::DeleteFile(assetPath);
    }
    resultSet->Close();
}

static void InsertAsset(const std::string &displayName, int32_t pending = 0)
{
    MEDIA_INFO_LOG("displayName:%{public}s pending:%{public}d", displayName.c_str(), pending);

    std::string ext;
    std::string title;
    int32_t errCode = ParameterUtils::GetTitleAndExtension(displayName, title, ext);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "GetTitleAndExtension errCode:%{public}d", errCode);

    std::string assetPath;
    int64_t now = MediaFileUtils::UTCTimeMilliSeconds();
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(ext);
    int32_t mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
    static int32_t staticId = 1;
    int32_t id = (now > 0 && now <= INT32_MAX) ? static_cast<int32_t>(now) : staticId++;
    errCode = MediaLibraryAssetOperations::CreateAssetPathById(id, mediaType, ext, assetPath);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "CreateAssetPathById errCode:%{public}d", errCode);

    std::vector<std::pair<std::string, std::string>> items = {
        {MediaColumn::MEDIA_FILE_PATH, Quote(assetPath)}, {MediaColumn::MEDIA_SIZE, "175258"},
        {MediaColumn::MEDIA_TITLE, Quote(title)}, {MediaColumn::MEDIA_NAME, Quote(displayName)},
        {MediaColumn::MEDIA_TYPE, to_string(mediaType)},
        {MediaColumn::MEDIA_OWNER_PACKAGE, Quote("com.ohos.camera")}, {MediaColumn::MEDIA_PACKAGE_NAME, Quote("相机")},
        {MediaColumn::MEDIA_DATE_ADDED, to_string(now)}, {MediaColumn::MEDIA_DATE_MODIFIED, "0"},
        {MediaColumn::MEDIA_DATE_TAKEN, to_string(now)}, {MediaColumn::MEDIA_DURATION, "0"},
        {MediaColumn::MEDIA_TIME_PENDING, to_string(pending)},
        {PhotoColumn::PHOTO_HEIGHT, "1280"}, {PhotoColumn::PHOTO_WIDTH, "960"},
        {PhotoColumn::PHOTO_SHOOTING_MODE, "'1'"},
    };

    std::string values;
    std::string columns;
    for (const auto &item : items) {
        if (!columns.empty()) {
            columns.append(",");
            values.append(",");
        }
        columns.append(item.first);
        values.append(item.second);
    }
    std::string sql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + columns + ") VALUES (" + values + ")";
    errCode = g_rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_LOG(errCode == E_OK, "ExecuteSql errCode:%{public}d", errCode);

    MEDIA_INFO_LOG("CreateFile assetPath:%{public}s", assetPath.c_str());
    MediaFileUtils::CreateFile(assetPath);
}

static int32_t GetAssetId(const std::string &displayName)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, displayName:%{public}s", displayName.c_str());
        return 0;
    }
    int32_t assetId = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetId = MediaLibraryRdbStore::GetInt(resultSet, MediaColumn::MEDIA_ID);
        MEDIA_INFO_LOG("resultSet: assetId:%{public}d", assetId);
    }
    resultSet->Close();
    return assetId;
}

static std::string GetAssetColumn(int32_t fileId, const std::string &column)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns = { column };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        return "";
    }

    std::string value;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        value = MediaLibraryRdbStore::GetString(resultSet, column);
    }

    resultSet->Close();
    return value;
}

void ModifyAssetsTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearAssetsFile();
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void ModifyAssetsTest::TearDownTestCase(void)
{
    ClearAssetsFile();
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void ModifyAssetsTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void ModifyAssetsTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

using ServiceCall = std::function<void(MessageParcel &data, MessageParcel &reply)>;

int32_t ServiceModifyAsset(ModifyAssetsReqBody &reqBody, ServiceCall call)
{
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    call(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    return respVo.GetErrCode();
}

int32_t SetAssetTitle(int32_t fileId, const std::string &title)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(fileId);
    reqBody.title = title;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SetAssetTitle(data, reply);
    };

    return ServiceModifyAsset(reqBody, call);
}

int32_t SetAssetPending(int32_t fileId, int32_t pending)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(fileId);
    reqBody.pending = pending;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SetAssetPending(data, reply);
    };

    return ServiceModifyAsset(reqBody, call);
}

int32_t SetAssetsFavorite(int32_t fileId, int32_t favorite)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(fileId);
    reqBody.favorite = favorite;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SetAssetsFavorite(data, reply);
    };

    return ServiceModifyAsset(reqBody, call);
}

int32_t SetAssetsHiddenStatus(int32_t fileId, int32_t hiddenStatus)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(fileId);
    reqBody.hiddenStatus = hiddenStatus;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SetAssetsHiddenStatus(data, reply);
    };

    return ServiceModifyAsset(reqBody, call);
}

int32_t SetAssetsRecentShowStatus(int32_t fileId, int32_t recentShowStatus)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(fileId);
    reqBody.recentShowStatus = recentShowStatus;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SetAssetsRecentShowStatus(data, reply);
    };

    return ServiceModifyAsset(reqBody, call);
}

int32_t SetAssetsUserComment(int32_t fileId, const std::string &userComment)
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(fileId);
    reqBody.userComment = userComment;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SetAssetsUserComment(data, reply);
    };

    return ServiceModifyAsset(reqBody, call);
}

HWTEST_F(ModifyAssetsTest, SetAssetPending_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAssetPending_Test_001");
    InsertAsset("Pending_Test_001.jpg", -2);
    int32_t assetId = GetAssetId("Pending_Test_001.jpg");
    ASSERT_GT(assetId, 0);

    int32_t errCode = SetAssetPending(assetId, -1);
    ASSERT_LT(errCode, 0);

    std::string data;
    errCode = SetAssetPending(assetId, 1);
    ASSERT_EQ(errCode, 0);

    errCode = SetAssetPending(assetId, 0);
    ASSERT_EQ(errCode, 0);
    data = GetAssetColumn(assetId, MediaColumn::MEDIA_TIME_PENDING);
    ASSERT_EQ(data, "0");
}

HWTEST_F(ModifyAssetsTest, SetAssetsFavorite_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAssetsFavorite_Test_001");
    InsertAsset("Favorite_Test_001.jpg");
    int32_t assetId = GetAssetId("Favorite_Test_001.jpg");
    ASSERT_GT(assetId, 0);

    int32_t errCode = SetAssetsFavorite(assetId, -1);
    ASSERT_LT(errCode, 0);

    std::string data;
    errCode = SetAssetsFavorite(assetId, 1);
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, MediaColumn::MEDIA_IS_FAV);
    ASSERT_EQ(data, "1");

    errCode = SetAssetsFavorite(assetId, 0);
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, MediaColumn::MEDIA_IS_FAV);
    ASSERT_EQ(data, "0");
}


HWTEST_F(ModifyAssetsTest, SetAssetsHiddenStatus_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAssetsHiddenStatus_Test_001");
    InsertAsset("Hidden_Test_001.jpg");
    int32_t assetId = GetAssetId("Hidden_Test_001.jpg");
    ASSERT_GT(assetId, 0);

    int32_t errCode = SetAssetsHiddenStatus(assetId, -1);
    ASSERT_LT(errCode, 0);

    std::string data;
    errCode = SetAssetsHiddenStatus(assetId, 1);
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, MediaColumn::MEDIA_HIDDEN);
    ASSERT_EQ(data, "1");

    errCode = SetAssetsHiddenStatus(assetId, 0);
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, MediaColumn::MEDIA_HIDDEN);
    ASSERT_EQ(data, "0");
}

HWTEST_F(ModifyAssetsTest, SetAssetsRecentShowStatus_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAssetsRecentShowStatus_Test_001");
    InsertAsset("RecentShow_Test_001.jpg");
    int32_t assetId = GetAssetId("RecentShow_Test_001.jpg");
    ASSERT_GT(assetId, 0);

    int32_t errCode = SetAssetsRecentShowStatus(assetId, -1);
    ASSERT_LT(errCode, 0);

    std::string data;
    errCode = SetAssetsRecentShowStatus(assetId, 1);
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, PhotoColumn::PHOTO_IS_RECENT_SHOW);
    ASSERT_EQ(data, "1");

    errCode = SetAssetsRecentShowStatus(assetId, 0);
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, PhotoColumn::PHOTO_IS_RECENT_SHOW);
    ASSERT_EQ(data, "0");
}

HWTEST_F(ModifyAssetsTest, SetAssetsUserComment_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAssetsUserComment_Test_001");
    InsertAsset("Comment_Test_001.jpg");
    int32_t assetId = GetAssetId("Comment_Test_001.jpg");
    ASSERT_GT(assetId, 0);

    std::string data;
    int32_t errCode = SetAssetsUserComment(assetId, "SetAssetsUserComment_Test_001");
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, PhotoColumn::PHOTO_USER_COMMENT);
    ASSERT_EQ(data, "SetAssetsUserComment_Test_001");

    errCode = SetAssetsUserComment(assetId, "");
    ASSERT_GT(errCode, 0);
    data = GetAssetColumn(assetId, PhotoColumn::PHOTO_USER_COMMENT);
    ASSERT_EQ(data, "");
}
}  // namespace OHOS::Media