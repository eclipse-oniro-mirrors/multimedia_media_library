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

#ifndef CUSTOM_RESTORE_INFO_H
#define CUSTOM_RESTORE_INFO_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "custom_restore_types.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT CustomRestoreInfo {
public:
    CustomRestoreInfo() = default;
    ~CustomRestoreInfo() = default;
    CustomRestoreInfo(const CustomRestoreInfo&) = default;
    CustomRestoreInfo& operator=(const CustomRestoreInfo&) = default;

    // 输入字段
    const std::vector<std::string>& GetFilePaths() const;
    void SetFilePaths(const std::vector<std::string>& paths);
    const std::vector<RestoreFileInfo>& GetFileInfos() const;
    void SetFileInfos(const std::vector<RestoreFileInfo>& fileInfos);
    const std::unordered_map<std::string, TimeInfo>& GetTimeInfoMap() const;
    void SetTimeInfoMap(const std::unordered_map<std::string, TimeInfo>& timeInfoMap);
    int32_t GetAlbumId() const;
    void SetAlbumId(int32_t albumId);
    bool GetIsDeduplication() const;
    void SetIsDeduplication(bool isDeduplication);
    bool GetHasPhotoCache() const;
    void SetHasPhotoCache(bool hasPhotoCache);
    const std::unordered_set<std::string>& GetPhotoCache() const;
    void SetPhotoCache(const std::unordered_set<std::string>& photoCache);
    const std::string& GetPackageName() const;
    void SetPackageName(const std::string& packageName);
    const std::string& GetBundleName() const;
    void SetBundleName(const std::string& bundleName);
    const std::string& GetAppId() const;
    void SetAppId(const std::string& appId);
    bool GetIsFirstBatch() const;
    void SetIsFirstBatch(bool isFirstBatch);

    // 输出字段
    const std::vector<RestoreFileInfo>& GetOutFileInfos() const;
    void SetOutFileInfos(std::vector<RestoreFileInfo> &&outFileInfos);
    int32_t GetOutSameFileNum() const;
    void SetOutSameFileNum(int32_t outSameFileNum);
    int32_t GetOutSuccessFileNum() const;
    void SetOutSuccessFileNum(int32_t outSuccessFileNum);

private:
    // 输入字段
    std::vector<std::string> filePaths;
    std::vector<RestoreFileInfo> fileInfos;
    std::unordered_map<std::string, TimeInfo> timeInfoMap;
    int32_t albumId = 0;
    bool isDeduplication = false;
    bool hasPhotoCache = false;
    std::unordered_set<std::string> photoCache;
    std::string packageName;
    std::string bundleName;
    std::string appId;
    bool isFirstBatch = true;

    // 输出字段 - 使用 shared_ptr 确保 ScanConfig 值拷贝后写回仍可见
    std::shared_ptr<std::vector<RestoreFileInfo>> outFileInfos = std::make_shared<std::vector<RestoreFileInfo>>();
    std::shared_ptr<int32_t> outSameFileNum = std::make_shared<int32_t>(0);
    std::shared_ptr<int32_t> outSuccessFileNum = std::make_shared<int32_t>(0);
};

} // namespace Media
} // namespace OHOS

#endif // CUSTOM_RESTORE_INFO_H
