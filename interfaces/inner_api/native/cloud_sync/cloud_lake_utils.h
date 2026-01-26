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
#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_LAKE_UTILS_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_LAKE_UTILS_H

#include <string>
#include <sstream>

namespace OHOS {
namespace Media {
class CloudLakeUtils {
public:
    static std::string GetAbsoluteLakeDir(int32_t userId)
    {
        return "/mnt/data/" + std::to_string(userId) + "/HO_MEDIA/";
    }

    static bool StartsWith(const std::string &str, const std::string &prefix)
    {
        return str.compare(0, prefix.size(), prefix) == 0;
    }

    // 提供给端云使用
    static std::string GetAbsoluteLakePath(const std::string &storagePath, int32_t userId)
    {
        std::string lakeDir = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";
        if (!StartsWith(storagePath, lakeDir)) {
            return "";
        }

        return GetAbsoluteLakeDir(userId) + storagePath.substr(lakeDir.length());
    }
};
} // namespace Media
} // namespace  OHOS
#endif // OHOS_MEDIA_CLOUD_SYNC_CLOUD_LAKE_UTILS_H
