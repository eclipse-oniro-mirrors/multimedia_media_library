/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_CLOUD_DOWNLOAD_CALLBACK_H
#define OHOS_MEDIALIBRARY_CLOUD_DOWNLOAD_CALLBACK_H

#include "cloud_download_callback.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using namespace FileManagement::CloudSync;
class CloudMediaAssetDownloadOperation;

class MediaCloudDownloadCallback : public CloudDownloadCallback {
public:
    MediaCloudDownloadCallback(std::shared_ptr<CloudMediaAssetDownloadOperation> operation) : operation_(operation) {}
    ~MediaCloudDownloadCallback() {}
    void OnDownloadProcess(const DownloadProgressObj& progress);

private:
    std::shared_ptr<CloudMediaAssetDownloadOperation> operation_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_CLOUD_DOWNLOAD_CALLBACK_H