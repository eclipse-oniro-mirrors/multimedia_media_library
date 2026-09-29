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

#include "add_visit_count_test.h"

#include <string>
#include <vector>

#include "media_assets_controller_service.h"

#include "add_visit_count_vo.h"
#include "media_empty_obj_vo.h"
#include "media_resp_vo.h"

#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_asset_operations.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_visit_count_manager.h"
#include "mimetype_utils.h"
#include "parameter_utils.h"
#include "rdb_predicates.h"

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
    NativeRdb::RdbPredicates predicates(table);
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
        assetPath = GetStringVal(columns.front(), resultSet);
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
    int32_t id = (now > 0 && now <= INT32_MAX) ? static_cast<int32_t>(now) : 1;
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
        assetId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        MEDIA_INFO_LOG("resultSet: assetId:%{public}d", assetId);
    }
    resultSet->Close();
    return assetId;
}

static int32_t GetAssetIntColumn(int32_t fileId, const std::string &column)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_ID, fileId);
    vector<string> columns = { column };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, fileId:%{public}d column:%{public}s", fileId, column.c_str());
        return 0;
    }

    int32_t value = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        value = GetInt32Val(column, resultSet);
    }

    MEDIA_INFO_LOG("fileId:%{public}d column:%{public}s value:%{public}d", fileId, column.c_str(), value);
    resultSet->Close();
    return value;
}

void AddAssetVisitCountTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearAssetsFile();
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    InsertAsset("AddAssetVisitCountTest.jpg");
    MEDIA_INFO_LOG("SetUpTestCase");
}

void AddAssetVisitCountTest::TearDownTestCase(void)
{
    ClearAssetsFile();
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void AddAssetVisitCountTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
}

void AddAssetVisitCountTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

int32_t AddAssetVisitCount(int32_t fileId, int32_t visitType)
{
    AddAssetVisitCountReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.visitType = visitType;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->AddAssetVisitCount(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    return respVo.GetErrCode();
}

HWTEST_F(AddAssetVisitCountTest, AddAssetVisitCount_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AddAssetVisitCount_Test_001");
    int32_t assetId = GetAssetId("AddAssetVisitCountTest.jpg");
    ASSERT_GT(assetId, 0);

    ASSERT_LT(AddAssetVisitCount(-1, 0), 0);
    ASSERT_LT(AddAssetVisitCount(assetId, 2), 0);
    ASSERT_LT(AddAssetVisitCount(assetId, -1), 0);
}

HWTEST_F(AddAssetVisitCountTest, AddAssetVisitCount_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start AddAssetVisitCount_Test_002");
    int32_t assetId = GetAssetId("AddAssetVisitCountTest.jpg");
    ASSERT_GT(assetId, 0);

    int32_t visitCount = GetAssetIntColumn(assetId, PhotoColumn::PHOTO_VISIT_COUNT);
    int32_t LcdVisitCount = GetAssetIntColumn(assetId, PhotoColumn::PHOTO_LCD_VISIT_COUNT);

    ASSERT_EQ(AddAssetVisitCount(assetId, 0), 0);
    ASSERT_EQ(AddAssetVisitCount(assetId, 1), 0);

    ASSERT_EQ(GetAssetIntColumn(assetId, PhotoColumn::PHOTO_VISIT_COUNT), visitCount);
    ASSERT_EQ(GetAssetIntColumn(assetId, PhotoColumn::PHOTO_LCD_VISIT_COUNT), LcdVisitCount);
}

}  // namespace OHOS::Media