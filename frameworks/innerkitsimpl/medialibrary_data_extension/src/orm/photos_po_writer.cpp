/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Media_ORM"

#include "photos_po_writer.h"

namespace OHOS::Media::ORM {
int32_t PhotosPoWriter::SetMemberVariable(
    const std::string &name, std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool isIdentify = false;
    auto it = this->HANDLERS.find(name);
    if (it != this->HANDLERS.end()) {
        (this->*(it->second.funSetPtr))(val);
        isIdentify = true;
    }
    auto extraIt = this->EXTRA_HANDLERS.find(name);
    if (extraIt != this->EXTRA_HANDLERS.end()) {
        (this->*(extraIt->second.funSetPtr))(val);
        isIdentify = true;
    }
    CHECK_AND_RETURN_RET(!isIdentify, E_OK);
    // set columnName and columnValue to attributes
    std::string columnValue = "";
    if (std::holds_alternative<int32_t>(val)) {
        columnValue = std::to_string(std::get<int32_t>(val));
    } else if (std::holds_alternative<int64_t>(val)) {
        columnValue = std::to_string(std::get<int64_t>(val));
    } else if (std::holds_alternative<double>(val)) {
        columnValue = std::to_string(std::get<double>(val));
    } else if (std::holds_alternative<std::string>(val)) {
        columnValue = std::get<std::string>(val);
    } else {
        MEDIA_ERR_LOG("PhotosPoWriter: SetMemberVariable: variant type is not supported");
    }
    CHECK_AND_RETURN_RET(!columnValue.empty(), E_OK);
    this->photosPo_.attributes[name] = columnValue;
    return E_OK;
}

std::unordered_map<std::string, std::string> PhotosPoWriter::ToMap(bool isIdentifyOnly)
{
    std::string val;
    std::unordered_map<std::string, std::string> res;
    for (const auto &pair : HANDLERS) {
        CHECK_AND_CONTINUE((this->*(pair.second.funGetPtr))(val));
        res[pair.first] = val;
    }
    // isIdentifyOnly is true, only return identify column values.
    CHECK_AND_RETURN_RET(!isIdentifyOnly, res);
    bool isValid = true;
    for (const auto &pair : this->photosPo_.attributes) {
        isValid = res.find(pair.first) == res.end();
        CHECK_AND_CONTINUE(isValid);  // if key is already in res, skip to avoid overwriting.
        res[pair.first] = pair.second;
    }
    return res;
}

void PhotosPoWriter::SetMediaFilePath(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.data = std::get<std::string>(val);
}

bool PhotosPoWriter::GetMediaFilePath(std::string &val)
{
    CHECK_AND_RETURN_RET(this->photosPo_.data.has_value(), false);
    val = this->photosPo_.data.value();
    return true;
}

void PhotosPoWriter::SetMediaTitle(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.title = std::get<std::string>(val);
}

bool PhotosPoWriter::GetMediaTitle(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.title.has_value(), false);
    val = this->photosPo_.title.value();
    return true;
}

void PhotosPoWriter::SetMediaSize(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.size = std::get<int64_t>(val);
}

bool PhotosPoWriter::GetMediaSize(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.size.has_value(), false);
    val = std::to_string(this->photosPo_.size.value());
    return true;
}

