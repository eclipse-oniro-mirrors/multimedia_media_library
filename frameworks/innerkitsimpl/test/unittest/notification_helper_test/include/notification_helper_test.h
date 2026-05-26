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

#ifndef NOTIFICATION_HELPER_TEST_H
#define NOTIFICATION_HELPER_TEST_H

#include "gtest/gtest.h"
#include "notification_helper.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
namespace NotificationHelper {

// Mock callback class for testing
class MockPhotoAlbumChangeCallback : public PhotoAlbumChangeCallback {
public:
    MockPhotoAlbumChangeCallback() : callCount_(0), lastChangeType_(NotifyChangeType::NOTIFY_CHANGE_INVALID) {}
    ~MockPhotoAlbumChangeCallback() = default;

    int32_t OnChange(AlbumChangeInfos info) override
    {
        callCount_++;
        lastChangeType_ = info.type;
        lastInfo_ = info;
        return NOTIFY_OK;
    }

    int32_t GetCallCount() const
    {
        return callCount_;
    }

    NotifyChangeType GetLastChangeType() const
    {
        return lastChangeType_;
    }

    AlbumChangeInfos GetLastInfo() const
    {
        return lastInfo_;
    }

    void Reset()
    {
        callCount_ = 0;
        lastChangeType_ = NotifyChangeType::NOTIFY_CHANGE_INVALID;
    }

private:
    int32_t callCount_;
    NotifyChangeType lastChangeType_;
    AlbumChangeInfos lastInfo_;
};

} // namespace NotificationHelper

class NotificationHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

} // namespace Media
} // namespace OHOS

#endif // NOTIFICATION_HELPER_TEST_H