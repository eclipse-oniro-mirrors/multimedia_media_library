/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef METADATA_H
#define METADATA_H

#include <unordered_map>
#include <variant>
#include "scanner_utils.h"
#include "fetch_result.h"
#include "abs_shared_result_set.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class Metadata {
public:
    EXPORT Metadata();
    EXPORT ~Metadata() = default;
    using VariantData = std::variant<int32_t, std::string, int64_t, double>;

    EXPORT void SetFileId(const VariantData &id);
    EXPORT int32_t GetFileId() const;

    EXPORT void SetFilePath(const VariantData &path);
    EXPORT const std::string &GetFilePath() const;

    EXPORT void SetUri(const VariantData &uri);
    EXPORT const std::string &GetUri() const;

    EXPORT void SetRelativePath(const VariantData &relativePath);
    EXPORT const std::string &GetRelativePath() const;

    EXPORT void SetFileMimeType(const VariantData &mimeType);
    EXPORT const std::string &GetFileMimeType() const;

    EXPORT void SetFileMediaType(const VariantData &mediaType);
    EXPORT MediaType GetFileMediaType() const;

    EXPORT void SetFileName(const VariantData &name);
    EXPORT const std::string &GetFileName() const;

    EXPORT void SetFileSize(const VariantData &size);
    EXPORT int64_t GetFileSize() const;

    EXPORT void SetFileDateAdded(const VariantData &dateAdded);
    EXPORT int64_t GetFileDateAdded() const;

    EXPORT void SetFileDateModified(const VariantData &dateModified);
    EXPORT int64_t GetFileDateModified() const;

    EXPORT void SetFileExtension(const VariantData &fileExt);
    EXPORT const std::string &GetFileExtension() const;

    EXPORT void SetFileTitle(const VariantData &title);
    EXPORT const std::string &GetFileTitle() const;

    EXPORT void SetFileArtist(const VariantData &artist);
    EXPORT const std::string &GetFileArtist() const;

    EXPORT void SetAlbum(const VariantData &album);
    EXPORT const std::string &GetAlbum() const;

    EXPORT void SetFileHeight(const VariantData &height);
    EXPORT int32_t GetFileHeight() const;

    EXPORT void SetFileWidth(const VariantData &width);
    EXPORT int32_t GetFileWidth() const;

    EXPORT void SetFileAspectRatio(const VariantData &aspectRatio);
    EXPORT double GetFileAspectRatio() const;

    EXPORT void SetOrientation(const VariantData &orientation);
    EXPORT int32_t GetOrientation() const;

    EXPORT void SetExifRotate(const VariantData &exifRotate);
    EXPORT int32_t GetExifRotate() const;

    EXPORT void SetFileDuration(const VariantData &duration);
    EXPORT int32_t GetFileDuration() const;

    EXPORT int32_t GetParentId() const;
    EXPORT void SetParentId(const VariantData &id);

    EXPORT void SetAlbumId(const VariantData &albumId);
    EXPORT int32_t GetAlbumId() const;

    EXPORT void SetAlbumName(const VariantData &album);
    EXPORT const std::string &GetAlbumName() const;

    EXPORT void SetRecyclePath(const VariantData &recyclePath);
    EXPORT const std::string &GetRecyclePath() const;

    EXPORT void SetDateTaken(const VariantData &dateTaken);
    EXPORT int64_t GetDateTaken() const;

    EXPORT void SetLongitude(const VariantData &longitude);
    EXPORT double GetLongitude() const;

    EXPORT void SetLatitude(const VariantData &latitude);
    EXPORT double GetLatitude() const;

    EXPORT void SetTimePending(const VariantData &timePending);
    EXPORT int64_t GetTimePending() const;

    EXPORT void SetUserComment(const VariantData &userComment);
    EXPORT const std::string &GetUserComment() const;

    EXPORT void SetAllExif(const VariantData &allExif);
    EXPORT const std::string &GetAllExif() const;

    EXPORT void SetDateYear(const VariantData &dateYear);
    EXPORT const std::string &GetDateYear() const;

    EXPORT void SetDateMonth(const VariantData &dateMonth);
    EXPORT const std::string &GetDateMonth() const;

    EXPORT void SetDateDay(const VariantData &dateDay);
    EXPORT const std::string &GetDateDay() const;

    EXPORT void SetShootingMode(const VariantData &shootingMode);
    EXPORT const std::string &GetShootingMode() const;

    EXPORT void SetShootingModeTag(const VariantData &shootingMode);
    EXPORT const std::string &GetShootingModeTag() const;

    EXPORT void SetLastVisitTime(const VariantData &lastVisitTime);
    EXPORT int64_t GetLastVisitTime() const;

    EXPORT void SetPhotoSubType(const VariantData &photoSubType);
    EXPORT int32_t GetPhotoSubType() const;
    
    EXPORT void SetMovingPhotoEffectMode(const VariantData &movingPhotoEffectMode);
    EXPORT int32_t GetMovingPhotoEffectMode() const;

    EXPORT void SetFrontCamera(const VariantData &forntcamera);
    EXPORT std::string GetFrontCamera() const;
    
    void SetMovingPhotoImagePath(const VariantData &imagePath);
    EXPORT std::string GetMovingPhotoImagePath() const;

    EXPORT void SetDynamicRangeType(const VariantData &type);
    EXPORT int32_t GetDynamicRangeType() const;

    EXPORT void SetHdrMode(const VariantData &type);
    EXPORT int32_t GetHdrMode() const;

    EXPORT void SetCoverPosition(const VariantData &coverPosition);
    EXPORT int64_t GetCoverPosition() const;

    void SetFrameIndex(const VariantData &frameIndex);
    int32_t GetFrameIndex() const;

    EXPORT void SetIsTemp(const VariantData &isTemp);
    EXPORT int32_t GetIsTemp() const;

    EXPORT void SetPhotoQuality(const VariantData &photoQuality);
    EXPORT int32_t GetPhotoQuality() const;

    EXPORT void SetDirty(const VariantData &dirty);
    EXPORT int32_t GetDirty() const;

    EXPORT void SetDetailTime(const VariantData &detailTime);
    EXPORT std::string GetDetailTime() const;

    EXPORT void SetForAdd(bool forAdd);
    EXPORT bool GetForAdd() const;
    EXPORT void SetTableName(const std::string &tableName);
    EXPORT std::string GetTableName();
    void SetOwnerPackage(const VariantData &ownerPackage);
    const std::string GetOwnerPackage() const;

    void SetBurstCoverLevel(const VariantData &burstCoverLevel);
    int32_t GetBurstCoverLevel() const;

    void SetStageVideoTaskStatus(const VariantData &stageVideoTaskStatus);
    int32_t GetStageVideoTaskStatus() const;

    EXPORT void SetFileSourceType(const VariantData &fileSourceType);
    EXPORT int32_t GetFileSourceType() const;

    EXPORT void SetVideoMode(const VariantData &videoMode);
    EXPORT int32_t GetVideoMode() const;

    EXPORT void Init();
    void InitV2();

    using MetadataFnPtr = void (Metadata::*)(const VariantData &);
    EXPORT std::unordered_map<std::string, std::pair<ResultSetDataType, MetadataFnPtr>> memberFuncMap_;

