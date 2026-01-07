/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include <cstdint>

#include "medialibrary_rdb_test.h"
#include "medialibrary_object_utils.h"
#define private public
#define MEDIALIBRARY_MTP_ENABLE
#include "medialibrary_subscriber.h"
#undef MEDIALIBRARY_MTP_ENABLE
#include "moving_photo_processor.h"
#include "medialibrary_subscriber_database_utils.h"
#undef private
#include "media_file_utils.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
constexpr int32_t SLEEP_TIME = 1;
static constexpr int32_t EVENTTYPE = 7;
static const std::string DATA_CLONE_DESCRIPTION_JSON =
    PhotoColumn::FILES_LOCAL_DIR + ".backup/restore/dataclone_description.json";
const static std::string CLONE_FOLDER_PATH = PhotoColumn::FILES_LOCAL_DIR + ".backup/clone";
const static std::string RESTORE_FOLDER_PATH = PhotoColumn::FILES_LOCAL_DIR + ".backup/restore";
const string CLOUD_UPDATE_EVENT = "usual.event.DUE_HAP_CFG_UPDATED";
const string CLOUD_EVENT_INFO_TYPE = "type";
const string CLOUD_EVENT_INFO_TYPE_VALUE = "medialibrary_kit_whitelist";
const string CLOUD_EVENT_INFO_TYPE_INVALID_VALUE = "invalidType";
HWTEST_F(MediaLibraryRdbTest, medialib_Subscribe_test_001, TestSize.Level1)
{
    bool ret = MedialibrarySubscriber::Subscribe();
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_001, TestSize.Level1)
{
    MedialibrarySubscriber medialibrarySubscriber;
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    medialibrarySubscriber.AbortCommonEvent();
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_002, TestSize.Level1)
{
    MedialibrarySubscriber medialibrarySubscriber;
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), EventFwk::CommonEventSupport::COMMON_EVENT_POWER_CONNECTED);
    medialibrarySubscriber.AbortCommonEvent();
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_003, TestSize.Level1)
{
    MedialibrarySubscriber medialibrarySubscriber;
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    medialibrarySubscriber.AbortCommonEvent();
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_004, TestSize.Level1)
{
    MedialibrarySubscriber medialibrarySubscriber;
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_TIME_CHANGED;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), EventFwk::CommonEventSupport::COMMON_EVENT_TIME_CHANGED);
    medialibrarySubscriber.AbortCommonEvent();
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_005, TestSize.Level1)
{
    MedialibrarySubscriber medialibrarySubscriber;
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), EventFwk::CommonEventSupport::COMMON_EVENT_POWER_DISCONNECTED);
    medialibrarySubscriber.DoBackgroundOperation();
    medialibrarySubscriber.StopBackgroundOperation();
    medialibrarySubscriber.AbortCommonEvent();
    sleep(SLEEP_TIME);
}

HWTEST_F(MediaLibraryRdbTest, medialib_MovingPhotoProcessor_test_001, TestSize.Level1)
{
    MovingPhotoProcessor::StartProcess();
    EXPECT_EQ(MovingPhotoProcessor::isProcessing_, false); // no moving photo to process
    MovingPhotoProcessor::StopProcess();
    EXPECT_EQ(MovingPhotoProcessor::isProcessing_, false);
}

HWTEST_F(MediaLibraryRdbTest, medialib_QueryThumbAstc_test, TestSize.Level1)
{
    int count = 0;
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    EXPECT_NE(medialibrarySubscriberPtr, nullptr);

    medialibrarySubscriberPtr->DoThumbnailBgOperation();
    medialibrarySubscriberPtr->WalCheckPointAsync();
    MedialibrarySubscriberDatabaseUtils::QueryThumbAstc(count);
    MedialibrarySubscriberDatabaseUtils::QueryThumbTotal(count);
}

HWTEST_F(MediaLibraryRdbTest, medialib_UpdateBackgroundOperationStatus_test, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    EXPECT_NE(medialibrarySubscriberPtr, nullptr);
    EventFwk::CommonEventData eventData;
    AAFwk::Want want = eventData.GetWant();
    std::array<StatusEventType, EVENTTYPE> events = {{
        StatusEventType::CHARGING,
        StatusEventType::DISCHARGING,
        StatusEventType::SCREEN_OFF,
        StatusEventType::SCREEN_ON,
        StatusEventType::BATTERY_CHANGED,
        StatusEventType::THERMAL_LEVEL_CHANGED,
        StatusEventType::TIME_TICK
    }};

    for (const auto& event : events) {
        medialibrarySubscriberPtr->UpdateBackgroundOperationStatus(want, event);
    }
    medialibrarySubscriberPtr->UpdateBackgroundOperationStatus(want, static_cast<StatusEventType>(100));
}

HWTEST_F(MediaLibraryRdbTest, medialib_WalCheckPointAsync_test, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    medialibrarySubscriberPtr->isScreenOff_ = true;
    medialibrarySubscriberPtr->isCharging_ = true;
    medialibrarySubscriberPtr->WalCheckPointAsync();
    EXPECT_NE(medialibrarySubscriberPtr->isScreenOff_, false);
    EXPECT_NE(medialibrarySubscriberPtr->isCharging_, false);
}

