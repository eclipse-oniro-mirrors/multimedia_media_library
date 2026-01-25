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

#define MLOG_TAG "AccurateRefresh::PhotoAssetChangeInfo"

#include <sstream>

#include "parameters.h"
#include "photo_asset_change_info.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "photo_album_column.h"
#include "accurate_debug_log.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media::AccurateRefresh {

int32_t accurateDebugLevel = std::stoi(system::GetParameter("ohos.medialibrary.accurate_refreh.debug", "3"));

const map<std::string, ResultSetDataType> PhotoAssetChangeInfo::photoAssetCloumnTypes_ = {
    { PhotoColumn::MEDIA_ID, TYPE_INT32 },
    { PhotoColumn::PHOTO_DATE_DAY, TYPE_STRING },

    { PhotoColumn::MEDIA_IS_FAV, TYPE_INT32 },
    { PhotoColumn::MEDIA_TYPE, TYPE_INT32 },
    { PhotoColumn::MEDIA_HIDDEN, TYPE_INT32 },
    { PhotoColumn::MEDIA_DATE_TRASHED, TYPE_INT64 },
    { PhotoColumn::PHOTO_STRONG_ASSOCIATION, TYPE_INT32 },
    { PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, TYPE_INT32 },

    { PhotoColumn::MEDIA_DATE_ADDED, TYPE_INT64 },
    { PhotoColumn::MEDIA_DATE_TAKEN, TYPE_INT64 },

    { PhotoColumn::PHOTO_SUBTYPE, TYPE_INT32 },
    { PhotoColumn::PHOTO_SYNC_STATUS, TYPE_INT32 },
    { PhotoColumn::PHOTO_CLEAN_FLAG, TYPE_INT32 },
    { PhotoColumn::MEDIA_TIME_PENDING, TYPE_INT32 },
    { PhotoColumn::PHOTO_IS_TEMP, TYPE_INT32 },
    { PhotoColumn::PHOTO_BURST_COVER_LEVEL, TYPE_INT32 },

    { PhotoColumn::PHOTO_OWNER_ALBUM_ID, TYPE_INT32 },
    { PhotoColumn::PHOTO_HIDDEN_TIME, TYPE_INT64 },
    { PhotoColumn::PHOTO_THUMBNAIL_READY, TYPE_INT64 },
    { PhotoColumn::MEDIA_NAME, TYPE_STRING },
    { PhotoColumn::MEDIA_FILE_PATH, TYPE_STRING },
    { PhotoColumn::PHOTO_DIRTY, TYPE_INT32 },
    { PhotoColumn::PHOTO_POSITION, TYPE_INT32 },
    { PhotoColumn::MEDIA_SIZE, TYPE_INT64 },
    { PhotoColumn::PHOTO_FILE_SOURCE_TYPE, TYPE_INT32 },
};

const vector<std::string> PhotoAssetChangeInfo::photoAssetColumns_ = []() {
    vector<string> result;
    for (const auto &item : photoAssetCloumnTypes_) {
        result.push_back(item.first);
    }
    return result;
}();

const vector<string>& PhotoAssetChangeInfo::GetPhotoAssetColumns()
{
    return photoAssetColumns_;
}

