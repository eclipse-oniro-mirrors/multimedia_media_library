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

#ifndef OHOS_MEDIA_MOCK_CLOUD_SYNC_UTILS_H
#define OHOS_MEDIA_MOCK_CLOUD_SYNC_UTILS_H

namespace OHOS::Media {

// 云同步工具类的测试桩开关: 由用例设置, 用于驱动共享相册端云同步开关的 true/false 分支
// 实现见 mock_cloud_sync_utils.cpp
class MockShareAlbumSyncSwitch {
public:
    MockShareAlbumSyncSwitch() = delete;
    ~MockShareAlbumSyncSwitch() = delete;

    static void SetSwitchOn(bool switchOn);
    static bool IsSwitchOn();
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_MOCK_CLOUD_SYNC_UTILS_H