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

#ifndef INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_NOTIFY_TYPES_H_
#define INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_NOTIFY_TYPES_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
constexpr int32_t MEDIA_LIBRARY_NOTIFY_INVALID_INT32 = -1;
constexpr int64_t MEDIA_LIBRARY_NOTIFY_INVALID_INT64 = -1;

struct AlbumChangeInfo;

struct PhotoAssetChangeInfo {
    int32_t fileId_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    std::string uri_;
    std::string dateDay_;
    std::string ownerAlbumUri_;
    bool isFavorite_ = false;
    int32_t mediaType_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    bool isHidden_ = false;
    int64_t dateTrashedMs_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
    int32_t strongAssociation_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t thumbnailVisible_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int64_t dateAddedMs_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
    int64_t dateTakenMs_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
    std::vector<std::shared_ptr<AlbumChangeInfo>> albumChangeInfos_;
    int64_t hiddenTime_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
    std::string displayName_;
    int32_t position_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int64_t size_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
    int32_t assetSourceType_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t livephoto4dStatus_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int64_t dateModifiedMs_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
};

struct AlbumChangeInfo {
    std::string lpath_;
    int32_t imageCount_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t videoCount_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t albumType_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t albumSubType_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    std::string albumName_;
    std::string albumUri_;
    int32_t count_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    std::string coverUri_;
    int32_t hiddenCount_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    std::string hiddenCoverUri_;
    bool isCoverChange_ = false;
    PhotoAssetChangeInfo coverInfo_;
    bool isHiddenCoverChange_ = false;
    PhotoAssetChangeInfo hiddenCoverInfo_;
    int32_t albumsOrder_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t orderSection_ = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int32_t hidden_ = 0;
};

struct AlbumChangeData {
    std::shared_ptr<AlbumChangeInfo> albumBeforeChange;
    std::shared_ptr<AlbumChangeInfo> albumAfterChange;
    int64_t version = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
};

struct AlbumChangeInfos {
    NotifyChangeType type = NotifyChangeType::NOTIFY_CHANGE_INVALID;
    std::vector<AlbumChangeData> albumChangeDatas;
    bool isForRecheck = false;
};

struct PhotoAssetChangeData {
    std::shared_ptr<PhotoAssetChangeInfo> assetBeforeChange;
    std::shared_ptr<PhotoAssetChangeInfo> assetAfterChange;
    bool isContentChanged = false;
    bool isDeleted = false;
    int32_t thumbnailChangeStatus = MEDIA_LIBRARY_NOTIFY_INVALID_INT32;
    int64_t version = MEDIA_LIBRARY_NOTIFY_INVALID_INT64;
};

struct PhotoAssetChangeInfos {
    NotifyChangeType type = NotifyChangeType::NOTIFY_CHANGE_INVALID;
    std::vector<PhotoAssetChangeData> assetChangeDatas;
    bool isForRecheck = false;
};
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_LIBRARY_NOTIFY_TYPES_H_
