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
#include "album_change_info.h"
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
    { MediaColumn::MEDIA_MIME_TYPE, TYPE_STRING },
    { PhotoColumn::MEDIA_HIDDEN, TYPE_INT32 },
    { PhotoColumn::MEDIA_DATE_TRASHED, TYPE_INT64 },
    { PhotoColumn::PHOTO_STRONG_ASSOCIATION, TYPE_INT32 },
    { PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, TYPE_INT32 },

    { PhotoColumn::MEDIA_DATE_ADDED, TYPE_INT64 },
    { PhotoColumn::MEDIA_DATE_TAKEN, TYPE_INT64 },
    { PhotoColumn::MEDIA_DATE_MODIFIED, TYPE_INT64 },

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
    { PhotoColumn::PHOTO_SHOOTING_MODE, TYPE_STRING },
    { PhotoColumn::MOVING_PHOTO_EFFECT_MODE, TYPE_INT32 },
    { PhotoColumn::PHOTO_FRONT_CAMERA, TYPE_STRING },
    { PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, TYPE_INT32 },
    { PhotoColumn::LOCAL_ASSET_SIZE, TYPE_INT64 },
};

static void AlbumChangeInfosToString(stringstream &ss,
    const vector<shared_ptr<AlbumChangeInfo>> &albumChangeInfos)
{
    ss << ", albumChangeInfos_: [";
    for (const auto &albumInfo : albumChangeInfos) {
        if (albumInfo == nullptr) {
            ss << "null, ";
            continue;
        }
        ss << "{ albumId_: " << albumInfo->albumId_ << ", albumSubType_: " << albumInfo->albumSubType_ << " }, ";
    }
    ss << "]";
}

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

static void FillFromResultSet(PhotoAssetChangeInfo &assetChangeInfo, const shared_ptr<ResultSet> &resultSet)
{
    auto getInt = [resultSet](const string &col) {
        return get<int32_t>(
            ResultSetUtils::GetValFromColumn(
                col, resultSet, PhotoAssetChangeInfo::GetDataType(col)));
    };
    auto getLong = [resultSet](const string &col) {
        return get<int64_t>(
            ResultSetUtils::GetValFromColumn(
                col, resultSet, PhotoAssetChangeInfo::GetDataType(col)));
    };
    auto getStr = [resultSet](const string &col) {
        return get<string>(
            ResultSetUtils::GetValFromColumn(
                col, resultSet, PhotoAssetChangeInfo::GetDataType(col)));
    };

    assetChangeInfo.fileSourceType_ = getInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE);
    assetChangeInfo.fileId_ = getInt(PhotoColumn::MEDIA_ID);
    assetChangeInfo.dateDay_ = getStr(PhotoColumn::PHOTO_DATE_DAY);
    assetChangeInfo.isFavorite_ = getInt(PhotoColumn::MEDIA_IS_FAV);
    assetChangeInfo.mediaType_ = getInt(PhotoColumn::MEDIA_TYPE);
    assetChangeInfo.mimeType_ = getStr(MediaColumn::MEDIA_MIME_TYPE);
    assetChangeInfo.isHidden_ = getInt(PhotoColumn::MEDIA_HIDDEN);
    assetChangeInfo.dateTrashedMs_ = getLong(PhotoColumn::MEDIA_DATE_TRASHED);
    assetChangeInfo.strongAssociation_ = getInt(PhotoColumn::PHOTO_STRONG_ASSOCIATION);
    assetChangeInfo.thumbnailVisible_ = getInt(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE);
    assetChangeInfo.dateAddedMs_ = getLong(PhotoColumn::MEDIA_DATE_ADDED);
    assetChangeInfo.dateTakenMs_ = getLong(PhotoColumn::MEDIA_DATE_TAKEN);
    assetChangeInfo.dateModifiedMs_ = getLong(PhotoColumn::MEDIA_DATE_MODIFIED);
    assetChangeInfo.subType_ = getInt(PhotoColumn::PHOTO_SUBTYPE);
    assetChangeInfo.syncStatus_ = getInt(PhotoColumn::PHOTO_SYNC_STATUS);
    assetChangeInfo.cleanFlag_ = getInt(PhotoColumn::PHOTO_CLEAN_FLAG);
    assetChangeInfo.timePending_ = getInt(PhotoColumn::MEDIA_TIME_PENDING);
    assetChangeInfo.isTemp_ = getInt(PhotoColumn::PHOTO_IS_TEMP);
    assetChangeInfo.burstCoverLevel_ = getInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL);
    assetChangeInfo.ownerAlbumId_ = getInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    assetChangeInfo.hiddenTime_ = getLong(PhotoColumn::PHOTO_HIDDEN_TIME);
    assetChangeInfo.thumbnailReady_ = getLong(PhotoColumn::PHOTO_THUMBNAIL_READY);
    assetChangeInfo.displayName_ = getStr(PhotoColumn::MEDIA_NAME);
    assetChangeInfo.path_ = getStr(PhotoColumn::MEDIA_FILE_PATH);
    assetChangeInfo.dirty_ = getInt(PhotoColumn::PHOTO_DIRTY);
    assetChangeInfo.position_ = getInt(PhotoColumn::PHOTO_POSITION);
    assetChangeInfo.size_ = getLong(PhotoColumn::MEDIA_SIZE);
    assetChangeInfo.shootingMode_ = getStr(PhotoColumn::PHOTO_SHOOTING_MODE);
    assetChangeInfo.movingPhotoEffectMode_ =
        getInt(PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
    assetChangeInfo.frontCamera_ = getStr(PhotoColumn::PHOTO_FRONT_CAMERA);
    assetChangeInfo.livephoto4dStatus_ = getInt(PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS);
    assetChangeInfo.localAssetSize_ = getLong(PhotoColumn::LOCAL_ASSET_SIZE);
}

