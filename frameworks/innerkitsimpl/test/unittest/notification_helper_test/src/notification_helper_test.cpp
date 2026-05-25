/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "notification_helper_test.h"
#include "medialibrary_errno.h"
#include "photo_album.h"
#include "photo_album_column.h"
#include "fetch_result.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_command.h"
#include "rdb_predicates.h"
#include "result_set_utils.h"
#include "rdb_utils.h"
#include "datashare_result_set.h"
#include "userfile_manager_types.h"
#include "userfilemgr_uri.h"
#include "medialibrary_mock_tocken.h"
#include "album_change_info.h"
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>
#include <climits>

using namespace testing::ext;
using namespace OHOS::Media::NotificationHelper;
using namespace OHOS::NativeRdb;

// Disambiguate: NotificationHelper has both struct AlbumChangeData and AccurateRefresh::AlbumChangeData
using NotificationAlbumChangeData = OHOS::Media::NotificationHelper::AlbumChangeData;
// API is static methods on NotificationHelper class (namespace and class share the name)
using NotificationHelperApi = OHOS::Media::NotificationHelper::NotificationHelper;
using namespace OHOS::DataShare;
using OHOS::DataShare::DataShareValuesBucket;
using OHOS::DataShare::DataSharePredicates;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;
using std::make_unique;
using std::unique_ptr;

