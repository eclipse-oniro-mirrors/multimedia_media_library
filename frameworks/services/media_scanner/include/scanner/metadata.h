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

    void SetUri(const VariantData &uri);
    const std::string &GetUri() const;

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
    const std::string &GetFileExtension() const;

    EXPORT void SetFileTitle(const VariantData &title);
    EXPORT const std::string &GetFileTitle() const;

    EXPORT void SetFileArtist(const VariantData &artist);
    EXPORT const std::string &GetFileArtist() const;

    EXPORT void SetAlbum(const VariantData &album);
    EXPORT const std::string &GetAlbum() const;

    void SetFileHeight(const VariantData &height);
    EXPORT int32_t GetFileHeight() const;

    void SetFileWidth(const VariantData &width);
    EXPORT int32_t GetFileWidth() const;

    void SetOrientation(const VariantData &orientation);
    EXPORT int32_t GetOrientation() const;

    void SetFileDuration(const VariantData &duration);
    EXPORT int32_t GetFileDuration() const;

    EXPORT int32_t GetParentId() const;
    EXPORT void SetParentId(const VariantData &id);

    void SetAlbumId(const VariantData &albumId);
    int32_t GetAlbumId() const;

    EXPORT void SetAlbumName(const VariantData &album);
    EXPORT const std::string &GetAlbumName() const;

    EXPORT void SetRecyclePath(const VariantData &recyclePath);
    EXPORT const std::string &GetRecyclePath() const;

    EXPORT void SetDateTaken(const VariantData &dateTaken);
    EXPORT int64_t GetDateTaken() const;

    void SetLongitude(const VariantData &longitude);
    EXPORT double GetLongitude() const;

    void SetLatitude(const VariantData &latitude);
    EXPORT double GetLatitude() const;

    void SetTimePending(const VariantData &timePending);
    int64_t GetTimePending() const;

    void SetUserComment(const VariantData &userComment);
    const std::string &GetUserComment() const;

    void SetAllExif(const VariantData &allExif);
    EXPORT const std::string &GetAllExif() const;

    void SetDateYear(const VariantData &dateYear);
    const std::string &getDateYear() const;

    void SetDateMonth(const VariantData &dateMonth);
    const std::string &getDateMonth() const;

    void SetDateDay(const VariantData &dateDay);
    const std::string &GetDateDay() const;

    void SetShootingMode(const VariantData &shootingMode);
    EXPORT const std::string &GetShootingMode() const;

    void SetShootingModeTag(const VariantData &shootingMode);
    EXPORT const std::string &GetShootingModeTag() const;

    void SetLastVisitTime(const VariantData &lastVisitTime);
    EXPORT int64_t GetLastVisitTime() const;

    void SetPhotoSubType(const VariantData &photoSubType);
    int32_t GetPhotoSubType() const;

    void SetForAdd(bool forAdd);
    bool GetForAdd() const;
    void SetTableName(const std::string &tableName);
    std::string GetTableName();
    void SetOwnerPackage(const VariantData &ownerPackage);
    const std::string GetOwnerPackage() const;

    EXPORT void Init();

    using MetadataFnPtr = void (Metadata::*)(const VariantData &);
    std::unordered_map<std::string, std::pair<ResultSetDataType, MetadataFnPtr>> memberFuncMap_;

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
    string dateYear_;
    string dateMonth_;
    string dateDay_;
    string shootingMode_;
    string shootingModeTag_;
    int64_t lastVisitTime_;

    // video, audio, image
    int64_t dateTaken_;

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

    bool forAdd_ = false;
    std::string tableName_;

    // packageName
    std::string ownerPackage_;
};
} // namespace Media
} // namespace OHOS

#endif // METADATA_H