vector<PhotoAssetChangeInfo> PhotoAssetChangeInfo::GetInfoFromResult(
    const shared_ptr<ResultSet> &resultSet)
{
    vector<PhotoAssetChangeInfo> assetChangeInfos;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        PhotoAssetChangeInfo assetChangeInfo;
        FillFromResultSet(assetChangeInfo, resultSet);
        if (assetChangeInfo.fileSourceType_ ==
            static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) {
            continue;
        }

        assetChangeInfo.uri_ = MediaFileUtils::GetUriByExtrConditions(
            PhotoColumn::PHOTO_URI_PREFIX, to_string(assetChangeInfo.fileId_),
            MediaFileUtils::GetExtraUri(
                assetChangeInfo.displayName_, assetChangeInfo.path_));
        if (assetChangeInfo.ownerAlbumId_ > 0) {
            assetChangeInfo.ownerAlbumUri_ = MediaFileUtils::GetUriByExtrConditions(
                PhotoAlbumColumns::ALBUM_URI_PREFIX,
                to_string(assetChangeInfo.ownerAlbumId_));
        }
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
        ss << ", displayName_: " << MediaFileUtils::DesensitizeName(displayName_);
        ss << ", path_: " << MediaFileUtils::DesensitizePath(path_) << ", dirty_: " << dirty_;
        ss << ", position_: " << position_ << ", size_: " << size_;
        ss << ", mimeType_: " << mimeType_ << ", shootingMode_: " << shootingMode_;
        ss << ", frontCamera_: " << frontCamera_ << ", movingPhotoEffectMode_: " << movingPhotoEffectMode_;
        ss << ", livephoto4dStatus_: " << livephoto4dStatus_ << ", dateModifiedMs_: " << dateModifiedMs_;
        ss << ", file_source_type: " << fileSourceType_;
        AlbumChangeInfosToString(ss, albumChangeInfos_);
    } else {
        ss << "fileId_: " << fileId_ << ", ownerAlbumId_: " << ownerAlbumId_;
    }
    return ss.str();
}

