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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESOURCE_PLAN_H
#define OHOS_MEDIA_REVERSE_CLONE_RESOURCE_PLAN_H

#include <cstdint>
#include <string>

#include "backup_const.h"
#include "medialibrary_db_const.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
enum class ReverseCloneMatchType {
    NONE = 0,
    SOURCE_ASSET,
    NORMAL_SIGNATURE,
    SAME_CLOUD_VERSION,
    SAME_CLOUD_CONFLICT,
};

enum class ReverseCloneResourceDecision {
    NONE = 0,
    INHERIT,
    SKIP_CLOUD_VERSION_CONFLICT,
    SKIP_NO_DONOR_RESOURCE,
};

struct ReverseCloneAssetFingerprint {
    std::string cloudId;
    std::string displayName;
    int64_t fileSize {0};
    int32_t orientation {0};
    int32_t fileType {0};
};

struct ReverseCloneAssetResource {
    int32_t fileId {0};
    int32_t originalFileId {0};
    std::string cloudPath;
    std::string localRoot;
    std::string originPath;
    std::string relativePath;
    std::string storagePath;
    std::string inode;
    std::string sourcePath;
    ReverseCloneAssetFingerprint fingerprint;
    int32_t fileSourceType {0};
    int32_t subtype {0};
    int32_t effectMode {0};
    int64_t dateTrashed {0};
    int32_t hidden {0};
    int64_t dateModified {0};
    int64_t dateTaken {0};
    // Source-row metadata only. Reverse clone resource readiness is decided by actual files.
    int64_t editTime {0};
    int64_t thumbnailReady {0};
    int32_t lcdVisitTime {0};
    int64_t realLcdVisitTime {0};
    int32_t lcdVisitCount {0};
    std::string lcdSize;
    std::string thumbSize;
    int64_t lcdFileSize {0};
    int32_t thumbStatus {RESTORE_THUMBNAIL_STATUS_NOT_ALL};
    int32_t position {static_cast<int32_t>(PhotoPositionType::LOCAL)};
    bool isPureCloud {false};

    bool HasResourcePath() const
    {
        return !cloudPath.empty() || !originPath.empty() || !storagePath.empty();
    }

    bool HasOriginCandidate() const
    {
        return HasResourcePath();
    }

    bool HasThumbnailCandidate() const
    {
        return !cloudPath.empty() || !relativePath.empty();
    }

    bool HasLakeStoragePath() const
    {
        return !storagePath.empty();
    }

    bool IsLakeAsset() const
    {
        return fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE) && HasLakeStoragePath();
    }
};

struct ReverseCloneCandidate {
    ReverseCloneMatchType matchType {ReverseCloneMatchType::NONE};
    ReverseCloneAssetResource donor;

    bool IsFound() const
    {
        return matchType != ReverseCloneMatchType::NONE && donor.fileId > 0;
    }

    bool CanInheritResource() const
    {
        return matchType == ReverseCloneMatchType::NORMAL_SIGNATURE ||
            matchType == ReverseCloneMatchType::SAME_CLOUD_VERSION;
    }
};

struct ReverseCloneResourcePlan {
    ReverseCloneResourceDecision decision {ReverseCloneResourceDecision::NONE};
    ReverseCloneMatchType matchType {ReverseCloneMatchType::NONE};
    ReverseCloneAssetResource absorbed;
    ReverseCloneAssetResource donor;
    ReverseCloneAssetResource fallbackSource;
    bool hasFallbackSource {false};
    bool inheritOrigin {false};
    bool inheritLcdThumbnail {false};
    bool inheritThumbnail {false};
    bool cloudRestoreSatisfied {false};
    bool lakeTargetRenamed {false};

    bool HasResourceAction() const
    {
        return inheritOrigin || inheritLcdThumbnail || inheritThumbnail;
    }
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_RESOURCE_PLAN_H
