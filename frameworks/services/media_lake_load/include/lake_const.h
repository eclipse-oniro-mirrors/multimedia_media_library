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

#include <string>
#include <unordered_map>

namespace OHOS {
namespace Media {
static const int64_t FILE_SIZE_1K = 1024;
const int STRONG_ASSOCIATION_ENABLE = 1;
const int CLOUD_ENHANCEMENT_PHOTO = 120;
const int DISPLAY_NAME_PREFIX_LENGTH = 20;
constexpr int ASSET_MAX_COMPLEMENT_ID = 999;
const std::string DEFAULT_IMAGE_NAME = "IMG_";
const std::string DEFAULT_VIDEO_NAME = "VID_";
const std::string DEFAULT_AUDIO_NAME = "AUD_";
const std::string RESTORE_CLOUD_DIR = "/storage/cloud/files/Photo";
const std::string TITLE_KEY_WORDS_OF_BURST = "_BURST";
const std::string LAKE_SCAN_DIR = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";

enum class OuterMediaType : int32_t {
    PICTURE = 1,
    VIDEO = 2,
};
enum class PrefixType {
    CLOUD = 0,
    LOCAL,
    CLOUD_EDIT_DATA,
    LOCAL_EDIT_DATA,
    CLOUD_THUMB,
};

enum class IsBurstType :int32_t {
    OTHER_TYPE = 0,
    BURST_COVER_TYPE = 1,
    BURST_MEMBER_TYPE = 2,
};

const std::unordered_map<PrefixType, std::string> PREFIX_MAP = {
    { PrefixType::CLOUD, "/storage/cloud/files" },
    { PrefixType::LOCAL, "/storage/media/local/files" },
    { PrefixType::CLOUD_EDIT_DATA, "/storage/cloud/files/.editData" },
    { PrefixType::LOCAL_EDIT_DATA, "/storage/media/local/files/.editData" },
    { PrefixType::CLOUD_THUMB, "/storage/cloud/files/.thumbs" },
};

enum class FileUpdateType : int32_t {
    UPDATE, // album unchanged
    UPDATE_ALBUM, // album changed
    INSERT,
    NO_CHANGE,
};

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
};

enum LakeScanMode : int32_t {
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