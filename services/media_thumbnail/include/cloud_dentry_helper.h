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

#ifndef OHOS_MEDIA_THUMBNAIL_CLOUD_DENTRY_HELPER_H
#define OHOS_MEDIA_THUMBNAIL_CLOUD_DENTRY_HELPER_H

#include <functional>
#include <string>

#include "thumbnail_const.h"

namespace OHOS::FileManagement::CloudSync {
struct DentryFileInfo;
}

namespace NativeRdb {
class ResultSet;
}

namespace OHOS::Media {

class CloudDentryHelper {
public:
    ~CloudDentryHelper() = default;

    /**
     * @brief Check if dentry needs to be created for the given file path
     * @param filePath The file path to check
     * @return true if dentry needs to be created, false otherwise
     */
    EXPORT static bool NeedCreateDentry(const std::string &filePath);

    /**
     * @brief Create dentry for a specific file
     * @param cloudId The cloud ID of the photo
     * @param filePath The file path for dentry
     * @param fileName The file name for dentry
     * @param fileType The file type (DENTRY_INFO_LCD, DENTRY_INFO_THM, etc.)
     * @return E_OK on success, error code on failure
     */
    EXPORT static int32_t CreateDentryForThumbnail(const std::string &fileId,
        const std::string &originalPath, ThumbnailType thumbType);

    /**
     * @brief Create dentry for cloud photo by file ID
     * @param fileId The file ID in database
     * @param filePath The file path for dentry
     * @return E_OK on success, error code on failure
     */
    EXPORT static int32_t CreateDentryForOrigin(const std::string &fileId,
        const std::string &filePath);

private:
    static int32_t CreateDentryInternal(const std::string &fileId,
        const std::string &fileName, int64_t size, const std::string &fileType);
};

}  // namespace OHOS::Media::Thumbnail

#endif  // OHOS_MEDIA_THUMBNAIL_CLOUD_DENTRY_HELPER_H