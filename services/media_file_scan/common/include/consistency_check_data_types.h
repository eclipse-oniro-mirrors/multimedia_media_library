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
#ifndef OHOS_MEDIA_CONSISTENCY_CHECK_DATA_TYPES_H
#define OHOS_MEDIA_CONSISTENCY_CHECK_DATA_TYPES_H

#include <string>

namespace OHOS::Media::ConsistencyCheck {
struct ScenarioProgress {
    int32_t lastFileId {0};
    int32_t lastAlbumId {0};
    std::string ToString() const;
};

struct DeviceStatus {
    bool isScreenOff {false};
    bool isCharging {false};
    bool isBackgroundTaskAllowed {false};
    int32_t batteryCapacity {-1};
    int32_t temperature {-1};
    std::string ToString() const;
};

struct DfxStats {
    int32_t photoAddCount {0};
    int32_t photoUpdateCount {0};
    int32_t photoDeleteCount {0};
    int32_t albumAddCount {0};
    int32_t albumUpdateCount {0};
    int32_t albumDeleteCount {0};
    uint64_t startTimeInMs {0};
    uint64_t endTimeInMs {0};
    std::string ToString() const;
};

struct AlbumRecord {
    int32_t albumId {-1};
    int32_t albumSubtype {-1};
    int32_t hasPhoto {0};
    std::string lpath;
    std::string ToString() const;
};

struct PhotoRecord {
    int32_t fileId {-1};
    int32_t position {-1};
    int32_t subtype {0};
    int32_t fileSourceType {-1};
    int64_t dateModified {0};
    int64_t dateTaken {0};
    std::string storagePath;
    std::string data;
    std::string displayName;
    std::string cloudId;
    AlbumRecord albumRecord;
    std::string ToString() const;
};
} // namespace OHOS::Media::ConsistencyCheck
#endif // OHOS_MEDIA_CONSISTENCY_CHECK_DATA_TYPES_H