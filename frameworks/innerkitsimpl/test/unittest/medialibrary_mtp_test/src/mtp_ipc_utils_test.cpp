/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpIpcUtilsTest"

#include "mtp_ipc_utils_test.h"

#include <thread>
#include "base_data_uri.h"
#include "iservice_registry.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_mock_tocken.h"
#include "mtp_ipc_utils.h"
#include "result_set_utils.h"
#include "photo_album_column.h"
#include "ptp_medialibrary_manager_uri.h"

namespace OHOS {
namespace Media {
namespace {
using ::testing::ext::TestSize;
constexpr int32_t E_SUCCESS = 0;
constexpr int STORAGE_MANAGER_UID_TEST = 5003;
constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const std::string NEW_ASSET = "test.jpg";
const std::string CHANGE_ASSET = "change.jpg";
const std::string CHANGE_ASSET_TITLE = "change";
const std::string NEW_ALBUM = "testAlbum";
const std::string CHANGE_ALBUM = "changeAlbum";
uint64_t g_shellToken = 0;
MediaLibraryMockHapToken* g_mockTokenIpc = nullptr;
std::shared_ptr<DataShare::DataShareHelper> g_dataShareHelper = nullptr;
} // namespace

void DeleteAssetIfExists(const std::string &displayName)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    std::vector<std::string> fetchColumns = { MediaColumn::MEDIA_ID };
    auto resultSet = MtpIpcUtils::GetAssets(g_dataShareHelper, predicates, fetchColumns);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == E_SUCCESS) {
        std::string assetId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);
        std::vector<std::string> photoIds = { assetId };
        MtpIpcUtils::DeletePhotos(g_dataShareHelper, photoIds);
    }
}

std::string GetAlbumIdByName(const std::string &albumName)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    std::vector<std::string> fetchColumns = { PhotoAlbumColumns::ALBUM_ID };
    auto resultSet = MtpIpcUtils::GetAlbums(g_dataShareHelper, predicates, fetchColumns);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_SUCCESS) {
        return "";
    }

    return GetStringVal(PhotoAlbumColumns::ALBUM_ID, resultSet);
}

void DeleteAlbumIfExists(const std::string &albumName)
{
    std::string albumId = GetAlbumIdByName(albumName);
    if (!albumId.empty()) {
        std::vector<std::string> deleteAlbumIds = { albumId };
        MtpIpcUtils::DeleteAlbums(g_dataShareHelper, deleteAlbumIds);
    }
}

void CreateAlbum(const std::string &albumName)
{
    int32_t albumId = 0;
    MtpIpcUtils::CreateAlbum(g_dataShareHelper, albumName, albumId);
}

void CreateAssetIfNotExits(const std::string &displayName)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, displayName);
    std::vector<std::string> fetchColumns = { MediaColumn::MEDIA_ID };
    auto resultSet = MtpIpcUtils::GetAssets(g_dataShareHelper, predicates, fetchColumns);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet != nullptr && resultSet->GoToFirstRow() != E_SUCCESS) {
        int32_t assetId = 0;
        MtpIpcUtils::CreateAsset(g_dataShareHelper, displayName, MediaType::MEDIA_TYPE_IMAGE, assetId);
    }
}

void CreateAlbumIfNotExits(const std::string &albumName)
{
    std::string albumId = GetAlbumIdByName(albumName);
    if (albumId.empty()) {
        CreateAlbum(albumName);
    }
}

void MtpIpcUtilsTest::SetUpTestCase(void)
{
    // mock hap token
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    std::vector<std::string> perms;
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO");
    g_mockTokenIpc = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);

    auto token = saManager->GetSystemAbility(STORAGE_MANAGER_UID_TEST);
    ASSERT_NE(token, nullptr);

    g_dataShareHelper = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    ASSERT_NE(g_dataShareHelper, nullptr);
}

void MtpIpcUtilsTest::TearDownTestCase(void)
{
    DeleteAssetIfExists(NEW_ASSET);
    DeleteAssetIfExists(CHANGE_ASSET);
    DeleteAlbumIfExists(NEW_ALBUM);
    DeleteAlbumIfExists(CHANGE_ALBUM);
    // recovery shell token id
    if (g_mockTokenIpc != nullptr) {
        delete g_mockTokenIpc;
        g_mockTokenIpc = nullptr;
    }

    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
    g_dataShareHelper = nullptr;
}

void MtpIpcUtilsTest::SetUp(void) {}
void MtpIpcUtilsTest::TearDown(void) {}

