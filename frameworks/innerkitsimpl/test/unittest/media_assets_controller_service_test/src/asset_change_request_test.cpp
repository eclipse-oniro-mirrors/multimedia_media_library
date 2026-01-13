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

#include "asset_change_request_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "asset_change_vo.h"
#include "add_image_vo.h"

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
static constexpr int32_t SLEEP_ONE_SECOND = 1;
static const string SQL_INSERT_PHOTO =
    "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE +
    ", " + MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " +
    MediaColumn::MEDIA_OWNER_PACKAGE + ", " + MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED +
    ", " + MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ", " + PhotoColumn::PHOTO_ID + ", " +
    PhotoColumn::PHOTO_IS_TEMP + ", " + PhotoColumn::PHOTO_DEFERRED_PROC_TYPE + ", " +
    PhotoColumn::PHOTO_CE_AVAILABLE + ", " + PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS + ")";
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

void AssetChangeRequestTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    ASSERT_TRUE(MediaLibraryUnitTestUtils::CreateBasicTables(g_rdbStore));
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void AssetChangeRequestTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND));
}

void AssetChangeRequestTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND));
    MEDIA_INFO_LOG("SetUp");
}

void AssetChangeRequestTest::TearDown(void)
{
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_ONE_SECOND * 2));
    MEDIA_INFO_LOG("TearDown");
}

static void InsertAsset()
{
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode, photo_id, is_temp, deferred_proc_type, ce_available
    // composite_display_status
    std::string insertSql =
        SQL_INSERT_PHOTO + " VALUES (" +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1', '202410011800', 0, 0, 0, 0" + VALUES_END;
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static void InsertTempAsset()
{
    // data, size, title, display_name, media_type,
    // owner_package, package_name, date_added, date_modified, date_taken, duration, is_favorite, date_trashed, hidden
    // height, width, edit_time, shooting_mode, photo_id, is_temp, deferred_proc_type, ce_available
    // composite_display_status
    std::string insertSql =
        SQL_INSERT_PHOTO + " VALUES (" +
        "'/storage/cloud/files/Photo/16/IMG_1501924305_000.jpg', 175258, 'cam_pic', 'cam_pic.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 0, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, '1', '202410011800', 1, 0, 0, 0" + VALUES_END;
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

static int32_t SetEffectMode(int32_t fileId, int32_t effectMode)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.effectMode = effectMode;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetEffectMode(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetEffectMode ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t SetOrientation(int32_t fileId, int32_t orientation)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.orientation = orientation;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetOrientation(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetOrientation ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t SetSupportedWatermarkType(int32_t fileId, int32_t watermarkType)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.watermarkType = watermarkType;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetSupportedWatermarkType(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetSupportedWatermarkType ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t SetHasAppLink(int32_t fileId, int32_t has)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.hasAppLink = has;
 
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }
 
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetHasAppLink(data, reply);
 
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetHasAppLink ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}
 
static int32_t SetAppLink(int32_t fileId, string link)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.appLink = link;
 
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }
 
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetAppLink(data, reply);
 
    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetAppLink ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t SetCompositeDisplayMode(int32_t fileId, int32_t compositeDisplayMode)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.compositeDisplayMode = compositeDisplayMode;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetCompositeDisplayMode(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetCompositeDisplayMode ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t SetVideoEnhancementAttr(int32_t fileId, string photoId, string path)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.photoId = photoId;
    reqBody.path = path;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->SetVideoEnhancementAttr(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("SetVideoEnhancementAttr ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t DiscardCameraPhoto(int32_t fileId)
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = fileId;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->DiscardCameraPhoto(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("DiscardCameraPhoto ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

static int32_t QueryFileIdByDisplayName(const string &displayName)
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

static int32_t QueryEffectModeByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get effectMode");
        return -1;
    }
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    return effectMode;
}

static int32_t QueryOrientationByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get orientation");
        return -1;
    }
    int32_t orientation = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
    return orientation;
}

static int32_t QueryWatermarkTypeByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get watermarkType");
        return -1;
    }
    int32_t watermarkType = GetInt32Val(PhotoColumn::SUPPORTED_WATERMARK_TYPE, resultSet);
    return watermarkType;
}

static int32_t UpdateCompositeDisplayStatus(int32_t fileId, int32_t compositeDisplayStatus)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t changedRows = -1;
    ValuesBucket valueBucket;
    valueBucket.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, compositeDisplayStatus);
    g_rdbStore->Update(changedRows, valueBucket, rdbPredicates);
    return changedRows;
}

static int32_t QueryCompositeDisplayStatusByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get watermarkType");
        return -1;
    }
    int32_t compositeDisplayStatus = GetInt32Val(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, resultSet);
    return compositeDisplayStatus;
}