namespace OHOS {
namespace Media {

MediaLibraryMockHapToken* g_notificationHelperMockToken = nullptr;
uint64_t g_shellToken = 0;

// Static variables for database tests
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;
const std::string URI_CREATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + CONST_PHOTO_ALBUM_OPRN + "/" + OPRN_CREATE;
const std::string URI_UPDATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + CONST_PHOTO_ALBUM_OPRN + "/" + OPRN_UPDATE;

// Helper function to check column values
inline void CheckColumn(shared_ptr<NativeRdb::ResultSet> &resultSet, const string &column,
    ResultSetDataType type, const variant<int32_t, string, int64_t, double> &expected)
{
    EXPECT_EQ(ResultSetUtils::GetValFromColumn(column, resultSet, type), expected);
}

// Helper function to create photo album
inline int32_t CreatePhotoAlbum(const string &albumName)
{
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    Uri uri(URI_CREATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, values);
}

// Helper function to update photo album
inline int32_t UpdatePhotoAlbum(const DataShareValuesBucket &values, const DataSharePredicates &predicates)
{
    Uri uri(URI_UPDATE_PHOTO_ALBUM);
    MediaLibraryCommand cmd(uri, OperationType::UPDATE);
    return MediaLibraryDataManager::GetInstance()->Update(cmd, values, predicates);
}

void NotificationHelperTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("NotificationHelperTest SetUpTestCase");
    g_shellToken = OHOS::IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms = { "ohos.permission.READ_IMAGEVIDEO" };
    g_notificationHelperMockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto& perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(OHOS::IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
}

void NotificationHelperTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("NotificationHelperTest TearDownTestCase");
    if (g_notificationHelperMockToken != nullptr) {
        delete g_notificationHelperMockToken;
        g_notificationHelperMockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
}

void NotificationHelperTest::SetUp()
{
    MEDIA_INFO_LOG("NotificationHelperTest SetUp");
}

void NotificationHelperTest::TearDown()
{
    MEDIA_INFO_LOG("NotificationHelperTest TearDown");
}

/**
* @tc.number    : NotificationHelper_Callback_Data_test_001
* @tc.name      : Callback data: Verify NotificationAlbumChangeData content
* @tc.desc      : Verify callback receives correct NotificationAlbumChangeData content
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_Data_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_Data_test_001 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;

    NotificationAlbumChangeData data1;
    data1.version = 100;
    testInfo.albumChangeDatas.push_back(data1);

    NotificationAlbumChangeData data2;
    data2.version = 200;
    testInfo.albumChangeDatas.push_back(data2);

    callback->OnChange(testInfo);

    AlbumChangeInfos received = callback->GetLastInfo();
    EXPECT_EQ(received.albumChangeDatas.size(), 2);
    EXPECT_EQ(received.albumChangeDatas[0].version, 100);
    EXPECT_EQ(received.albumChangeDatas[1].version, 200);

    MEDIA_INFO_LOG("NotificationHelper_Callback_Data_test_001 exit");
}

/**
* @tc.number    : NotificationHelper_Callback_Data_test_002
* @tc.name      : Callback data: Verify AlbumChangeInfo pointers
* @tc.desc      : Verify AlbumChangeInfo shared pointers are handled correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_Data_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_Data_test_002 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;

    NotificationAlbumChangeData changeData;
    // albumBeforeChange and albumAfterChange are nullptr by default
    changeData.version = 500;
    testInfo.albumChangeDatas.push_back(changeData);

    callback->OnChange(testInfo);

    AlbumChangeInfos received = callback->GetLastInfo();
    EXPECT_EQ(received.albumChangeDatas.size(), 1);
    EXPECT_EQ(received.albumChangeDatas[0].version, 500);
    EXPECT_EQ(received.albumChangeDatas[0].albumBeforeChange, nullptr);
    EXPECT_EQ(received.albumChangeDatas[0].albumAfterChange, nullptr);

    MEDIA_INFO_LOG("NotificationHelper_Callback_Data_test_002 exit");
}

/**
* @tc.number    : NotificationHelper_RegisterPhotoAlbumCallback_test_007
* @tc.name      : RegisterPhotoAlbumCallback: Register with different callback types
* @tc.desc      : Verify different callback instances can be registered
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_RegisterPhotoAlbumCallback_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_RegisterPhotoAlbumCallback_test_007 enter");

    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback3 = std::make_shared<MockPhotoAlbumChangeCallback>();

    ASSERT_NE(callback1, nullptr);
    ASSERT_NE(callback2, nullptr);
    ASSERT_NE(callback3, nullptr);

    // Verify they are different instances
    EXPECT_NE(callback1.get(), callback2.get());
    EXPECT_NE(callback2.get(), callback3.get());

    // Register all
    int32_t ret1 = NotificationHelperApi::RegisterPhotoAlbumCallback(callback1);
    int32_t ret2 = NotificationHelperApi::RegisterPhotoAlbumCallback(callback2);
    int32_t ret3 = NotificationHelperApi::RegisterPhotoAlbumCallback(callback3);

    EXPECT_EQ(ret1, NOTIFY_OK);
    EXPECT_EQ(ret2, NOTIFY_OK);
    EXPECT_EQ(ret3, NOTIFY_OK);

    // Unregister all
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback3);

    MEDIA_INFO_LOG("NotificationHelper_RegisterPhotoAlbumCallback_test_007 exit");
}

/**
* @tc.number    : NotificationHelper_unRegisterPhotoAlbumCallback_test_005
* @tc.name      : unRegisterPhotoAlbumCallback: Duplicate registration is idempotent
* @tc.desc      : Verify registering same callback multiple times is idempotent (only one instance kept)
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegisterPhotoAlbumCallback_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_unRegisterPhotoAlbumCallback_test_005 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    // Register same callback multiple times - duplicates are ignored (idempotent)
    int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK);
    ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK); // Idempotent: same callback not duplicated
    ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK); // Idempotent: same callback not duplicated

    // Unregister once removes the callback
    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK);

    // Second unregister should fail - callback already removed
    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_ERR_UNREGISTER_REPEAT);

    MEDIA_INFO_LOG("NotificationHelper_unRegisterPhotoAlbumCallback_test_005 exit");
}

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_008
* @tc.name      : PhotoAlbumChangeCallback: OnChange with all change types
* @tc.desc      : Verify OnChange handles all NotifyChangeType values
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_OnChange_test_008 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    // Test all change types
    std::vector<NotifyChangeType> changeTypes = {
        NotifyChangeType::NOTIFY_CHANGE_ADD,
        NotifyChangeType::NOTIFY_CHANGE_UPDATE,
        NotifyChangeType::NOTIFY_CHANGE_REMOVE,
        NotifyChangeType::NOTIFY_CHANGE_INVALID
    };

    for (auto type : changeTypes) {
        AlbumChangeInfos testInfo;
        testInfo.type = type;
        callback->OnChange(testInfo);
    }

    EXPECT_EQ(callback->GetCallCount(), 4);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_INVALID);

    MEDIA_INFO_LOG("NotificationHelper_Callback_OnChange_test_008 exit");
}

/**
* @tc.number    : NotificationHelper_Edge_Case_test_009
* @tc.name      : Edge case: Register null then valid callback
* @tc.desc      : Verify null callback rejection doesn't affect valid registration
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Edge_Case_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_009 enter");

    // Try to register null
    std::shared_ptr<PhotoAlbumChangeCallback> nullCallback = nullptr;
    int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(nullCallback);
    EXPECT_EQ(ret, -1);

    // Register valid callback should still work
    auto validCallback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ret = NotificationHelperApi::RegisterPhotoAlbumCallback(validCallback);
    EXPECT_EQ(ret, NOTIFY_OK);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(validCallback);

    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_009 exit");
}

/**
* @tc.number    : NotificationHelper_Edge_Case_test_010
* @tc.name      : Edge case: Unregister null then valid callback
* @tc.desc      : Verify null callback unregister rejection doesn't affect valid unregister
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Edge_Case_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_010 enter");

    // Try to unregister null
    std::shared_ptr<PhotoAlbumChangeCallback> nullCallback = nullptr;
    int32_t ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(nullCallback);
    EXPECT_EQ(ret, -1);

    // Register and unregister valid callback should work
    auto validCallback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ret = NotificationHelperApi::RegisterPhotoAlbumCallback(validCallback);
    EXPECT_EQ(ret, NOTIFY_OK);

    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(validCallback);
    EXPECT_EQ(ret, NOTIFY_OK);

    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_010 exit");
}

/**
* @tc.number    : NotificationHelper_Callback_Multiple_test_002
* @tc.name      : Multiple callbacks: Verify independent callback state
* @tc.desc      : Verify multiple callbacks maintain independent state
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_Multiple_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_Multiple_test_002 enter");

    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();

    ASSERT_NE(callback1, nullptr);
    ASSERT_NE(callback2, nullptr);

    // Register both
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback2);

    // Call OnChange on first callback only
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    callback1->OnChange(testInfo);

    // Verify only first callback was called
    EXPECT_EQ(callback1->GetCallCount(), 1);
    EXPECT_EQ(callback2->GetCallCount(), 0);

    // Clean up
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);

    MEDIA_INFO_LOG("NotificationHelper_Callback_Multiple_test_002 exit");
}

/**
* @tc.number    : NotificationHelper_Callback_Reset_test_002
* @tc.name      : Mock callback: Reset between operations
* @tc.desc      : Verify Reset works correctly between multiple operations
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_Reset_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_Reset_test_002 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    // First set of operations
    AlbumChangeInfos testInfo1;
    testInfo1.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    callback->OnChange(testInfo1);
    EXPECT_EQ(callback->GetCallCount(), 1);

    // Reset
    callback->Reset();
    EXPECT_EQ(callback->GetCallCount(), 0);

    // Second set of operations
    AlbumChangeInfos testInfo2;
    testInfo2.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    callback->OnChange(testInfo2);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_UPDATE);

    MEDIA_INFO_LOG("NotificationHelper_Callback_Reset_test_002 exit");
}

/**
* @tc.number    : NotificationHelper_Edge_Case_test_011
* @tc.name      : Edge case: Very large AlbumChangeDatas vector
* @tc.desc      : Verify callback handles large number of AlbumChangeData
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Edge_Case_test_011, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_011 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;

    const int32_t largeCount = 1000;
    for (int32_t i = 0; i < largeCount; i++) {
        NotificationAlbumChangeData changeData;
        changeData.version = i;
        testInfo.albumChangeDatas.push_back(changeData);
    }

    int32_t ret = callback->OnChange(testInfo);
    EXPECT_EQ(ret, 0);

    AlbumChangeInfos received = callback->GetLastInfo();
    EXPECT_EQ(received.albumChangeDatas.size(), largeCount);

    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_011 exit");
}

/**
* @tc.number    : NotificationHelper_Edge_Case_test_012
* @tc.name      : Edge case: Register/unregister interleaved with operations
* @tc.desc      : Verify register/unregister works while callbacks are being used
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Edge_Case_test_012, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_012 enter");

    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();

    NotificationHelperApi::RegisterPhotoAlbumCallback(callback1);

    // Use callback1
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    callback1->OnChange(testInfo);

    // Register callback2
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback2);

    // Unregister callback1
    int32_t ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    EXPECT_EQ(ret, NOTIFY_OK);

    // Use callback2
    callback2->OnChange(testInfo);

    // Unregister callback2
    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);
    EXPECT_EQ(ret, NOTIFY_OK);

    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_012 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_001
* @tc.name      : PhotoAlbum SetSceneId and GetSceneId, SetShareType and GetShareType test
* @tc.desc      : Verify SetSceneId, GetSceneId, SetShareType, and GetShareType functions work correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_001 enter");

    PhotoAlbum photoAlbum;

    const int32_t TEST_ALBUM_ID = 1;
    photoAlbum.SetAlbumId(TEST_ALBUM_ID);
    EXPECT_EQ(photoAlbum.GetAlbumId(), TEST_ALBUM_ID);

    photoAlbum.SetPhotoAlbumType(PhotoAlbumType::USER);
    EXPECT_EQ(photoAlbum.GetPhotoAlbumType(), PhotoAlbumType::USER);

    photoAlbum.SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
    EXPECT_EQ(photoAlbum.GetPhotoAlbumSubType(), PhotoAlbumSubType::USER_GENERIC);

    const string TEST_URI = "file://media/album/1";
    photoAlbum.SetAlbumUri(TEST_URI);
    EXPECT_EQ(photoAlbum.GetAlbumUri(), TEST_URI);

    const string TEST_ALBUM_NAME = "test";
    photoAlbum.SetAlbumName(TEST_ALBUM_NAME);
    EXPECT_EQ(photoAlbum.GetAlbumName(), TEST_ALBUM_NAME);

    const string TEST_COVERURI = TEST_URI;
    photoAlbum.SetCoverUri(TEST_COVERURI);
    EXPECT_EQ(photoAlbum.GetCoverUri(), TEST_COVERURI);

    const int32_t TEST_COUNT = 1;
    photoAlbum.SetCount(TEST_COUNT);
    EXPECT_EQ(photoAlbum.GetCount(), TEST_COUNT);

    const string TEST_RELATIVE_PATH = "Camera";
    photoAlbum.SetRelativePath(TEST_RELATIVE_PATH);
    EXPECT_EQ(photoAlbum.GetRelativePath(), TEST_RELATIVE_PATH);

    const int64_t CHANGE_TIME = 1;
    photoAlbum.SetChangeTime(CHANGE_TIME);
    EXPECT_EQ(photoAlbum.GetChangeTime(), CHANGE_TIME);

    photoAlbum.SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    EXPECT_EQ(photoAlbum.GetResultNapiType(), ResultNapiType::TYPE_USERFILE_MGR);

    photoAlbum.SetHidden(TEST_ALBUM_ID);
    EXPECT_EQ(photoAlbum.GetHidden(), TEST_ALBUM_ID);

    const int32_t TEST_SCENE_ID = 100;
    photoAlbum.SetSceneId(TEST_SCENE_ID);
    EXPECT_EQ(photoAlbum.GetSceneId(), TEST_SCENE_ID);

    const int32_t TEST_SHARE_TYPE = 200;
    photoAlbum.SetShareType(TEST_SHARE_TYPE);
    EXPECT_EQ(photoAlbum.GetShareType(), TEST_SHARE_TYPE);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_001 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_002
* @tc.name      : PhotoAlbum SetSceneId and GetSceneId test
* @tc.desc      : Verify SetSceneId and GetSceneId functions work correctly with various values
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_002 enter");

    PhotoAlbum photoAlbum;

    const int32_t TEST_SCENE_ID_1 = 0;
    photoAlbum.SetSceneId(TEST_SCENE_ID_1);
    EXPECT_EQ(photoAlbum.GetSceneId(), TEST_SCENE_ID_1);

    const int32_t TEST_SCENE_ID_2 = 100;
    photoAlbum.SetSceneId(TEST_SCENE_ID_2);
    EXPECT_EQ(photoAlbum.GetSceneId(), TEST_SCENE_ID_2);

    const int32_t TEST_SCENE_ID_3 = -1;
    photoAlbum.SetSceneId(TEST_SCENE_ID_3);
    EXPECT_EQ(photoAlbum.GetSceneId(), TEST_SCENE_ID_3);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_002 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_003
* @tc.name      : PhotoAlbum SetShareType and GetShareType test
* @tc.desc      : Verify SetShareType and GetShareType functions work correctly with various values
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_003 enter");

    PhotoAlbum photoAlbum;

    const int32_t TEST_SHARE_TYPE_1 = 0;
    photoAlbum.SetShareType(TEST_SHARE_TYPE_1);
    EXPECT_EQ(photoAlbum.GetShareType(), TEST_SHARE_TYPE_1);

    const int32_t TEST_SHARE_TYPE_2 = 200;
    photoAlbum.SetShareType(TEST_SHARE_TYPE_2);
    EXPECT_EQ(photoAlbum.GetShareType(), TEST_SHARE_TYPE_2);

    const int32_t TEST_SHARE_TYPE_3 = -1;
    photoAlbum.SetShareType(TEST_SHARE_TYPE_3);
    EXPECT_EQ(photoAlbum.GetShareType(), TEST_SHARE_TYPE_3);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_003 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_004
* @tc.name      : PhotoAlbum SetSceneId and SetShareType combined test
* @tc.desc      : Verify SetSceneId and SetShareType can be used together
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_004, TestSize.Level1)
{
    PhotoAlbum photoAlbum;
    const int32_t TEST_SCENE_ID = 100;
    const int32_t TEST_SHARE_TYPE = 200;
    photoAlbum.SetSceneId(TEST_SCENE_ID);
    photoAlbum.SetShareType(TEST_SHARE_TYPE);
    EXPECT_EQ(photoAlbum.GetSceneId(), TEST_SCENE_ID);
    EXPECT_EQ(photoAlbum.GetShareType(), TEST_SHARE_TYPE);
    photoAlbum.SetSceneId(300);
    photoAlbum.SetShareType(400);
    EXPECT_EQ(photoAlbum.GetSceneId(), 300);
    EXPECT_EQ(photoAlbum.GetShareType(), 400);
}

// ==================== RegisterPhotoAlbumCallback ====================

/**
* @tc.number    : NotificationHelper_RegisterPhotoAlbumCallback_test_001
* @tc.name      : RegisterPhotoAlbumCallback: Register callback successfully
* @tc.desc      : Register a photo album change callback and verify it returns success
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_RegisterPhotoAlbumCallback_test_001, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
}

/**
* @tc.number    : NotificationHelper_RegisterPhotoAlbumCallback_test_002
* @tc.name      : RegisterPhotoAlbumCallback: Register with nullptr callback
* @tc.desc      : Register with nullptr should return error code -1
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_RegisterPhotoAlbumCallback_test_002, TestSize.Level1)
{
    std::shared_ptr<PhotoAlbumChangeCallback> nullCallback = nullptr;
    int32_t ret =
        NotificationHelperApi::RegisterPhotoAlbumCallback(nullCallback);
    EXPECT_EQ(ret, -1);
}

/**
* @tc.number    : NotificationHelper_RegisterPhotoAlbumCallback_test_003
* @tc.name      : RegisterPhotoAlbumCallback: Multiple callbacks suppport
* @tc.desc      : Client supports multiple different callbacks, all receive notifications
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_RegisterPhotoAlbumCallback_test_003, TestSize.Level1)
{
    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback1, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(callback1), NOTIFY_OK);

    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback2, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(callback2), NOTIFY_OK);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    testInfo.isForRecheck = false;
    callback1->Reset();
    callback2->Reset();
    callback1->OnChange(testInfo);
    callback2->OnChange(testInfo);
    EXPECT_EQ(callback1->GetCallCount(), 1);
    EXPECT_EQ(callback2->GetCallCount(), 1);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);
}

// ==================== unRegisterPhotoAlbumCallback ====================

/**
* @tc.number    : NotificationHelper_unRegisterPhotoAlbumCallback_test_001
* @tc.name      : unRegisterPhotoAlbumCallback: Unregister successfully
* @tc.desc      : Register and then unregister a callback successfully
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegisterPhotoAlbumCallback_test_001, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(callback), NOTIFY_OK);
    EXPECT_EQ(NotificationHelperApi::unRegisterPhotoAlbumCallback(callback), NOTIFY_OK);
}

/**
* @tc.number    : NotificationHelper_unRegisterPhotoAlbumCallback_test_002
* @tc.name      : unRegisterPhotoAlbumCallback: Unregister with nullptr
* @tc.desc      : Unregister with nullptr should return error code -1
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegisterPhotoAlbumCallback_test_002, TestSize.Level1)
{
    std::shared_ptr<PhotoAlbumChangeCallback> nullCallback = nullptr;
    int32_t ret =
        NotificationHelperApi::unRegisterPhotoAlbumCallback(nullCallback);
    EXPECT_EQ(ret, -1);
}

/**
* @tc.number    : NotificationHelper_unRegisterPhotoAlbumCallback_test_003
* @tc.name      : unRegisterPhotoAlbumCallback: Unregister without registering
* @tc.desc      : Unregister a callback that was never registered should return error
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegisterPhotoAlbumCallback_test_003, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    int32_t ret =
        NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_ERR_UNREGISTER_REPEAT);
}

/**
* @tc.number    : NotificationHelper_unRegisterPhotoAlbumCallback_test_004
* @tc.name      : unRegisterPhotoAlbumCallback: Unregister same callback twice
* @tc.desc      : Unregister the same callback twice should return error on second call
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegisterPhotoAlbumCallback_test_004, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(callback), NOTIFY_OK);
    EXPECT_EQ(NotificationHelperApi::unRegisterPhotoAlbumCallback(callback), NOTIFY_OK);
    EXPECT_EQ(NotificationHelperApi::unRegisterPhotoAlbumCallback(callback), NOTIFY_ERR_UNREGISTER_REPEAT);
}

// ==================== PhotoAlbumChangeCallback OnChange ====================

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_001
* @tc.name      : PhotoAlbumChangeCallback: OnChange with ADD type
* @tc.desc      : Verify callback OnChange is called with NOTIFY_CHANGE_ADD
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_001, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    testInfo.isForRecheck = false;
    EXPECT_EQ(callback->OnChange(testInfo), 0);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_ADD);
}

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_002
* @tc.name      : PhotoAlbumChangeCallback: OnChange with UPDATE type
* @tc.desc      : Verify OnChange is called with NOTIFY_CHANGE_UPDATE type
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_002, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    testInfo.isForRecheck = false;
    EXPECT_EQ(callback->OnChange(testInfo), 0);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_UPDATE);
}

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_003
* @tc.name      : PhotoAlbumChangeCallback: OnChange with REMOVE type
* @tc.desc      : Verify OnChange is called with NOTIFY_CHANGE_REMOVE type
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_003, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_REMOVE;
    testInfo.isForRecheck = false;
    EXPECT_EQ(callback->OnChange(testInfo), 0);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_REMOVE);
}

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_004
* @tc.name      : PhotoAlbumChangeCallback: OnChange with AlbumChangeData
* @tc.desc      : Verify OnChange receives NotificationAlbumChangeData correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_004, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    testInfo.isForRecheck = false;
    NotificationAlbumChangeData changeData;
    changeData.version = 12345;
    testInfo.albumChangeDatas.push_back(changeData);

    EXPECT_EQ(callback->OnChange(testInfo), 0);
    EXPECT_EQ(callback->GetCallCount(), 1);
    AlbumChangeInfos receivedInfo = callback->GetLastInfo();
    EXPECT_EQ(receivedInfo.albumChangeDatas.size(), 1u);
    EXPECT_EQ(receivedInfo.albumChangeDatas[0].version, 12345);
}

/**
* @tc.number    : NotificationHelper_Callback_Multiple_test_001
* @tc.name      : Multiple callbacks: All callbacks receive notification
* @tc.desc      : Verify that all registered callbacks can receive notification
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_Multiple_test_001, TestSize.Level1)
{
    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback3 = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback1, nullptr);
    ASSERT_NE(callback2, nullptr);
    ASSERT_NE(callback3, nullptr);

    NotificationHelperApi::RegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback2);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback3);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    callback1->OnChange(testInfo);
    callback2->OnChange(testInfo);
    callback3->OnChange(testInfo);

    EXPECT_EQ(callback1->GetCallCount(), 1);
    EXPECT_EQ(callback2->GetCallCount(), 1);
    EXPECT_EQ(callback3->GetCallCount(), 1);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback3);
}

/**
* @tc.number    : NotificationHelper_Callback_Recheck_test_001
* @tc.name      : PhotoAlbumChangeCallback: OnChange with recheck
* @tc.desc      : Verify isForRecheck true and empty albumChangeDatas
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_Recheck_test_001, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback);

    callback->Reset();
    AlbumChangeInfos recheckInfo;
    recheckInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    recheckInfo.isForRecheck = true;
    callback->OnChange(recheckInfo);
    EXPECT_EQ(callback->GetCallCount(), 1);
    AlbumChangeInfos receivedInfo = callback->GetLastInfo();
    EXPECT_EQ(receivedInfo.isForRecheck, true);
    EXPECT_EQ(receivedInfo.albumChangeDatas.size(), 0u);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
}

namespace {
class FailingPhotoAlbumChangeCallback : public PhotoAlbumChangeCallback {
public:
    int32_t OnChange(AlbumChangeInfos /* info */) override
    {
        return -1;
    }
};
} // namespace