bool PhotoAssetChangeInfo::Marshalling(Parcel &parcel) const
{
    return Marshalling(parcel, false);
}

bool PhotoAssetChangeInfo::MarshallingAlbumChangeInfos(Parcel &parcel) const
{
    bool ret = parcel.WriteInt32(static_cast<int32_t>(albumChangeInfos_.size()));
    for (auto &info : albumChangeInfos_) {
        CHECK_AND_CONTINUE(info != nullptr);
        info->isHiddenCoverChange_ = false;
        info->coverInfo_ = PhotoAssetChangeInfo();
        info->hiddenCoverInfo_ = PhotoAssetChangeInfo();
        ret = ret && info->Marshalling(parcel, true);
    }
    return ret;
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
    ret = ret && parcel.WriteBool(isFavorite_);
    if (isSystem) {
        ret = ret && parcel.WriteString(dateDay_);
        ret = ret && parcel.WriteBool(isHidden_);
        ret = ret && parcel.WriteInt64(hiddenTime_);
        ret = ret && parcel.WriteInt32(strongAssociation_);
        ret = ret && parcel.WriteInt32(thumbnailVisible_);
        ret = ret && parcel.WriteInt64(dateTrashedMs_);
        ret = ret && parcel.WriteInt64(dateAddedMs_);
        ret = ret && parcel.WriteInt64(dateTakenMs_);
        ret = ret && parcel.WriteInt64(dateModifiedMs_);
        ret = ret && parcel.WriteInt32(ownerAlbumId_);
        ret = ret && parcel.WriteInt32(position_);
        ret = ret && parcel.WriteString(displayName_);
        ret = ret && parcel.WriteInt64(size_);
        ret = ret && MarshallingAlbumChangeInfos(parcel);
        ret = ret && parcel.WriteInt32(livephoto4dStatus_);
        ret = ret && parcel.WriteInt32(fileSourceType_);
    }
    return ret;
}

bool PhotoAssetChangeInfo::ReadAlbumChangeInfos(Parcel &parcel)
{
    int32_t size = 0;
    bool ret = parcel.ReadInt32(size);
    if (!ret || size < 0 || size > MAX_MARSHALL_SIZE) {
        return false;
    }
    albumChangeInfos_.clear();
    albumChangeInfos_.reserve(size);
    for (int32_t i = 0; i < size; ++i) {
        AlbumChangeInfo info;
        if (!info.ReadFromParcel(parcel)) {
            return false;
        }
        albumChangeInfos_.push_back(make_shared<AlbumChangeInfo>(info));
    }
    return true;
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
    ret = ret && parcel.ReadBool(isFavorite_);
    if (isSystem_) {
        ret = ret && parcel.ReadString(dateDay_);
        ret = ret && parcel.ReadBool(isHidden_);
        ret = ret && parcel.ReadInt64(hiddenTime_);
        ret = ret && parcel.ReadInt32(strongAssociation_);
        ret = ret && parcel.ReadInt32(thumbnailVisible_);
        ret = ret && parcel.ReadInt64(dateTrashedMs_);
        ret = ret && parcel.ReadInt64(dateAddedMs_);
        ret = ret && parcel.ReadInt64(dateTakenMs_);
        ret = ret && parcel.ReadInt64(dateModifiedMs_);
        ret = ret && parcel.ReadInt32(ownerAlbumId_);
        ret = ret && parcel.ReadInt32(position_);
        ret = ret && parcel.ReadString(displayName_);
        ret = ret && parcel.ReadInt64(size_);
        ret = ret && ReadAlbumChangeInfos(parcel);
        ret = ret && parcel.ReadInt32(livephoto4dStatus_);
        ret = ret && parcel.ReadInt32(fileSourceType_);
    }
    return ret;
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
        mimeType_ = info.mimeType_;
        isHidden_ = info.isHidden_;
        dateTrashedMs_ = info.dateTrashedMs_;
        strongAssociation_ = info.strongAssociation_;
        thumbnailVisible_ = info.thumbnailVisible_;
        dateAddedMs_ = info.dateAddedMs_;
        dateTakenMs_ = info.dateTakenMs_;
        dateModifiedMs_ = info.dateModifiedMs_;
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
        shootingMode_ = info.shootingMode_;
        movingPhotoEffectMode_ = info.movingPhotoEffectMode_;
        frontCamera_ = info.frontCamera_;
        albumChangeInfos_ = info.albumChangeInfos_;
        livephoto4dStatus_ = info.livephoto4dStatus_;
        fileSourceType_ = info.fileSourceType_;
    }
    return *this;
}

