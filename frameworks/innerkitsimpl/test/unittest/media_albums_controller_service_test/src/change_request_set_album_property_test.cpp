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

#define MLOG_TAG "MediaAlbumsControllerServiceTest"

#include "change_request_set_album_property_test.h"

#include <memory>
#include <string>

#define private public
#define protected public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#undef protected

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "permission_utils.h"
#include "result_set_utils.h"
#include "analysis_album_attribute_request_utils.h"
#include "vision_db_sqls.h"
#include "story_db_sqls.h"
#include "vision_db_sqls_more.h"
#include "vision_portrait_nickname_column.h"
#include "create_album_vo.h"
#include "create_asset_vo.h"
#include "change_request_dismiss_vo.h"
#include "change_request_set_album_name_vo.h"
#include "change_request_operate_album_attribute_vo.h"
#include "change_request_set_cover_uri_vo.h"
#include "change_request_set_display_level_vo.h"
#include "change_request_set_is_me_vo.h"
#include "media_assets_controller_service.h"
#include "set_subtitle_vo.h"
#include "set_highlight_user_action_data_vo.h"
#include "media_upgrade.h"
#include "analysis_album_attribute_const.h"
#include "get_self_permissions.h"
#include "medialibrary_client_errno.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace IPC;
using namespace OHOS::Security::AccessToken;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;
static std::vector<std::string> createTableSqlLists = {
    PhotoAlbumColumns::CREATE_TABLE,
    PhotoUpgrade::CREATE_PHOTO_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_ANALYSIS_ALBUM_MAP,
    CREATE_ANALYSIS_NICK_NAME_TABLE,
    CREATE_ANALYSIS_NICK_NAME_UNIQUE_INDEX,
    CREATE_HIGHLIGHT_ALBUM_TABLE,
};

static std::vector<std::string> testTables = {
    PhotoAlbumColumns::TABLE,
    PhotoColumn::PHOTOS_TABLE,
    ANALYSIS_ALBUM_TABLE,
    ANALYSIS_PHOTO_MAP_TABLE,
    ANALYSIS_NICK_NAME_TABLE,
    HIGHLIGHT_ALBUM_TABLE,
};

class SelfTokenGuard {
public:
    SelfTokenGuard() : tokenId_(GetSelfTokenID()) {}
    ~SelfTokenGuard()
    {
        EXPECT_EQ(0, SetSelfTokenID(tokenId_));
        EXPECT_EQ(RET_SUCCESS, AccessTokenKit::ReloadNativeTokenInfo());
    }

private:
    uint64_t tokenId_;
};

void ChangeRequestSetAlbumPropertyTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    EXPECT_EQ(MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(),
        PERM_ACCESS_MEDIALIB_THUMB_DB, 0), E_OK);
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(g_rdbStore, nullptr);
    MediaLibraryUnitTestUtils::CreateTestTables(g_rdbStore, createTableSqlLists);
    MEDIA_INFO_LOG("ChangeRequestSetAlbumPropertyTest SetUpTestCase");
}

void ChangeRequestSetAlbumPropertyTest::TearDownTestCase(void)
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables, true);
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
    MEDIA_INFO_LOG("ChangeRequestSetAlbumPropertyTest TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void ChangeRequestSetAlbumPropertyTest::SetUp()
{
    MediaLibraryUnitTestUtils::CleanTestTables(g_rdbStore, testTables);
    MEDIA_INFO_LOG("ChangeRequestSetAlbumPropertyTest SetUp");
}

void ChangeRequestSetAlbumPropertyTest::TearDown(void) {}

static int32_t ServiceCreateAlbum(const std::string &albumName)
{
    CreateAlbumReqBody reqBody;
    reqBody.albumName = albumName;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->CreatePhotoAlbum(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    return respVo.GetErrCode();
}

int32_t CreatePortraitAlbum(string albumName)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PhotoAlbumSubType::PORTRAIT);
    valuesBucket.Put(ALBUM_NAME, albumName);
    valuesBucket.Put(COUNT, 0);
    valuesBucket.Put(DATE_MODIFIED, 0);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int32_t CreateGroupAlbum(string albumName)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PhotoAlbumSubType::GROUP_PHOTO);
    valuesBucket.Put(ALBUM_NAME, albumName);
    valuesBucket.Put(COUNT, 0);
    valuesBucket.Put(DATE_MODIFIED, 0);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int32_t CreateHighLightAlbum(string albumName)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PhotoAlbumSubType::HIGHLIGHT);
    valuesBucket.Put(ALBUM_NAME, albumName);
    valuesBucket.Put(COUNT, 0);
    valuesBucket.Put(DATE_MODIFIED, 0);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