private:
    int32_t id_;
    std::string uri_;
    std::string filePath_;
    std::string relativePath_;

    std::string mimeType_;
    MediaType mediaType_;
    std::string name_;

    int64_t size_;
    int64_t dateModified_;
    int64_t dateAdded_;

    std::string fileExt_;
    int32_t parentId_;

    // audio
    std::string title_;
    std::string artist_;
    std::string album_;

    // video, image
    int32_t height_;
    int32_t width_;
    int32_t duration_;
    int32_t orientation_;
    int32_t exifRotate_ {0};
    string dateYear_;
    string dateMonth_;
    string dateDay_;
    string shootingMode_;
    string shootingModeTag_;
    int64_t lastVisitTime_;
    int32_t dynamicRangeType_;
    int32_t hdrMode_;
    double aspectRatio_;

    // video, audio, image
    int64_t dateTaken_ {0};

    // image
    double longitude_;
    double latitude_;
    std::string userComment_;
    std::string allExif_;

    // album
    int32_t albumId_;
    std::string albumName_;

    // recycle
    std::string recyclePath_;

    // pending
    int64_t timePending_;

    // photo subtype
    int32_t photoSubType_ = 0;

    int32_t movingPhotoEffectMode_ = 0;

    bool forAdd_ = false;
    std::string tableName_;

    // packageName
    std::string ownerPackage_;
    
    // image path
    std::string movingPhotoImagePath_;

    // cover position of moving photo
    int64_t coverPosition_ = 0;

    // frame index of live photo
    int32_t frameIndex_ = 0;

    int32_t isTemp_;
    int32_t photoQuality_ = 0;
    int32_t dirty_ = 0;

     // front camera
    std::string frontcamera_;

    std::string detailTime_;

    int32_t burstCoverLevel_;

    int32_t stageVideoTaskStatus_;

    int32_t fileSourceType_;

    int32_t videoMode_;
};
} // namespace Media
} // namespace OHOS

#endif // METADATA_H
