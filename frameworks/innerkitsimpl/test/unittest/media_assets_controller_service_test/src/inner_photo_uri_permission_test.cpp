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

#include "inner_photo_uri_permission_test.h"

#include <string>
#include <vector>

#define private public
#define protected public
#include "media_assets_controller_service.h"
#undef private
#undef protected

#include "grant_photo_uri_permission_inner_vo.h"
#include "check_photo_uri_permission_inner_vo.h"
#include "cancel_photo_uri_permission_inner_vo.h"
#include "get_uris_by_old_uris_inner_vo.h"
#include "create_asset_vo.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "ipc_skeleton.h"

#include "user_define_ipc_client.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_library_extend_manager.h"
#include "media_library_tab_old_photos_client.h"

#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "result_set_utils.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_old_photos_column.h"
#include "media_column.h"
#include "media_audio_column.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using OHOS::DataShare::DataSharePredicates;
using ServiceCall = std::function<void(MessageParcel &data, MessageParcel &reply)>;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_SECONDS = 1;

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

void InnerPhotoUriPermissionTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryPhotoOperationsTest failed, can not get g_rdbStore");
        exit(1);
    }
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    ClearTable(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    ClearTable(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    MEDIA_INFO_LOG("SetUpTestCase");
}

void InnerPhotoUriPermissionTest::TearDownTestCase(void)
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    ClearTable(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    ClearTable(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    MEDIA_INFO_LOG("TearDownTestCase");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
}

void InnerPhotoUriPermissionTest::SetUp()
{
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    ClearTable(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    ClearTable(AppUriSensitiveColumn::APP_URI_SENSITIVE_TABLE);
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS));
    MEDIA_INFO_LOG("SetUp");
}

void InnerPhotoUriPermissionTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

std::string InnerServiceCreateAsset(CreateAssetReqBody &reqBody, ServiceCall call)
{
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return "";
    }

    MessageParcel reply;
    call(data, reply);

    IPC::MediaRespVo<CreateAssetRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return "";
    }

    int32_t errCode = respVo.GetErrCode();
    if (errCode != 0) {
        MEDIA_ERR_LOG("respVo.GetErrCode: %{public}d", errCode);
        return "";
    }

    return respVo.GetBody().outUri;
}

std::string InnerCreateAsset(const std::string &displayName,
    int32_t photoSubtype, const std::string &cameraShotKey = "")
{
    CreateAssetReqBody reqBody;
    reqBody.mediaType = MEDIA_TYPE_IMAGE;
    reqBody.photoSubtype = photoSubtype;
    reqBody.displayName = displayName;
    reqBody.cameraShotKey = cameraShotKey;

    ServiceCall call = [](MessageParcel &data, MessageParcel &reply) {
        auto service = make_shared<MediaAssetsControllerService>();
        service->SystemCreateAsset(data, reply);
    };
    return InnerServiceCreateAsset(reqBody, call);
}

std::unordered_map<std::string, std::string> GetUrisByOldUrisInner(std::vector<std::string> &uris,
    TabOldPhotosClient &tabOldPhotosClient)
{
    std::unordered_map<std::string, std::string> resultMap;
    GetUrisByOldUrisInnerReqBody reqBody;
    reqBody.uris = uris;
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "file_id");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "data");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_file_id");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_data");
    reqBody.columns.push_back(PhotoColumn::PHOTOS_TABLE + "." + "display_name");
    
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return resultMap;
    }
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->GetUrisByOldUrisInner(data, reply);

    IPC::MediaRespVo<GetUrisByOldUrisInnerRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return resultMap;
    }
    MEDIA_INFO_LOG("Grant ErrCode:%{public}d", respVo.GetErrCode());
    auto fileIds_size = respVo.GetBody().fileIds.size();
    auto datas_size = respVo.GetBody().datas.size();
    auto displayNames_size = respVo.GetBody().displayNames.size();
    auto oldFileIds_size = respVo.GetBody().oldFileIds.size();
    auto oldDatas_size = respVo.GetBody().oldDatas.size();
    bool isValid = true;
    isValid &= fileIds_size == datas_size;
    isValid &= datas_size == displayNames_size;
    isValid &= displayNames_size == oldFileIds_size;
    isValid &= oldFileIds_size == oldDatas_size;
    CHECK_AND_RETURN_RET_LOG(isValid, resultMap, "GetUrisByOldUrisInner Failed");
    
    std::vector<std::vector<int32_t>> file_and_outFile_Ids{
        respVo.GetBody().fileIds,
        respVo.GetBody().oldFileIds
    };
    std::vector<std::vector<std::string>> stringParams{
        respVo.GetBody().datas,
        respVo.GetBody().displayNames,
        respVo.GetBody().oldDatas
    };
    return tabOldPhotosClient.UrisByOldUrisTest(uris, file_and_outFile_Ids, stringParams);
}

static std::string CreatePhotoAsset(string displayName)
{
    return InnerCreateAsset(displayName,
        static_cast<int32_t>(PhotoSubType::DEFAULT));
}

