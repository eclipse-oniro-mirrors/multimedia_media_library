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

#include "mock_cloud_sync_utils.h"

#include "cloud_sync_utils.h"

namespace OHOS::Media {
namespace {
// 由用例控制共享相册端云同步开关状态
bool g_shareAlbumSyncSwitchOn = false;
} // namespace

void MockShareAlbumSyncSwitch::SetSwitchOn(bool switchOn)
{
    g_shareAlbumSyncSwitchOn = switchOn;
}

bool MockShareAlbumSyncSwitch::IsSwitchOn()
{
    return g_shareAlbumSyncSwitchOn;
}

CloudSyncUtils::CloudSyncUtils()
{
}

CloudSyncUtils::~CloudSyncUtils()
{
}

bool CloudSyncUtils::IsCloudSyncSwitchOn()
{
    return true;
}

bool CloudSyncUtils::IsUnlimitedTrafficStatusOn()
{
    return true;
}

bool CloudSyncUtils::IsSwitchOn(const std::string &bundleName)
{
    (void)bundleName;
    return MockShareAlbumSyncSwitch::IsSwitchOn();
}

bool CloudSyncUtils::IsSharedAlbumCloudSyncSwitchOn()
{
    return MockShareAlbumSyncSwitch::IsSwitchOn();
}

} // namespace OHOS::Media