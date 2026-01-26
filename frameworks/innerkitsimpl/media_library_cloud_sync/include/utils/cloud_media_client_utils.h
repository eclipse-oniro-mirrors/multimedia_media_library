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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CLIENT_UTILS_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CLIENT_UTILS_H

#include <map>
#include <vector>

#include "cloud_mdkrecord_photos_vo.h"

namespace OHOS::Media::CloudSync {
class CloudMediaClientUtils {
public:
    static std::string GetLowerPath(const std::string &path, int32_t userId);
    static int32_t GetLocalPathByPhotosVo(const CloudMdkRecordPhotosVo &photosVo, std::string &localPath,
        int32_t userId);
    static std::string GetLocalPath(const std::string &path);
    static std::string FindLocalPathFromCloudPath(const std::string &path, int32_t userId);
    static std::string GetVideoCachePath(const std::string &filePath);
    static void InvalidVideoCache(const std::string &localPath);

private:
    static std::string AppendUserId(const std::string &path, int32_t userId);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_CLIENT_UTILS_H