int32_t GrantUrisPermissionInner(std::vector<std::string>& fileIds,
    std::vector<int32_t>& permissionTypes, std::vector<int32_t>& uriTypes, int32_t hideSensitiveType)
{
    GrantUrisPermissionInnerReqBody reqBody;
    reqBody.tokenId = IPCSkeleton::GetCallingTokenID();
    reqBody.srcTokenId = IPCSkeleton::GetCallingTokenID();
    reqBody.fileIds = fileIds;
    reqBody.permissionTypes = permissionTypes;
    reqBody.hideSensitiveType = hideSensitiveType;
    reqBody.uriTypes = uriTypes;

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->GrantPhotoUriPermissionInner(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("Grant ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}


int32_t CheckUrisPermissionInner(std::vector<std::string>& fileIds,
    std::vector<std::string>& outfileIds,
    std::vector<int32_t>& outpermissionTypes)
{
    CheckUriPermissionInnerReqBody reqBody;
    reqBody.targetTokenId = IPCSkeleton::GetCallingTokenID();
    reqBody.uriType = to_string(static_cast<int32_t>(TableType::TYPE_PHOTOS));
    reqBody.fileIds = fileIds;
    reqBody.columns.emplace_back(AppUriPermissionColumn::FILE_ID);
    reqBody.columns.emplace_back(AppUriPermissionColumn::PERMISSION_TYPE);

    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }

    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->CheckUriPermissionInner(data, reply);

    IPC::MediaRespVo<CheckUriPermissionInnerRespBody> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    for (const auto& tmp : respVo.GetBody().fileIds) {
        outfileIds.push_back(tmp);
    }
    for (const auto& tmp : respVo.GetBody().permissionTypes) {
        outpermissionTypes.push_back(tmp);
    }
    
    MEDIA_INFO_LOG("Grant ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}

int32_t CancelUrisPermissionInner(std::vector<std::string>& fileIds,
    std::vector<int32_t> &uriTypes,
    std::vector<std::vector<std::string>> &permissionTypes)
{
    CancelUriPermissionInnerReqBody reqBody;
    reqBody.targetTokenId = IPCSkeleton::GetCallingTokenID();
    reqBody.srcTokenId = IPCSkeleton::GetCallingTokenID();
    reqBody.fileIds = fileIds;
    reqBody.uriTypes = uriTypes;
    reqBody.permissionTypes = permissionTypes;
    
    MessageParcel data;
    if (reqBody.Marshalling(data) != true) {
        MEDIA_ERR_LOG("reqBody.Marshalling failed");
        return -1;
    }
    MessageParcel reply;
    auto service = make_shared<MediaAssetsControllerService>();
    service->CancelPhotoUriPermissionInner(data, reply);

    IPC::MediaRespVo<IPC::MediaEmptyObjVo> respVo;
    if (respVo.Unmarshalling(reply) != true) {
        MEDIA_ERR_LOG("respVo.Unmarshalling failed");
        return -1;
    }
    MEDIA_INFO_LOG("Grant ErrCode:%{public}d", respVo.GetErrCode());
    return respVo.GetErrCode();
}


static map<string, TableType> tableMap = {
    { MEDIALIBRARY_TYPE_IMAGE_URI, TableType::TYPE_PHOTOS },
    { MEDIALIBRARY_TYPE_VIDEO_URI, TableType::TYPE_PHOTOS },
    { MEDIALIBRARY_TYPE_AUDIO_URI, TableType::TYPE_AUDIOS },
    { PhotoColumn::PHOTO_TYPE_URI, TableType::TYPE_PHOTOS },
    { AudioColumn::AUDIO_TYPE_URI, TableType::TYPE_AUDIOS }
};

int32_t GetInnerParames(std::vector<string> &uris, std::vector<PhotoPermissionType> photoPermissionTypes,
    std::vector<string> &fileIds, std::vector<int32_t> &uriTypes, std::vector<int32_t> &permissionTypes)
{
    for (size_t i = 0; i < uris.size(); i++) {
        auto uri = uris.at(i);
        auto photoPermissionType = photoPermissionTypes.at(i);
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }
        CHECK_AND_RETURN_RET_LOG(tableType != -1, E_ERR, "Uri invalid error, uri:%{private}s", uri.c_str());
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        fileIds.emplace_back(fileId);
        uriTypes.emplace_back(tableType);
        if (photoPermissionType == PhotoPermissionType::GRANT_PERSIST_READWRITE_IMAGEVIDEO) {
            permissionTypes.emplace_back(static_cast<int32_t>(PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO));
            fileIds.emplace_back(fileId);
            uriTypes.emplace_back(tableType);
            permissionTypes.emplace_back(static_cast<int32_t>(PhotoPermissionType::PERSIST_READ_IMAGEVIDEO));
            continue;
        }
        permissionTypes.emplace_back(static_cast<int32_t>(photoPermissionType));
    }
    return 0;
}

static void InnerQueryUriPermissionTable()
{
    std::vector<std::string> columns;
    RdbPredicates rdbPredicates(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get fileId");
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string fileId = GetStringVal(AppUriPermissionColumn::FILE_ID, resultSet);
        int32_t permissionType = GetInt32Val(AppUriPermissionColumn::PERMISSION_TYPE, resultSet);
    }
    return;
}

HWTEST_F(InnerPhotoUriPermissionTest, GrantUrisPermissionInner_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GrantUrisPermissionInner_Test_001 Begin");

    vector<string> uris;
    for (int i = 0; i < 5; i++) {
        std::string file_name = "file://media/Photo/" + std::to_string(486 + i) +"/IMG_1750165421_032/test.jpg";
        uris.push_back(file_name);
    }

    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO,
        PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_WRITE_IMAGEVIDEO,
    };

    std::vector<std::string> fileIds;
    std::vector<int32_t> uriTypes;
    std::vector<int32_t> innerPermissionTypes;
    auto ret = GetInnerParames(uris, permissionTypes, fileIds, uriTypes, innerPermissionTypes);
    ASSERT_EQ(ret, 0);

    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;

    ret = GrantUrisPermissionInner(fileIds, innerPermissionTypes, uriTypes, static_cast<int32_t>(SensitiveType));
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.number    : MediaLibraryManager_GrantPhotoUriPermission_test_002
 * @tc.name      : 仅授权临时读权限
 */
HWTEST_F(InnerPhotoUriPermissionTest, GrantUrisPermissionInner_Test_002, TestSize.Level0)
{
    MEDIA_INFO_LOG("GrantUrisPermissionInner_Test_002 Begin");

    vector<string> uris;
    for (int i = 0; i < 6; i++) {
        auto uri = CreatePhotoAsset("test.jpg");
        MEDIA_INFO_LOG("GrantUrisPermissionInner_Test_001 uri %{public}s", uri.c_str());
        uris.push_back(uri);
    }
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_SECONDS * 2));
    vector<PhotoPermissionType> permissionTypes{
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
        PhotoPermissionType::PERSIST_READ_IMAGEVIDEO,
    };

    std::vector<std::string> fileIds;
    std::vector<int32_t> uriTypes;
    std::vector<int32_t> innerPermissionTypes;
    auto ret = GetInnerParames(uris, permissionTypes, fileIds, uriTypes, innerPermissionTypes);
    ASSERT_EQ(ret, 0);
    
    std::vector<std::vector<std::string>> cancelPermissionTypes{
        {to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO))},
        {to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO))},
        {to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO))},
        {to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO))},
        {to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO))},
        {to_string(static_cast<uint32_t>(PhotoPermissionType::PERSIST_READWRITE_IMAGEVIDEO))},
    };
     
    ret = CancelUrisPermissionInner(fileIds, uriTypes, cancelPermissionTypes);
    ASSERT_EQ(ret, 0);

    auto SensitiveType = HideSensitiveType::GEOGRAPHIC_LOCATION_DESENSITIZE;
    ret = GrantUrisPermissionInner(fileIds, innerPermissionTypes, uriTypes, static_cast<int32_t>(SensitiveType));
    ASSERT_EQ(ret, 0);

    std::vector<std::string> outfileIds;
    std::vector<int32_t> outpermissionTypes;

    ret = CheckUrisPermissionInner(fileIds, outfileIds, outpermissionTypes);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_EQ(innerPermissionTypes, outpermissionTypes);
}

