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
#include "medialibrary_subscriber.h"
#include "moving_photo_processor.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
constexpr int32_t SLEEP_TIME = 1;
HWTEST_F(MediaLibraryRdbTest, medialib_Subscribe_test_001, TestSize.Level0)
{
    bool ret = MedialibrarySubscriber::Subscribe();
    EXPECT_EQ(ret, true);
}

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_001, TestSize.Level0)
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

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_002, TestSize.Level0)
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

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_003, TestSize.Level0)
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

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_004, TestSize.Level0)
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

HWTEST_F(MediaLibraryRdbTest, medialib_OnReceiveEvent_test_005, TestSize.Level0)
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

HWTEST_F(MediaLibraryRdbTest, medialib_MovingPhotoProcessor_test_001, TestSize.Level0)
{
    MovingPhotoProcessor::isProcessing_ = true;
    MovingPhotoProcessor::StartProcess();
    EXPECT_EQ(MovingPhotoProcessor::isProcessing_, false); // no moving photo to process
    MovingPhotoProcessor::StopProcess();
    EXPECT_EQ(MovingPhotoProcessor::isProcessing_, false);
}
} // namespace Media
} // namespace OHOS