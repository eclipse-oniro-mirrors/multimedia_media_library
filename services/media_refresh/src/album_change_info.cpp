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

#define MLOG_TAG "AccurateRefresh::AlbumChangeInfo"

#include <sstream>

#include "album_change_info.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "accurate_debug_log.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media::AccurateRefresh {

const map<std::string, ResultSetDataType> AlbumChangeInfo::albumInfoCloumnTypes_ = {
    { PhotoAlbumColumns::ALBUM_LPATH, TYPE_STRING },
    { PhotoAlbumColumns::ALBUM_IMAGE_COUNT, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUM_VIDEO_COUNT, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUM_TYPE, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUM_SUBTYPE, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUM_NAME, TYPE_STRING },
    { PhotoAlbumColumns::ALBUM_COUNT, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUM_COVER_URI, TYPE_STRING },
    { PhotoAlbumColumns::HIDDEN_COUNT, TYPE_INT32 },
    { PhotoAlbumColumns::HIDDEN_COVER, TYPE_STRING },
    { PhotoAlbumColumns::ALBUM_ID, TYPE_INT32 },
    { PhotoAlbumColumns::COVER_DATE_TIME, TYPE_INT64 },
    { PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, TYPE_INT64 },
    { PhotoAlbumColumns::ALBUM_DIRTY, TYPE_INT32 },
    { PhotoAlbumColumns::COVER_URI_SOURCE, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUMS_ORDER, TYPE_INT32 },
    { PhotoAlbumColumns::ORDER_SECTION, TYPE_INT32},
    { PhotoAlbumColumns::ALBUM_CLOUD_ID, TYPE_STRING },
    { PhotoAlbumColumns::ALBUM_IS_LOCAL, TYPE_INT32 },
    { PhotoAlbumColumns::ALBUM_HIDDEN, TYPE_INT32 }
};

const vector<std::string> AlbumChangeInfo::albumInfoColumns_ = []() {
    vector<string> result;
    for (const auto &item : albumInfoCloumnTypes_) {
        result.push_back(item.first);
    }
    return result;
}();

const std::vector<std::string>& AlbumChangeInfo::GetAlbumInfoColumns()
{
    return albumInfoColumns_;
}

vector<AlbumChangeInfo> AlbumChangeInfo::GetInfoFromResult(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const std::vector<std::string> &columns)
{
    vector<AlbumChangeInfo> albumChangeInfos;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AlbumChangeInfo albumChangeInfo;
        albumChangeInfo.lpath_ = get<string>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::ALBUM_LPATH, resultSet, GetDataType(PhotoAlbumColumns::ALBUM_LPATH)));
        albumChangeInfo.imageCount_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet, GetDataType(PhotoAlbumColumns::ALBUM_IMAGE_COUNT)));
        albumChangeInfo.videoCount_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet, GetDataType(PhotoAlbumColumns::ALBUM_VIDEO_COUNT)));
        albumChangeInfo.albumType_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_TYPE,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_TYPE)));
        albumChangeInfo.albumSubType_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet, GetDataType(PhotoAlbumColumns::ALBUM_SUBTYPE)));
        albumChangeInfo.albumName_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_NAME,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_NAME)));
        albumChangeInfo.count_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COUNT,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_COUNT)));
        albumChangeInfo.coverUri_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COVER_URI,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_COVER_URI)));
        albumChangeInfo.hiddenCount_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::HIDDEN_COUNT,
            resultSet, GetDataType(PhotoAlbumColumns::HIDDEN_COUNT)));
        albumChangeInfo.hiddenCoverUri_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::HIDDEN_COVER,
            resultSet, GetDataType(PhotoAlbumColumns::HIDDEN_COVER)));
        albumChangeInfo.albumId_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_ID)));
        albumChangeInfo.albumUri_ = MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
            to_string(albumChangeInfo.albumId_));
        albumChangeInfo.coverDateTime_ = get<int64_t>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::COVER_DATE_TIME, resultSet, GetDataType(PhotoAlbumColumns::COVER_DATE_TIME)));
        albumChangeInfo.hiddenCoverDateTime_ = get<int64_t>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, resultSet,
            GetDataType(PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME)));
        albumChangeInfo.dirty_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_DIRTY,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_DIRTY)));
        albumChangeInfo.coverUriSource_ = get<int32_t>(ResultSetUtils::GetValFromColumn(
            PhotoAlbumColumns::COVER_URI_SOURCE, resultSet, GetDataType(PhotoAlbumColumns::COVER_URI_SOURCE)));
        albumChangeInfo.orderSection_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ORDER_SECTION,
            resultSet, GetDataType(PhotoAlbumColumns::ORDER_SECTION)));
        albumChangeInfo.albumsOrder_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUMS_ORDER,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUMS_ORDER)));
        albumChangeInfo.cloudId_ = get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_CLOUD_ID,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_CLOUD_ID)));
        albumChangeInfo.isLocal_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_IS_LOCAL,
            resultSet, GetDataType(PhotoAlbumColumns::ALBUM_IS_LOCAL)));
        SetPhotoAlbumHidden(albumChangeInfo, resultSet);
        albumChangeInfos.push_back(albumChangeInfo);
    }

    return albumChangeInfos;
}

