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

#ifndef LAKE_FILE_UTILS_H
#define LAKE_FILE_UTILS_H

#include <string>

#include "lake_const.h"
#include "metadata.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class LakeFileUtils {
public:
    static int32_t GetFileMetadata(std::unique_ptr<Metadata> &data);
    static int32_t FillMetadata(std::unique_ptr<Metadata> &data);
    static std::string GetFileTitle(const std::string &displayName);
    static int32_t CreateAssetRealName(int32_t fileId, int32_t mediaType,
        const std::string &extension, std::string &name);
    static std::string GetReplacedPathByPrefixType(PrefixType srcPrefixType, PrefixType dstPrefixType,
        const std::string &path);
    static int32_t CreateAssetPathById(int32_t fileId, int32_t mediaType, const std::string &extension,
        std::string &filePath);
    static std::string FindObjectHash(InnerFileInfo &fileInfo);
    static std::string GenerateUuid();
    static int32_t FindGroupIndex(InnerFileInfo &fileInfo);
    static std::string FindTitlePrefix(InnerFileInfo &fileInfo);
    static std::string FindGroupHash(InnerFileInfo &fileInfo);
    static void SetBurstKey(InnerFileInfo &fileInfo);
    static int32_t FindSubtype(const InnerFileInfo &fileInfo);
    static std::string GarbleFilePath(const std::string &filePath);
    static std::string GarbleFile(const std::string &file);

    // 文件操作
    EXPORT static std::string GetAssetRealPath(const std::string &path);
    static int32_t OpenFile(const std::string &filePath, int flags);
    static int32_t CopyFile(const std::string &srcPath, std::string &targetPath);
    static int32_t RenameFileCrossPolicy(const string &oldPath, const string &newPath, bool deleteOld = false);
    static bool DeleteFile(const string &fileName);
    static int32_t MoveFileInEditScene(const string &oldPath, const string &newPath);
    static bool CoverLakeFile(const string &filePath, const string &newPath);
    // Create file path in lake.
    static int32_t BuildLakeFilePath(const std::string &displayName, const int32_t mediaType, std::string &targetPath);

    // 多用户场景
    static int32_t GetCurrentAccountId();
    static std::string GetCurrentInLakeLogicPrefix();
    static bool IsDefaultAccount();

private:
    static bool HasExtension(const std::string &file);
    static std::string GarbleFileWithExtension(const std::string &file);
    static std::string GarbleFileWithoutExtension(const std::string &file);
    static size_t GetGarbleSize(const std::string &file);
};
} // namespace OHOS::Media
#endif // LAKE_FILE_UTILS_H