/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_ALBUM_ASSET_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_ALBUM_ASSET_H_

#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class PhotoAlbum {
public:
    EXPORT PhotoAlbum();
    EXPORT virtual ~PhotoAlbum();

    EXPORT void SetAlbumId(const int32_t photoAlbumId);
    EXPORT int32_t GetAlbumId() const;

    EXPORT void SetPhotoAlbumType(const PhotoAlbumType type);
    EXPORT PhotoAlbumType GetPhotoAlbumType() const;

    EXPORT void SetPhotoAlbumSubType(const PhotoAlbumSubType subType);
    EXPORT PhotoAlbumSubType GetPhotoAlbumSubType() const;

    EXPORT void SetAlbumUri(const std::string &uri);
    EXPORT const std::string& GetAlbumUri() const;

    EXPORT void SetAlbumName(const std::string &photoAlbumName);
    EXPORT const std::string& GetAlbumName() const;

    EXPORT void SetCoverUri(const std::string &coverUri);
    EXPORT const std::string& GetCoverUri() const;

    EXPORT void SetDateModified(const int64_t dateModified);
    EXPORT int64_t GetDateModified() const;

    EXPORT void SetCount(const int32_t count);
    EXPORT int32_t GetCount() const;

    EXPORT void SetImageCount(const int32_t count);
    EXPORT int32_t GetImageCount() const;

    EXPORT void SetVideoCount(const int32_t count);
    EXPORT int32_t GetVideoCount() const;

    EXPORT void SetLatitude(const double latitude);
    EXPORT double GetLatitude() const;

    EXPORT void SetLongitude(const double longitude);
    EXPORT double GetLongitude() const;

    EXPORT void SetRelativePath(const std::string &logicalRelativePath);
    EXPORT const std::string& GetRelativePath() const;

    EXPORT void SetPriority(const int32_t priority);
    EXPORT int32_t GetPriority() const;

    EXPORT void SetLPath(const std::string &path);
    EXPORT const std::string& GetLPath() const;

    EXPORT void SetBundleName(const std::string &name);
    EXPORT const std::string& GetBundleName() const;

    EXPORT void SetDateAdded(const int64_t date);
    EXPORT int64_t GetDateAdded() const;

    EXPORT void SetContainsHidden(const int32_t hidden);
    EXPORT int32_t GetContainsHidden() const;

    EXPORT void SetOrder(const int32_t order);
    EXPORT int32_t GetOrder() const;

    EXPORT void SetLocalLanguage(const std::string &language);
    EXPORT const std::string& GetLocalLanguage() const;

    EXPORT void SetIsLocal(const int32_t isLocal);
    EXPORT int32_t GetIsLocal() const;

    EXPORT void SetResultNapiType(const ResultNapiType resultNapiType);
    EXPORT ResultNapiType GetResultNapiType() const;

    EXPORT void SetHiddenOnly(const bool hiddenOnly);
    EXPORT bool GetHiddenOnly() const;

    EXPORT void SetDisplayLevel(const int32_t displayLevel);
    EXPORT int32_t GetDisplayLevel() const;

    EXPORT void SetLocationOnly(const bool locationOnly);
    EXPORT bool GetLocationOnly() const;

    EXPORT void SetChangeTime(const int64_t changeTime);
    EXPORT int64_t GetChangeTime() const;

    EXPORT static bool IsUserPhotoAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsTrashAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool CheckPhotoAlbumType(const PhotoAlbumType albumType);
    EXPORT static bool CheckPhotoAlbumSubType(const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsSmartPortraitPhotoAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsSmartGroupPhotoAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsSmartClassifyAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsSourceAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsPetAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsSystemAlbum(const PhotoAlbumType albumType);
    EXPORT static bool IsHiddenAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsHighlightAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool IsAnalysisAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType);
    EXPORT static bool CheckOrderStyleType(const OrderStyleType orderStyle);
    EXPORT static bool IsUserPhotoAlbumByType(const PhotoAlbumType albumType);

    EXPORT void SetUserId(int32_t userId);
    EXPORT int32_t GetUserId();

    EXPORT void SetCoverUriSource(int32_t coverUriSource);
    EXPORT int32_t GetCoverUriSource();

    EXPORT void SetUploadStatus(int32_t uploadStatus);
    EXPORT int32_t GetUploadStatus() const;

    EXPORT void SetHidden(const int32_t hidden);
    EXPORT int32_t GetHidden() const;
private:
    int32_t albumId_;
    PhotoAlbumType type_;
    PhotoAlbumSubType subType_;
    std::string uri_;
    std::string albumName_;
    std::string coverUri_;
    int64_t dateModified_ {0};
    int32_t count_;
    int32_t imageCount_ {0};
    int32_t videoCount_ {0};
    double latitude_ {0.0};
    double longitude_ {0.0};
    std::string relativePath_;
    int32_t displayLevel_ {0};
    int64_t dateAdded_ {0};
    int32_t containsHidden_ {0};
    int32_t order_ {0};
    std::string bundleName_;
    std::string localLanguage_;
    int32_t isLocal_ {0};
    std::string lPath_;
    int32_t priority_ {0};

    ResultNapiType resultNapiType_ = ResultNapiType::TYPE_MEDIALIBRARY;
    bool hiddenOnly_ = false;
    bool locationOnly_ = false;
    int32_t targetUserId_;
    int32_t coverUriSource_ = static_cast<int32_t>(CoverUriSource::DEFAULT_COVER);
    int32_t uploadStatus_ = 1;
    int32_t hidden_ = 0;
    int64_t changeTime_ {0};
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_ALBUM_ASSET_H_
