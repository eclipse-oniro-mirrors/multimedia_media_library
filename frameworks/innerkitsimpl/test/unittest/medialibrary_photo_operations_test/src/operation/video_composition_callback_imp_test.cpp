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

#define MLOG_TAG "VideoCompositionCallbackImplTest"

#include "video_composition_callback_imp_test.h"

#include "result_set.h"
#include "result_set_utils.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_photo_operations.h"
#include "video_composition_callback_imp.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static  std::string EDITDATA_VALUE = "{\"imageEffect\":{\"filters\":[{\"name\":\"InplaceSticker\","
    "\"values\":{\"RESOURCE_DIRECTORY\":\"/sys_prod/resource/camera\"}}],\"name\":\"brandWaterMark\"}}";
static std::string EDITDATA_WATERMARK = "{\"imageEffect\":{\"filters\":[{\"name\":\"FrameSticker\","
    "\"values\":{\"RESOURCE_DIRECTORY\":\"/sys_prod/resource/camera\"},"
    "\"FILTER_CATEGORY\":\"BORDER_WATERMARK\"}],\"name\":\"imageEdit\"}}";
static std::string EDITDATA_FILTER = "{\"imageEffect\":{\"filters\":[{\"name\":\"Moody\","
    "\"values\":{\"FILTER_PARA\":0}}],\"name\":\"imageEdit\"}}";
static std::string EDITDATA_WATERMARK_AND_FILTER = "{\"imageEffect\":{\"filters\":[{\"name\":\"Moody\","
    "\"values\":{\"FILTER_PARA\":0}},{\"name\":\"FrameSticker\","
    "\"values\":{\"RESOURCE_DIRECTORY\":\"/sys_prod/resource/camera\"},"
    "\"FILTER_CATEGORY\":\"BORDER_WATERMARK\"}],\"name\":\"imageEdit\"}}";
static std::string EDITDATA_WITHOUT_WATERMARK = "{\"imageEffect\":{\"filters\":[],\"name\":\"imageEdit\"}}";

static void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
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

static void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        if (g_rdbStore == nullptr) {
            MEDIA_ERR_LOG("can not get g_rdbstore");
            return;
        }
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void ClearAndRestart()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    system("rm -rf /storage/cloud/files/.thumbs");
    system("rm -rf /storage/cloud/files/.editData");
    system("rm -rf /storage/cloud/files/.cache");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    SetTables();
}

static string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

static int32_t MakePhotoUnpending(int fileId, bool isMovingPhoto = false)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return E_INVALID_FILEID;
    }

    string path = GetFilePath(fileId);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get path failed");
        return E_INVALID_VALUES;
    }
    int32_t errCode = MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Can not create asset");
        return errCode;
    }

    if (isMovingPhoto) {
        string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
        errCode = MediaFileUtils::CreateAsset(videoPath);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Can not create video asset");
            return errCode;
        }
    }

    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return E_HAS_DB_ERROR;
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    ValuesBucket values;
    values.PutLong(PhotoColumn::MEDIA_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = g_rdbStore->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    return E_OK;
}

static int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }

    int32_t errCode = MakePhotoUnpending(ret);
    if (errCode != E_OK) {
        return errCode;
    }
    return ret;
}

void VideoCompositionCallbackImplTest::SetUpTestCase()
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    SetTables();
}

void VideoCompositionCallbackImplTest::TearDownTestCase()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    system("rm -rf /storage/cloud/files/*");
    ClearAndRestart();
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("Clean is finish");
}

void VideoCompositionCallbackImplTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get rdbstore");
        exit(1);
    }
    ClearAndRestart();
}

void VideoCompositionCallbackImplTest::TearDown() {}

HWTEST_F(VideoCompositionCallbackImplTest, VideoComposition_Test_onProgress, TestSize.Level1)
{
    auto imp = make_shared<VideoCompositionCallbackImpl>();
    ASSERT_NE(imp, nullptr);
    imp->onProgress(10);
    imp->onProgress(11);
}

HWTEST_F(VideoCompositionCallbackImplTest, VideoComposition_Test_CallStartComposite_Invalid_Path, TestSize.Level1)
{
    MEDIA_INFO_LOG("start VideoComposition_Test_CallStartComposite_Invalid_Path");
    string srcPatch;
    string videoPath;
    string effectDescription;
    string assetPath;

    auto imp = make_shared<VideoCompositionCallbackImpl>();
    ASSERT_NE(imp, nullptr);
    auto ret =  imp->CallStartComposite(srcPatch, videoPath, effectDescription, assetPath, false);
    EXPECT_EQ(ret, E_HAS_FS_ERROR);
    MEDIA_INFO_LOG("end VideoComposition_Test_CallStartComposite_Invalid_Path");
}

HWTEST_F(VideoCompositionCallbackImplTest, VideoComposition_Test_AddCompositionTask, TestSize.Level1)
{
    MEDIA_INFO_LOG("start VideoComposition_Test_AddCompositionTask");

    auto imp = make_shared<VideoCompositionCallbackImpl>();
    ASSERT_NE(imp, nullptr);

    auto fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "photo.jpg");
    ASSERT_GT(fileId, 0);
    string assetPath;
    imp->AddCompositionTask(assetPath, assetPath, false);

    assetPath = GetFilePath(fileId);
    imp->AddCompositionTask(assetPath, EDITDATA_VALUE, false);

    MEDIA_INFO_LOG("end VideoComposition_Test_AddCompositionTask");
}

HWTEST_F(VideoCompositionCallbackImplTest, VideoComposition_Test_EraseWatermarkTagAndStickerField, TestSize.Level1)
{
    MEDIA_INFO_LOG("start VideoComposition_Test_EraseWatermarkTag");
    auto imp = make_shared<VideoCompositionCallbackImpl>();
    ASSERT_NE(imp, nullptr);
    std::string editData = "";
    bool isFiltersFieldEmpty = false;
    int32_t errCode = imp->EraseWatermarkTagAndStickerField(editData, isFiltersFieldEmpty);
    EXPECT_EQ(editData, "");
    EXPECT_EQ(isFiltersFieldEmpty, false);
    EXPECT_EQ(errCode, E_ERR);

    editData = EDITDATA_WATERMARK;
    isFiltersFieldEmpty = false;
    errCode = imp->EraseWatermarkTagAndStickerField(editData, isFiltersFieldEmpty);
    EXPECT_EQ(editData, EDITDATA_WITHOUT_WATERMARK);
    EXPECT_EQ(isFiltersFieldEmpty, true);
    EXPECT_EQ(errCode, E_OK);

    editData = EDITDATA_FILTER;
    isFiltersFieldEmpty = false;
    errCode = imp->EraseWatermarkTagAndStickerField(editData, isFiltersFieldEmpty);
    EXPECT_EQ(editData, EDITDATA_FILTER);
    EXPECT_EQ(isFiltersFieldEmpty, false);
    EXPECT_EQ(errCode, E_OK);

    editData = EDITDATA_WATERMARK_AND_FILTER;
    isFiltersFieldEmpty = false;
    errCode = imp->EraseWatermarkTagAndStickerField(editData, isFiltersFieldEmpty);
    EXPECT_EQ(editData, EDITDATA_FILTER);
    EXPECT_EQ(isFiltersFieldEmpty, false);
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("end VideoComposition_Test_EraseWatermarkTag");
}

} // namespace Media
} // namespace OHOS
