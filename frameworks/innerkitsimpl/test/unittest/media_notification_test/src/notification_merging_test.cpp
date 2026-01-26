/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MLOG_TAG "NotificationMergingTest"

#include "notification_merging_test.h"
#include "notification_merging.h"
#include "media_log.h"
#include "notify_info.h"
#include "notification_test_data.h"
#include "get_self_permissions.h"
#include "media_change_info.h"
#include "medialibrary_mock_tocken.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

static uint64_t g_shellToken = 0;
MediaLibraryMockHapToken* mockToken = nullptr;

void NotificationMergingTest::SetUpTestCase(void)
{
    std::vector<string> perms;
    MEDIA_INFO_LOG("NotificationMergingTest start");
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");

    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
    MEDIA_INFO_LOG("NotificationMergingTest end");
}

void NotificationMergingTest::TearDownTestCase(void)
{
    if (mockToken!= nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
}

void NotificationMergingTest::SetUp()
{}

void NotificationMergingTest::TearDown(void)
{
    MEDIA_INFO_LOG("NotificationMergingTest TearDown start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    observerManager->observers_.clear();
    observerManager->obsCallbackPecipients_.clear();
    MEDIA_INFO_LOG("NotificationMergingTest TearDown end");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test001, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test001");
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo({});
    EXPECT_TRUE(resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test001");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test002, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test002");
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo =
        mergeNotifyInfo.MergeNotifyInfo(OHOS::Media::Notification::NotificationTestData::buildAssetsMediaChangeInfo());
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test002");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test003, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test003");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "10";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo =
        mergeNotifyInfo.MergeNotifyInfo(OHOS::Media::Notification::NotificationTestData::buildAssetsMediaChangeInfo());
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test003");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test004, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test004");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "7";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo =
        mergeNotifyInfo.MergeNotifyInfo(OHOS::Media::Notification::NotificationTestData::buildAssetsMediaChangeInfo());
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test004");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test005, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test005");
    Notification::NotifyUriType albumUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(albumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleAlbumId = "7";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleAlbumId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo =
        mergeNotifyInfo.MergeNotifyInfo(OHOS::Media::Notification::NotificationTestData::buildAssetsMediaChangeInfo());
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test005");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test006, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test006");
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleAlbumId = "7";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleAlbumId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo =
        mergeNotifyInfo.MergeNotifyInfo(OHOS::Media::Notification::NotificationTestData::buildAssetsMediaChangeInfo());
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test006");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test007, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test007");
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    std::vector<Notification::MediaChangeInfo> mediaChangeInfos;
    Media::Notification::MediaChangeInfo photoAlbumUriChangeInfo;
    photoAlbumUriChangeInfo.notifyUri = singlePhotoAlbumUri;
    photoAlbumUriChangeInfo.isForRecheck = false;
    photoAlbumUriChangeInfo.isSystem = true;
    photoAlbumUriChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ALBUM_ADD;
    mediaChangeInfos.push_back(photoAlbumUriChangeInfo);
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleAlbumId = "7";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleAlbumId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo(mediaChangeInfos);
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test007");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test008, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test008");
    Notification::NotifyUriType albumUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    std::vector<Notification::MediaChangeInfo> mediaChangeInfos;
    Media::Notification::MediaChangeInfo photoAlbumUriChangeInfo;
    photoAlbumUriChangeInfo.notifyUri = singlePhotoAlbumUri;
    photoAlbumUriChangeInfo.isForRecheck = true;
    photoAlbumUriChangeInfo.isSystem = true;
    photoAlbumUriChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ALBUM_ADD;
    mediaChangeInfos.push_back(photoAlbumUriChangeInfo);
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(albumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleAlbumId = "7";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleAlbumId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo(mediaChangeInfos);
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test008");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test009, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test009");
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    std::vector<Notification::MediaChangeInfo> mediaChangeInfos;
    Media::Notification::MediaChangeInfo photoAlbumUriChangeInfo;
    photoAlbumUriChangeInfo.notifyUri = singlePhotoAlbumUri;
    photoAlbumUriChangeInfo.isForRecheck = true;
    photoAlbumUriChangeInfo.isSystem = true;
    photoAlbumUriChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ALBUM_ADD;
    mediaChangeInfos.push_back(photoAlbumUriChangeInfo);
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleAlbumId = "7";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleAlbumId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo(mediaChangeInfos);
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test009");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test0010, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test0010");
    Notification::NotifyUriType photoAlbumUri = Notification::NotifyUriType::TRASH_ALBUM_URI;
    std::vector<Notification::MediaChangeInfo> mediaChangeInfos;
    Media::Notification::MediaChangeInfo photoAlbumUriChangeInfo;
    photoAlbumUriChangeInfo.notifyUri = photoAlbumUri;
    photoAlbumUriChangeInfo.isForRecheck = false;
    photoAlbumUriChangeInfo.isSystem = true;
    photoAlbumUriChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ALBUM_REMOVE;
    mediaChangeInfos.push_back(photoAlbumUriChangeInfo);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo(mediaChangeInfos);
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test0010");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test0011, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test0011");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    std::vector<Notification::MediaChangeInfo> mediaChangeInfos;
    Media::Notification::MediaChangeInfo photoUriChangeInfo;
    auto deletePhotoData = OHOS::Media::Notification::deletePhotoData1;
    deletePhotoData.isDelete_ = true;
    photoUriChangeInfo.changeInfos.push_back(deletePhotoData);
    photoUriChangeInfo.isForRecheck = false;
    photoUriChangeInfo.notifyUri = singlePhotoUri;
    photoUriChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ASSET_REMOVE;
    mediaChangeInfos.push_back(photoUriChangeInfo);
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "10";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo(mediaChangeInfos);
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test0011");
}

HWTEST_F(NotificationMergingTest, medialib_notification_test0012, TestSize.Level1)
{
    MEDIA_ERR_LOG("enter medialib_notification_test0012");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    std::vector<Notification::MediaChangeInfo> mediaChangeInfos;
    Media::Notification::MediaChangeInfo photoUriChangeInfo;
    Media::AccurateRefresh::PhotoAssetChangeInfo assetChangeInfo;
    auto deletePhotoData = OHOS::Media::Notification::deletePhotoData1;
    deletePhotoData.infoAfterChange_.fileId_ = 10;
    deletePhotoData.infoBeforeChange_.fileId_ = 100;
    photoUriChangeInfo.changeInfos.push_back(deletePhotoData);
    photoUriChangeInfo.isForRecheck = false;
    photoUriChangeInfo.notifyUri = singlePhotoUri;
    photoUriChangeInfo.notifyType = AccurateNotifyType::NOTIFY_ASSET_ADD;
    mediaChangeInfos.push_back(photoUriChangeInfo);
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<AAFwk::IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "10";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_OK);
    OHOS::Media::Notification::NotificationMerging mergeNotifyInfo;
    std::vector<Notification::NotifyInfo> resultNotifyInfo = mergeNotifyInfo.MergeNotifyInfo(mediaChangeInfos);
    EXPECT_TRUE(!resultNotifyInfo.empty());
    MEDIA_ERR_LOG("end medialib_notification_test0012");
}

} // namespace Media
} // namespace OHOS