bool PhotoAssetChangeInfo::operator==(const PhotoAssetChangeInfo &info) const
{
    return fileId_ == info.fileId_ &&
        dateDay_ == info.dateDay_ &&
        isFavorite_ == info.isFavorite_ &&
        mediaType_ == info.mediaType_ &&
        mimeType_ == info.mimeType_ &&
        isHidden_ == info.isHidden_ &&
        dateTrashedMs_ == info.dateTrashedMs_ &&
        strongAssociation_ == info.strongAssociation_ &&
        thumbnailVisible_ == info.thumbnailVisible_ &&
        dateAddedMs_ == info.dateAddedMs_ &&
        dateTakenMs_ == info.dateTakenMs_ &&
        dateModifiedMs_ == info.dateModifiedMs_ &&
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
        size_ == info.size_ &&
        shootingMode_ == info.shootingMode_ &&
        movingPhotoEffectMode_ == info.movingPhotoEffectMode_ &&
        frontCamera_ == info.frontCamera_ &&
        livephoto4dStatus_ == info.livephoto4dStatus_ &&
        albumChangeInfos_ == info.albumChangeInfos_ &&
        fileSourceType_ == info.fileSourceType_;
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
    if (asset.uri_ != compare.uri_) {
        ss << "uri_: " << MediaFileUtils::DesensitizeUri(asset.uri_) << " -> ";
        ss << MediaFileUtils::DesensitizeUri(compare.uri_);
    }
    GET_ASSET_DIFF(dateDay_);
    GET_ASSET_DIFF(ownerAlbumUri_);
    GET_ASSET_DIFF(isFavorite_);
    GET_ASSET_DIFF(mediaType_);
    GET_ASSET_DIFF(mimeType_);
    GET_ASSET_DIFF(isHidden_);
    GET_ASSET_DIFF(dateTrashedMs_);
    GET_ASSET_DIFF(strongAssociation_);
    GET_ASSET_DIFF(thumbnailVisible_);
    GET_ASSET_DIFF(dateAddedMs_);
    GET_ASSET_DIFF(dateTakenMs_);
    GET_ASSET_DIFF(dateModifiedMs_);
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
    GET_ASSET_DIFF(livephoto4dStatus_);
    GET_ASSET_DIFF(fileSourceType_);
    if (asset.displayName_ != compare.displayName_) {
        ss << "displayName_: " << MediaFileUtils::DesensitizeName(asset.displayName_) << " -> ";
        ss << MediaFileUtils::DesensitizeName(compare.displayName_) << ", ";
    }
    if (asset.path_ != compare.path_) {
        ss << "path_: " << MediaFileUtils::DesensitizePath(asset.path_) << " -> ";
        ss << MediaFileUtils::DesensitizePath(compare.path_);
    }
    if (asset.albumChangeInfos_ != compare.albumChangeInfos_) {
        AlbumChangeInfosToString(ss, asset.albumChangeInfos_);
        ss << " -> ";
        AlbumChangeInfosToString(ss, compare.albumChangeInfos_);
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

int32_t PhotoAssetChangeData::GetFileId() const
{
    return infoBeforeChange_.fileId_ != INVALID_INT32_VALUE ? infoBeforeChange_.fileId_ : infoAfterChange_.fileId_;
}

} // namespace Media
} // namespace OHOS
