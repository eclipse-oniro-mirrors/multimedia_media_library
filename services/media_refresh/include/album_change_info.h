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

#ifndef OHOS_MEDIALIBRARY_ALBUM_CHANGE_INFO_H
#define OHOS_MEDIALIBRARY_ALBUM_CHANGE_INFO_H

#include <string>
#include <vector>
#include <unordered_set>

#include "photo_album_column.h"
#include "photo_asset_change_info.h"
#include "result_set.h"
#include "medialibrary_type_const.h"
#include "values_bucket.h"
#include "album_accurate_refresh_manager.h"

namespace OHOS {
namespace Media::AccurateRefresh {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT AlbumChangeInfo : public Parcelable {
public:
    AlbumChangeInfo() {}
    AlbumChangeInfo(int32_t albumId, std::string lpath, int32_t imageCount, int32_t videoCount, int32_t albumType,
        int32_t albumSubType, std::string albumName, std::string albumUri, int32_t count, std::string coverUri,
        int32_t hiddenCount, std::string hiddenCoverUri, bool isCoverChange, bool isHiddenCoverChange,
        int64_t dateTimeForCover, int64_t dateTimeForHiddenCover, int32_t dirty, int32_t albumsOrder,
        int32_t orderSection, int32_t isCoverSatisfied, std::string groupTag, int32_t hidden)
        : albumId_(albumId), lpath_(lpath), imageCount_(imageCount), videoCount_(videoCount), albumType_(albumType),
        albumSubType_(albumSubType), albumName_(albumName), albumUri_(albumUri), count_(count), coverUri_(coverUri),
        hiddenCount_(hiddenCount), hiddenCoverUri_(hiddenCoverUri), isCoverChange_(isCoverChange),
        isHiddenCoverChange_(isHiddenCoverChange), coverDateTime_(dateTimeForCover),
        hiddenCoverDateTime_(dateTimeForHiddenCover), dirty_(dirty), albumsOrder_(albumsOrder),
        orderSection_(orderSection), isCoverSatisfied_(isCoverSatisfied), groupTag_(groupTag), hidden_(hidden) {}

    int32_t albumId_ = INVALID_INT32_VALUE;
    std::string lpath_ = EMPTY_STR;
    int32_t imageCount_ = INVALID_INT32_VALUE;
    int32_t videoCount_ = INVALID_INT32_VALUE;
    int32_t albumType_ = INVALID_INT32_VALUE;
    int32_t albumSubType_ = INVALID_INT32_VALUE;
    std::string albumName_ = EMPTY_STR;
    std::string albumUri_ = EMPTY_STR;
    int32_t count_ = INVALID_INT32_VALUE;
    std::string coverUri_ = EMPTY_STR;
    int32_t hiddenCount_ = INVALID_INT32_VALUE;
    std::string hiddenCoverUri_ = EMPTY_STR;
    bool isCoverChange_ = false;
    PhotoAssetChangeInfo coverInfo_;
    bool isHiddenCoverChange_ = false;
    PhotoAssetChangeInfo hiddenCoverInfo_;
    int64_t coverDateTime_ = INVALID_INT64_VALUE;
    int64_t hiddenCoverDateTime_ = INVALID_INT64_VALUE;
    int32_t dirty_ = INVALID_INT32_VALUE;
    int32_t coverUriSource_ = 0;
    int32_t albumsOrder_ = INVALID_INT32_VALUE;
    int32_t orderSection_ = INVALID_INT32_VALUE;
    std::string cloudId_ = EMPTY_STR;
    int32_t isLocal_ = INVALID_INT32_VALUE;
    int32_t isCoverSatisfied_ = INVALID_INT32_VALUE;
    std::string groupTag_ = EMPTY_STR;
    int32_t hidden_ = 0;
    bool needForceSelectCover = false;
    bool needForceSelectHiddenCover = false;

    // Album cover order info
    std::string coverOrderKey_ = EMPTY_STR;
    std::string coverOrderSubKey_ = EMPTY_STR;
    int32_t coverOrderType_ = INVALID_INT32_VALUE;
    std::string hiddenCoverOrderKey_ = EMPTY_STR;
    std::string hiddenCoverOrderSubKey_ = EMPTY_STR;
    int32_t hiddenCoverOrderType_ = INVALID_INT32_VALUE;

    NativeRdb::ValuesBucket GetUpdateValues(const AlbumChangeInfo &oldAlbumInfo, NotifyType &type);
    std::string ToString(bool isDetail = false) const;
    string GetDataDiff(const AlbumChangeInfo &compare);