HWTEST_F(InnerPhotoUriPermissionTest, GetUrisByOldUris_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("GetUrisByOldUris_001 Begin");

    std::vector<std::string> uris;
    // test case 1 empty uris will return emty map

    TabOldPhotosClient tabOldPhotosClient = TabOldPhotosClient(manager);
    auto ret = GetUrisByOldUrisInner(uris, tabOldPhotosClient);
    EXPECT_EQ(ret.empty(), true);

    // test case 2 normal uris
    uris.emplace_back(UFM_CREATE_PHOTO);
    uris.emplace_back(UFM_CREATE_AUDIO);
    uris.emplace_back(UFM_CREATE_PHOTO_ALBUM);
    ret = GetUrisByOldUrisInner(uris, tabOldPhotosClient);
    EXPECT_EQ(ret.empty(), true);

    uris.clear();
    for (int32_t i = 0; i <= 5; i++) {
        uris.emplace_back("testuri");
    }
    ret = GetUrisByOldUrisInner(uris, tabOldPhotosClient);
    EXPECT_EQ(ret.empty(), true);

    // invalid uris
    uris.clear();
    uris.emplace_back("you_look_only_once");
    uris.emplace_back("//media/we_shall_never_surrender/");
    uris.emplace_back("//media/we_shall_never/_surrender");
    uris.emplace_back("/storage/emulated/love_and_peace/");
    uris.emplace_back("12345");
    uris.emplace_back("");
    ret = GetUrisByOldUrisInner(uris, tabOldPhotosClient);
    EXPECT_EQ(ret.empty(), false);
}
}  // namespace OHOS::Media