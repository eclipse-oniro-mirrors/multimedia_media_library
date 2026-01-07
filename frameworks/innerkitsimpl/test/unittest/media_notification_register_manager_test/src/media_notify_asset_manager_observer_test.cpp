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
#define MLOG_TAG "NotifyAssetManagerObserverTest"
 
#include "media_notify_asset_manager_observer_test.h"
#include "medialibrary_notify_asset_manager_observer.h"
#include "medialibrary_notify_new_observer.h"
#include "media_notification_utils.h"
 
#include "napi/native_api.h"
#include "parcel.h"
#include "media_change_info.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify_utils.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
 
static auto REGISTER_URI_TYPE = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
static auto REGISTER_URI = "";
static auto g_env = nullptr;
static auto g_ref = nullptr;
    
static auto observer = make_shared<MediaOnNotifyAssetManagerObserver>(REGISTER_URI_TYPE, REGISTER_URI, g_env);
const std::vector<std::shared_ptr<ClientObserver>> clientObserver = {
    make_shared<ClientObserver>(REGISTER_URI_TYPE, g_ref)
};

void NotifyAssetManagerObserverTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("NotifyAssetManagerObserverTest SetUpTest");
}
 
void NotifyAssetManagerObserverTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("NotifyAssetManagerObserverTest TearDownTest");
}
 
void NotifyAssetManagerObserverTest::SetUp(void)
{
    MEDIA_INFO_LOG("NotifyAssetManagerObserverTest SetUp");
}
 
void NotifyAssetManagerObserverTest::TearDown(void)
{
    MEDIA_INFO_LOG("NotifyAssetManagerObserverTest TearDown");
}
 
shared_ptr<Parcel> GetChangeInfoData()
{
    shared_ptr<Parcel> parcel = std::make_shared<Parcel>();
    int fileId = 3;
    int progress = 20;
    int autoPauseReason = -1;
    parcel->WriteUint16(static_cast<uint16_t>(Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI));
    parcel->WriteUint16(static_cast<uint16_t>(Notification::DownloadAssetsNotifyType::DOWNLOAD_ASSET_DELETE));
    parcel->WriteInt32(static_cast<int32_t>(fileId));
    parcel->WriteInt32(static_cast<int32_t>(progress));
    parcel->WriteInt32(static_cast<int32_t>(autoPauseReason));
    return parcel;
}
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_OnChange_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_OnChange_Test_001 Start");
    shared_ptr<Parcel> parcel = GetChangeInfoData();
    uintptr_t buf = parcel->GetData();
    auto *uBuf = new (std::nothrow) uint8_t[parcel->GetDataSize()];
    EXPECT_NE(uBuf, nullptr);
    memcpy_s(uBuf, parcel->GetDataSize(), reinterpret_cast<uint8_t *>(buf), parcel->GetDataSize());
    OHOS::DataShare::DataShareObserver::ChangeInfo changeInfo;
    changeInfo.data_ = uBuf;
    changeInfo.size_ = parcel->GetDataSize();
    observer->OnChange(changeInfo);
    MEDIA_INFO_LOG("Namot_OnChange_Test_001 End");
}
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_OnChangeForBatchDownloadProgress_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_OnChangeForBatchDownloadProgress_Test_001 Start");
    NewJsOnChangeCallbackWrapper callbackWrapper;
    shared_ptr<Parcel> parcel = GetChangeInfoData();
    callbackWrapper.assetManagerInfo_ = NotificationUtils::UnmarshalAssetManagerNotify(*parcel);
    EXPECT_NE(callbackWrapper.assetManagerInfo_, nullptr);
 
    Notification::NotifyUriType infoUriType = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
    callbackWrapper.env_ = g_env;
    callbackWrapper.observerUriType_ = infoUriType;
    callbackWrapper.clientObservers_ = clientObserver;
    observer->OnChangeForBatchDownloadProgress(callbackWrapper);
    MEDIA_INFO_LOG("Namot_OnChangeForBatchDownloadProgress_Test_001 End");
}
 
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_GetAssetManagerNotifyTypeAndUri_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_GetAssetManagerNotifyTypeAndUri_Test_001 Start");
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    const Notification::NotifyUriType uriType = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
    string uri;
    int32_t ret = MediaLibraryNotifyUtils::GetAssetManagerNotifyTypeAndUri(uriType, registerUriType, uri);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("Namot_GetAssetManagerNotifyTypeAndUri_Test_001 End");
}
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_BuildFileIdPercentSubInfos_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_BuildFileIdPercentSubInfos_Test_001 Start");
    shared_ptr<Parcel> parcel = GetChangeInfoData();
    shared_ptr<Notification::AssetManagerNotifyInfo> info = NotificationUtils::UnmarshalAssetManagerNotify(*parcel);
    napi_value result = nullptr;
    auto ret = MediaLibraryNotifyUtils::BuildFileIdPercentSubInfos(g_env, info, result);
    EXPECT_EQ(ret, napi_invalid_arg);
    MEDIA_INFO_LOG("Namot_BuildFileIdPercentSubInfos_Test_001 End");
}
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_BuildFileIdSubInfos_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_BuildFileIdSubInfos_Test_001 Start");
    shared_ptr<Parcel> parcel = GetChangeInfoData();
    shared_ptr<Notification::AssetManagerNotifyInfo> info = NotificationUtils::UnmarshalAssetManagerNotify(*parcel);
    napi_value result = nullptr;
    auto ret = MediaLibraryNotifyUtils::BuildFileIdSubInfos(g_env, info, result);
    EXPECT_EQ(ret, napi_invalid_arg);
    MEDIA_INFO_LOG("Namot_BuildFileIdSubInfos_Test_001 End");
}
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_BuildPauseReasonSubInfos_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_BuildPauseReasonSubInfos_Test_001 Start");
    shared_ptr<Parcel> parcel = GetChangeInfoData();
    shared_ptr<Notification::AssetManagerNotifyInfo> info = NotificationUtils::UnmarshalAssetManagerNotify(*parcel);
    napi_value result = nullptr;
    auto ret = MediaLibraryNotifyUtils::BuildPauseReasonSubInfos(g_env, info, result);
    EXPECT_EQ(ret, napi_invalid_arg);
    MEDIA_INFO_LOG("Namot_BuildPauseReasonSubInfos_Test_001 End");
}
 
HWTEST_F(NotifyAssetManagerObserverTest, Namot_BuildBatchDownloadProgressInfos_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Namot_BuildBatchDownloadProgressInfos_Test_001 Start");
    shared_ptr<Parcel> parcel = GetChangeInfoData();
    shared_ptr<Notification::AssetManagerNotifyInfo> info = NotificationUtils::UnmarshalAssetManagerNotify(*parcel);
    auto ret = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(g_env, info);
    EXPECT_EQ(ret, nullptr);
    MEDIA_INFO_LOG("Namot_BuildBatchDownloadProgressInfos_Test_001 End");
}
}
}