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

#ifndef MEDIA_LIBRARY_MANAGER_NOTIFY_TEST_H
#define MEDIA_LIBRARY_MANAGER_NOTIFY_TEST_H

#include "media_library_manager_test.h"
#include "media_library_notify_callback.h"

namespace OHOS {
namespace Media {
class MockPhotoAlbumChangeCallback final : public PhotoAlbumChangeCallback {
public:
    void OnChange(const AlbumChangeInfos &changeInfos) override
    {
        changeInfos_ = changeInfos;
        callTimes_++;
    }

    int32_t GetCallTimes() const
    {
        return callTimes_;
    }

    AlbumChangeInfos GetChangeInfos() const
    {
        return changeInfos_;
    }

private:
    int32_t callTimes_ = 0;
    AlbumChangeInfos changeInfos_;
};

class MockPhotoAssetChangeCallback final : public PhotoAssetChangeCallback {
public:
    void OnChange(const PhotoAssetChangeInfos &changeInfos) override
    {
        changeInfos_ = changeInfos;
        callTimes_++;
    }

    int32_t GetCallTimes() const
    {
        return callTimes_;
    }

    PhotoAssetChangeInfos GetChangeInfos() const
    {
        return changeInfos_;
    }

private:
    int32_t callTimes_ = 0;
    PhotoAssetChangeInfos changeInfos_;
};
} // namespace Media
} // namespace OHOS

#endif // MEDIA_LIBRARY_MANAGER_NOTIFY_TEST_H