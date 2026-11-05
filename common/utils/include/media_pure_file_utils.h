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

#ifndef COMMON_UTILS_MEDIA_PURE_FILE_UTILS_H_
#define COMMON_UTILS_MEDIA_PURE_FILE_UTILS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <sys/types.h>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * media pure file utils which the ability process file
 */
class MediaPureFileUtils {
public:
    EXPORT MediaPureFileUtils();
    EXPORT ~MediaPureFileUtils();
    EXPORT static bool IsFileExists(const std::string &fileName);
    EXPORT static bool DeleteDir(const std::string &dirName);
    EXPORT static bool IsDirectory(const std::string &dirName, std::shared_ptr<int> errCodePtr = nullptr);
    EXPORT static bool DeleteFile(const std::string &fileName);
    EXPORT static bool CreateDirectory(const std::string &dirPath, std::shared_ptr<int> errCodePtr = nullptr);
    EXPORT static bool CopyFileUtil(const std::string &filePath, const std::string &newPath,
        bool skipCheckPermit = false);
    EXPORT static void PrintStatInformation(const std::string &path);
private:
    static int32_t RemoveDirectory(const std::string &path);
    static bool Mkdir(const std::string &subStr, std::shared_ptr<int> errCodePtr);
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_PURE_FILE_UTILS_H_