HWTEST_F(MtpIpcUtilsTest, CreateAsset_Test_001, TestSize.Level1)
{
    MEDIA_DEBUG_LOG("MtpIpcUtilsTest CreateAsset_Test_001 start");
    DeleteAssetIfExists(NEW_ASSET);
    MediaType mediaType = MediaType::MEDIA_TYPE_IMAGE;
    int32_t assetId = 0;
    auto ret = MtpIpcUtils::CreateAsset(g_dataShareHelper, NEW_ASSET, mediaType, assetId);
    EXPECT_EQ(ret, E_SUCCESS);
    EXPECT_GT(assetId, 0);
}

HWTEST_F(MtpIpcUtilsTest, ChangeAssetTitle_Test_001, TestSize.Level1)
{
    MEDIA_DEBUG_LOG("MtpIpcUtilsTest ChangeAssetTitle_Test_001 start");
    CreateAssetIfNotExits(NEW_ASSET);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, NEW_ASSET);
    std::vector<std::string> fetchColumns = { MediaColumn::MEDIA_ID };
    auto resultSet = MtpIpcUtils::GetAssets(g_dataShareHelper, predicates, fetchColumns);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_SUCCESS);

    int32_t assetId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    auto ret = MtpIpcUtils::ChangeAssetTitle(g_dataShareHelper, assetId, CHANGE_ASSET_TITLE);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MtpIpcUtilsTest, CreateAlbum_Test_001, TestSize.Level1)
{
    MEDIA_DEBUG_LOG("MtpIpcUtilsTest CreateAlbum_Test_001 start");
    DeleteAlbumIfExists(NEW_ALBUM);
    int32_t albumId = 0;
    auto ret = MtpIpcUtils::CreateAlbum(g_dataShareHelper, NEW_ALBUM, albumId);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MtpIpcUtilsTest, ChangeAlbumName_Test_001, TestSize.Level1)
{
    MEDIA_DEBUG_LOG("MtpIpcUtilsTest ChangeAlbumName_Test_001 start");
    DeleteAlbumIfExists(CHANGE_ALBUM);
    CreateAlbumIfNotExits(NEW_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, NEW_ALBUM);
    const std::vector<std::string> fetchColumns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_TYPE,
        PhotoAlbumColumns::ALBUM_SUBTYPE
    };
    auto resultSet = MtpIpcUtils::GetAlbums(g_dataShareHelper, predicates, fetchColumns);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_SUCCESS);

    std::string albumId = GetStringVal(PhotoAlbumColumns::ALBUM_ID, resultSet);
    int32_t albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    int32_t albumSubType = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
    auto ret = MtpIpcUtils::ChangeAlbumName(g_dataShareHelper, albumId, CHANGE_ALBUM, albumType, albumSubType);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MtpIpcUtilsTest, DeletePhotos_Test_001, TestSize.Level1)
{
    MEDIA_DEBUG_LOG("MtpIpcUtilsTest DeletePhotos_Test_001 start");
    DeleteAssetIfExists(CHANGE_ASSET);
    CreateAssetIfNotExits(CHANGE_ASSET);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_NAME, CHANGE_ASSET);
    std::vector<std::string> fetchColumns = { MediaColumn::MEDIA_ID };
    auto resultSet = MtpIpcUtils::GetAssets(g_dataShareHelper, predicates, fetchColumns);
    EXPECT_NE(resultSet, nullptr);
    EXPECT_EQ(resultSet->GoToFirstRow(), E_SUCCESS);

    std::string assetId = GetStringVal(MediaColumn::MEDIA_ID, resultSet);

    std::vector<std::string> photoIds = { assetId };
    auto ret = MtpIpcUtils::DeletePhotos(g_dataShareHelper, photoIds);
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MtpIpcUtilsTest, DeleteAlbums_Test_001, TestSize.Level1)
{
    MEDIA_DEBUG_LOG("MtpIpcUtilsTest DeleteAlbums_Test_001 start");
    CreateAlbumIfNotExits(CHANGE_ALBUM);
    std::string albumId = GetAlbumIdByName(CHANGE_ALBUM);
    vector<string> deleteAlbumIds = { albumId };
    auto ret = MtpIpcUtils::DeleteAlbums(g_dataShareHelper, deleteAlbumIds);
    EXPECT_EQ(ret, E_SUCCESS);
}
} // namespace Media
} // namespace OHOS