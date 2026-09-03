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

#ifndef FRAMEWORKS_UTILS_C2PA_UTILS_H_
#define FRAMEWORKS_UTILS_C2PA_UTILS_H_

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT C2paUtils {
public:
    static bool HasImageSignature(const std::string &filePath);
    static int32_t SignForCreate(const std::string &filePath,
        const std::string &authorId, const std::string &authorName);
    static int32_t SignForTranscode(const std::string &sourcePath, const std::string &targetPath);
    static int32_t SignForRevert(const std::string &sourcePath, const std::string &targetPath);
    static int32_t SignatureToLcd(const std::string &sourcePath, const std::string &targetPath);
    static int32_t SignForEnhancement(const std::string &sourcePath, const std::string &targetPath);
private:
    C2paUtils() = delete;
    ~C2paUtils() = delete;

    struct ExifInfo {
        std::string dateTime;
        std::string description;
    };

    static int32_t CertInitialize(bool isTranscode = false);
    static int32_t GetExifInfo(const std::string &filePath, ExifInfo &info);
    static int32_t CopySignatureInner(const std::string &sourcePath, const std::string &targetPath,
        bool isTranscode = false);
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_UTILS_C2PA_UTILS_H_