static int32_t ServicePublicCreateAsset(const std::string &ext, const std::string &title = "")
{
    CreateAssetReqBody reqBody;
    reqBody.mediaType = MEDIA_TYPE_IMAGE;
    reqBody.title = title;
    reqBody.extension = ext;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->PublicCreateAsset(data, reply);

    IPC::MediaRespVo<CreateAssetRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }

    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return errCode;
    }

    return respVo.GetBody().fileId;
}

static int32_t ServiceOperateAlbumAttribute(const ChangeRequestOperateAlbumAttributeReqBody &reqBody)
{
    MessageParcel data;
    if (!reqBody.Marshalling(data)) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestOperateAlbumAttribute(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    if (!respVo.Unmarshalling(reply)) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    return respVo.GetErrCode();
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_001");
    int32_t albumId = ServiceCreateAlbum("Album_Test_001");
    ASSERT_GT(albumId, 0);
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    string coverUri = "file://media/Photo/" + to_string(fileId);
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.coverUri = coverUri;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetCoverUri(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    ChangeRequestSetAlbumNameReqBody setAlbumNameReqBody;
    setAlbumNameReqBody.albumId = reqBody.albumId;
    setAlbumNameReqBody.albumName = "hello";
    setAlbumNameReqBody.albumType = reqBody.albumType;
    setAlbumNameReqBody.albumSubType = reqBody.albumSubType;
    result = setAlbumNameReqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetAlbumName(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, 1);
    MEDIA_INFO_LOG("End SetAlbumProperty_Test_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_002");
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::USER);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIDDEN);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetCoverUri(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);

    ChangeRequestSetAlbumNameReqBody setAlbumNameReqBody;
    setAlbumNameReqBody.albumType = reqBody.albumType;
    setAlbumNameReqBody.albumSubType = reqBody.albumSubType;
    result = setAlbumNameReqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetAlbumName(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);
    MEDIA_INFO_LOG("End SetAlbumProperty_Test_002");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_003, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_003");
    CreatePortraitAlbum("Album_Test_001");
    string albumId = "2";
    int32_t fileId = ServicePublicCreateAsset("jpg");
    ASSERT_GT(fileId, 0);
    string coverUri = "file://media/Photo/" + to_string(fileId);
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetCoverUri(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);

    ChangeRequestSetAlbumNameReqBody setAlbumNameReqBody;
    setAlbumNameReqBody.albumType = reqBody.albumType;
    setAlbumNameReqBody.albumSubType = reqBody.albumSubType;
    setAlbumNameReqBody.albumName = "hello";
    setAlbumNameReqBody.albumId = albumId;
    result = setAlbumNameReqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetAlbumName(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("End SetAlbumProperty_Test_003");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_004, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_004");
    CreateGroupAlbum("3");
    string albumId = "3";
    string coverUri = "file://media/Photo/3";
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetCoverUri(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);

    ChangeRequestSetAlbumNameReqBody setAlbumNameReqBody;
    setAlbumNameReqBody.albumType = reqBody.albumType;
    setAlbumNameReqBody.albumSubType = reqBody.albumSubType;
    setAlbumNameReqBody.albumName = "hello";
    setAlbumNameReqBody.albumId = albumId;
    result = setAlbumNameReqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetAlbumName(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("End SetAlbumProperty_Test_004");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_005, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_005");
    CreateGroupAlbum("4");
    string albumId = "4";
    string coverUri = "file://media/Photo/4";
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<MediaAlbumsControllerService>();
    service->ChangeRequestSetCoverUri(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);

    ChangeRequestSetAlbumNameReqBody setAlbumNameReqBody;
    setAlbumNameReqBody.albumType = reqBody.albumType;
    setAlbumNameReqBody.albumSubType = reqBody.albumSubType;
    setAlbumNameReqBody.albumName = "hello";
    setAlbumNameReqBody.albumId = albumId;
    result = setAlbumNameReqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetAlbumName(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_OK);
    MEDIA_INFO_LOG("End SetAlbumProperty_Test_005");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_006, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_006");
    ChangeRequestSetIsMeReqBody reqBody;
    reqBody.albumId = "1";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->ChangeRequestSetIsMe(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);
    reqBody.isMe = 1;
    result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetIsMe(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_NE(errCode, E_OK);
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_006");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_007, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_007");
    ChangeRequestSetDisplayLevelReqBody reqBody;
    reqBody.albumId = "1";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->ChangeRequestSetDisplayLevel(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);
    reqBody.displayLevel = 2;
    result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestSetDisplayLevel(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);

    MEDIA_INFO_LOG("End SetAlbumProperty_Test_007");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_008, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_008");
    ChangeRequesDismissReqBody reqBody;
    reqBody.albumId = "1";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->ChangeRequestDismiss(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);

    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO);
    result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->ChangeRequestDismiss(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_OK);

    MEDIA_INFO_LOG("End SetAlbumProperty_Test_008");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_009, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_009");
    SetHighlightUserActionDataReqBody reqBody;
    reqBody.albumId = "1";
    reqBody.userActionType = static_cast<int32_t>(HighlightUserActionType::INVALID_USER_ACTION);
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->SetHighlightUserActionData(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);

    reqBody.userActionType = static_cast<int32_t>(HighlightUserActionType::INSERTED_PIC_COUNT);
    result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->SetHighlightUserActionData(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);

    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);

    MEDIA_INFO_LOG("End SetAlbumProperty_Test_009");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, SetAlbumProperty_Test_010, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start SetAlbumProperty_Test_010");
    SetSubtitleReqBody reqBody;
    reqBody.albumId = "1";
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    MessageParcel data;
    bool result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);

    MessageParcel reply;
    auto service = make_shared<AnalysisData::MediaAnalysisDataControllerService>();
    service->SetSubtitle(data, reply);

    IPC::MediaRespVo<MediaEmptyObjVo> respVo;
    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    int32_t errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_INVALID_VALUES);

    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    result = reqBody.Marshalling(data);
    ASSERT_NE(result, false);
    service->SetSubtitle(data, reply);

    result = respVo.Unmarshalling(reply);
    ASSERT_NE(result, false);

    errCode = respVo.GetErrCode();
    EXPECT_EQ(errCode, E_OK);

    MEDIA_INFO_LOG("End SetAlbumProperty_Test_010");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_InvalidAttr_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_InvalidAttr_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_invalid_attr");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = "invalid_attr";
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { "nick_a" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_InvalidAttr_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_UnsupportedType_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_UnsupportedType_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_unsupported_type");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_UPDATE;
    reqBody.values = { "nick_a" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_OPERATION_NOT_SUPPORT);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_UnsupportedType_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_NonPortraitAlbum_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_NonPortraitAlbum_001");
    int32_t albumId = CreateGroupAlbum("group_album_not_supported");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { "nick_a" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_OPERATION_NOT_SUPPORT);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_NonPortraitAlbum_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_EmptyNickName_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_EmptyNickName_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_empty_nick_name");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { "" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_OK);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_EmptyNickName_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_EmptyValues_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_EmptyValues_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_empty_values");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = {};
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_EmptyValues_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_EmptyValuesClient_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_EmptyValuesClient_001");
    AnalysisAlbumOperation operation { ANALYSIS_ALBUM_ATTR_NICK_NAME, ANALYSIS_ALBUM_OP_ADD, {} };
    int32_t errorCode = E_OK;
    bool result = CheckAnalysisAlbumOperationOrThrow(operation,
        [&errorCode]() {
            errorCode = JS_E_PARAM_INVALID;
        },
        [&errorCode]() {
            errorCode = JS_E_OPR_TYPE_NOT_SUPPORT;
        });

    EXPECT_FALSE(result);
    EXPECT_EQ(errorCode, JS_E_PARAM_INVALID);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_EmptyValuesClient_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_ValuesExceedLimit_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_ValuesExceedLimit_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_values_exceed_limit");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values.assign(ANALYSIS_ALBUM_MAX_OPERATION_VALUES + 1, "nick_limit");
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_ValuesExceedLimit_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_NickNameTooLong_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_NickNameTooLong_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_nick_name_too_long");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { string(ANALYSIS_ALBUM_MAX_VALUE_LENGTH + 1, 'n') };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_NickNameTooLong_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_InvalidNickNameCharacter_001,
    TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_InvalidNickNameCharacter_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_invalid_nick_name_character");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { "nick/name" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_InvalidNickNameCharacter_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_DbPermissionDenied_001,
    TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_DbPermissionDenied_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_db_permission_denied");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_NICK_NAME;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { "nick_a" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    SelfTokenGuard tokenGuard;
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("OperateAttributeDbPermissionDenied",
        { PERM_WRITE_IMAGEVIDEO }, tokenId);
    ASSERT_NE(tokenId, 0);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), JS_INNER_FAIL);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_DbPermissionDenied_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_UnsupportedType_ExtraInfo_001,
    TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_UnsupportedType_ExtraInfo_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_unsupported_type");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_EXTRA_INFO;
    reqBody.type = ANALYSIS_ALBUM_OP_ADD;
    reqBody.values = { R"({"contact_key":"123456789"})" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_OPERATION_NOT_SUPPORT);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_UnsupportedType_ExtraInfo_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_EmptyExtraInfo_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_EmptyExtraInfo_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_empty_extra_info");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_EXTRA_INFO;
    reqBody.type = ANALYSIS_ALBUM_OP_UPDATE;
    reqBody.values = { "" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_OK);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_EmptyExtraInfo_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_EmptyValues_ExtraInfo_001,
    TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_EmptyValues_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_empty_values");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_EXTRA_INFO;
    reqBody.type = ANALYSIS_ALBUM_OP_UPDATE;
    reqBody.values = {};
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_EmptyValues_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_ExtraInfo_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_ExtraInfo_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_extra_info");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_EXTRA_INFO;
    reqBody.type = ANALYSIS_ALBUM_OP_UPDATE;
    reqBody.values = { R"({"contact_key":"123456789"})", R"({"contact_key":"987654321"})" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_ExtraInfo_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_ExtraInfoTooLong_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_ExtraInfoTooLong_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_extra_info_too_long");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_EXTRA_INFO;
    reqBody.type = ANALYSIS_ALBUM_OP_UPDATE;
    reqBody.values = { string(ANALYSIS_ALBUM_MAX_VALUE_LENGTH + 1, 'n') };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), E_INVALID_VALUES);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_ExtraInfoTooLong_001");
}

