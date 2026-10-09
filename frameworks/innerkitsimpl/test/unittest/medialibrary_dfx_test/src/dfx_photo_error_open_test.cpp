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

#include <gtest/gtest.h>
#include <map>

#include "dfx_collector.h"
#include "dfx_manager.h"
#include "dfx_photo_error_helper.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {

class DfxPhotoErrorOpenTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        DfxManager::GetInstance();
    }
    static void TearDownTestCase(void) {}
    void SetUp()
    {
        auto mgr = DfxManager::GetInstance();
        ASSERT_NE(mgr, nullptr);
        mgr->isInitSuccess_ = true;
        if (mgr->dfxCollector_ != nullptr) {
            mgr->dfxCollector_->GetPhotoError();
        }
    }
    void TearDown() {}
};

HWTEST_F(DfxPhotoErrorOpenTest, handle_photo_error_collects_open_create_empty_file, TestSize.Level0)
{
    auto mgr = DfxManager::GetInstance();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(mgr->dfxCollector_, nullptr);

    PhotoErrorData data{};
    data.fileId = 42;
    data.fileSourceType = 0;
    data.southDeviceType = 0;
    data.position = 1;
    data.mediaType = 1;
    data.path = "/test/open_empty.jpg";
    data.displayName = "open_empty.jpg";

    mgr->HandlePhotoError(data);

    auto collected = mgr->dfxCollector_->GetPhotoError();
    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(collected.begin()->second, 1);
    EXPECT_EQ(collected.begin()->first % MEDIA_TYPE_BASE,
        static_cast<int32_t>(PhotoErrorType::OPEN_CREATE_EMPTY_FILE));
}

HWTEST_F(DfxPhotoErrorOpenTest, handle_photo_error_lake_video_encodes_correctly, TestSize.Level0)
{
    auto mgr = DfxManager::GetInstance();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(mgr->dfxCollector_, nullptr);

    PhotoErrorData data{};
    data.fileId = 99;
    data.fileSourceType = 3;
    data.southDeviceType = 2;
    data.position = 3;
    data.mediaType = 2;
    data.path = "/lake/video.mp4";
    data.displayName = "video.mp4";

    mgr->HandlePhotoError(data);

    auto collected = mgr->dfxCollector_->GetPhotoError();
    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(collected.begin()->second, 1);
    EXPECT_EQ(collected.begin()->first % MEDIA_TYPE_BASE,
        static_cast<int32_t>(PhotoErrorType::OPEN_CREATE_EMPTY_FILE));
    EXPECT_EQ(collected.begin()->first / FILE_SOURCE_TYPE_BASE, data.fileSourceType);
}

} // namespace Media
} // namespace OHOS