vector<PhotoAssetChangeInfo> PhotoAssetChangeInfo::GetInfoFromResult(const shared_ptr<ResultSet> &resultSet,
    const vector<string> &columns)
{
    vector<PhotoAssetChangeInfo> assetChangeInfos;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PhotoAssetChangeInfo assetChangeInfo;
        assetChangeInfo.fileSourceType_ =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
            resultSet, GetDataType(PhotoColumn::PHOTO_FILE_SOURCE_TYPE)));
        if (assetChangeInfo.fileSourceType_ == static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) {
            continue;
        }
        assetChangeInfo.fileId_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet,
            GetDataType(PhotoColumn::MEDIA_ID)));
        assetChangeInfo.dateDay_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_DATE_DAY,
            resultSet, GetDataType(PhotoColumn::PHOTO_DATE_DAY)));
        assetChangeInfo.isFavorite_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_IS_FAV,
            resultSet, GetDataType(PhotoColumn::MEDIA_IS_FAV)));
        assetChangeInfo.mediaType_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_TYPE, resultSet,
            GetDataType(PhotoColumn::MEDIA_TYPE)));
        assetChangeInfo.isHidden_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_HIDDEN, resultSet,
            GetDataType(PhotoColumn::MEDIA_HIDDEN)));
        assetChangeInfo.dateTrashedMs_ = get<int64_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_DATE_TRASHED,
            resultSet, GetDataType(PhotoColumn::MEDIA_DATE_TRASHED)));
        assetChangeInfo.strongAssociation_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_STRONG_ASSOCIATION, resultSet, GetDataType(PhotoColumn::PHOTO_STRONG_ASSOCIATION)));
        assetChangeInfo.thumbnailVisible_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, resultSet, GetDataType(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE)));
        assetChangeInfo.dateAddedMs_ = get<int64_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_DATE_ADDED,
            resultSet, GetDataType(PhotoColumn::MEDIA_DATE_ADDED)));
        assetChangeInfo.dateTakenMs_ = get<int64_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_DATE_TAKEN,
            resultSet, GetDataType(PhotoColumn::MEDIA_DATE_TAKEN)));
        assetChangeInfo.subType_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet,
            GetDataType(PhotoColumn::PHOTO_SUBTYPE)));
        assetChangeInfo.syncStatus_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SYNC_STATUS,
            resultSet, GetDataType(PhotoColumn::PHOTO_SYNC_STATUS)));
        assetChangeInfo.cleanFlag_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_CLEAN_FLAG,
            resultSet, GetDataType(PhotoColumn::PHOTO_CLEAN_FLAG)));
        assetChangeInfo.timePending_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_TIME_PENDING,
            resultSet, GetDataType(PhotoColumn::MEDIA_TIME_PENDING)));
        assetChangeInfo.isTemp_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_IS_TEMP, resultSet,
            GetDataType(PhotoColumn::PHOTO_IS_TEMP)));
        assetChangeInfo.burstCoverLevel_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet, GetDataType(PhotoColumn::PHOTO_BURST_COVER_LEVEL)));
        assetChangeInfo.ownerAlbumId_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet, GetDataType(PhotoColumn::PHOTO_OWNER_ALBUM_ID)));
        assetChangeInfo.hiddenTime_ = get<int64_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_HIDDEN_TIME,
            resultSet, GetDataType(PhotoColumn::PHOTO_HIDDEN_TIME)));
        assetChangeInfo.thumbnailReady_ = get<int64_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_THUMBNAIL_READY, resultSet, GetDataType(PhotoColumn::PHOTO_THUMBNAIL_READY)));
        assetChangeInfo.displayName_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_NAME, resultSet,
            GetDataType(PhotoColumn::MEDIA_NAME)));
        assetChangeInfo.path_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_FILE_PATH, resultSet,
            GetDataType(PhotoColumn::MEDIA_FILE_PATH)));
        assetChangeInfo.dirty_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_DIRTY, resultSet, GetDataType(PhotoColumn::PHOTO_DIRTY)));

        assetChangeInfo.uri_ = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX,
            to_string(assetChangeInfo.fileId_), MediaFileUtils::GetExtraUri(assetChangeInfo.displayName_,
            assetChangeInfo.path_));
        if (assetChangeInfo.ownerAlbumId_ > 0) {
            assetChangeInfo.ownerAlbumUri_ = MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
                to_string(assetChangeInfo.ownerAlbumId_));
        }
        assetChangeInfo.position_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::PHOTO_POSITION, resultSet, GetDataType(PhotoColumn::PHOTO_POSITION)));
        assetChangeInfo.size_ = get<int64_t>(ResultSetUtils::GetValFromColumn(
            PhotoColumn::MEDIA_SIZE, resultSet, GetDataType(PhotoColumn::MEDIA_SIZE)));
        assetChangeInfos.push_back(assetChangeInfo);
    }
    return assetChangeInfos;
}

ResultSetDataType PhotoAssetChangeInfo::GetDataType(const std::string &column)
{
    auto const &item = photoAssetCloumnTypes_.find(column);
    if (item == photoAssetCloumnTypes_.end()) {
        return TYPE_NULL;
    }
    return item->second;
}

