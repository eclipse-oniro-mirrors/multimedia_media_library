/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef COMMON_UTILS_MEDIA_URI_UTILS_H_
#define COMMON_UTILS_MEDIA_URI_UTILS_H_

#include <string>
#include <ctime>

#include "uri.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * media path utils which the ability process path
 */
class MediaUriUtils {
public:
    EXPORT MediaUriUtils();
    EXPORT ~MediaUriUtils();
    EXPORT static void AppendKeyValue(std::string &uri, const std::string &key, std::string value);
    EXPORT static std::string GetUriByExtrConditions(const std::string &prefix, const std::string &fileId,
        const std::string &suffix = "");
    EXPORT static std::string GetPathFromUri(const std::string &uri);
    EXPORT static int32_t CreateAssetBucket(int32_t fileId, int32_t &bucketNum);
    EXPORT static int32_t GetFileId(const std::string &uri);
    EXPORT static std::string GetFileIdStr(const std::string &uri);
    EXPORT static Uri GetMultiUri(Uri &uri, int32_t userId);
    EXPORT static bool CheckUri(const std::string &uri);
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_URI_UTILS_H_
