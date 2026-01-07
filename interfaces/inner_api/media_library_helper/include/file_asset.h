/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_FILE_ASSET_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_FILE_ASSET_H_

#include <memory>
#include <mutex>
#include <string>
#include <variant>
#include <unordered_map>
#include "medialibrary_type_const.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

constexpr int MEMBER_TYPE_INT32 = 0;
constexpr int MEMBER_TYPE_INT64 = 1;
constexpr int MEMBER_TYPE_STRING = 2;
constexpr int MEMBER_TYPE_DOUBLE = 3;

constexpr int OPEN_TYPE_READONLY = 0;
constexpr int OPEN_TYPE_WRITE = 1;

/**
 * @brief Class for filling all file asset parameters
 *
 * @since 1.0
 * @version 1.0
 */
class FileAsset {
public:
    EXPORT FileAsset();
    EXPORT virtual ~FileAsset() = default;

    EXPORT int32_t GetId() const;
    EXPORT void SetId(int32_t id);

    EXPORT int32_t GetCount() const;
    EXPORT void SetCount(int32_t count);

    EXPORT const std::string &GetUri() const;
    EXPORT void SetUri(const std::string &uri);

    EXPORT const std::string &GetPath() const;
    EXPORT void SetPath(const std::string &path);

    EXPORT const std::string &GetRelativePath() const;
    EXPORT void SetRelativePath(const std::string &relativePath);

    EXPORT const std::string &GetMimeType() const;
    EXPORT void SetMimeType(const std::string &mimeType);

    EXPORT MediaType GetMediaType() const;
    EXPORT void SetMediaType(MediaType mediaType);

    EXPORT const std::string &GetDisplayName() const;
    EXPORT void SetDisplayName(const std::string &displayName);

    EXPORT int64_t GetSize() const;
    EXPORT void SetSize(int64_t size);

    EXPORT const std::string &GetCloudId() const;
    EXPORT void SetCloudId(const std::string &cloudId);

    EXPORT int64_t GetDateAdded() const;
    EXPORT void SetDateAdded(int64_t dataAdded);

    EXPORT int64_t GetDateModified() const;
    EXPORT void SetDateModified(int64_t dateModified);

    EXPORT const std::string &GetTitle() const;
    EXPORT void SetTitle(const std::string &title);

    EXPORT const std::string &GetArtist() const;
    EXPORT void SetArtist(const std::string &artist);

    EXPORT const std::string &GetAlbum() const;
    EXPORT void SetAlbum(const std::string &album);

    EXPORT int32_t GetPosition() const;
    EXPORT void SetPosition(int32_t position);

    EXPORT int32_t GetWidth() const;
    EXPORT void SetWidth(int32_t width);

    EXPORT int32_t GetHeight() const;
    EXPORT void SetHeight(int32_t height);

    EXPORT double GetAspectRatio();
    EXPORT void SetAspectRatio(double aspectRatio);

    EXPORT int32_t GetDuration() const;
    EXPORT void SetDuration(int32_t duration);

    EXPORT int32_t GetOrientation() const;
    EXPORT void SetOrientation(int32_t orientation);

    EXPORT int32_t GetAlbumId() const;
    EXPORT void SetAlbumId(int32_t albumId);

    EXPORT int32_t GetOwnerAlbumId() const;
    EXPORT void SetOwnerAlbumId(int32_t albumId);

    EXPORT const std::string &GetAlbumName() const;
    EXPORT void SetAlbumName(const std::string &albumName);

    EXPORT int32_t GetParent() const;
    EXPORT void SetParent(int32_t parent);
    EXPORT const std::string &GetAlbumUri() const;
    EXPORT void SetAlbumUri(const std::string &albumUri);
    EXPORT int64_t GetDateTaken() const;
    EXPORT void SetDateTaken(int64_t dataTaken);

    EXPORT int64_t GetTimePending() const;
    EXPORT void SetTimePending(int64_t timePending);

    EXPORT int32_t GetVisitCount() const;
    EXPORT void SetVisitCount(int32_t visitCount);
    EXPORT int32_t GetLcdVisitCount() const;
    EXPORT void SetLcdVisitCount(int32_t lcdVisitCount);
    EXPORT bool IsFavorite() const;
    EXPORT void SetFavorite(bool isFavorite);
    EXPORT bool IsRecentShow() const;
    EXPORT void SetRecentShow(bool isRecentShow);
    EXPORT int64_t GetDateTrashed() const;
    EXPORT void SetDateTrashed(int64_t dateTrashed);

    EXPORT std::string GetPhotoId() const; 
    EXPORT void SetPhotoId(const std::string &photoId);

    EXPORT std::pair<std::string, int> GetPhotoIdAndQuality() const;
    EXPORT void SetPhotoIdAndQuality(const std::string &photoId, int photoQuality);

    EXPORT void SetLatitude(double latitude);
    EXPORT double GetLatitude();
    EXPORT void SetLongitude(double longitude);
    EXPORT double GetLongitude();

    EXPORT const std::string &GetSelfId() const;
    EXPORT void SetSelfId(const std::string &selfId);
    EXPORT int32_t GetIsTrash() const;
    EXPORT void SetIsTrash(int32_t isTrash);
    EXPORT const std::string GetOwnerPackage() const;
    EXPORT void SetOwnerPackage(const std::string &ownerPackage);
    EXPORT const std::string GetOwnerAppId() const;
    EXPORT void SetOwnerAppId(const std::string &ownerAppId);
    EXPORT const std::string GetPackageName() const;
    EXPORT void SetPackageName(const std::string &packageName);

    EXPORT const std::string &GetRecyclePath() const;
    EXPORT void SetRecyclePath(const std::string &recyclePath);

