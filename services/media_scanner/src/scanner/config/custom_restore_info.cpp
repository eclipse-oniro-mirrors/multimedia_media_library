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

#define MLOG_TAG "CustomRestoreInfo"

#include "custom_restore_info.h"

// LCOV_EXCL_START
namespace OHOS {
namespace Media {

// 输入字段

const std::vector<std::string>& CustomRestoreInfo::GetFilePaths() const
{
    return filePaths;
}

void CustomRestoreInfo::SetFilePaths(const std::vector<std::string>& paths)
{
    filePaths = paths;
}

const std::vector<RestoreFileInfo>& CustomRestoreInfo::GetFileInfos() const
{
    return fileInfos;
}

void CustomRestoreInfo::SetFileInfos(const std::vector<RestoreFileInfo>& fileInfos)
{
    this->fileInfos = fileInfos;
}

const std::unordered_map<std::string, TimeInfo>& CustomRestoreInfo::GetTimeInfoMap() const
{
    return timeInfoMap;
}

void CustomRestoreInfo::SetTimeInfoMap(const std::unordered_map<std::string, TimeInfo>& timeInfoMap)
{
    this->timeInfoMap = timeInfoMap;
}

int32_t CustomRestoreInfo::GetAlbumId() const
{
    return albumId;
}

void CustomRestoreInfo::SetAlbumId(int32_t albumId)
{
    this->albumId = albumId;
}

bool CustomRestoreInfo::GetIsDeduplication() const
{
    return isDeduplication;
}

void CustomRestoreInfo::SetIsDeduplication(bool isDeduplication)
{
    this->isDeduplication = isDeduplication;
}

bool CustomRestoreInfo::GetHasPhotoCache() const
{
    return hasPhotoCache;
}

void CustomRestoreInfo::SetHasPhotoCache(bool hasPhotoCache)
{
    this->hasPhotoCache = hasPhotoCache;
}

const std::unordered_set<std::string>& CustomRestoreInfo::GetPhotoCache() const
{
    return photoCache;
}

void CustomRestoreInfo::SetPhotoCache(const std::unordered_set<std::string>& photoCache)
{
    this->photoCache = photoCache;
}

const std::string& CustomRestoreInfo::GetPackageName() const
{
    return packageName;
}

void CustomRestoreInfo::SetPackageName(const std::string& packageName)
{
    this->packageName = packageName;
}

const std::string& CustomRestoreInfo::GetBundleName() const
{
    return bundleName;
}

void CustomRestoreInfo::SetBundleName(const std::string& bundleName)
{
    this->bundleName = bundleName;
}

const std::string& CustomRestoreInfo::GetAppId() const
{
    return appId;
}

void CustomRestoreInfo::SetAppId(const std::string& appId)
{
    this->appId = appId;
}

bool CustomRestoreInfo::GetIsFirstBatch() const
{
    return isFirstBatch;
}

void CustomRestoreInfo::SetIsFirstBatch(bool isFirstBatch)
{
    this->isFirstBatch = isFirstBatch;
}

// 输出字段

const std::vector<RestoreFileInfo>& CustomRestoreInfo::GetOutFileInfos() const
{
    return *outFileInfos;
}

void CustomRestoreInfo::SetOutFileInfos(std::vector<RestoreFileInfo> outFileInfos)
{
    *this->outFileInfos = std::move(outFileInfos);
}

int32_t CustomRestoreInfo::GetOutSameFileNum() const
{
    return *outSameFileNum;
}

void CustomRestoreInfo::SetOutSameFileNum(int32_t outSameFileNum)
{
    *this->outSameFileNum = outSameFileNum;
}

int32_t CustomRestoreInfo::GetOutSuccessFileNum() const
{
    return *outSuccessFileNum;
}

void CustomRestoreInfo::SetOutSuccessFileNum(int32_t outSuccessFileNum)
{
    *this->outSuccessFileNum = outSuccessFileNum;
}

} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