void AlbumChangeInfo::SetPhotoAlbumHidden(AlbumChangeInfo &albumChangeInfo,
    const shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    albumChangeInfo.hidden_ = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_HIDDEN,
        resultSet, GetDataType(PhotoAlbumColumns::ALBUM_HIDDEN)));
}

string AlbumChangeInfo::ToString(bool isDetail) const
{
    stringstream ss;
    if (isDetail) {
        ss << "albumId_: " << albumId_ << ", albumSubType_: "<< albumSubType_ << ", count_: " << count_;
        ss << ", imageCount_: " << imageCount_ << ", videoCount_: " << videoCount_;
        ss << ", hiddenCount_: " << hiddenCount_ << ", albumType_: " << albumType_  << ", albumName_: " << albumName_;
        ss << ", albumUri_: " << albumUri_ << ", coverUri_: " << MediaFileUtils::DesensitizeUri(coverUri_);
        ss <<  ", hiddenCoverUri_: " << MediaFileUtils::DesensitizeUri(hiddenCoverUri_);
        ss << ", isCoverChange_: " << isCoverChange_ << ", isHiddenCoverChange_: " << isHiddenCoverChange_;
        ss << ", coverDateTime_: " << coverDateTime_ << ", hiddenCoverDateTime_: " << hiddenCoverDateTime_;
        ss << ", dirty_: " << dirty_ << ", coverUriSource_: " << coverUriSource_;
        ss << ", albumsOrder_: " << albumsOrder_ << ", orderSection_: " << orderSection_ << ", lpath_: " << lpath_;
        ss << ", isLocal_: " << isLocal_ << ", cloudId_: " << cloudId_ << ", hidden_: " << hidden_;
        if (isCoverChange_) {
            ss << ", cover info: " << coverInfo_.ToString().c_str();
        }
        if (isHiddenCoverChange_) {
            ss << ", hidden cover info: " << hiddenCoverInfo_.ToString().c_str();
        }
    } else {
        ss << "albumId_: " << albumId_ << ", albumSubType_: "<< albumSubType_;
    }

    return ss.str();
}

ResultSetDataType AlbumChangeInfo::GetDataType(const std::string &column)
{
    auto const &item = albumInfoCloumnTypes_.find(column);
    if (item == albumInfoCloumnTypes_.end()) {
        return TYPE_NULL;
    }
    return item->second;
}

