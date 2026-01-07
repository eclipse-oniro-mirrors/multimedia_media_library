/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIALIBRARY_MIMETYPE_UTILS_H
#define OHOS_MEDIALIBRARY_MIMETYPE_UTILS_H

#include <unordered_map>

#include "nlohmann/json.hpp"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MimeTypeUtils {
public:
    EXPORT MimeTypeUtils(const MimeTypeUtils&) = delete;
    EXPORT MimeTypeUtils(MimeTypeUtils&&) = delete;
    EXPORT MimeTypeUtils& operator=(const MimeTypeUtils&) = delete;
    EXPORT MimeTypeUtils& operator=(MimeTypeUtils&&) = delete;
    EXPORT static bool IsMimeTypeMapEmpty();
    EXPORT static int32_t InitMimeTypeMap();
    EXPORT static std::string GetMimeTypeFromContent(const std::string &filePath);
    EXPORT static std::string GetMimeTypeFromExtension(const std::string &extension);
    EXPORT static std::string GetMimeTypeFromExtension(const std::string &extension,
        const std::unordered_map<std::string, std::vector<std::string>> &mimeTypeMap);
    EXPORT static MediaType GetMediaTypeFromMimeType(const std::string &mimeType);
    EXPORT static MediaType GetMediaType(const std::string &filePath);

private:
    static void CreateMapFromJson();
    static int32_t GetVideoMimetype(const std::string &filePath, std::string &mimeType);
    static int32_t GetImageMimetype(const std::string &filePath, std::string &mimeType);

    static std::mutex lockCreateMap_;

    static std::unordered_map<std::string, std::vector<std::string>> mediaJsonMap_;
};
}
}
#endif //OHOS_MEDIALIBRARY_MIMETYPE_UTILS_H
