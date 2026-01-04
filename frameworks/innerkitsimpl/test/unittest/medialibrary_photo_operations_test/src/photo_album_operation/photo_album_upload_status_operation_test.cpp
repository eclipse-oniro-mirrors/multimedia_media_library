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

#define MLOG_TAG "PhotoAlbumUploadStatusOperationTest"

#include "photo_album_upload_status_operation_test.h"

#include "medialibrary_mock_tocken.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_unistore_manager.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "photo_album_upload_status_operation.h"
#include "settings_data_manager.h"

using namespace testing::ext;

namespace OHOS::Media {
using namespace OHOS::NativeRdb;
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static constexpr int32_t SLEEP_THREE_SECONDS = 3;
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

static int32_t ClearTable(const string &table)
{
    NativeRdb::RdbPredicates predicates(table);
    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    MEDIA_INFO_LOG("clear table: %{public}s, rows: %{public}d, err: %{public}d", table.c_str(), rows, err);
    EXPECT_EQ(err, E_OK);
    return E_OK;
}

static int32_t InsertPhotoAlbum(const string &albumName, const int32_t albumType, const int32_t uploadStatus)
{
    EXPECT_NE((g_rdbStore == nullptr), true);

    int64_t albumId = -1;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutString(PhotoAlbumColumns::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbumColumns::UPLOAD_STATUS, uploadStatus);
    int32_t ret = g_rdbStore->Insert(albumId, PhotoAlbumColumns::TABLE, values);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhotoAlbum albumId is %{public}s", to_string(albumId).c_str());
    return E_OK;
}

void PhotoAlbumUploadStatusOperationTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE((g_rdbStore == nullptr), true);

    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms;
    perms.push_back("ohos.permission.MANAGE_SETTINGS");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
}

void PhotoAlbumUploadStatusOperationTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearTable(PhotoAlbumColumns::TABLE);
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_THREE_SECONDS));
}

void PhotoAlbumUploadStatusOperationTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    ClearTable(PhotoAlbumColumns::TABLE);
}

void PhotoAlbumUploadStatusOperationTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(PhotoAlbumUploadStatusOperationTest, album_upload_status_operation_test_001, TestSize.Level0)
{
    bool result = PhotoAlbumUploadStatusOperation::IsAllAlbumUploadOnInDb();
    EXPECT_FALSE(result);
    InsertPhotoAlbum("album1", PhotoAlbumType::USER, 1);
    result = PhotoAlbumUploadStatusOperation::IsAllAlbumUploadOnInDb();
    EXPECT_TRUE(result);
    InsertPhotoAlbum("album1", PhotoAlbumType::SOURCE, 0);
    result = PhotoAlbumUploadStatusOperation::IsAllAlbumUploadOnInDb();
    EXPECT_FALSE(result);
    int32_t status = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatus();
    EXPECT_GE(status, 0);
}

HWTEST_F(PhotoAlbumUploadStatusOperationTest, album_upload_status_operation_test_002, TestSize.Level0)
{
    int32_t result = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath(PhotoAlbumColumns::LPATH_CAMERA);
    EXPECT_EQ(result, 1);
    result = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath(PhotoAlbumColumns::LPATH_SCREENSHOT);
    EXPECT_EQ(result, 1);
    result = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath(PhotoAlbumColumns::LPATH_SCREENRECORD);
    EXPECT_EQ(result, 1);
    InsertPhotoAlbum("album1", PhotoAlbumType::USER, 1);
    result = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath("/Pictures/Users/album1");
    EXPECT_GE(result, 0);
    SettingsDataManager::UpdateOrInsertAllPhotosAlbumUpload();
    result = PhotoAlbumUploadStatusOperation::GetAlbumUploadStatusWithLpath("/Pictures/Users/album1");
    EXPECT_EQ(result, 0);
}
}  // namespace OHOS::Media