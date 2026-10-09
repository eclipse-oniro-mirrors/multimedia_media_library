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

#ifndef COMMON_UTILS_MEDIA_LOG_UTILS_H_
#define COMMON_UTILS_MEDIA_LOG_UTILS_H_

#include <cstddef>
#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaLogUtils {
public:
    EXPORT static std::string GarbleFilePath(const std::string &filePath);
    EXPORT static std::string GarbleFile(const std::string &file);

private:
    static bool HasExtension(const std::string &file);
    static std::string GarbleFileWithExtension(const std::string &file);
    static std::string GarbleFileWithoutExtension(const std::string &file);
    static size_t GetGarbleSize(const std::string &file);
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_LOG_UTILS_H_