    EXPORT ResultNapiType GetResultNapiType() const;
    EXPORT void SetResultNapiType(const ResultNapiType type);

    EXPORT int32_t GetPhotoSubType() const;
    EXPORT void SetPhotoSubType(int32_t photoSubType);

    EXPORT int32_t GetPhotoIndex() const;

    EXPORT int32_t GetOriginalSubType() const;

    EXPORT const std::string &GetCameraShotKey() const;
    EXPORT void SetCameraShotKey(const std::string &cameraShotKey);

    EXPORT bool IsHidden() const;
    EXPORT void SetHidden(bool isHidden);

    EXPORT void SetOpenStatus(int32_t fd, int32_t openStatus);
    EXPORT void RemoveOpenStatus(int32_t fd);
    EXPORT int32_t GetOpenStatus(int32_t fd);

    EXPORT std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> &GetMemberMap();
    EXPORT std::variant<int32_t, int64_t, std::string, double> &GetMemberValue(const std::string &name);

    EXPORT std::string GetAssetJson();
    EXPORT void SetResultTypeMap(const std::string &colName, ResultSetDataType type);

    EXPORT const std::string &GetAllExif() const;
    EXPORT void SetAllExif(const std::string &allExif);

    EXPORT const std::string &GetFrontCamera() const;
    EXPORT void SetFrontCamera(const std::string &frontCamera);

    EXPORT const std::string &GetUserComment() const;
    EXPORT void SetUserComment(const std::string &userComment);

    EXPORT const std::string &GetFilePath() const;
    EXPORT void SetFilePath(const std::string &filePath);

    EXPORT int64_t GetPhotoEditTime() const;
    EXPORT void SetPhotoEditTime(int64_t photoEditTime);

    EXPORT int32_t GetMovingPhotoEffectMode() const;
    EXPORT void SetMovingPhotoEffectMode(int32_t effectMode);

    EXPORT int64_t GetCoverPosition() const;
    EXPORT void SetCoverPosition(int64_t coverPosition);

    EXPORT const std::string &GetBurstKey() const;
    EXPORT void SetBurstKey(const std::string &burstKey);

    EXPORT int32_t GetBurstCoverLevel() const;
    EXPORT void SetBurstCoverLevel(int32_t burstCoverLevel);

    EXPORT const std::string &GetDetailTime() const;
    EXPORT void SetDetailTime(const std::string &detailTime);

    EXPORT int32_t GetCEAvailable() const;
    EXPORT void SetCEAvailable(int32_t ceAvailable);

    EXPORT int32_t GetSupportedWatermarkType() const;
    EXPORT void SetSupportedWatermarkType(int32_t watermarkType);

    EXPORT int32_t GetHasAppLink() const;
    EXPORT void SetHasAppLink(int32_t hasAppLink);
 
    EXPORT const std::string &GetAppLink() const;
    EXPORT void SetAppLink(const std::string appLink);

    EXPORT int32_t GetCompositeDisplayStatus() const;
    EXPORT void SetCompositeDisplayStatus(int32_t compositeDisplayStatus);

    EXPORT int32_t GetIsAuto() const;
    EXPORT void SetIsAuto(int32_t isAuto);

    EXPORT int32_t GetFileResourceType() const;
    EXPORT void SetFileResourceType(int32_t fileResourceType);
    EXPORT int32_t GetFileSourceType() const;
    EXPORT void SetFileSourceType(int32_t fileSourceType);

    EXPORT std::string GetStoragePath() const;
    EXPORT void SetStoragePath(const std::string &storagePath);

    EXPORT int32_t GetIsStylePhoto() const;
    EXPORT void SetIsStylePhoto(int32_t isStylePhoto);

    EXPORT void SetChangeTime(const int64_t changeTime);
    EXPORT int64_t GetChangeTime() const;

    EXPORT const std::string &GetStrMember(const std::string &name) const;
    EXPORT int32_t GetInt32Member(const std::string &name) const;
    EXPORT int64_t GetInt64Member(const std::string &name) const;
    EXPORT double GetDoubleMember(const std::string &name) const;
    EXPORT void SetMemberValue(const std::string &name,
                               const std::variant<int32_t, int64_t, std::string, double> &value)
    {
        member_[name] = value;
    }

    EXPORT void SetUserId(int32_t userId);
    EXPORT int32_t GetUserId();
    EXPORT int64_t GetTransCodeTime() const;
    EXPORT int32_t GetExistCompatibleDuplicate() const;
    EXPORT void SetExistCompatibleDuplicate(int32_t existCompatibleDuplicate);

    EXPORT void SetStageVideoTaskStatus(int32_t stageVideoTaskStatus);
    EXPORT int32_t GetStageVideoTaskStatus() const;

    EXPORT void SetPhotoIsTemp(int32_t isTemp);
    EXPORT int32_t GetPhotoIsTemp() const;

    EXPORT void SetExifRotate(int32_t exifRotate);
    EXPORT int32_t GetExifRotate() const;

    // Safe Album: critical type for children's watch
    EXPORT void SetCritical(int32_t IsCritical);
    EXPORT int32_t GetCritical() const;
    EXPORT void SetCriticalType(int32_t IsCriticalType);
    EXPORT int32_t GetCriticalType() const;

private:
    int32_t userId_ = -1;
    std::string albumUri_;
    ResultNapiType resultNapiType_;
    std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> member_;
    std::mutex openStatusMapMutex_;
    std::shared_ptr<std::unordered_map<int32_t, int32_t>> openStatusMap_;
    std::mutex resultTypeMapMutex_;
    std::unordered_map<std::string, ResultSetDataType> resultTypeMap_;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_FILE_ASSET_H_
