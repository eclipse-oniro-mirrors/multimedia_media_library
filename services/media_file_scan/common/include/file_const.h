/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_LAKE_DEFINES_H
#define OHOS_MEDIA_LAKE_DEFINES_H

#include <filesystem>
#include <string>
#include <unordered_map>

namespace OHOS {
namespace Media {
static const int64_t FILE_SIZE_1K = 1024;
const int STRONG_ASSOCIATION_ENABLE = 1;
const int CLOUD_ENHANCEMENT_PHOTO = 120;
const std::string DEFAULT_IMAGE_NAME = "IMG_";
const std::string DEFAULT_VIDEO_NAME = "VID_";
const std::string DEFAULT_AUDIO_NAME = "AUD_";
const std::string LAKE_SCAN_DIR = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
const std::string FILE_MANAGER_SCAN_DIR = "/storage/media/local/files/Docs";
const std::string FILE_MANAGER_LPATH_PREFIX = "/FromDocs";
const std::string FILE_MANAGER_TRASH_PATH = FILE_MANAGER_SCAN_DIR + "/.Trash";

enum class OuterMediaType : int32_t {
    PICTURE = 1,
    VIDEO = 2,
};

enum class IsBurstType :int32_t {
    OTHER_TYPE = 0,
    BURST_COVER_TYPE = 1,
    BURST_MEMBER_TYPE = 2,
};

enum class FileUpdateType : int32_t {
    UPDATE, // album unchanged
    UPDATE_ALBUM, // album changed
    INSERT,
    NO_CHANGE,
    DELETE, // only for clone restore
    TRASH,  // for file manager assets
    RECOVER,
    DOT_RENAME_RELEASE, // for dot-file rename: release local only
    DOT_RENAME_RECOVER, // for dot-file rename: recover to local and merge
};

// DOT_RENAME 枚举成员须与其余成员互异，编译期防护（成员显式赋值重复在 C++ 中合法）
static_assert(FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::UPDATE &&
    FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::UPDATE_ALBUM &&
    FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::INSERT &&
    FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::NO_CHANGE &&
    FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::DELETE &&
    FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::TRASH &&
    FileUpdateType::DOT_RENAME_RELEASE != FileUpdateType::RECOVER,
    "DOT_RENAME_RELEASE must be distinct from other FileUpdateType members");
static_assert(FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::UPDATE &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::UPDATE_ALBUM &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::INSERT &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::NO_CHANGE &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::DELETE &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::TRASH &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::RECOVER &&
    FileUpdateType::DOT_RENAME_RECOVER != FileUpdateType::DOT_RENAME_RELEASE,
    "DOT_RENAME_RECOVER must be distinct from other FileUpdateType members");

// 点号重命名释放/恢复的文件名首字符判定（FileManagerParser 与 LakeFileParser 共用逻辑，
// 与 FileManagerParser::IsDotRenameRelease / LakeFileParser::IsDotRenameRelease 语义一致）
// beforePath 不以.开头 且 afterPath 以.开头 -> 释放本地
inline bool IsDotRenameReleaseLogic(const std::string &beforePath, const std::string &afterPath)
{
    if (beforePath.empty() || afterPath.empty()) {
        return false;
    }
    std::string beforeFileName = std::filesystem::path(beforePath).filename().string();
    std::string afterFileName = std::filesystem::path(afterPath).filename().string();
    return !beforeFileName.empty() && beforeFileName[0] != '.' &&
           !afterFileName.empty() && afterFileName[0] == '.';
}

// beforePath 以.开头 且 afterPath 不以.开头 -> 恢复名称重新入库
inline bool IsDotRenameRecoverLogic(const std::string &beforePath, const std::string &afterPath)
{
    if (beforePath.empty() || afterPath.empty()) {
        return false;
    }
    std::string beforeFileName = std::filesystem::path(beforePath).filename().string();
    std::string afterFileName = std::filesystem::path(afterPath).filename().string();
    return !beforeFileName.empty() && beforeFileName[0] == '.' &&
           !afterFileName.empty() && afterFileName[0] != '.';
}

struct InnerFileInfo {
    std::string bundleName;
    std::string burstKey;
    std::string cloudPath; // outer lake path
    std::string dateDay;
    std::string dateMonth;
    std::string dateYear;
    std::string detailTime;
    std::string displayName;
    std::string allExif;
    std::string filePath; // inner lake path
    std::string frontCamera;
    std::string inode;
    std::string mediaSuffix;
    std::string mimeType;
    std::string packageName;
    std::string shootingMode;
    std::string shootingModeTag;
    std::string title;
    std::string userComment;
    int32_t ceAvailable {0};
    int32_t dynamicRangeType {0};
    int32_t fileId {0};
    int32_t fileType {0};
    int32_t fileSourceType {0};
    int32_t height {0};
    int32_t orientation {0};
    int32_t ownerAlbumId {0};
    int32_t strongAssociation {0};
    int32_t subtype {0};
    int32_t width {0};
    int32_t exifRotate {0};
    int64_t dateAdded {0};
    int64_t dateModified {0};
    int64_t dateTaken {0};
    int64_t duration {0};
    int64_t fileSize {0};
    int64_t editTime {0};
    int64_t lastVisitTime {0};
    double latitude {0.0};
    double longitude {0.0};
    bool needInsert {true};
    IsBurstType isBurst {IsBurstType::OTHER_TYPE};
    int32_t hdrMode {0};
    int32_t videoMode {-1};
    int64_t localAssetSize {0};
    int64_t coverPosition {0};
    int32_t livePhoto4dStatus {0};
    int32_t musicMasterMode {0};
};

enum ScanMode : int32_t {
    FULL,
    INCREMENT,
    VALIDATION,
};

enum class FolderScannerInitialType {
    DEFAULT,
    PATH,
    NOTIFY_INFO,
};


struct ThumbnailInfo {
    int32_t fileId;
    std::string displayName;
    std::string path;
    int64_t dateTaken;
    int64_t dateModified;
};

} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_LAKE_DEFINES_H