    bool Marshalling(Parcel &parcel) const override;
    bool Marshalling(Parcel &parcel, bool isSystem) const;
    bool ReadFromParcel(Parcel &parcel);
    static std::shared_ptr<AlbumChangeInfo> Unmarshalling(Parcel &parcel);
    
    static const std::vector<std::string>& GetAlbumInfoColumns();
    static const std::vector<std::string>& GetAnalysisAlbumInfoColumns();
    static std::vector<AlbumChangeInfo> GetInfoFromResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    static std::vector<AlbumChangeInfo> GetAnalysisAlbumInfoFromResult(
        const std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    static ResultSetDataType GetDataType(const std::string &column);
    static ResultSetDataType GetAnalysisAlbumDataType(const std::string &column);
private:
    std::string GetAlbumDiff(const AlbumChangeInfo &album, const AlbumChangeInfo &compare);
    static void SetPhotoAlbumHidden(AlbumChangeInfo &albumChangeInfo,
        const shared_ptr<NativeRdb::ResultSet> &resultSet);
    static void SetPhotoAlbumForCoverOrder(AlbumChangeInfo &albumChangeInfo,
        const shared_ptr<NativeRdb::ResultSet> &resultSet);
    static void SetPhotoAlbumExecute(AlbumChangeInfo &albumChangeInfo,
        const shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetUpdatePhotoAlbumHidden(const AlbumChangeInfo &oldAlbumInfo, NativeRdb::ValuesBucket &values,
        stringstream &ss);
private:
    static const std::vector<std::string> albumInfoColumns_;
    static const std::vector<std::string> analysisAlbumInfoColumns_;
    static const std::map<std::string, ResultSetDataType> albumInfoColumnTypes_;
    static const std::map<std::string, ResultSetDataType> analysisAlbumInfoColumnTypes_;
    bool isSystem_ = false;
};

class AlbumChangeData : public AccurateRefreshChangeData<AlbumChangeInfo> {
public:
    bool Marshalling(Parcel &parcel) const override;
    static std::shared_ptr<AlbumChangeData> Unmarshalling(Parcel &parcel);
    bool Marshalling(Parcel &parcel, bool isSystem) const;
    bool IsAlbumInfoChange();
    bool IsAlbumHiddenInfoChange();
    bool IsAnalysisAlbumInfoChange();
};

class AlbumRefreshInfo {
public:
    int32_t deltaCount_ = 0;
    int32_t deltaVideoCount_ = 0;
    int32_t deltaHiddenCount_ = 0;
    PhotoAssetChangeInfo deltaAddCover_;
    PhotoAssetChangeInfo deltaAddHiddenCover_;
    std::unordered_set<int32_t> removeFileIds;
    std::unordered_set<int32_t> removeHiddenFileIds;
    int32_t assetModifiedCnt_ = 0;
    int32_t hiddenAssetModifiedCnt_ = 0;
    bool isForceRefresh_ = false;
    bool isHiddenForceRefresh_ = false;
    AlbumRefreshTimestamp albumRefreshTimestamp_;
    AlbumRefreshTimestamp albumHiddenRefreshTimestamp_;

    std::string ToString() const;
    bool IsAlbumInfoRefresh() const
    {
        return assetModifiedCnt_ > 0;
    }
    bool IsAlbumHiddenInfoRefresh() const
    {
        return hiddenAssetModifiedCnt_ > 0;
    }
};

class AnalysisAlbumRefreshInfo {
public:
    int32_t deltaCount_ {0};
    PhotoAssetChangeInfo deltaAddCover_;
    std::unordered_set<int32_t> removeFileIds_;
    std::string refreshCover_;
    bool needRefreshCover_ {false};
    bool isForceRefresh_ {false};
    AlbumRefreshTimestamp albumRefreshTimestamp_;

    std::string ToString() const;

    bool HasValidRefreshInfo() const
    {
        return deltaCount_ != 0 || deltaAddCover_.fileId_ != INVALID_INT32_VALUE ||
            !removeFileIds_.empty();
    }

    void MergeFromGroup(const AnalysisAlbumRefreshInfo &groupInfo)
    {
        deltaCount_ = groupInfo.deltaCount_;
        deltaAddCover_ = groupInfo.deltaAddCover_;
        removeFileIds_ = groupInfo.removeFileIds_;
        isForceRefresh_ = groupInfo.isForceRefresh_;
        refreshCover_ = groupInfo.refreshCover_;
        needRefreshCover_ = groupInfo.needRefreshCover_;
        albumRefreshTimestamp_ = groupInfo.albumRefreshTimestamp_;
    }
};

} // namespace Media
} // namespace OHOS

#endif