string PhotoAssetChangeInfo::ToString(bool isDetail) const
{
    stringstream ss;
    if (isDetail) {
        ss << "fileId_: " << fileId_ << ", ownerAlbumId_: " << ownerAlbumId_ << ", uri_: ";
        ss << MediaFileUtils::DesensitizeUri(uri_);
        ss << ", dateDay_: " << dateDay_ << ", ownerAlbumUri_: " << ownerAlbumUri_ << ", isFavorite_: " << isFavorite_;
        ss << ", mediaType_: " << mediaType_;
        ss << ", isHidden_: " << isHidden_ << ", dateTrashedMs_: " << dateTrashedMs_;
        ss << ", strongAssociation_: " << strongAssociation_;
        ss << ", thumbnailVisible_: " << thumbnailVisible_ << ", dateAddedMs_: " << dateAddedMs_;
        ss << ", dateTakenMs_: " << dateTakenMs_ << ", subType_: " << subType_;
        ss << ", syncStatus_: " << syncStatus_ << ", cleanFlag_: " << cleanFlag_;
        ss << ", timePending_: " << timePending_ << ", isTemp_: " << isTemp_;
        ss << ", burstCoverLevel_: " << burstCoverLevel_;
        ss << ", hiddenTime_: " << hiddenTime_ << ", thumbnailReady_: " << thumbnailReady_;
        ss << ", displayName_: " << MediaFileUtils::DesensitizeDisplayName(displayName_);
        ss << ", path_: " << MediaFileUtils::DesensitizePath(path_) << ", dirty_: " << dirty_;
        ss << ", position_: " <<position_ << ", size_: " << size_;
    } else {
        ss << "fileId_: " << fileId_ << ", ownerAlbumId_: " << ownerAlbumId_;
    }
    return ss.str();
}

bool PhotoAssetChangeInfo::Marshalling(Parcel &parcel) const
{
    return Marshalling(parcel, false);
}

bool PhotoAssetChangeInfo::Marshalling(Parcel &parcel, bool isSystem) const
{
    bool isValid = fileId_ != INVALID_INT32_VALUE;
    bool ret = parcel.WriteBool(isValid);
    if (ret && !isValid) {
        return ret;
    }
    ret = ret && parcel.WriteString(uri_);
    ret = ret && parcel.WriteInt32(mediaType_);
    ret = ret && parcel.WriteString(ownerAlbumUri_);
    ret = ret && parcel.WriteBool(isSystem);
    ret = ret && parcel.WriteInt32(fileId_);
    if (isSystem) {
        ret = ret && parcel.WriteString(dateDay_);
        ret = ret && parcel.WriteBool(isFavorite_);
        ret = ret && parcel.WriteBool(isHidden_);
        ret = ret && parcel.WriteInt32(strongAssociation_);
        ret = ret && parcel.WriteInt32(thumbnailVisible_);
        ret = ret && parcel.WriteInt64(dateTrashedMs_);
        ret = ret && parcel.WriteInt64(dateAddedMs_);
        ret = ret && parcel.WriteInt64(dateTakenMs_);
        ret = ret && parcel.WriteInt32(ownerAlbumId_);
        ret = ret && parcel.WriteInt32(position_);
        ret = ret && parcel.WriteString(displayName_);
        ret = ret && parcel.WriteInt64(size_);
    }
    return ret;
}

bool PhotoAssetChangeInfo::ReadFromParcel(Parcel &parcel)
{
    bool isValid = false;
    bool ret = parcel.ReadBool(isValid);
    if (ret && !isValid) {
        return ret;
    }
    ret = ret && parcel.ReadString(uri_);
    ret = ret && parcel.ReadInt32(mediaType_);
    ret = ret && parcel.ReadString(ownerAlbumUri_);
    ret = ret && parcel.ReadBool(isSystem_);
    ret = ret && parcel.ReadInt32(fileId_);
    if (isSystem_) {
        ret = ret && parcel.ReadString(dateDay_);
        ret = ret && parcel.ReadBool(isFavorite_);
        ret = ret && parcel.ReadBool(isHidden_);
        ret = ret && parcel.ReadInt32(strongAssociation_);
        ret = ret && parcel.ReadInt32(thumbnailVisible_);
        ret = ret && parcel.ReadInt64(dateTrashedMs_);
        ret = ret && parcel.ReadInt64(dateAddedMs_);
        ret = ret && parcel.ReadInt64(dateTakenMs_);
        ret = ret && parcel.ReadInt32(ownerAlbumId_);
        ret = ret && parcel.ReadInt32(position_);
        ret = ret && parcel.ReadString(displayName_);
        ret = ret && parcel.ReadInt64(size_);
    }
    return true;
}

