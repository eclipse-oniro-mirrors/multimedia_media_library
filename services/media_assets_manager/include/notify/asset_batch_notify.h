/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ASSET_BATCH_NOTIFY_H
#define OHOS_MEDIA_ASSET_BATCH_NOTIFY_H

#include <stdint.h>
#include <string>
#include <vector>

#include "medialibrary_errno.h"

// LCOV_EXCL_START
namespace OHOS::Media {

// 分批通知资产变更，凑满一批后统一刷新相册
class AssetBatchNotify {
public:
    int32_t TryNotifyAssetsChange(const std::vector<std::string> &fileIds);
    int32_t FinalNotifyAssetsChange();

private:
    int32_t TryUpdateAllAlbums();
    // 逐条通知指定资产移除，并触发精准刷新与分析相册重查
    void NotifyAssetsChange(const std::vector<std::string> &notifyFileIds);

private:
    std::vector<std::string> fileIds_;
    std::int32_t totalFileCount_ = 0;
    const int32_t BATCH_NOTIFY_CLOUD_FILE = 2000;
};
} // namespace OHOS::Media
// LCOV_EXCL_STOP
#endif // OHOS_MEDIA_ASSET_BATCH_NOTIFY_H