static int32_t QueryPhotoQualityByDisplayName(const string &displayName)
{
    vector<string> columns;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get photoQuality");
        return -1;
    }
    int32_t photoQuality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    return photoQuality;
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_001 for SetEffectMode Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
    int32_t effectMode = QueryEffectModeByDisplayName("cam_pic.jpg");
    ASSERT_EQ(effectMode, 0);

    int32_t result = SetEffectMode(fileId, 1);
    ASSERT_EQ(result, 1);

    effectMode = QueryEffectModeByDisplayName("cam_pic.jpg");
    ASSERT_EQ(effectMode, 1);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_002 for SetEffectMode Begin");
    int32_t result = SetEffectMode(-1, 1);
    ASSERT_LT(result, 0);

    result = SetEffectMode(1, 1000);
    ASSERT_LT(result, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_003 for setOrientation Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
    int32_t orientation = QueryOrientationByDisplayName("cam_pic.jpg");
    ASSERT_EQ(orientation, 0);

    int32_t result = SetOrientation(fileId, 90);
    ASSERT_LT(result, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_004 for SetEffectMode Begin");
    // support 0, 90, 180, 270
    int32_t result = SetOrientation(1, 91);
    ASSERT_LT(result, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_005 for setSupportedWatermarkType Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
    int32_t watermarkType = QueryWatermarkTypeByDisplayName("cam_pic.jpg");
    ASSERT_EQ(watermarkType, 0);

    int32_t result = SetSupportedWatermarkType(fileId, 1);
    ASSERT_EQ(result, 1);

    watermarkType = QueryWatermarkTypeByDisplayName("cam_pic.jpg");
    ASSERT_EQ(watermarkType, 1);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_006 for setSupportedWatermarkType Begin");
    int32_t result = SetSupportedWatermarkType(1, 10);
    ASSERT_LT(result, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_007 for SetVideoEnhancementAttr Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
    string path = "file//media/Photo/";
    int32_t photoQuality = QueryPhotoQualityByDisplayName("cam_pic.jpg");
    ASSERT_EQ(photoQuality, 0);

    int32_t result = SetVideoEnhancementAttr(fileId, "202410011800", path);
    ASSERT_EQ(result, 0);

    photoQuality = QueryPhotoQualityByDisplayName("cam_pic.jpg");
    ASSERT_EQ(photoQuality, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_008 for SetVideoEnhancementAttr Begin");
    string path = "file//media/Photo/";
    int32_t result = SetVideoEnhancementAttr(-1, "202410011800", path);
    ASSERT_LT(result, 0);

    result = SetVideoEnhancementAttr(1, "123456789012345678901234567890123", path);
    ASSERT_LT(result, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_009 for DiscardCameraPhoto Begin");
    InsertTempAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    int32_t result = DiscardCameraPhoto(fileId);
    ASSERT_EQ(result, 1);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_010 for DiscardCameraPhoto Begin");
    int32_t result = DiscardCameraPhoto(-1);
    ASSERT_EQ(result, 0);

    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    result = DiscardCameraPhoto(fileId);
    ASSERT_EQ(result, 0);

    fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_011, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_011 for AssetChangeCreateAsset Begin");
    AssetChangeReqBody reqBody;
    reqBody.values.Put(ASSET_EXTENTION, "jpg");
    reqBody.values.Put(MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    reqBody.values.Put(PhotoColumn::MEDIA_TITLE, "20250602162718617");

    MessageParcel data;
    ASSERT_EQ(reqBody.Marshalling(data), true);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeCreateAsset(data, reply);

    IPC::MediaRespVo<AssetChangeRespBody> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);

    MEDIA_INFO_LOG("AssetChangeCreateAsset ErrCode:%{public}d", respVo.GetErrCode());
    ASSERT_EQ(respVo.GetErrCode(), 0);

    AssetChangeRespBody respBody = respVo.GetBody();
    ASSERT_GT(respBody.fileId, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_012, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_012 for AssetChangeCreateAsset Begin");
    auto service = make_shared<MediaAssetsControllerService>();
    MessageParcel data;
    MessageParcel reply;
    service->AssetChangeCreateAsset(data, reply);

    IPC::MediaRespVo<AssetChangeRespBody> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    MEDIA_INFO_LOG("AssetChangeCreateAsset ErrCode:%{public}d", respVo.GetErrCode());
    ASSERT_LT(respVo.GetErrCode(), 0);

    MessageParcel data1;
    MessageParcel reply1;
    AssetChangeReqBody reqBody;
    ASSERT_EQ(reqBody.Marshalling(data1), true);

    service->AssetChangeCreateAsset(data1, reply1);

    IPC::MediaRespVo<AssetChangeRespBody> respVo1;
    ASSERT_EQ(respVo1.Unmarshalling(reply1), true);

    MEDIA_INFO_LOG("AssetChangeCreateAsset ErrCode1:%{public}d", respVo1.GetErrCode());
    ASSERT_LT(respVo1.GetErrCode(), 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_013, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_013 for AssetChangeAddImage Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
    AddImageReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.photoId = "20250527162718617";
    reqBody.deferredProcType = 0;

    MessageParcel data;
    ASSERT_EQ(reqBody.Marshalling(data), true);

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->AssetChangeAddImage(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);

    MEDIA_INFO_LOG("AssetChangeAddImage ErrCode:%{public}d", respVo.GetErrCode());
    ASSERT_EQ(respVo.GetErrCode(), 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_014, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_014 for AssetChangeAddImage Begin");
    auto service = make_shared<MediaAssetsControllerService>();
    MessageParcel data;
    MessageParcel reply;
    service->AssetChangeAddImage(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    ASSERT_EQ(respVo.Unmarshalling(reply), true);
    MEDIA_INFO_LOG("AssetChangeAddImage ErrCode:%{public}d", respVo.GetErrCode());
    ASSERT_LT(respVo.GetErrCode(), 0);

    MessageParcel data1;
    MessageParcel reply1;
    AddImageReqBody reqBody;
    ASSERT_EQ(reqBody.Marshalling(data1), true);

    service->AssetChangeAddImage(data1, reply1);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo1;
    ASSERT_EQ(respVo1.Unmarshalling(reply1), true);

    MEDIA_INFO_LOG("AssetChangeAddImage ErrCode1:%{public}d", respVo1.GetErrCode());
    ASSERT_LT(respVo1.GetErrCode(), 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_015, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_015 for setHasAppLink Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
 
    int32_t result = SetHasAppLink(fileId, 1);
    MEDIA_INFO_LOG("AssetChangeRequest_Test_015 result:%{public}d", result);
    ASSERT_EQ(result, 0);
}
 
HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_Test_016, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_Test_016 for SetAppLink Begin");
    InsertAsset();
    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);
 
    int32_t result = SetAppLink(fileId, "www.baid.com");
    MEDIA_INFO_LOG("AssetChangeRequest_Test_016 result:%{public}d", result);
    ASSERT_EQ(result, 0);
}

// 测试云增强复合图切换场景
HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_001 Begin");
    system("mkdir -p /storage/cloud/files/Photo/16/");
    system("touch /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("mkdir -p /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");

    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::CLOUD_ENHANCEMENT));
    ASSERT_EQ(result, 0);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    system("rm -rf /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_002 Begin");
    system("mkdir -p /storage/cloud/files/Photo/16/");
    system("touch /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("mkdir -p /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");

    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::DEFAULT));
    ASSERT_EQ(result, 0);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));

    system("rm -rf /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_003 for setCompositeDisplayMode Begin");
    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::DEFAULT));
    ASSERT_EQ(result, -1);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_004 Begin");
    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::CLOUD_ENHANCEMENT));
    ASSERT_EQ(result, -1);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_005 Begin");
    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::CLOUD_ENHANCEMENT));
    ASSERT_EQ(result, -1);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT));
}