HWTEST_F(MediaLibraryRdbTest, medialib_UpdateCloudMediaAssetDownloadStatus_test, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    AAFwk::Want want;
    StatusEventType statusEventType = StatusEventType::THERMAL_LEVEL_CHANGED;
    medialibrarySubscriberPtr->UpdateCloudMediaAssetDownloadStatus(want, statusEventType);
    EXPECT_EQ(medialibrarySubscriberPtr->isCharging_, false);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_006, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_CONN_STATE;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    eventData.SetCode(4);
    medialibrarySubscriberPtr->OnReceiveEvent(eventData);
    EXPECT_EQ(MedialibrarySubscriber::isWifiConnected_, true);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_007, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_CONNECTIVITY_CHANGE;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
#define MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    medialibrarySubscriberPtr->OnReceiveEvent(eventData);
#undef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    EXPECT_EQ(MedialibrarySubscriber::isWifiConnected_, true);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_008, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriberPtr->OnReceiveEvent(eventData);
    EXPECT_EQ(MedialibrarySubscriber::isWifiConnected_, true);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_009, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    EventFwk::CommonEventData eventData;
    string action = EventFwk::CommonEventSupport::COMMON_EVENT_HWID_LOGOUT;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriberPtr->OnReceiveEvent(eventData);
    EXPECT_EQ(MedialibrarySubscriber::isWifiConnected_, true);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_0010, TestSize.Level1)
{
    MedialibrarySubscriber medialibrarySubscriber;
    EventFwk::CommonEventData eventData;
    string action = CLOUD_UPDATE_EVENT;
    AAFwk::Want want = eventData.GetWant();
    want.SetAction(action);
    want.SetParam(CLOUD_EVENT_INFO_TYPE, CLOUD_EVENT_INFO_TYPE_VALUE);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), CLOUD_UPDATE_EVENT);
    action = "invalidAction";
    want.SetAction(action);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), "invalidAction");
    action = CLOUD_UPDATE_EVENT;
    want.SetAction(action);
    want.SetParam(CLOUD_EVENT_INFO_TYPE, CLOUD_EVENT_INFO_TYPE_INVALID_VALUE);
    eventData.SetWant(want);
    medialibrarySubscriber.OnReceiveEvent(eventData);
    EXPECT_EQ(want.GetAction(), CLOUD_UPDATE_EVENT);
    medialibrarySubscriber.AbortCommonEvent();
}

HWTEST_F(MediaLibraryRdbTest, medialib_GetNowTime_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    int64_t ret = -1;
    ret = medialibrarySubscriberPtr->GetNowTime();
    EXPECT_NE(ret, -1);
}

HWTEST_F(MediaLibraryRdbTest, medialib_Init_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    int64_t ret = medialibrarySubscriberPtr->GetNowTime();
    medialibrarySubscriberPtr->Init();
    bool ret2 = (ret > medialibrarySubscriberPtr->lockTime_);
    EXPECT_EQ(medialibrarySubscriberPtr->agingCount_, 0);
    EXPECT_EQ(ret2, false);
}

HWTEST_F(MediaLibraryRdbTest, medialib_IsClearContinueCloneData_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    EXPECT_TRUE(medialibrarySubscriberPtr->IsClearContinueCloneData(""));
    EXPECT_TRUE(medialibrarySubscriberPtr->IsClearContinueCloneData(DATA_CLONE_DESCRIPTION_JSON));
}

HWTEST_F(MediaLibraryRdbTest, medialib_ClearContinueCloneData_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    medialibrarySubscriberPtr->TryClearContinueCloneData();
    medialibrarySubscriberPtr->TryClearContinueCloneData();
}

HWTEST_F(MediaLibraryRdbTest, medialib_DoClearContinueCloneData_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    auto ret = medialibrarySubscriberPtr->DoClearContinueCloneData();
    EXPECT_EQ(ret, E_SUCCESS);
}

HWTEST_F(MediaLibraryRdbTest, medialib_ClearContinueCloneData_test_002, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    AsyncTaskData data;
    medialibrarySubscriberPtr->ClearContinueCloneData(&data);
}

HWTEST_F(MediaLibraryRdbTest, medialib_AgingTmpCompatibleDuplicates_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("AgingTmpCompatibleDuplicates_test_001 start");
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    medialibrarySubscriberPtr->AgingTmpCompatibleDuplicates(true);
    medialibrarySubscriberPtr->AgingTmpCompatibleDuplicates(false);
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
}

HWTEST_F(MediaLibraryRdbTest, medialib_DoAgingOperation_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    medialibrarySubscriberPtr->DoAgingOperation();
    int64_t ret = -1;
    ret = medialibrarySubscriberPtr->GetNowTime();
    EXPECT_NE(ret, -1);
}

HWTEST_F(MediaLibraryRdbTest, medialib_StopThumbnailBgOperation_test_001, TestSize.Level1)
{
    shared_ptr<MedialibrarySubscriber> medialibrarySubscriberPtr = make_shared<MedialibrarySubscriber>();
    ASSERT_NE(medialibrarySubscriberPtr, nullptr);
    medialibrarySubscriberPtr->StopThumbnailBgOperation();
    medialibrarySubscriberPtr->GetNowTime();
    EXPECT_EQ(medialibrarySubscriberPtr->agingCount_, 0);
}
} // namespace Media
} // namespace OHOS