/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SCANNER_MANAGER_H
#define MEDIA_SCANNER_MANAGER_H

#include <memory>

#include "media_scanner.h"
#include "media_scan_executor.h"
#include "medialibrary_errno.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaScannerManager final {
public:
    EXPORT static std::shared_ptr<MediaScannerManager> GetInstance();

    EXPORT virtual ~MediaScannerManager() = default;

    void Start();
    void Stop();
    void ScanError();
    void ErrorRecord(const std::string &path = ROOT_MEDIA_DIR);

    EXPORT int32_t ScanFile(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback,
        MediaLibraryApi api = MediaLibraryApi::API_OLD, bool isCameraShotMovingPhoto = false);
    int32_t ScanFileSync(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback,
        MediaLibraryApi api = MediaLibraryApi::API_OLD, bool isForceScan = false, int32_t fileId = 0);
    int32_t ScanFileSyncWithoutAlbumUpdate(const std::string &path,
        const std::shared_ptr<IMediaScannerCallback> &callback, MediaLibraryApi api = MediaLibraryApi::API_OLD,
        bool isForceScan = false, int32_t fileId = 0);
    EXPORT int32_t ScanDir(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback);
    EXPORT int32_t ScanDirSync(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback);
private:
    MediaScannerManager() = default;

    static std::shared_ptr<MediaScannerManager> instance_;
    static std::mutex instanceMutex_;

    MediaScanExecutor executor_;
};
} // namespace Media
} // namespace OHOS

#endif // MEDIA_SCANNER_MANAGER_H