shared_ptr<PhotoAssetChangeInfo> PhotoAssetChangeInfo::Unmarshalling(Parcel &parcel)
{
    shared_ptr<PhotoAssetChangeInfo> assetChangeInfo = make_shared<PhotoAssetChangeInfo>();
    if (assetChangeInfo->ReadFromParcel(parcel)) {
        return assetChangeInfo;
    }
    return nullptr;
}

PhotoAssetChangeInfo& PhotoAssetChangeInfo::operator=(const PhotoAssetChangeInfo &info)
{
    if (this != &info) {
        fileId_ = info.fileId_;
        dateDay_ = info.dateDay_;
        isFavorite_ = info.isFavorite_;
        mediaType_ = info.mediaType_;
        isHidden_ = info.isHidden_;
        dateTrashedMs_ = info.dateTrashedMs_;
        strongAssociation_ = info.strongAssociation_;
        thumbnailVisible_ = info.thumbnailVisible_;
        dateAddedMs_ = info.dateAddedMs_;
        dateTakenMs_ = info.dateTakenMs_;
        subType_ = info.subType_;
        syncStatus_ = info.syncStatus_;
        cleanFlag_ = info.cleanFlag_;
        timePending_ = info.timePending_;
        isTemp_ = info.isTemp_;
        burstCoverLevel_ = info.burstCoverLevel_;
        ownerAlbumId_ = info.ownerAlbumId_;
        hiddenTime_ = info.hiddenTime_;
        thumbnailReady_ = info.thumbnailReady_;
        displayName_ = info.displayName_;
        path_ = info.path_;
        uri_ = info.uri_;
        ownerAlbumUri_ = info.ownerAlbumUri_;
        dirty_ = info.dirty_;
        position_ = info.position_;
        size_ = info.size_;
    }
    return *this;
}
bool PhotoAssetChangeInfo::operator==(const PhotoAssetChangeInfo &info) const
{
    return fileId_ == info.fileId_ &&
        dateDay_ == info.dateDay_ &&
        isFavorite_ == info.isFavorite_ &&
        mediaType_ == info.mediaType_ &&
        isHidden_ == info.isHidden_ &&
        dateTrashedMs_ == info.dateTrashedMs_ &&
        strongAssociation_ == info.strongAssociation_ &&
        thumbnailVisible_ == info.thumbnailVisible_ &&
        dateAddedMs_ == info.dateAddedMs_ &&
        dateTakenMs_ == info.dateTakenMs_ &&
        subType_ == info.subType_ &&
        syncStatus_ == info.syncStatus_ &&
        cleanFlag_ == info.cleanFlag_ &&
        timePending_ == info.timePending_ &&
        isTemp_ == info.isTemp_ &&
        burstCoverLevel_ == info.burstCoverLevel_ &&
        ownerAlbumId_ == info.ownerAlbumId_ &&
        hiddenTime_ == info.hiddenTime_ &&
        thumbnailReady_ == info.thumbnailReady_ &&
        displayName_ == info.displayName_ &&
        path_ == info.path_ &&
        uri_ == info.uri_ &&
        ownerAlbumUri_ == info.ownerAlbumUri_ &&
        dirty_ == info.dirty_ &&
        position_ == info.position_ &&
        size_ == info.size_;
}

bool PhotoAssetChangeInfo::operator!=(const PhotoAssetChangeInfo &info) const
{
    return !((*this) == info);
}