ValuesBucket AlbumChangeInfo::GetUpdateValues(const AlbumChangeInfo &oldAlbumInfo, NotifyType &type)
{
    stringstream ss;
    ValuesBucket values;
    if (imageCount_ != oldAlbumInfo.imageCount_ && imageCount_ != INVALID_INT32_VALUE) {
        values.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, imageCount_);
        ss << "imageCount_: " << oldAlbumInfo.imageCount_ << " -> " << imageCount_ << ", ";
    }
    if (videoCount_ != oldAlbumInfo.videoCount_ && videoCount_ != INVALID_INT32_VALUE) {
        values.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, videoCount_);
        ss << "videoCount_: " << oldAlbumInfo.videoCount_ << " -> " << videoCount_ << ", ";
    }
    if (count_ != oldAlbumInfo.count_ && count_ != INVALID_INT32_VALUE) {
        values.PutInt(PhotoAlbumColumns::ALBUM_COUNT, count_);
        ss << "count_: " << oldAlbumInfo.count_ << " -> " << count_ << ", ";
        GetUpdatePhotoAlbumHidden(oldAlbumInfo, values, ss);
    }
    if (hiddenCount_ != oldAlbumInfo.hiddenCount_ && hiddenCount_ != INVALID_INT32_VALUE) {
        values.PutInt(PhotoAlbumColumns::HIDDEN_COUNT, hiddenCount_);
        ss << "hiddenCount_: " << oldAlbumInfo.hiddenCount_ << " -> " << hiddenCount_ << ", ";
        values.PutInt(PhotoAlbumColumns::CONTAINS_HIDDEN, hiddenCount_ != 0);
        ss << "contains_hidden: true, ";
        GetUpdatePhotoAlbumHidden(oldAlbumInfo, values, ss);
    }
    if (coverUri_ != oldAlbumInfo.coverUri_ && coverUri_ != EMPTY_STR) {
        values.PutString(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri_);
        ss << "coverUri_: " << MediaFileUtils::DesensitizeUri(oldAlbumInfo.coverUri_) << " -> ";
        ss << MediaFileUtils::DesensitizeUri(coverUri_) << ", ";
    }
    if (hiddenCoverUri_ != oldAlbumInfo.hiddenCoverUri_ && hiddenCoverUri_ != EMPTY_STR) {
        values.PutString(PhotoAlbumColumns::HIDDEN_COVER, hiddenCoverUri_);
        ss << "hiddenCoverUri_: " << MediaFileUtils::DesensitizeUri(oldAlbumInfo.hiddenCoverUri_) << " -> ";
        ss << MediaFileUtils::DesensitizeUri(hiddenCoverUri_) << ", ";
    }
    if (coverDateTime_ != oldAlbumInfo.coverDateTime_ && coverDateTime_ != INVALID_INT64_VALUE) {
        values.PutLong(PhotoAlbumColumns::COVER_DATE_TIME, coverDateTime_);
        ss << "coverDateTime_: " << oldAlbumInfo.coverDateTime_ << " -> " << coverDateTime_ << ", ";
    }
    if (hiddenCoverDateTime_ != oldAlbumInfo.hiddenCoverDateTime_ && hiddenCoverDateTime_ != INVALID_INT64_VALUE) {
        values.PutLong(PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, hiddenCoverDateTime_);
        ss << "hiddenCoverDateTime_: " << oldAlbumInfo.hiddenCoverDateTime_ << " -> " << hiddenCoverDateTime_;
    }

    type = oldAlbumInfo.count_ < count_ ? NOTIFY_ALBUM_ADD_ASSET :
        (oldAlbumInfo.count_ > count_ ? NOTIFY_ALBUM_REMOVE_ASSET : NOTIFY_UPDATE);
    ACCURATE_INFO("Update albumInfo[%{public}d], notifyType[%{public}d]: %{public}s", oldAlbumInfo.albumId_,
        static_cast<int32_t>(type), ss.str().c_str());
    return values;
}

void AlbumChangeInfo::GetUpdatePhotoAlbumHidden(const AlbumChangeInfo &oldAlbumInfo, ValuesBucket &values,
    stringstream &ss)
{
    if (!(oldAlbumInfo.albumSubType_ >= static_cast<int32_t>(PhotoAlbumSubType::ANALYSIS_START) &&
        oldAlbumInfo.albumSubType_ <= static_cast<int32_t>(PhotoAlbumSubType::ANALYSIS_END))) {
        values.PutInt(PhotoAlbumColumns::ALBUM_HIDDEN, count_ < 1);
        ss << "hidden_: " << oldAlbumInfo.hidden_ << " -> " << hidden_ << ", ";
    }
}

bool AlbumChangeInfo::Marshalling(Parcel &parcel) const
{
    return Marshalling(parcel, false);
}

