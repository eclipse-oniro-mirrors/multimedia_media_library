/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#include "burst_key_generator.h"

#include <uuid.h>
#include <algorithm>
#include <sstream>

#include "media_log.h"

namespace OHOS::Media {
/**
 * @brief find prefix contains "_BURST" of fileInfo.title
 *
 * @param fileInfo row data from gallery.db # gallery_media
 * @return std::string prefix of fileInfo.title
 */
std::string BurstKeyGenerator::FindTitlePrefix(const FileInfo &fileInfo)
{
    std::string displayName = fileInfo.displayName;
    auto pos = displayName.find(this->TITLE_KEY_WORDS_OF_BURST);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("Media_Restore: cannot find _BURST. Object: %{public}s", this->ToString(fileInfo).c_str());
        return "";
    }
    return displayName.substr(0, std::min<int32_t>(pos, DISPLAY_NAME_PREFIX_LENGTH) + 1);
}

/**
 * @brief find group hash based on fileInfo.relativeBucketId, fileInfo.title and groupIndex
 *
 * @param fileInfo row data from gallery.db # gallery_media
 * @return std::string hash to identify a group of burst photo
 */
std::string BurstKeyGenerator::FindGroupHash(const FileInfo &fileInfo)
{
    return fileInfo.relativeBucketId + "#" + FindTitlePrefix(fileInfo) + "#" + std::to_string(FindGroupIndex(fileInfo));
}

/**
 * @brief find groupIndex based on objectHash
 *
 * @param fileInfo row data from gallery.db # gallery_media
 * @return int32_t groupIndex to identify which group the fileInfo belongs to
 */
int32_t BurstKeyGenerator::FindGroupIndex(const FileInfo &fileInfo)
{
    // the photo do not in recycle bin.
    if (fileInfo.recycleFlag == 0) {
        return 0;
    }
    std::string objectHash = FindObjectHash(fileInfo);
    auto it = objectHashMap_.find(objectHash);
    int32_t groupIndex = 1;
    if (it != objectHashMap_.end()) {
        groupIndex = it->second + 1;
    }
    objectHashMap_[objectHash] = groupIndex;
    return groupIndex;
}

/**
 * @brief find objectHash based on fileInfo.relativeBucketId, fileInfo.title and fileInfo.hashCode
 *
 * @param fileInfo row data from gallery.db # gallery_media
 * @return std::string objectHash to identify fileInfo
 */
std::string BurstKeyGenerator::FindObjectHash(const FileInfo &fileInfo)
{
    return fileInfo.relativeBucketId + "#" + FindTitlePrefix(fileInfo) + "#" + fileInfo.hashCode;
}

/**
 * @brief generate a uuid, like xxxxxxxx-xxxx-xxxx-xxxxxxxx-xxxx
 *
 * @return std::string uuid with 36 characters
 */
std::string BurstKeyGenerator::GenerateUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

/**
 * @brief find burstKey for burst photo in Album and Recycle-Bin
 *
 * @param fileInfo row data from gallery.db # gallery_media
 * @return std::string burstKey to identify burst photo group
 */
std::string BurstKeyGenerator::FindBurstKey(const FileInfo &fileInfo)
{
    // isBurst, 1=burst cover photo, 2=burst photo, 0=others
    if (fileInfo.isBurst != BURST_COVER_TYPE && fileInfo.isBurst != BURST_MEMBER_TYPE) {
        return "";
    }
    std::unique_lock<std::mutex> lock(this->burstKeyLock_);
    std::string groupHash = FindGroupHash(fileInfo);
    auto it = groupHashMap_.find(groupHash);
    if (it == groupHashMap_.end()) {
        groupHashMap_[groupHash] = GenerateUuid();
    }
    MEDIA_DEBUG_LOG("Media_Restore: burst photo, objectHash: %{public}s, groupHash: %{public}s, burstKey: %{public}s",
        FindObjectHash(fileInfo).c_str(),
        groupHash.c_str(),
        groupHashMap_[groupHash].c_str());
    return groupHashMap_[groupHash];
}

std::string BurstKeyGenerator::ToString(const FileInfo &fileInfo)
{
    std::stringstream ss;
    ss << "FileInfo[ fileId: " << fileInfo.fileIdOld << ", displayName: " << fileInfo.displayName
       << ", bundleName: " << fileInfo.bundleName << ", lPath: " << fileInfo.lPath << ", size: " << fileInfo.fileSize
       << ", fileType: " << fileInfo.fileType << ", oldPath: " << fileInfo.oldPath
       << ", sourcePath: " << fileInfo.sourcePath << " ]";
    return ss.str();
}
}  // namespace OHOS::Media