HWTEST_F(ChangeRequestSetAlbumPropertyTest, ChangeRequestOperateAlbumAttribute_DbPermissionDenied_ExtraInfo_001,
    TestSize.Level0)
{
    MEDIA_INFO_LOG("Start ChangeRequestOperateAlbumAttribute_DbPermissionDenied_ExtraInfo_001");
    int32_t albumId = CreatePortraitAlbum("portrait_album_db_permission_denied");
    ASSERT_GT(albumId, 0);

    ChangeRequestOperateAlbumAttributeReqBody reqBody;
    reqBody.albumId = to_string(albumId);
    reqBody.attr = ANALYSIS_ALBUM_ATTR_EXTRA_INFO;
    reqBody.type = ANALYSIS_ALBUM_OP_UPDATE;
    reqBody.values = { "extra_info_a" };
    reqBody.albumType = static_cast<int32_t>(PhotoAlbumType::SMART);
    reqBody.albumSubType = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);

    SelfTokenGuard tokenGuard;
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("OperateAttributeDbPermissionDenied",
        { PERM_WRITE_IMAGEVIDEO }, tokenId);
    ASSERT_NE(tokenId, 0);

    EXPECT_EQ(ServiceOperateAlbumAttribute(reqBody), JS_INNER_FAIL);
    MEDIA_INFO_LOG("End ChangeRequestOperateAlbumAttribute_DbPermissionDenied_ExtraInfo_001");
}
}  // namespace OHOS::Media
