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

#include "delete_assets_permanently_test.h"

#include <string>
#include <vector>

#include "media_assets_controller_service.h"
#include "delete_photos_completed_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static const string SQL_INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";

static int32_t ClearTable(const string &table)
{
    RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to clear album table, err: %{public}d", err);
        return E_HAS_DB_ERROR;
    }
    system("rm -rf /storage/cloud/files/Photo/16/");
    return E_OK;
}

static int32_t GetAssetId(const std::string &title)
{
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_TITLE, title);
    vector<string> columns = { PhotoColumn::MEDIA_ID };
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicate, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query failed, title:%{public}s", title.c_str());
        return 0;
    }
    int32_t assetId = 0;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        assetId = MediaLibraryRdbStore::GetInt(resultSet, PhotoColumn::MEDIA_ID);
        MEDIA_INFO_LOG("resultSet: assetId:%{public}d", assetId);
    }
    resultSet->Close();
    return assetId;
}

static void InsertAssetIntoPhotosTable()
{
    // file_id,
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode
    g_rdbStore->ExecuteSql(SQL_INSERT_PHOTO + " VALUES(" +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'del_pic', 'del_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1' )"); // cam, pic, shootingmode = 1
    system("mkdir -p /storage/cloud/files/Photo/16/ && touch /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
}

void DeleteAssetsPermanentlyTest::SetUpTestCase(void)
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

void DeleteAssetsPermanentlyTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void DeleteAssetsPermanentlyTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUp");
}

void DeleteAssetsPermanentlyTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

int32_t DeleteAssetsPermanently(const std::string &fileId)
{
    DeletePhotosCompletedReqBody reqBody;
    if (!fileId.empty()) {
        reqBody.fileIds.push_back(fileId);
    }

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->DeleteAssetsPermanentlyWithUri(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    return respVo.GetErrCode();
}

HWTEST_F(DeleteAssetsPermanentlyTest, DeleteAssetsPermanently_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DeleteAssetsPermanently_Test_001");
    InsertAssetIntoPhotosTable();
    int32_t assetId = GetAssetId("del_pic");
    EXPECT_GT(assetId, 0);

    int32_t errCode = DeleteAssetsPermanently("");
    EXPECT_LT(errCode, 0);

    errCode = DeleteAssetsPermanently("0");
    EXPECT_EQ(errCode, 0);

    errCode = DeleteAssetsPermanently(to_string(assetId));
    EXPECT_EQ(errCode, 1);

    MEDIA_INFO_LOG("End DeleteAssetsPermanently_Test_001");
}
}  // namespace OHOS::Media