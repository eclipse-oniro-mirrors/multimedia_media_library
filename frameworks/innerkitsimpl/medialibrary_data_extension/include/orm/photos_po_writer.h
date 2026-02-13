/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ORM_PHOTOS_PO_WRITER_H
#define OHOS_MEDIA_ORM_PHOTOS_PO_WRITER_H

#include <string>
#include <map>

#include "media_column.h"
#include "i_object_writer.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "photos_po.h"
#include "cloud_media_define.h"

namespace OHOS::Media::ORM {
class EXPORT PhotosPoWriter : public IObjectWriter {
private:
    PhotosPo &photosPo_;

public:
    explicit PhotosPoWriter(PhotosPo &photosPo) : photosPo_(photosPo)
    {}
    virtual ~PhotosPoWriter() = default;

public:
    std::map<std::string, MediaColumnType::DataType> GetColumns() override
    {
        return MediaColumnType::PHOTOS_COLUMNS;
    }
    int32_t SetMemberVariable(
        const std::string &name, std::variant<int32_t, int64_t, double, std::string> &val) override;
    /**
    * @param isIdentifyOnly true: only get identify columns; false: get all columns.
    */

    std::unordered_map<std::string, std::string> ToMap(bool isIdentifyOnly = true);

private:
    using SetHandle = void (PhotosPoWriter::*)(std::variant<int32_t, int64_t, double, std::string> &);
    using GetHandle = bool (PhotosPoWriter::*)(std::string &);
    struct GetSetNode {
        GetHandle funGetPtr;
        SetHandle funSetPtr;
    };
    const std::map<std::string, GetSetNode> HANDLERS = {
        {PhotoColumn::MEDIA_FILE_PATH, {&PhotosPoWriter::GetMediaFilePath, &PhotosPoWriter::SetMediaFilePath}},
        {PhotoColumn::MEDIA_TITLE, {&PhotosPoWriter::GetMediaTitle, &PhotosPoWriter::SetMediaTitle}},
        {PhotoColumn::MEDIA_SIZE, {&PhotosPoWriter::GetMediaSize, &PhotosPoWriter::SetMediaSize}},
        {PhotoColumn::MEDIA_NAME, {&PhotosPoWriter::GetMediaName, &PhotosPoWriter::SetMediaName}},
        {PhotoColumn::MEDIA_TYPE, {&PhotosPoWriter::GetMediaType, &PhotosPoWriter::SetMediaType}},
        {PhotoColumn::MEDIA_MIME_TYPE, {&PhotosPoWriter::GetMediaMimeType, &PhotosPoWriter::SetMediaMimeType}},
        {PhotoColumn::MEDIA_DEVICE_NAME, {&PhotosPoWriter::GetMediaDeviceName, &PhotosPoWriter::SetMediaDeviceName}},
        {PhotoColumn::MEDIA_DATE_ADDED, {&PhotosPoWriter::GetMediaDataAdded, &PhotosPoWriter::SetMediaDataAdded}},
        {PhotoColumn::MEDIA_DATE_MODIFIED, {&PhotosPoWriter::GetDataModified, &PhotosPoWriter::SetDataModified}},
        {PhotoColumn::MEDIA_DATE_TAKEN, {&PhotosPoWriter::GetDataTaken, &PhotosPoWriter::SetDataTaken}},
        {PhotoColumn::MEDIA_DURATION, {&PhotosPoWriter::GetDuration, &PhotosPoWriter::SetDuration}},
        {PhotoColumn::MEDIA_IS_FAV, {&PhotosPoWriter::GetIsFavorite, &PhotosPoWriter::SetIsFavorite}},
        {PhotoColumn::MEDIA_DATE_TRASHED, {&PhotosPoWriter::GetDataTrashed, &PhotosPoWriter::SetDataTrashed}},
        {PhotoColumn::MEDIA_HIDDEN, {&PhotosPoWriter::GetHidden, &PhotosPoWriter::SetHidden}},
        {PhotoColumn::PHOTO_HIDDEN_TIME, {&PhotosPoWriter::GetHiddenTime, &PhotosPoWriter::SetHiddenTime}},
        {PhotoColumn::MEDIA_RELATIVE_PATH, {&PhotosPoWriter::GetRelativePath, &PhotosPoWriter::SetRelativePath}},
        {PhotoColumn::MEDIA_VIRTUAL_PATH, {&PhotosPoWriter::GetVirtualPath, &PhotosPoWriter::SetVirtualPath}},
        {PhotoColumn::PHOTO_META_DATE_MODIFIED,
            {&PhotosPoWriter::GetMetaDataModified, &PhotosPoWriter::SetMetaDataModified}},
        {PhotoColumn::PHOTO_ORIENTATION, {&PhotosPoWriter::GetOrientation, &PhotosPoWriter::SetOrientation}},
        {PhotoColumn::PHOTO_LATITUDE, {&PhotosPoWriter::GetLatitude, &PhotosPoWriter::SetLatitude}},
        {PhotoColumn::PHOTO_LONGITUDE, {&PhotosPoWriter::GetLongitude, &PhotosPoWriter::SetLongitude}},
        {PhotoColumn::PHOTO_HEIGHT, {&PhotosPoWriter::GetHeight, &PhotosPoWriter::SetHeight}},
        {PhotoColumn::PHOTO_WIDTH, {&PhotosPoWriter::GetWidth, &PhotosPoWriter::SetWidth}},
        {PhotoColumn::PHOTO_SUBTYPE, {&PhotosPoWriter::GetSubType, &PhotosPoWriter::SetSubType}},
        {PhotoColumn::PHOTO_BURST_COVER_LEVEL,
            {&PhotosPoWriter::GetBurstCoverLevel, &PhotosPoWriter::SetBurstCoverLevel}},
        {PhotoColumn::PHOTO_BURST_KEY, {&PhotosPoWriter::GetBurstKey, &PhotosPoWriter::SetBurstKey}},
        {PhotoColumn::PHOTO_DATE_YEAR, {&PhotosPoWriter::GetDataYear, &PhotosPoWriter::SetDataYear}},
        {PhotoColumn::PHOTO_DATE_MONTH, {&PhotosPoWriter::GetDataMonth, &PhotosPoWriter::SetDataMonth}},
        {PhotoColumn::PHOTO_DATE_DAY, {&PhotosPoWriter::GetDataDay, &PhotosPoWriter::SetDataDay}},
        {PhotoColumn::PHOTO_USER_COMMENT, {&PhotosPoWriter::GetUserCommnt, &PhotosPoWriter::SetUserCommnt}},
        {PhotoColumn::PHOTO_THUMB_STATUS, {&PhotosPoWriter::GetThumbStatus, &PhotosPoWriter::SetThumbStatus}},
        {PhotoColumn::PHOTO_SYNC_STATUS, {&PhotosPoWriter::GetSyncStatus, &PhotosPoWriter::SetSyncStatus}},
        {PhotoColumn::PHOTO_SHOOTING_MODE, {&PhotosPoWriter::GetShootingMode, &PhotosPoWriter::SetShootingMode}},
        {PhotoColumn::PHOTO_SHOOTING_MODE_TAG,
            {&PhotosPoWriter::GetShootingModeTag, &PhotosPoWriter::SetShootingModeTag}},
        {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
            {&PhotosPoWriter::GetDynamicRangType, &PhotosPoWriter::SetDynamicRangType}},
        {PhotoColumn::PHOTO_FRONT_CAMERA, {&PhotosPoWriter::GetFrontCamera, &PhotosPoWriter::SetFrontCamera}},
        {PhotoColumn::PHOTO_DETAIL_TIME, {&PhotosPoWriter::GetDetailTime, &PhotosPoWriter::SetDetailTime}},
        {PhotoColumn::PHOTO_EDIT_TIME, {&PhotosPoWriter::GetEditTime, &PhotosPoWriter::SetEditTime}},
        {PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
            {&PhotosPoWriter::GetOriginalSubtype, &PhotosPoWriter::SetOriginalSubtype}},
        {PhotoColumn::PHOTO_COVER_POSITION, {&PhotosPoWriter::GetCoverPosition, &PhotosPoWriter::SetCoverPosition}},
        {PhotoColumn::PHOTO_IS_RECTIFICATION_COVER,
            {&PhotosPoWriter::GetIsRectificationCover, &PhotosPoWriter::SetIsRectificationCover}},
        {PhotoColumn::PHOTO_EXIF_ROTATE, {&PhotosPoWriter::GetExifRotate, &PhotosPoWriter::SetExifRotate}},
        {PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
            {&PhotosPoWriter::GetPhotoEffectMode, &PhotosPoWriter::SetPhotoEffectMode}},
        {PhotoColumn::PHOTO_OWNER_ALBUM_ID, {&PhotosPoWriter::GetOwnerAlbumId, &PhotosPoWriter::SetOwnerAlbumId}},
        {PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID,
            {&PhotosPoWriter::GetOriginalAssetCloudId, &PhotosPoWriter::SetOriginalAssetCloudId}},
        {PhotoColumn::PHOTO_SOURCE_PATH, {&PhotosPoWriter::GetSourcePath, &PhotosPoWriter::SetSourcePath}},
        {PhotoColumn::SUPPORTED_WATERMARK_TYPE,
            {&PhotosPoWriter::GetSupportedWatermarkType, &PhotosPoWriter::SetSupportedWatermarkType}},
        {PhotoColumn::PHOTO_STRONG_ASSOCIATION,
            {&PhotosPoWriter::GetStrongAssociation, &PhotosPoWriter::SetStrongAssociation}},
        {MediaColumn::MEDIA_ID, {&PhotosPoWriter::GetMediaId, &PhotosPoWriter::SetMediaId}},
        {PhotoColumn::PHOTO_CLOUD_ID, {&PhotosPoWriter::GetCloudId, &PhotosPoWriter::SetCloudId}},
        {PhotoColumn::PHOTO_DIRTY, {&PhotosPoWriter::GetDirty, &PhotosPoWriter::SetDirty}},
        {PhotoColumn::PHOTO_POSITION, {&PhotosPoWriter::GetPosition, &PhotosPoWriter::SetPosition}},
        {PhotoColumn::PHOTO_CLOUD_VERSION, {&PhotosPoWriter::GetCloudVersion, &PhotosPoWriter::SetCloudVersion}},
        {PhotoColumn::PHOTO_LCD_SIZE, {&PhotosPoWriter::GetLcdSize, &PhotosPoWriter::SetLcdSize}},
        {PhotoColumn::PHOTO_THUMB_SIZE, {&PhotosPoWriter::GetThumbSize, &PhotosPoWriter::SetThumbSize}},
        {PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE,
            {&PhotosPoWriter::GetSouthDeviceType, &PhotosPoWriter::SetSouthDeviceType}},
    };
    const std::map<std::string, GetSetNode> EXTRA_HANDLERS = {
        {"album_cloud_id", {&PhotosPoWriter::GetAlbumCloudId, &PhotosPoWriter::SetAlbumCloudId}},
        {"lpath", {&PhotosPoWriter::GetlPath, &PhotosPoWriter::SetlPath}},
    };
    const int32_t PRECISION_LOCATION = 15; // The precision of latitude and longitude is 15 digits.

private:
    void SetMediaFilePath(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaFilePath(std::string &val);
    void SetMediaTitle(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaTitle(std::string &val);
    void SetMediaSize(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaSize(std::string &val);
    void SetMediaName(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaName(std::string &val);
    void SetMediaType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaType(std::string &val);
    void SetMediaMimeType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaMimeType(std::string &val);
    void SetMediaDeviceName(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaDeviceName(std::string &val);
    void SetMediaDataAdded(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaDataAdded(std::string &val);
    void SetDataModified(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDataModified(std::string &val);
    void SetDataTaken(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDataTaken(std::string &val);
    void SetDuration(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDuration(std::string &val);
    void SetIsFavorite(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetIsFavorite(std::string &val);
    void SetDataTrashed(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDataTrashed(std::string &val);
    void SetHidden(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetHidden(std::string &val);
    void SetHiddenTime(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetHiddenTime(std::string &val);
    void SetRelativePath(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetRelativePath(std::string &val);
    void SetVirtualPath(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetVirtualPath(std::string &val);
    void SetMetaDataModified(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMetaDataModified(std::string &val);
    void SetOrientation(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetOrientation(std::string &val);
    void SetLatitude(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetLatitude(std::string &val);
    void SetLongitude(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetLongitude(std::string &val);
    void SetHeight(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetHeight(std::string &val);
    void SetWidth(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetWidth(std::string &val);
    void SetSubType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetSubType(std::string &val);
    void SetBurstCoverLevel(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetBurstCoverLevel(std::string &val);
    void SetBurstKey(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetBurstKey(std::string &val);
    void SetDataYear(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDataYear(std::string &val);
    void SetDataMonth(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDataMonth(std::string &val);
    void SetDataDay(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDataDay(std::string &val);
    void SetUserCommnt(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetUserCommnt(std::string &val);
    void SetThumbStatus(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetThumbStatus(std::string &val);
    void SetSyncStatus(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetSyncStatus(std::string &val);
    void SetShootingMode(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetShootingMode(std::string &val);
    void SetShootingModeTag(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetShootingModeTag(std::string &val);
    void SetDynamicRangType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDynamicRangType(std::string &val);
    void SetFrontCamera(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetFrontCamera(std::string &val);
    void SetDetailTime(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDetailTime(std::string &val);
    void SetEditTime(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetEditTime(std::string &val);
    void SetOriginalSubtype(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetOriginalSubtype(std::string &val);
    void SetCoverPosition(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetCoverPosition(std::string &val);
    void SetIsRectificationCover(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetIsRectificationCover(std::string &val);
    void SetExifRotate(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetExifRotate(std::string &val);
    void SetPhotoEffectMode(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetPhotoEffectMode(std::string &val);
    void SetOwnerAlbumId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetOwnerAlbumId(std::string &val);
    void SetOriginalAssetCloudId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetOriginalAssetCloudId(std::string &val);
    void SetSourcePath(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetSourcePath(std::string &val);
    void SetSupportedWatermarkType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetSupportedWatermarkType(std::string &val);
    void SetStrongAssociation(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetStrongAssociation(std::string &val);
    void SetMediaId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetMediaId(std::string &val);
    void SetCloudId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetCloudId(std::string &val);
    void SetDirty(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDirty(std::string &val);
    void SetPosition(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetPosition(std::string &val);
    void SetCloudVersion(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetCloudVersion(std::string &val);
    void SetAlbumCloudId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumCloudId(std::string &val);
    void SetlPath(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetlPath(std::string &val);
    void SetLcdSize(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetLcdSize(std::string &val);
    void SetThumbSize(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetThumbSize(std::string &val);
    std::string GetStringValByPrecision(const double doubleVal, const int32_t precision);
    void SetSouthDeviceType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetSouthDeviceType(std::string &val);
};
}  // namespace OHOS::Media::ORM
#endif  // OHOS_MEDIA_ORM_PHOTOS_PO_WRITER_H