void PhotosPoWriter::SetMediaName(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.displayName = std::get<std::string>(val);
}
bool PhotosPoWriter::GetMediaName(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.displayName.has_value(), false);
    val = this->photosPo_.displayName.value();
    return true;
}
void PhotosPoWriter::SetMediaType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.mediaType = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetMediaType(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.mediaType.has_value(), false);
    val = std::to_string(this->photosPo_.mediaType.value());
    return true;
}
void PhotosPoWriter::SetMediaMimeType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.mimeType = std::get<std::string>(val);
}
bool PhotosPoWriter::GetMediaMimeType(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.mimeType.has_value(), false);
    val = this->photosPo_.mimeType.value();
    return true;
}
void PhotosPoWriter::SetMediaDeviceName(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.deviceName = std::get<std::string>(val);
}
bool PhotosPoWriter::GetMediaDeviceName(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.deviceName.has_value(), false);
    val = this->photosPo_.deviceName.value();
    return true;
}
void PhotosPoWriter::SetMediaDataAdded(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateAdded = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetMediaDataAdded(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateAdded.has_value(), false);
    val = std::to_string(this->photosPo_.dateAdded.value());
    return true;
}
void PhotosPoWriter::SetDataModified(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateModified = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetDataModified(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateModified.has_value(), false);
    val = std::to_string(this->photosPo_.dateModified.value());
    return true;
}
void PhotosPoWriter::SetDataTaken(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateTaken = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetDataTaken(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateTaken.has_value(), false);
    val = std::to_string(this->photosPo_.dateTaken.value());
    return true;
}
void PhotosPoWriter::SetDuration(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.duration = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetDuration(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.duration.has_value(), false);
    val = std::to_string(this->photosPo_.duration.value());
    return true;
}
void PhotosPoWriter::SetIsFavorite(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.isFavorite = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetIsFavorite(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.isFavorite.has_value(), false);
    val = std::to_string(this->photosPo_.isFavorite.value());
    return true;
}
void PhotosPoWriter::SetDataTrashed(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateTrashed = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetDataTrashed(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateTrashed.has_value(), false);
    val = std::to_string(this->photosPo_.dateTrashed.value());
    return true;
}
void PhotosPoWriter::SetHidden(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.hidden = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetHidden(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.hidden.has_value(), false);
    val = std::to_string(this->photosPo_.hidden.value());
    return true;
}
void PhotosPoWriter::SetHiddenTime(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.hiddenTime = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetHiddenTime(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.hiddenTime.has_value(), false);
    val = std::to_string(this->photosPo_.hiddenTime.value());
    return true;
}
void PhotosPoWriter::SetRelativePath(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.relativePath = std::get<std::string>(val);
}
bool PhotosPoWriter::GetRelativePath(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.relativePath.has_value(), false);
    val = this->photosPo_.relativePath.value();
    return true;
}
void PhotosPoWriter::SetVirtualPath(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.virtualPath = std::get<std::string>(val);
}
bool PhotosPoWriter::GetVirtualPath(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.virtualPath.has_value(), false);
    val = this->photosPo_.virtualPath.value();
    return true;
}
void PhotosPoWriter::SetMetaDataModified(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.metaDateModified = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetMetaDataModified(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.metaDateModified.has_value(), false);
    val = std::to_string(this->photosPo_.metaDateModified.value());
    return true;
}
void PhotosPoWriter::SetOrientation(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.orientation = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetOrientation(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.orientation.has_value(), false);
    val = std::to_string(this->photosPo_.orientation.value());
    return true;
}
void PhotosPoWriter::SetLatitude(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<double>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.latitude = std::get<double>(val);
}
bool PhotosPoWriter::GetLatitude(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.latitude.has_value(), false);
    val = this->GetStringValByPrecision(photosPo_.latitude.value(), this->PRECISION_LOCATION);
    return true;
}
void PhotosPoWriter::SetLongitude(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<double>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.longitude = std::get<double>(val);
}
bool PhotosPoWriter::GetLongitude(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.longitude.has_value(), false);
    val = this->GetStringValByPrecision(photosPo_.longitude.value(), this->PRECISION_LOCATION);
    return true;
}
void PhotosPoWriter::SetHeight(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.height = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetHeight(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.height.has_value(), false);
    val = std::to_string(this->photosPo_.height.value());
    return true;
}
void PhotosPoWriter::SetWidth(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.width = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetWidth(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.width.has_value(), false);
    val = std::to_string(this->photosPo_.width.value());
    return true;
}
void PhotosPoWriter::SetSubType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.subtype = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetSubType(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.subtype.has_value(), false);
    val = std::to_string(this->photosPo_.subtype.value());
    return true;
}
void PhotosPoWriter::SetBurstCoverLevel(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.burstCoverLevel = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetBurstCoverLevel(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.burstCoverLevel.has_value(), false);
    val = std::to_string(this->photosPo_.burstCoverLevel.value());
    return true;
}
void PhotosPoWriter::SetBurstKey(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.burstKey = std::get<std::string>(val);
}
bool PhotosPoWriter::GetBurstKey(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.burstKey.has_value(), false);
    val = this->photosPo_.burstKey.value();
    return true;
}
void PhotosPoWriter::SetDataYear(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateYear = std::get<std::string>(val);
}
bool PhotosPoWriter::GetDataYear(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateYear.has_value(), false);
    val = this->photosPo_.dateYear.value();
    return true;
}
void PhotosPoWriter::SetDataMonth(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateMonth = std::get<std::string>(val);
}
bool PhotosPoWriter::GetDataMonth(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateMonth.has_value(), false);
    val = this->photosPo_.dateMonth.value();
    return true;
}
void PhotosPoWriter::SetDataDay(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dateDay = std::get<std::string>(val);
}
bool PhotosPoWriter::GetDataDay(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dateDay.has_value(), false);
    val = this->photosPo_.dateDay.value();
    return true;
}
void PhotosPoWriter::SetUserCommnt(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.userComment = std::get<std::string>(val);
}
bool PhotosPoWriter::GetUserCommnt(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.userComment.has_value(), false);
    val = this->photosPo_.userComment.value();
    return true;
}
void PhotosPoWriter::SetThumbStatus(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.thumbStatus = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetThumbStatus(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.thumbStatus.has_value(), false);
    val = std::to_string(this->photosPo_.thumbStatus.value());
    return true;
}
void PhotosPoWriter::SetSyncStatus(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.syncStatus = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetSyncStatus(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.syncStatus.has_value(), false);
    val = std::to_string(this->photosPo_.syncStatus.value());
    return true;
}
void PhotosPoWriter::SetShootingMode(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.shootingMode = std::get<std::string>(val);
}
bool PhotosPoWriter::GetShootingMode(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.shootingMode.has_value(), false);
    val = this->photosPo_.shootingMode.value();
    return true;
}
void PhotosPoWriter::SetShootingModeTag(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.shootingModeTag = std::get<std::string>(val);
}
bool PhotosPoWriter::GetShootingModeTag(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.shootingModeTag.has_value(), false);
    val = this->photosPo_.shootingModeTag.value();
    return true;
}
void PhotosPoWriter::SetDynamicRangType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dynamicRangeType = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetDynamicRangType(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dynamicRangeType.has_value(), false);
    val = std::to_string(this->photosPo_.dynamicRangeType.value());
    return true;
}
void PhotosPoWriter::SetFrontCamera(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.frontCamera = std::get<std::string>(val);
}
bool PhotosPoWriter::GetFrontCamera(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.frontCamera.has_value(), false);
    val = this->photosPo_.frontCamera.value();
    return true;
}
void PhotosPoWriter::SetDetailTime(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.detailTime = std::get<std::string>(val);
}
bool PhotosPoWriter::GetDetailTime(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.detailTime.has_value(), false);
    val = this->photosPo_.detailTime.value();
    return true;
}
void PhotosPoWriter::SetEditTime(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.editTime = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetEditTime(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.editTime.has_value(), false);
    val = std::to_string(this->photosPo_.editTime.value());
    return true;
}
void PhotosPoWriter::SetOriginalSubtype(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.originalSubtype = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetOriginalSubtype(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.originalSubtype.has_value(), false);
    val = std::to_string(this->photosPo_.originalSubtype.value());
    return true;
}
void PhotosPoWriter::SetCoverPosition(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.coverPosition = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetCoverPosition(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.coverPosition.has_value(), false);
    val = std::to_string(this->photosPo_.coverPosition.value());
    return true;
}
void PhotosPoWriter::SetIsRectificationCover(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.isRectificationCover = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetIsRectificationCover(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.isRectificationCover.has_value(), false);
    val = std::to_string(this->photosPo_.isRectificationCover.value());
    return true;
}
void PhotosPoWriter::SetExifRotate(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.exifRotate = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetExifRotate(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.exifRotate.has_value(), false);
    val = std::to_string(this->photosPo_.exifRotate.value());
    return true;
}
void PhotosPoWriter::SetPhotoEffectMode(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.movingPhotoEffectMode = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetPhotoEffectMode(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.movingPhotoEffectMode.has_value(), false);
    val = std::to_string(this->photosPo_.movingPhotoEffectMode.value());
    return true;
}
void PhotosPoWriter::SetOwnerAlbumId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.ownerAlbumId = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetOwnerAlbumId(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.ownerAlbumId.has_value(), false);
    val = std::to_string(this->photosPo_.ownerAlbumId.value());
    return true;
}
void PhotosPoWriter::SetOriginalAssetCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.originalAssetCloudId = std::get<std::string>(val);
}
bool PhotosPoWriter::GetOriginalAssetCloudId(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.originalAssetCloudId.has_value(), false);
    val = this->photosPo_.originalAssetCloudId.value();
    return true;
}
void PhotosPoWriter::SetSourcePath(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.sourcePath = std::get<std::string>(val);
}
bool PhotosPoWriter::GetSourcePath(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.sourcePath.has_value(), false);
    val = this->photosPo_.sourcePath.value();
    return true;
}
void PhotosPoWriter::SetSupportedWatermarkType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.supportedWatermarkType = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetSupportedWatermarkType(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.supportedWatermarkType.has_value(), false);
    val = std::to_string(this->photosPo_.supportedWatermarkType.value());
    return true;
}
void PhotosPoWriter::SetStrongAssociation(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.strongAssociation = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetStrongAssociation(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.strongAssociation.has_value(), false);
    val = std::to_string(this->photosPo_.strongAssociation.value());
    return true;
}
void PhotosPoWriter::SetMediaId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.fileId = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetMediaId(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.fileId.has_value(), false);
    val = std::to_string(this->photosPo_.fileId.value());
    return true;
}
void PhotosPoWriter::SetCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.cloudId = std::get<std::string>(val);
}
bool PhotosPoWriter::GetCloudId(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.cloudId.has_value(), false);
    val = this->photosPo_.cloudId.value();
    return true;
}
void PhotosPoWriter::SetDirty(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.dirty = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetDirty(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.dirty.has_value(), false);
    val = std::to_string(this->photosPo_.dirty.value());
    return true;
}
void PhotosPoWriter::SetPosition(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.position = std::get<int32_t>(val);
}
bool PhotosPoWriter::GetPosition(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.position.has_value(), false);
    val = std::to_string(this->photosPo_.position.value());
    return true;
}
void PhotosPoWriter::SetCloudVersion(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int64_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.cloudVersion = std::get<int64_t>(val);
}
bool PhotosPoWriter::GetCloudVersion(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.cloudVersion.has_value(), false);
    val = std::to_string(this->photosPo_.cloudVersion.value());
    return true;
}
void PhotosPoWriter::SetAlbumCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.albumCloudId = std::get<std::string>(val);
}
bool PhotosPoWriter::GetAlbumCloudId(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.albumCloudId.has_value(), false);
    val = this->photosPo_.albumCloudId.value();
    return true;
}
void PhotosPoWriter::SetlPath(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.albumLPath = std::get<std::string>(val);
}
bool PhotosPoWriter::GetlPath(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.albumLPath.has_value(), false);
    val = this->photosPo_.albumLPath.value();
    return true;
}
void PhotosPoWriter::SetLcdSize(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.lcdSize = std::get<std::string>(val);
}
bool PhotosPoWriter::GetLcdSize(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.lcdSize.has_value(), false);
    val = this->photosPo_.lcdSize.value();
    return true;
}
void PhotosPoWriter::SetThumbSize(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<std::string>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.thumbSize = std::get<std::string>(val);
}
bool PhotosPoWriter::GetThumbSize(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.thumbSize.has_value(), false);
    val = this->photosPo_.thumbSize.value();
    return true;
}

std::string PhotosPoWriter::GetStringValByPrecision(const double doubleVal, const int32_t precision)
{
    std::stringstream stream;
    stream.precision(precision);
    stream << doubleVal;
    return stream.str();
}

void PhotosPoWriter::SetSouthDeviceType(std::variant<int32_t, int64_t, double, std::string> &val)
{
    bool errConn = !std::holds_alternative<int32_t>(val);
    CHECK_AND_RETURN(!errConn);
    this->photosPo_.southDeviceType = std::get<int32_t>(val);
}

bool PhotosPoWriter::GetSouthDeviceType(std::string &val)
{
    CHECK_AND_RETURN_RET(photosPo_.southDeviceType.has_value(), false);
    val = std::to_string(this->photosPo_.southDeviceType.value());
    return true;
}
}  // namespace OHOS::Media::ORM