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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_URI_UTILS_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_URI_UTILS_H_

#include "pixel_map.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class ThumbnailUriUtils {
public:
    EXPORT ThumbnailUriUtils() = delete;
    EXPORT virtual ~ThumbnailUriUtils() = delete;
    EXPORT static bool ParseFileUri(const std::string &uriString, std::string &outFileId,
        std::string &outNetworkId, std::string &outTableName);
    EXPORT static bool ParseThumbnailInfo(const std::string &uriString, std::string &outFileId,
        Size &outSize, std::string &outNetworkId, std::string &outTableName);
    EXPORT static bool ParseKeyFrameThumbnailInfo(const std::string &uriString, std::string &outFileId,
        int32_t &outBeginStamp, int32_t &outType, std::string &outPath);
    EXPORT static std::string GetDateTakenFromUri(const std::string &uri);
    EXPORT static std::string GetDateModifiedFromUri(const std::string &uri);
    EXPORT static std::string GetFileUriFromUri(const std::string &uri);
private:
    EXPORT static bool IsOriginalImg(const Size &outSize, const std::string &outPath);
    EXPORT static bool CheckSize(Size &outSize, const std::string &outPath);
    EXPORT static std::string GetTableFromUri(const std::string &uri);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_URI_UTILS_H_
