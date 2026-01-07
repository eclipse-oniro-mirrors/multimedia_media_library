/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SCAN_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_SCAN_SERVICE_H

#include <string>
#include <vector>

#include "metadata.h"

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaScanService {
public:
    class ScanResult {
    public:
        bool scanSuccess = false;
        std::string shootingMode;
        std::string shootingModeTag;
        std::string frontCamera;
        int32_t subType;
        std::string filePath;
        int32_t hdrMode;
        int64_t dateTaken{0};
        std::string detailTime;
        std::string dateYear;
        std::string dateMonth;
        std::string dateDay;

    public:
        std::string ToString() const;
    };

public:
    int32_t FillMetadata(std::unique_ptr<Metadata>& data);
    int32_t ScanMetaData(const std::string& path, std::unique_ptr<Metadata>& data);
    int32_t ScanDownloadedFile(const std::string& path, CloudMediaScanService::ScanResult& result);
    void UpdateAndNotifyShootingModeAlbumIfNeeded(const CloudMediaScanService::ScanResult& scanResult);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DATA_SERVICE_H