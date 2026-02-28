/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define private public
#define protected public

#include "media_notification_register_manager_test.h"
#include "media_datashare_stub_impl.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "get_self_permissions.h"
#include "medialibrary_mock_tocken.h"
#include "media_change_info.h"
#undef private
#undef protected

using namespace testing::ext;
using namespace OHOS::AbilityRuntime;
using namespace OHOS::AAFwk;

namespace OHOS {
namespace Media {

static uint64_t g_shellToken = 0;
MediaLibraryMockHapToken* mockToken = nullptr;

void NotificationRegisterManagerTest::SetUpTestCase(void)
{
    std::vector<string> perms;
    MEDIA_INFO_LOG("NotificationRegisterManagerTest start");
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");

    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
    MEDIA_INFO_LOG("NotificationRegisterManagerTest end");
}

void NotificationRegisterManagerTest::TearDownTestCase(void)
{
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }
    SetSelfTokenID(g_shellToken);
    MediaLibraryMockTokenUtils::ResetToken();
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
}

void NotificationRegisterManagerTest::SetUp(void)
{
}

void NotificationRegisterManagerTest::TearDown(void)
{
    MEDIA_INFO_LOG("NotificationRegisterManagerTest TearDown start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    observerManager->observers_.clear();
    observerManager->obsCallbackPecipients_.clear();
    MEDIA_INFO_LOG("NotificationRegisterManagerTest TearDown end");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_001, TestSize.Level1) {
    // 1个uri,1个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_001::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    size_t observerSize = observerManager->FindObserver(uri).size();
    EXPECT_EQ(observerSize, 1);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_002, TestSize.Level1) {
    // 1个uri,1个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    size_t observerSize = observerManager->FindObserver(uri).size();
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(observerSize, 1);
    EXPECT_EQ(obsCallbackSize, 1);
    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_003, TestSize.Level1) {
    // 1个uri ，2个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_003::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    size_t observerSize = observerManager->FindObserver(uri).size();
    EXPECT_EQ(observerSize, 2);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 2);

    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    observerManager->RemoveObserver(secondDataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_004, TestSize.Level1) {
    // 1个uri ，2个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    size_t observerSize = observerManager->FindObserver(uri).size();
    EXPECT_EQ(observerSize, 2);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 2);

    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    observerManager->RemoveObserver(secondDataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_005, TestSize.Level1) {
    // 2个uri ，1个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_005::Start");
    Notification::NotifyUriType photoUri = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType albumUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;

    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(photoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(albumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 2);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    size_t observerSize = observerManager->observers_.at(photoUri).size();
    EXPECT_EQ(observerSize, 1);
    observerSize = observerManager->observers_.at(albumUri).size();
    EXPECT_EQ(observerSize, 1);

    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_006, TestSize.Level1) {
    // 2个uri ，2个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_006::Start");
    Notification::NotifyUriType photoUri = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType albumUri = Notification::NotifyUriType::PHOTO_ALBUM_URI;

    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(photoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(albumUri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 2);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 2);
    size_t observerSize = observerManager->observers_.at(photoUri).size();
    EXPECT_EQ(observerSize, 1);
    observerSize = observerManager->observers_.at(albumUri).size();
    EXPECT_EQ(observerSize, 1);

    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    observerManager->RemoveObserver(secondDataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_006::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_007, TestSize.Level1) {
    // 1个uri,1个observer
    MEDIA_INFO_LOG("NotificationRegisterManager_test_007::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);

    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    size_t observerSize = observerManager->FindObserver(uri).size();
    EXPECT_EQ(observerSize, 1);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_DATAOBSERVER_IS_REPEATED);

    observerManager->RemoveObserver(dataObserver->AsObject());
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_007::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationRegisterManager_test_008, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationRegisterManager_test_008::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver[50] ;
    for (int i = 0; i < 50; i++) {
        dataObserver[i] = new (std::nothrow) IDataAbilityObserverTest();
        EXPECT_NE(dataObserver[i], nullptr);
        int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver[i]);
        EXPECT_EQ(ret, E_OK);
        ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver[i]);
        EXPECT_EQ(ret, E_OK);
    }
    size_t total_element_count = 0;
    for (const auto& pair : observerManager->observers_) {
        total_element_count += pair.second.size();
    }
    EXPECT_EQ(total_element_count, 100);
    sptr<IDataAbilityObserver> nextAssetDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(nextAssetDataObserver, nullptr);
    sptr<IDataAbilityObserver> nextAlbumDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(nextAlbumDataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, nextAssetDataObserver);
    EXPECT_EQ(ret, E_MAX_ON_SINGLE_NUM);
    ret = observerManager->AddObserver(singlePhotoAlbumUri, nextAlbumDataObserver);
    EXPECT_EQ(ret, E_MAX_ON_SINGLE_NUM);
    total_element_count = 0;
    for (const auto& pair : observerManager->observers_) {
        total_element_count += pair.second.size();
    }
    EXPECT_EQ(total_element_count, 100);
    MEDIA_INFO_LOG("NotificationRegisterManager_test_008::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_001::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    std::string singleId = "errSingleId";
    int32_t ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_URI_IS_INVALID);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_002::Start");
    Notification::NotifyUriType errUri = Notification::NotifyUriType::INVALID;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    std::string singleId = "1";
    int32_t ret = observerManager->AddSingleObserverSingleIds(errUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_003::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "1";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->RemoveSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_004::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    for (int i = 0; i < 999; i++) {
        std::string singleId = to_string(i);
        ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
        EXPECT_EQ(ret, E_OK);
    }
    std::string singleId = "1000";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_MAX_ON_SINGLE_NUM);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_005, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_005::Start");
    Notification::NotifyUriType singlePhotoAlbumUri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoAlbumUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    for (int i = 0; i < 249; i++) {
        std::string singleId = to_string(i);
        observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleId);
        EXPECT_EQ(ret, E_OK);
    }
    std::string singleId = "250";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoAlbumUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_MAX_ON_SINGLE_NUM);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_006, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_006::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    std::string singleId = "1";
    auto ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_URI_NOT_EXIST);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_006::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_007, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_007::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    sptr<IDataAbilityObserver> nextDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(nextDataObserver, nullptr);
    std::string singleId = "1";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, nextDataObserver, singleId);
    EXPECT_EQ(ret, E_DATAOBSERVER_IS_NULL);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_007::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_008, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_008::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    std::string singleId = "1";
    auto ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, dataObserver, singleId);
    EXPECT_EQ(ret, E_URI_NOT_EXIST);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_008::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotificationSingleRegisterManager_test_009, TestSize.Level1) {
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_009::Start");
    Notification::NotifyUriType singlePhotoUri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(singlePhotoUri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);
    std::string singleId = "1";
    ret = observerManager->AddSingleObserverSingleIds(singlePhotoUri, secondDataObserver, singleId);
    EXPECT_EQ(ret, E_DATAOBSERVER_IS_NULL);
    MEDIA_INFO_LOG("NotificationSingleRegisterManager_test_009::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObsDeathRecipient_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_001::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    wptr<IRemoteObject> object = dataObserver->AsObject();
    observerManager->RemoveObsDeathRecipient(object);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 0);
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObsDeathRecipient_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_002::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    wptr<IRemoteObject> nullObject;
    int32_t ret = observerManager->RemoveObsDeathRecipient(nullObject);
    EXPECT_EQ(ret, E_DATAOBSERVER_IS_NULL);
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObsDeathRecipient_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_003::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    wptr<IRemoteObject> object = dataObserver->AsObject();
    int32_t ret = observerManager->RemoveObsDeathRecipient(object);
    EXPECT_EQ(ret, E_DATAOBSERVER_IS_NULL);
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObserverWithUri_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_001::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 1);
    ret = observerManager->RemoveObserverWithUri(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObserverWithUri_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->RemoveObserverWithUri(uri, dataObserver);
    EXPECT_EQ(ret, E_URI_NOT_EXIST);
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObserverWithUri_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_003::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t observerSize = observerManager->FindObserver(uri).size();
    EXPECT_EQ(observerSize, 2);
    ret = observerManager->RemoveObserverWithUri(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    observerSize = observerManager->FindObserver(uri).size();
    EXPECT_EQ(observerSize, 1);
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserverWithUri_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_001::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    bool found = observerManager->FindSingleObserverWithUri(uri, callingTokenId);
    EXPECT_TRUE(found);
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserverWithUri_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    uint32_t callingTokenId = 12345;
    bool found = observerManager->FindSingleObserverWithUri(uri, callingTokenId);
    EXPECT_FALSE(found);
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserverWithUri_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_003::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::INVALID;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    bool found = observerManager->FindSingleObserverWithUri(uri, callingTokenId);
    EXPECT_FALSE(found);
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, GetObservers_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("GetObservers_test_001::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observers =
        observerManager->GetObservers();
    EXPECT_EQ(observers.size(), 0);
    MEDIA_INFO_LOG("GetObservers_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, GetObservers_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("GetObservers_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observers =
        observerManager->GetObservers();
    EXPECT_EQ(observers.size(), 1);
    MEDIA_INFO_LOG("GetObservers_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleOperationPermissionsAndLimit_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleOperationPermissionsAndLimit_test_001::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::INVALID;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::string singleId = "1";
    int32_t ret = observerManager->CheckSingleOperationPermissionsAndLimit(uri, singleId);
    EXPECT_EQ(ret, E_PERMISSION_DENIED);
    MEDIA_INFO_LOG("CheckSingleOperationPermissionsAndLimit_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleOperationPermissionsAndLimit_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleOperationPermissionsAndLimit_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::string singleId = "1";
    int32_t ret = observerManager->CheckSingleOperationPermissionsAndLimit(uri, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckSingleOperationPermissionsAndLimit_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, ProcessSingleObserverSingleIds_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_001::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "1";
    ret = observerManager->ProcessSingleObserverSingleIds(uri, true, dataObserver, singleId,
        [](std::unordered_set<std::string>& singleIds, const std::string& id) {
            singleIds.insert(id);
        });
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, ProcessSingleObserverSingleIds_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    std::string singleId = "abc";
    int32_t ret = observerManager->ProcessSingleObserverSingleIds(uri, true, dataObserver, singleId,
        [](std::unordered_set<std::string>& singleIds, const std::string& id) {
            singleIds.insert(id);
        });
    EXPECT_EQ(ret, E_URI_IS_INVALID);
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, ProcessSingleObserverSingleIds_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_003::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    std::string singleId = "1";
    int32_t ret = observerManager->ProcessSingleObserverSingleIds(uri, true, dataObserver, singleId,
        [](std::unordered_set<std::string>& singleIds, const std::string& id) {
            singleIds.insert(id);
        });
    EXPECT_EQ(ret, E_URI_NOT_EXIST);
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserver_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserver_test_001::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::vector<Notification::ObserverInfo> obsInfos;
    bool found = observerManager->FindSingleObserver(uri, obsInfos);
    EXPECT_TRUE(found);
    EXPECT_EQ(obsInfos.size(), 1);
    MEDIA_INFO_LOG("FindSingleObserver_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserver_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserver_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::vector<Notification::ObserverInfo> obsInfos;
    bool found = observerManager->FindSingleObserver(uri, obsInfos);
    EXPECT_FALSE(found);
    MEDIA_INFO_LOG("FindSingleObserver_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, IsSingleIdDataPresentInSingleObserver_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver_test_001::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::unordered_set<std::string> singleIds;
    singleIds.insert("1");
    singleIds.insert("2");
    std::string singleId = "1";
    bool present = observerManager->IsSingleIdDataPresentInSingleObserver(singleIds, singleId);
    EXPECT_TRUE(present);
    MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, IsSingleIdDataPresentInSingleObserver_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver_test_002::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::unordered_set<std::string> singleIds;
    singleIds.insert("1");
    singleIds.insert("2");
    std::string singleId = "3";
    bool present = observerManager->IsSingleIdDataPresentInSingleObserver(singleIds, singleId);
    EXPECT_FALSE(present);
    MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleListenSize_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleListenSize_test_001::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    bool result = observerManager->CheckSingleListenSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleListenSize_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleListenSize_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleListenSize_test_002::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    bool result = observerManager->CheckSingleListenSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleListenSize_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleListenSize_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleListenSize_test_003::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    bool result = observerManager->CheckSingleListenSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleListenSize_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleProcessSize_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_001::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    bool result = observerManager->CheckSingleProcessSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleProcessSize_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_002::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    bool result = observerManager->CheckSingleProcessSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleProcessSize_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_003::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    bool result = observerManager->CheckSingleProcessSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_001::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_002::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::PHOTO_ALBUM_URI);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_003::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_004::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_005, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_005::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::SINGLE_PHOTO_URI);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_006, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_006::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_006::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_007, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_007::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::INVALID);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_007::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_008, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_008::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    std::string singleId = "1";
    int32_t ret = permissionHandle.SinglePermissionCheck(Notification::NotifyUriType::SINGLE_PHOTO_URI, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_008::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_009, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_009::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    std::string singleId = "1";
    int32_t ret =
        permissionHandle.SinglePermissionCheck(Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_009::End");
}

HWTEST_F(NotificationRegisterManagerTest, ObserverCallbackRecipient_test_001, TestSize.Level1) {
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_001::Start");
    sptr<Notification::ObserverCallbackRecipient> callbackRecipient =
        new (std::nothrow) Notification::ObserverCallbackRecipient();
    EXPECT_NE(callbackRecipient, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    wptr<IRemoteObject> object = dataObserver->AsObject();
    callbackRecipient->OnRemoteDied(object);
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_001::End");
}

HWTEST_F(NotificationRegisterManagerTest, ObserverCallbackRecipient_test_002, TestSize.Level1) {
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_002::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    sptr<Notification::ObserverCallbackRecipient> callbackRecipient =
        new (std::nothrow) Notification::ObserverCallbackRecipient();
    EXPECT_NE(callbackRecipient, nullptr);
    wptr<IRemoteObject> object = dataObserver->AsObject();
    callbackRecipient->OnRemoteDied(object);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_002::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObserverWithUri_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->RemoveObserverWithUri(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->RemoveObserverWithUri(uri, dataObserver);
    EXPECT_EQ(ret, E_URI_NOT_EXIST);
    MEDIA_INFO_LOG("RemoveObserverWithUri_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserverWithUri_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    uint32_t callingTokenId = IPCSkeleton::GetCallingTokenID();
    bool found = observerManager->FindSingleObserverWithUri(uri, callingTokenId);
    EXPECT_TRUE(found);
    observerManager->RemoveObserverWithUri(uri, dataObserver);
    found = observerManager->FindSingleObserverWithUri(uri, callingTokenId);
    EXPECT_FALSE(found);
    MEDIA_INFO_LOG("FindSingleObserverWithUri_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, GetObservers_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("GetObservers_test_003::Start");
    Notification::NotifyUriType uri1 = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType uri2 = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri1, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri2, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observers =
        observerManager->GetObservers();
    EXPECT_EQ(observers.size(), 2);
    MEDIA_INFO_LOG("GetObservers_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleOperationPermissionsAndLimit_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleOperationPermissionsAndLimit_test_003::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::string singleId = "1";
    int32_t ret = observerManager->CheckSingleOperationPermissionsAndLimit(uri, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("CheckSingleOperationPermissionsAndLimit_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, ProcessSingleObserverSingleIds_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "1";
    ret = observerManager->ProcessSingleObserverSingleIds(uri, false, dataObserver, singleId,
        [](std::unordered_set<std::string>& singleIds, const std::string& id) {
            singleIds.erase(id);
        });
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, ProcessSingleObserverSingleIds_test_005, TestSize.Level1) {
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_005::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "1";
    ret = observerManager->ProcessSingleObserverSingleIds(uri, true, dataObserver, singleId,
        [](std::unordered_set<std::string>& singleIds, const std::string& id) {
            singleIds.insert(id);
        });
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserver_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserver_test_003::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    std::vector<Notification::ObserverInfo> obsInfos;
    bool found = observerManager->FindSingleObserver(uri, obsInfos);
    EXPECT_TRUE(found);
    EXPECT_EQ(obsInfos.size(), 2);
    MEDIA_INFO_LOG("FindSingleObserver_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, IsSingleIdDataPresentInSingleObserver_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver_test_003::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    std::unordered_set<std::string> singleIds;
    std::string singleId = "1";
    bool present = observerManager->IsSingleIdDataPresentInSingleObserver(singleIds, singleId);
    EXPECT_FALSE(present);
    MEDIA_INFO_LOG("IsSingleIdDataPresentInSingleObserver_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleListenSize_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleListenSize_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    bool result = observerManager->CheckSingleListenSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleListenSize_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleProcessSize_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    bool result = observerManager->CheckSingleProcessSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_013, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_013::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    std::string singleId = "123";
    int32_t ret = permissionHandle.SinglePermissionCheck(Notification::NotifyUriType::SINGLE_PHOTO_URI, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_013::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_014, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_014::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    std::string singleId = "999999";
    int32_t ret =
        permissionHandle.SinglePermissionCheck(Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI, singleId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_014::End");
}

HWTEST_F(NotificationRegisterManagerTest, ObserverCallbackRecipient_test_003, TestSize.Level1) {
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_003::Start");
    sptr<Notification::ObserverCallbackRecipient> callbackRecipient =
        new (std::nothrow) Notification::ObserverCallbackRecipient();
    EXPECT_NE(callbackRecipient, nullptr);
    wptr<IRemoteObject> nullObject;
    callbackRecipient->OnRemoteDied(nullObject);
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_003::End");
}

HWTEST_F(NotificationRegisterManagerTest, ObserverCallbackRecipient_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    sptr<Notification::ObserverCallbackRecipient> callback =
        new (std::nothrow) Notification::ObserverCallbackRecipient();
    EXPECT_NE(callback, nullptr);
    wptr<IRemoteObject> object = dataObserver->AsObject();
    callback->OnRemoteDied(object);
    size_t uriSize = observerManager->observers_.size();
    EXPECT_EQ(uriSize, 0);
    MEDIA_INFO_LOG("ObserverCallbackRecipient_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, RemoveObsDeathRecipient_test_005, TestSize.Level1) {
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_005::Start");
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    size_t obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 2);
    wptr<IRemoteObject> object = dataObserver->AsObject();
    observerManager->RemoveObsDeathRecipient(object);
    obsCallbackSize = observerManager->obsCallbackPecipients_.size();
    EXPECT_EQ(obsCallbackSize, 1);
    MEDIA_INFO_LOG("RemoveObsDeathRecipient_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, GetObservers_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("GetObservers_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    sptr<IDataAbilityObserver> secondDataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(secondDataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    ret = observerManager->AddObserver(uri, secondDataObserver);
    EXPECT_EQ(ret, E_OK);
    std::unordered_map<Notification::NotifyUriType, std::vector<Notification::ObserverInfo>> observers =
        observerManager->GetObservers();
    EXPECT_EQ(observers.size(), 1);
    EXPECT_EQ(observers[uri].size(), 2);
    MEDIA_INFO_LOG("GetObservers_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, FindSingleObserver_test_004, TestSize.Level1) {
    MEDIA_INFO_LOG("FindSingleObserver_test_004::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::vector<Notification::ObserverInfo> obsInfos;
    bool found = observerManager->FindSingleObserver(uri, obsInfos);
    EXPECT_TRUE(found);
    EXPECT_EQ(obsInfos.size(), 1);
    EXPECT_EQ(obsInfos[0].observer, dataObserver);
    MEDIA_INFO_LOG("FindSingleObserver_test_004::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleListenSize_test_005, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleListenSize_test_005::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    bool result = observerManager->CheckSingleListenSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleListenSize_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, CheckSingleProcessSize_test_005, TestSize.Level1) {
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_005::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    bool result = observerManager->CheckSingleProcessSize(uri);
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("CheckSingleProcessSize_test_005::End");
}

HWTEST_F(NotificationRegisterManagerTest, ProcessSingleObserverSingleIds_test_006, TestSize.Level1) {
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_006::Start");
    Notification::NotifyUriType uri = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    auto observerManager = Notification::MediaObserverManager::GetObserverManager();
    EXPECT_NE(observerManager, nullptr);
    sptr<IDataAbilityObserver> dataObserver = new (std::nothrow) IDataAbilityObserverTest();
    EXPECT_NE(dataObserver, nullptr);
    int32_t ret = observerManager->AddObserver(uri, dataObserver);
    EXPECT_EQ(ret, E_OK);
    std::string singleId = "1234567890";
    ret = observerManager->ProcessSingleObserverSingleIds(uri, true, dataObserver, singleId,
        [](std::unordered_set<std::string>& singleIds, const std::string& id) {
            singleIds.insert(id);
        });
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("ProcessSingleObserverSingleIds_test_006::End");
}

HWTEST_F(NotificationRegisterManagerTest, NotifyRegisterPermission_test_015, TestSize.Level1) {
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_015::Start");
    Notification::NotifyRegisterPermission permissionHandle;
    int32_t ret = permissionHandle.ExecuteCheckPermission(Notification::NotifyUriType::INVALID);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("NotifyRegisterPermission_test_015::End");
}
}
}