bool AlbumChangeInfo::Marshalling(Parcel &parcel, bool isSystem) const
{
    bool isValid = albumId_ != INVALID_INT32_VALUE;
    bool ret = parcel.WriteBool(isValid);
    if (ret && !isValid) {
        return ret;
    }
    ret = ret && parcel.WriteInt32(imageCount_);
    ret = ret && parcel.WriteInt32(videoCount_);
    ret = ret && parcel.WriteInt32(albumType_);
    ret = ret && parcel.WriteInt32(albumSubType_);
    ret = ret && parcel.WriteString(albumName_);
    ret = ret && parcel.WriteString(albumUri_);
    ret = ret && parcel.WriteInt32(count_);
    ret = ret && parcel.WriteString(coverUri_);
    ret = ret && parcel.WriteBool(isSystem);
    ret = ret && parcel.WriteInt32(hidden_);
    if (isSystem) {
        ret = ret && parcel.WriteInt32(hiddenCount_);
        ret = ret && parcel.WriteString(hiddenCoverUri_);
        ret = ret && parcel.WriteBool(isCoverChange_);
        if (isCoverChange_) {
            ret = ret && coverInfo_.Marshalling(parcel, isSystem);
        }

        ret = ret && parcel.WriteBool(isHiddenCoverChange_);
        if (isHiddenCoverChange_) {
            ret = ret && hiddenCoverInfo_.Marshalling(parcel, isSystem);
        }
        ret = ret && parcel.WriteInt32(orderSection_);
        ret = ret && parcel.WriteInt32(albumsOrder_);
    }
    ret = ret && parcel.WriteInt32(albumId_);
    return ret;
}

bool AlbumChangeInfo::ReadFromParcel(Parcel &parcel)
{
    bool isValid = false;
    bool ret = parcel.ReadBool(isValid);
    if (ret && !isValid) {
        return ret;
    }
    ret = ret && parcel.ReadInt32(imageCount_);
    ret = ret && parcel.ReadInt32(videoCount_);
    ret = ret && parcel.ReadInt32(albumType_);
    ret = ret && parcel.ReadInt32(albumSubType_);
    ret = ret && parcel.ReadString(albumName_);
    ret = ret && parcel.ReadString(albumUri_);
    ret = ret && parcel.ReadInt32(count_);
    ret = ret && parcel.ReadString(coverUri_);
    ret = ret && parcel.ReadBool(isSystem_);
    ret = ret && parcel.ReadInt32(hidden_);
    if (isSystem_) {
        ret = ret && parcel.ReadInt32(hiddenCount_);
        ret = ret && parcel.ReadString(hiddenCoverUri_);
        ret = ret && parcel.ReadBool(isCoverChange_);
        if (ret && isCoverChange_) {
            ret = ret && coverInfo_.ReadFromParcel(parcel);
        }
        ret = ret && parcel.ReadBool(isHiddenCoverChange_);
        if (ret && isHiddenCoverChange_) {
            ret = ret && hiddenCoverInfo_.ReadFromParcel(parcel);
        }
        ret = ret && parcel.ReadInt32(orderSection_);
        ret = ret && parcel.ReadInt32(albumsOrder_);
    }
    ret = ret && parcel.ReadInt32(albumId_);
    return ret;
}

shared_ptr<AlbumChangeInfo> AlbumChangeInfo::Unmarshalling(Parcel &parcel)
{
    shared_ptr<AlbumChangeInfo> albumChangeInfo = make_shared<AlbumChangeInfo>();
    if (albumChangeInfo->ReadFromParcel(parcel)) {
        return albumChangeInfo;
    }
    return nullptr;
}

