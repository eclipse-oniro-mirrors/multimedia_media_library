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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORKING_UTIL_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORKING_UTIL_H

#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaFileInterworkUtil {
public:
    EXPORT static int32_t GetFileAlbumLPath(const std::string &path, std::string &lPath);
    EXPORT static std::string GetLowerString(const std::string &str);
    EXPORT static int32_t InsertOrUpdateAlbum(const std::string &albumPath, int32_t &albumId);
    EXPORT static int32_t GetScannerTaskStatus();
    EXPORT static int32_t SetScannerTaskStatus(int32_t status);
    EXPORT static int32_t SetLoadFirstTime();
    EXPORT static int32_t ReportFileManagerFirstLoad();
    EXPORT static int32_t AddImageAndVideoCount(int32_t imageCount, int32_t videoCount);
    EXPORT static int32_t AddAlbumCount(int32_t albumCount);
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_INTERWORKING_UTIL_H