template <typename T>
void GetDiff(const T &value, const T &compareValue, const std::string &name, std::stringstream &ss)
{
    if (value != compareValue) {
        ss << name << ": " << value << " -> " << compareValue << ", ";
    }
}
#define GET_ASSET_DIFF(member) GetDiff(asset.member, compare.member, #member, ss)
string PhotoAssetChangeInfo::GetAssetDiff(const PhotoAssetChangeInfo &asset, const PhotoAssetChangeInfo &compare)
{
    stringstream ss;
    if (asset.fileId_ != compare.fileId_) {
        ss << "diff asset info fileId: " << asset.fileId_ << ", compare fileId: " << compare.fileId_;
        return ss.str();
    }
    ss << "asset info fileId[" << asset.fileId_ << "]: ";
    GET_ASSET_DIFF(uri_);
    GET_ASSET_DIFF(dateDay_);
    GET_ASSET_DIFF(ownerAlbumUri_);
    GET_ASSET_DIFF(isFavorite_);
    GET_ASSET_DIFF(mediaType_);
    GET_ASSET_DIFF(isHidden_);
    GET_ASSET_DIFF(dateTrashedMs_);
    GET_ASSET_DIFF(strongAssociation_);
    GET_ASSET_DIFF(thumbnailVisible_);
    GET_ASSET_DIFF(dateAddedMs_);
    GET_ASSET_DIFF(dateTakenMs_);
    GET_ASSET_DIFF(subType_);
    GET_ASSET_DIFF(syncStatus_);
    GET_ASSET_DIFF(cleanFlag_);
    GET_ASSET_DIFF(timePending_);
    GET_ASSET_DIFF(isTemp_);
    GET_ASSET_DIFF(burstCoverLevel_);
    GET_ASSET_DIFF(ownerAlbumId_);
    GET_ASSET_DIFF(hiddenTime_);
    GET_ASSET_DIFF(dirty_);
    GET_ASSET_DIFF(position_);
    GET_ASSET_DIFF(size_);
    if (asset.displayName_ != compare.displayName_) {
        ss << "displayName_: " << MediaFileUtils::DesensitizeDisplayName(asset.displayName_) << " -> ";
        ss << MediaFileUtils::DesensitizeDisplayName(compare.displayName_);
    }

    if (asset.path_ != compare.path_) {
        ss << "path_: " << MediaFileUtils::DesensitizePath(asset.path_) << " -> ";
        ss << MediaFileUtils::DesensitizePath(compare.path_);
    }
    return ss.str();
}

string PhotoAssetChangeInfo::GetDataDiff(const PhotoAssetChangeInfo &compare)
{
    return GetAssetDiff(*this, compare);
}

bool PhotoAssetChangeData::Marshalling(Parcel &parcel) const
{
    return Marshalling(parcel, false);
}

bool PhotoAssetChangeData::Marshalling(Parcel &parcel, bool isSystem) const
{
    bool ret = AccurateRefreshChangeData<PhotoAssetChangeInfo>::Marshalling(parcel, isSystem);
    ret = ret && parcel.WriteBool(isContentChanged_);
    if (isSystem) {
        ret = ret && parcel.WriteInt32(thumbnailChangeStatus_);
    }
    return ret;
}

bool PhotoAssetChangeData::ReadFromParcel(Parcel &parcel)
{
    bool ret = AccurateRefreshChangeData<PhotoAssetChangeInfo>::ReadFromParcel(parcel);
    ret = ret && parcel.ReadBool(isContentChanged_);
    if (isSystem_) {
        ret = ret && parcel.ReadInt32(thumbnailChangeStatus_);
    }
    return ret;
}

shared_ptr<PhotoAssetChangeData> PhotoAssetChangeData::Unmarshalling(Parcel &parcel)
{
    shared_ptr<PhotoAssetChangeData> assetChangeData = make_shared<PhotoAssetChangeData>();
    if (assetChangeData->ReadFromParcel(parcel)) {
        return assetChangeData;
    }
    return nullptr;
}

int32_t PhotoAssetChangeData::GetFileId()
{
    return infoBeforeChange_.fileId_ != INVALID_INT32_VALUE ? infoBeforeChange_.fileId_ : infoAfterChange_.fileId_;
}

} // namespace Media
} // namespace OHOS