template <typename T>
void GetDiff(const T &value, const T &compareValue, const std::string &name, std::stringstream &ss)
{
    if (value != compareValue) {
        ss << name << ": " << value << " -> " << compareValue << ", ";
    }
}
#define GET_ALBUM_DIFF(member) GetDiff(album.member, compare.member, #member, ss)
string AlbumChangeInfo::GetAlbumDiff(const AlbumChangeInfo &album, const AlbumChangeInfo &compare)
{
    stringstream ss;
    if (album.albumId_ != compare.albumId_) {
        ss << "diff album info albumId: " << album.albumId_ << ", compare albumId: " << compare.albumId_;
        return ss.str();
    }
    ss << "albumId[" << album.albumId_ << "]: ";
    GET_ALBUM_DIFF(lpath_);
    GET_ALBUM_DIFF(imageCount_);
    GET_ALBUM_DIFF(videoCount_);
    GET_ALBUM_DIFF(albumType_);
    GET_ALBUM_DIFF(albumSubType_);
    GET_ALBUM_DIFF(albumName_);
    GET_ALBUM_DIFF(albumUri_);
    GET_ALBUM_DIFF(count_);
    GET_ALBUM_DIFF(hiddenCount_);
    GET_ALBUM_DIFF(isCoverChange_);
    GET_ALBUM_DIFF(isHiddenCoverChange_);
    GET_ALBUM_DIFF(coverDateTime_);
    GET_ALBUM_DIFF(hiddenCoverDateTime_);
    GET_ALBUM_DIFF(dirty_);
    GET_ALBUM_DIFF(coverUriSource_);
    GET_ALBUM_DIFF(albumsOrder_);
    GET_ALBUM_DIFF(orderSection_);
    GET_ALBUM_DIFF(hidden_);

    if (album.coverUri_ != compare.coverUri_) {
        ss << "coverUri_: " << MediaFileUtils::DesensitizeUri(album.coverUri_) << " -> ";
        ss << MediaFileUtils::DesensitizeUri(compare.coverUri_) << ", ";
    }

    if (album.hiddenCoverUri_ != compare.hiddenCoverUri_) {
        ss << "hiddenCoverUri_: " << MediaFileUtils::DesensitizeUri(album.hiddenCoverUri_) << " -> ";
        ss << MediaFileUtils::DesensitizeUri(compare.hiddenCoverUri_);
    }
    return ss.str();
}
string AlbumChangeInfo::GetDataDiff(const AlbumChangeInfo &compare)
{
    return GetAlbumDiff(*this, compare);
}

shared_ptr<AlbumChangeData> AlbumChangeData::Unmarshalling(Parcel &parcel)
{
    shared_ptr<AlbumChangeData> albumChangeData = make_shared<AlbumChangeData>();
    if (albumChangeData->ReadFromParcel(parcel)) {
        return albumChangeData;
    }
    return nullptr;
}