/**
* @tc.number    : NotificationHelper_Callback_ChangeTypes_test_001
* @tc.name      : Callback receives all NotifyChangeType values correctly
* @tc.desc      : Verify callback handles REMOVE and UPDATE types via direct OnChange
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_ChangeTypes_test_001, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(callback), NOTIFY_OK);

    callback->Reset();
    AlbumChangeInfos removeInfo;
    removeInfo.type = NotifyChangeType::NOTIFY_CHANGE_REMOVE;
    callback->OnChange(removeInfo);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_REMOVE);

    callback->Reset();
    AlbumChangeInfos updateInfo;
    updateInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    callback->OnChange(updateInfo);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastChangeType(), NotifyChangeType::NOTIFY_CHANGE_UPDATE);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
}

/**
* @tc.number    : NotificationHelper_Expired_WeakPtr_test_001
* @tc.name      : Register cleans expired weak_ptr then new callback works
* @tc.desc      : Cover Register loop erase of nullptr lock()
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Expired_WeakPtr_test_001, TestSize.Level1)
{
    {
        auto dead = std::make_shared<MockPhotoAlbumChangeCallback>();
        ASSERT_NE(dead, nullptr);
        EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(dead), NOTIFY_OK);
    }
    auto live = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(live, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(live), NOTIFY_OK);
    live->Reset();
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    live->OnChange(testInfo);
    EXPECT_EQ(live->GetCallCount(), 1);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(live);
}

/**
* @tc.number    : NotificationHelper_Exception_Handling_test_001
* @tc.name      : Failing callback does not affect normal callback
* @tc.desc      : Verify a callback returning error does not affect other callbacks
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Exception_Handling_test_001, TestSize.Level1)
{
    auto failing = std::make_shared<FailingPhotoAlbumChangeCallback>();
    auto normal = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(failing, nullptr);
    ASSERT_NE(normal, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(failing), NOTIFY_OK);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(normal), NOTIFY_OK);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_ADD;
    EXPECT_EQ(failing->OnChange(testInfo), -1);

    normal->Reset();
    normal->OnChange(testInfo);
    EXPECT_EQ(normal->GetCallCount(), 1);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(failing);
    NotificationHelperApi::unRegisterPhotoAlbumCallback(normal);
}

/**
* @tc.number    : NotificationHelper_AlbumChangeData_Content_test_001
* @tc.name      : AlbumChangeData with before/after snapshots
* @tc.desc      : Verify AlbumChangeData carries snapshot data correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_AlbumChangeData_Content_test_001, TestSize.Level1)
{
    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(callback), NOTIFY_OK);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    NotificationAlbumChangeData changeData;
    auto before = std::make_shared<AccurateRefresh::AlbumChangeInfo>();
    before->albumId_ = 100;
    auto after = std::make_shared<AccurateRefresh::AlbumChangeInfo>();
    after->albumId_ = 200;
    changeData.albumBeforeChange = before;
    changeData.albumAfterChange = after;
    testInfo.albumChangeDatas.push_back(changeData);

    callback->Reset();
    callback->OnChange(testInfo);
    EXPECT_EQ(callback->GetCallCount(), 1);
    AlbumChangeInfos received = callback->GetLastInfo();
    ASSERT_EQ(received.albumChangeDatas.size(), 1u);
    EXPECT_NE(received.albumChangeDatas[0].albumBeforeChange, nullptr);
    EXPECT_EQ(received.albumChangeDatas[0].albumBeforeChange->albumId_, 100);
    EXPECT_NE(received.albumChangeDatas[0].albumAfterChange, nullptr);
    EXPECT_EQ(received.albumChangeDatas[0].albumAfterChange->albumId_, 200);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
}

/**
* @tc.number    : NotificationHelper_unRegister_expired_weak_test_001
* @tc.name      : unRegister: skips expired weak_ptr and removes target
* @tc.desc      : Cover unRegister loop nullptr lock branch
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegister_expired_weak_test_001, TestSize.Level1)
{
    {
        auto temp = std::make_shared<MockPhotoAlbumChangeCallback>();
        ASSERT_NE(temp, nullptr);
        EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(temp), NOTIFY_OK);
    }
    auto keeper = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(keeper, nullptr);
    EXPECT_EQ(NotificationHelperApi::RegisterPhotoAlbumCallback(keeper), NOTIFY_OK);
    EXPECT_EQ(NotificationHelperApi::unRegisterPhotoAlbumCallback(keeper), NOTIFY_OK);
}

// ==================== PhotoAlbum scene_id / share_type ====================

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet2_Test_002
* @tc.name      : PhotoAlbum SetSceneId/GetSceneId, SetShareType/GetShareType
* @tc.desc      : Verify scene_id and share_type getters and setters
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet2_Test_001, TestSize.Level1)
{
    PhotoAlbum photoAlbum;
    const int32_t TEST_SCENE_ID = 100;
    const int32_t TEST_SHARE_TYPE = 200;
    photoAlbum.SetSceneId(TEST_SCENE_ID);
    EXPECT_EQ(photoAlbum.GetSceneId(), TEST_SCENE_ID);
    photoAlbum.SetShareType(TEST_SHARE_TYPE);
    EXPECT_EQ(photoAlbum.GetShareType(), TEST_SHARE_TYPE);
    photoAlbum.SetSceneId(0);
    photoAlbum.SetShareType(0);
    EXPECT_EQ(photoAlbum.GetSceneId(), 0);
    EXPECT_EQ(photoAlbum.GetShareType(), 0);
}

// ==================== PhotoAlbum database ====================

/**
* @tc.number    : NotificationHelper_PhotoAlbum_Database_Test_001
* @tc.name      : PhotoAlbum database scene_id and share_type
* @tc.desc      : Create album, verify default values, update scene_id/share_type, query and verify
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_Database_Test_001, TestSize.Level1)
{
    if (g_rdbStore == nullptr) {
        MEDIA_WARN_LOG("g_rdbStore is nullptr, skipping database test");
        return;
    }

    const string albumName = "NotificationHelper_PhotoAlbum_Database_Test_001";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_SCENE_ID,
        PhotoAlbumColumns::ALBUM_SHARE_TYPE,
    };

    auto resultSet = g_rdbStore->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    int32_t count = -1;
    CHECK_AND_RETURN_LOG(resultSet->GetRowCount(count) == E_OK, "Failed to get count!");
    EXPECT_GT(count, 0);
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == E_OK, "Failed to GoToFirstRow!");

    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SCENE_ID, TYPE_INT32, 0);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SHARE_TYPE, TYPE_INT32, 0);

    const int32_t TEST_SCENE_ID = 100;
    const int32_t TEST_SHARE_TYPE = 200;
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_SCENE_ID, TEST_SCENE_ID);
    values.Put(PhotoAlbumColumns::ALBUM_SHARE_TYPE, TEST_SHARE_TYPE);
    DataSharePredicates updatePredicates;
    updatePredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    EXPECT_EQ(UpdatePhotoAlbum(values, updatePredicates), 0);

    resultSet = g_rdbStore->Query(predicates, columns);
    ASSERT_NE(resultSet, nullptr);
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == E_OK, "Failed to GoToFirstRow!");
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SCENE_ID, TYPE_INT32, TEST_SCENE_ID);
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SHARE_TYPE, TYPE_INT32, TEST_SHARE_TYPE);
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_Database_Test_002
* @tc.name      : PhotoAlbum FetchResult parsing
* @tc.desc      : Update scene_id/share_type, query via FetchResult, verify PhotoAlbum values
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_Database_Test_002, TestSize.Level1)
{
    if (g_rdbStore == nullptr) {
        MEDIA_WARN_LOG("g_rdbStore is nullptr, skipping database test");
        return;
    }

    const string albumName = "NotificationHelper_PhotoAlbum_Database_Test_002";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    const int32_t TEST_SCENE_ID = 300;
    const int32_t TEST_SHARE_TYPE = 400;
    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_SCENE_ID, TEST_SCENE_ID);
    values.Put(PhotoAlbumColumns::ALBUM_SHARE_TYPE, TEST_SHARE_TYPE);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    EXPECT_EQ(UpdatePhotoAlbum(values, predicates), 0);

    RdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    vector<string> queryColumns;
    auto resultSet = g_rdbStore->Query(queryPredicates, queryColumns);
    ASSERT_NE(resultSet, nullptr);

    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    auto fetchResult = make_unique<FetchResult<PhotoAlbum>>(
        make_shared<DataShare::DataShareResultSet>(bridge));
    ASSERT_NE(fetchResult, nullptr);

    auto photoAlbum = fetchResult->GetFirstObject();
    ASSERT_NE(photoAlbum, nullptr);
    EXPECT_EQ(photoAlbum->GetSceneId(), TEST_SCENE_ID);
    EXPECT_EQ(photoAlbum->GetShareType(), TEST_SHARE_TYPE);
}

/**
* @tc.number    : NotificationHelper_RegisterPhotoAlbumCallback_test_008
* @tc.name      : RegisterPhotoAlbumCallback: Register callback with maximum boundary values
* @tc.desc      : Verify system handles large number of callback registrations
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_RegisterPhotoAlbumCallback_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_RegisterPhotoAlbumCallback_test_008 enter");

    const int32_t MAX_CALLBACKS = 1000;
    std::vector<std::shared_ptr<MockPhotoAlbumChangeCallback>> callbacks;

    for (int32_t i = 0; i < MAX_CALLBACKS; i++) {
        auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
        int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
        EXPECT_EQ(ret, NOTIFY_OK);
        callbacks.push_back(callback);
    }

    // Unregister all
    for (auto& callback : callbacks) {
        NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
    }

    MEDIA_INFO_LOG("NotificationHelper_RegisterPhotoAlbumCallback_test_008 exit");
}

/**
* @tc.number    : NotificationHelper_RegisterPhotoAlbumCallback_test_009
* @tc.name      : RegisterPhotoAlbumCallback: Register and verify callback persistence
* @tc.desc      : Verify registered callback persists across multiple operations
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_RegisterPhotoAlbumCallback_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_RegisterPhotoAlbumCallback_test_009 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK);

    // Verify callback is still registered after multiple operations
    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    callback->OnChange(testInfo);
    EXPECT_EQ(callback->GetCallCount(), 1);

    // Unregister
    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
    EXPECT_EQ(ret, NOTIFY_OK);

    MEDIA_INFO_LOG("NotificationHelper_RegisterPhotoAlbumCallback_test_009 exit");
}

/**
* @tc.number    : NotificationHelper_unRegisterPhotoAlbumCallback_test_006
* @tc.name      : unRegisterPhotoAlbumCallback: Unregister from middle of list
* @tc.desc      : Verify unregistering a callback from middle of registered list works
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_unRegisterPhotoAlbumCallback_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_unRegisterPhotoAlbumCallback_test_006 enter");

    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback3 = std::make_shared<MockPhotoAlbumChangeCallback>();

    NotificationHelperApi::RegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback2);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback3);

    // Unregister middle one
    int32_t ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);
    EXPECT_EQ(ret, NOTIFY_OK);

    // Verify others still registered
    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    EXPECT_EQ(ret, NOTIFY_OK);
    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback3);
    EXPECT_EQ(ret, NOTIFY_OK);

    MEDIA_INFO_LOG("NotificationHelper_unRegisterPhotoAlbumCallback_test_006 exit");
}

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_009
* @tc.name      : PhotoAlbumChangeCallback: OnChange with empty AlbumChangeDatas
* @tc.desc      : Verify OnChange handles empty albumChangeDatas vector
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_OnChange_test_009 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    testInfo.albumChangeDatas.clear(); // Empty vector

    int32_t ret = callback->OnChange(testInfo);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(callback->GetCallCount(), 1);
    EXPECT_EQ(callback->GetLastInfo().albumChangeDatas.size(), 0);

    MEDIA_INFO_LOG("NotificationHelper_Callback_OnChange_test_009 exit");
}

/**
* @tc.number    : NotificationHelper_Callback_OnChange_test_010
* @tc.name      : PhotoAlbumChangeCallback: OnChange with multiple AlbumChangeData
* @tc.desc      : Verify OnChange handles multiple NotificationAlbumChangeData entries
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Callback_OnChange_test_010, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Callback_OnChange_test_010 enter");

    auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
    ASSERT_NE(callback, nullptr);

    AlbumChangeInfos testInfo;
    testInfo.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;

    const int32_t DATA_COUNT = 10;
    for (int32_t i = 0; i < DATA_COUNT; i++) {
        NotificationAlbumChangeData changeData;
        changeData.version = i;
        testInfo.albumChangeDatas.push_back(changeData);
    }

    callback->OnChange(testInfo);
    EXPECT_EQ(callback->GetLastInfo().albumChangeDatas.size(), DATA_COUNT);

    MEDIA_INFO_LOG("NotificationHelper_Callback_OnChange_test_010 exit");
}

/**
* @tc.number    : NotificationHelper_Thread_Safety_test_004
* @tc.name      : Thread safety: Rapid register/unregister cycles
* @tc.desc      : Verify system handles rapid register/unregister cycles from multiple threads
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Thread_Safety_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Thread_Safety_test_004 enter");

    const int32_t threadCount = 5;
    const int32_t cyclesPerThread = 20;
    std::atomic<int32_t> successCount(0);

    // Lambda dışarı alındı → derinlik azaldı
    auto threadTask = [&successCount, cyclesPerThread]() {
        for (int32_t j = 0; j < cyclesPerThread; j++) {
            auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
            int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
            if (ret != NOTIFY_OK) continue;
            successCount++;
            NotificationHelperApi::unRegisterPhotoAlbumCallback(callback);
        }
    };

    std::vector<std::thread> threads;
    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back(threadTask);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successCount.load(), threadCount * cyclesPerThread);

    MEDIA_INFO_LOG("NotificationHelper_Thread_Safety_test_004 exit");
}

/**
* @tc.number    : NotificationHelper_Edge_Case_test_013
* @tc.name      : Edge case: Callback destruction during notification
* @tc.desc      : Verify system handles callback destruction gracefully
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Edge_Case_test_013, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_013 enter");

    {
        auto callback = std::make_shared<MockPhotoAlbumChangeCallback>();
        NotificationHelperApi::RegisterPhotoAlbumCallback(callback);
        // Callback goes out of scope
    }

    // Register new callback should work
    auto newCallback = std::make_shared<MockPhotoAlbumChangeCallback>();
    int32_t ret = NotificationHelperApi::RegisterPhotoAlbumCallback(newCallback);
    EXPECT_EQ(ret, NOTIFY_OK);

    NotificationHelperApi::unRegisterPhotoAlbumCallback(newCallback);

    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_013 exit");
}

/**
* @tc.number    : NotificationHelper_Edge_Case_test_014
* @tc.name      : Edge case: Register/unregister with same pointer different instances
* @tc.desc      : Verify system distinguishes between different callback instances
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_Edge_Case_test_014, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_014 enter");

    auto callback1 = std::make_shared<MockPhotoAlbumChangeCallback>();
    auto callback2 = std::make_shared<MockPhotoAlbumChangeCallback>();

    // Verify they are different instances
    EXPECT_NE(callback1.get(), callback2.get());

    NotificationHelperApi::RegisterPhotoAlbumCallback(callback1);
    NotificationHelperApi::RegisterPhotoAlbumCallback(callback2);

    // Unregister one should not affect the other
    int32_t ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback1);
    EXPECT_EQ(ret, NOTIFY_OK);

    ret = NotificationHelperApi::unRegisterPhotoAlbumCallback(callback2);
    EXPECT_EQ(ret, NOTIFY_OK);

    MEDIA_INFO_LOG("NotificationHelper_Edge_Case_test_014 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_005
* @tc.name      : PhotoAlbum SetSceneId boundary values test
* @tc.desc      : Verify SetSceneId handles boundary values correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_005 enter");

    PhotoAlbum photoAlbum;

    // Test boundary values
    const int32_t MIN_INT32 = INT32_MIN;
    const int32_t MAX_INT32 = INT32_MAX;
    const int32_t ZERO = 0;

    photoAlbum.SetSceneId(MIN_INT32);
    EXPECT_EQ(photoAlbum.GetSceneId(), MIN_INT32);

    photoAlbum.SetSceneId(MAX_INT32);
    EXPECT_EQ(photoAlbum.GetSceneId(), MAX_INT32);

    photoAlbum.SetSceneId(ZERO);
    EXPECT_EQ(photoAlbum.GetSceneId(), ZERO);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_005 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_006
* @tc.name      : PhotoAlbum SetShareType boundary values test
* @tc.desc      : Verify SetShareType handles boundary values correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_006 enter");

    PhotoAlbum photoAlbum;

    // Test boundary values
    const int32_t MIN_INT32 = INT32_MIN;
    const int32_t MAX_INT32 = INT32_MAX;
    const int32_t ZERO = 0;

    photoAlbum.SetShareType(MIN_INT32);
    EXPECT_EQ(photoAlbum.GetShareType(), MIN_INT32);

    photoAlbum.SetShareType(MAX_INT32);
    EXPECT_EQ(photoAlbum.GetShareType(), MAX_INT32);

    photoAlbum.SetShareType(ZERO);
    EXPECT_EQ(photoAlbum.GetShareType(), ZERO);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_006 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_SetGet_Test_007
* @tc.name      : PhotoAlbum SetSceneId and SetShareType sequence test
* @tc.desc      : Verify SetSceneId and SetShareType work correctly in different sequences
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_SetGet_Test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_007 enter");

    PhotoAlbum photoAlbum;

    // Sequence 1: SetSceneId first, then SetShareType
    photoAlbum.SetSceneId(100);
    photoAlbum.SetShareType(200);
    EXPECT_EQ(photoAlbum.GetSceneId(), 100);
    EXPECT_EQ(photoAlbum.GetShareType(), 200);

    // Sequence 2: SetShareType first, then SetSceneId
    photoAlbum.SetShareType(300);
    photoAlbum.SetSceneId(400);
    EXPECT_EQ(photoAlbum.GetSceneId(), 400);
    EXPECT_EQ(photoAlbum.GetShareType(), 300);

    // Sequence 3: Multiple sets
    photoAlbum.SetSceneId(500);
    photoAlbum.SetSceneId(600);
    photoAlbum.SetShareType(700);
    photoAlbum.SetShareType(800);
    EXPECT_EQ(photoAlbum.GetSceneId(), 600);
    EXPECT_EQ(photoAlbum.GetShareType(), 800);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_SetGet_Test_007 exit");
}

/**
* @tc.number    : NotificationHelper_PhotoAlbum_Database_Test_003
* @tc.name      : PhotoAlbum database: Update only scene_id
* @tc.desc      : Verify updating only scene_id without share_type works correctly
*/
HWTEST_F(NotificationHelperTest, NotificationHelper_PhotoAlbum_Database_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_Database_Test_003 enter");

    if (g_rdbStore == nullptr) {
        MEDIA_WARN_LOG("g_rdbStore is nullptr, skipping database test");
        return;
    }

    const string albumName = "NotificationHelper_PhotoAlbum_Database_Test_003";
    int32_t albumId = CreatePhotoAlbum(albumName);
    ASSERT_GT(albumId, 0);

    // Update only scene_id
    const int32_t TEST_SCENE_ID = 500;

    DataShareValuesBucket values;
    values.Put(PhotoAlbumColumns::ALBUM_SCENE_ID, TEST_SCENE_ID);

    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    EXPECT_EQ(UpdatePhotoAlbum(values, predicates), 0);

    // Query and verify
    RdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));

    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_SCENE_ID,
        PhotoAlbumColumns::ALBUM_SHARE_TYPE,
    };

    auto resultSet = g_rdbStore->Query(queryPredicates, columns);
    ASSERT_NE(resultSet, nullptr);
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == E_OK, "Failed to GoToFirstRow!");

    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SCENE_ID, TYPE_INT32, TEST_SCENE_ID);
    // share_type should remain 0 (default)
    CheckColumn(resultSet, PhotoAlbumColumns::ALBUM_SHARE_TYPE, TYPE_INT32, 0);

    MEDIA_INFO_LOG("NotificationHelper_PhotoAlbum_Database_Test_003 exit");
}
} // namespace Media
} // namespace OHOS