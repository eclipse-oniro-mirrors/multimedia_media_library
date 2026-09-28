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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_HANDLER_TEST_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_HANDLER_TEST_H

#include "gtest/gtest.h"

#include "database_data_mock.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::TestUtils;
/**
 * 共享相册下行(相册侧)与 OnCompletePull 收尾流程用例.
 *
 * 隔离方式: SetUpTestCase 里 CheckPoint(), TearDownTestCase 里按 cloud_id 前缀清理 + Rollback(),
 * 造数只使用 share_tdd_ 前缀, 不会影响既有用例.
 */
class CloudMediaShareAlbumHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

private:
    static DatabaseDataMock dbDataMock_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SHARE_ALBUM_HANDLER_TEST_H