// when there exists no file
HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_006 Begin");

    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    int32_t result = SetCompositeDisplayMode(fileId, 0);
    ASSERT_EQ(result, -1);
}

// when fileID not exist
HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_007 Begin");
     
    int32_t result = SetCompositeDisplayMode(10, 1);
    ASSERT_LT(result, 0);
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_008 Begin");
    system("mkdir -p /storage/cloud/files/Photo/16/");
    system("touch /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("mkdir -p /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source.jpg");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/editdata_camera");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");

    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::CLOUD_ENHANCEMENT));
    ASSERT_EQ(result, 0);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    system("rm -rf /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/editdata_camera");
}

HWTEST_F(AssetChangeRequestTest, AssetChangeRequest_SetCompositeDisplayMode_Test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("AssetChangeRequest_SetCompositeDisplayMode_Test_009 Begin");
    system("mkdir -p /storage/cloud/files/Photo/16/");
    system("touch /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("mkdir -p /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source.jpg");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/editdata_camera");
    system("touch /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");

    InsertAsset();

    int32_t fileId = QueryFileIdByDisplayName("cam_pic.jpg");
    ASSERT_GT(fileId, 0);

    UpdateCompositeDisplayStatus(fileId, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));
    int32_t compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));

    int result = SetCompositeDisplayMode(fileId, static_cast<int32_t>(CompositeDisplayMode::DEFAULT));
    ASSERT_EQ(result, 0);

    compositeDisplayStatus = QueryCompositeDisplayStatusByDisplayName("cam_pic.jpg");
    ASSERT_EQ(compositeDisplayStatus, static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));

    system("rm -rf /storage/cloud/files/Photo/16/IMG_1501924305_000.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/source_back.jpg");
    system("rm -rf /storage/cloud/files/.editData/Photo/16/IMG_1501924305_000.jpg/editdata_camera");
}

}  // namespace OHOS::Media