bool AlbumChangeData::IsAlbumInfoChange()
{
    MEDIA_DEBUG_LOG("albumIdOld: %{public}d, albumIdNew: %{public}d",
        infoBeforeChange_.albumId_, infoAfterChange_.albumId_);
    MEDIA_DEBUG_LOG("lpathOld: %{public}s, lpathNew: %{public}s",
        infoBeforeChange_.lpath_.c_str(), infoAfterChange_.lpath_.c_str());
    MEDIA_DEBUG_LOG("imageCountOld: %{public}d, imageCountNew: %{public}d",
        infoBeforeChange_.imageCount_, infoAfterChange_.imageCount_);
    MEDIA_DEBUG_LOG("videoCountOld: %{public}d, videoCountNew: %{public}d",
        infoBeforeChange_.videoCount_, infoAfterChange_.videoCount_);
    MEDIA_DEBUG_LOG("albumTypeOld: %{public}d, albumTypeNew: %{public}d",
        infoBeforeChange_.albumType_, infoAfterChange_.albumType_);
    MEDIA_DEBUG_LOG("albumSubTypeOld: %{public}d, albumSubTypeNew: %{public}d",
        infoBeforeChange_.albumSubType_, infoAfterChange_.albumSubType_);
    MEDIA_DEBUG_LOG("albumNameOld: %{public}s, albumNameNew: %{public}s",
        infoBeforeChange_.albumName_.c_str(), infoAfterChange_.albumName_.c_str());
    MEDIA_DEBUG_LOG("albumUriOld: %{public}s, albumUriNew: %{public}s",
        infoBeforeChange_.albumUri_.c_str(), infoAfterChange_.albumUri_.c_str());
    MEDIA_DEBUG_LOG("countOld: %{public}d, countNew: %{public}d",
        infoBeforeChange_.count_, infoAfterChange_.count_);
    MEDIA_DEBUG_LOG("coverUriOld: %{public}s, coverUriNew: %{public}s",
        infoBeforeChange_.coverUri_.c_str(), infoAfterChange_.coverUri_.c_str());
    MEDIA_DEBUG_LOG("hiddenCountOld: %{public}d, hiddenCountNew: %{public}d",
        infoBeforeChange_.hiddenCount_, infoAfterChange_.hiddenCount_);
    MEDIA_DEBUG_LOG("hiddenCoverUriOld: %{public}s, hiddenCoverUriNew: %{public}s",
        infoBeforeChange_.hiddenCoverUri_.c_str(), infoAfterChange_.hiddenCoverUri_.c_str());
    MEDIA_DEBUG_LOG("albumsOrderOld: %{public}d, albumsOrderNew: %{public}d",
        infoBeforeChange_.albumsOrder_, infoAfterChange_.albumsOrder_);
    MEDIA_DEBUG_LOG("orderSectionOld: %{public}d, orderSectionNew: %{public}d",
        infoBeforeChange_.orderSection_, infoAfterChange_.orderSection_);
    MEDIA_DEBUG_LOG("dirtyOld: %{public}d, dirtyNew: %{public}d",
        infoBeforeChange_.dirty_, infoAfterChange_.dirty_);
    MEDIA_DEBUG_LOG("hiddenOld: %{public}d, hiddenNew: %{public}d",
        infoBeforeChange_.hidden_, infoAfterChange_.hidden_);
    return (infoAfterChange_.isCoverChange_)||
        (infoBeforeChange_.imageCount_ != infoAfterChange_.imageCount_) ||
        (infoBeforeChange_.videoCount_ != infoAfterChange_.videoCount_) ||
        (infoBeforeChange_.count_ != infoAfterChange_.count_) ||
        (infoBeforeChange_.lpath_ != infoAfterChange_.lpath_) ||
        (infoBeforeChange_.albumName_ != infoAfterChange_.albumName_) ||
        (infoBeforeChange_.albumsOrder_ != infoAfterChange_.albumsOrder_) ||
        (infoBeforeChange_.albumSubType_ != infoAfterChange_.albumSubType_) ||
        (infoBeforeChange_.albumType_ != infoAfterChange_.albumType_) ||
        (infoBeforeChange_.orderSection_ != infoAfterChange_.orderSection_) ||
        (infoBeforeChange_.hidden_ != infoAfterChange_.hidden_);
}
 
bool AlbumChangeData::IsAlbumHiddenInfoChange()
{
    return infoAfterChange_.isHiddenCoverChange_ || (infoBeforeChange_.hiddenCount_ != infoAfterChange_.hiddenCount_);
}

string AlbumRefreshInfo::ToString() const
{
    stringstream ss;
    if (IsAlbumInfoRefresh()) {
        ss << "deltaCount_: " << deltaCount_ << ", deltaVideoCount_: " << deltaVideoCount_;
        ss << ", deltaAddCover_ fileId: " << deltaAddCover_.fileId_ << ", remove asset size: " << removeFileIds.size();
        ss << ", assetModifiedCnt_: " << assetModifiedCnt_;
        ss << ", album timestamp: " << albumRefreshTimestamp_.ToString();
    }
    if (IsAlbumHiddenInfoRefresh()) {
        ss << ", deltaHiddenCount_: " << deltaHiddenCount_ << ", deltaAddHiddenCover_ fileId: ";
        ss << deltaAddHiddenCover_.fileId_ <<  ", remove hidden asset size: " << removeHiddenFileIds.size();
        ss << ", hiddenAssetModifiedCnt_: " << hiddenAssetModifiedCnt_;
        ss << ", album hidden timestamp: " << albumHiddenRefreshTimestamp_.ToString();
    }
    if (isForceRefresh_) {
        ss << ", isForceRefresh_: " << isForceRefresh_;
    }
    if (isHiddenForceRefresh_) {
        ss << ", isHiddenForceRefresh_: " << isHiddenForceRefresh_;
    }
    
    return ss.str();
}

} // namespace Media
} // namespace OHOS
