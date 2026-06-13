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

#define MLOG_TAG "AccurateRefresh::AlbumRefreshExecution"

#include <sstream>

#include "album_refresh_execution.h"
#include "owner_album_info_calculation.h"
#include "cloud_enhancement_asset_helper.h"
#include "favorite_asset_helper.h"
#include "hidden_asset_helper.h"
#include "image_asset_helper.h"
#include "trash_asset_helper.h"
#include "video_asset_helper.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "accurate_debug_log.h"
#include "medialibrary_notify.h"
#include "album_accurate_refresh_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"
#include "dfx_refresh_manager.h"
#include "dfx_refresh_hander.h"
#include "media_values_bucket_utils.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media;

namespace OHOS {
namespace Media::AccurateRefresh {
namespace {
const std::unordered_map<std::string, std::string> validOrderKeys = {
    {CONST_MEDIA_DATA_DB_DATE_TAKEN, CONST_MEDIA_DATA_DB_NAME},
    {CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_NAME},
    {CONST_MEDIA_DATA_DB_NAME, CONST_MEDIA_DATA_DB_DATE_TAKEN},
    {CONST_MEDIA_DATA_DB_SIZE, CONST_MEDIA_DATA_DB_NAME}
};
const std::unordered_map<std::string, std::string> validHiddenOrderKeys = {
    {CONST_MEDIA_DATA_DB_DATE_TAKEN, CONST_MEDIA_DATA_DB_NAME},
    {CONST_MEDIA_DATA_DB_DATE_ADDED, CONST_MEDIA_DATA_DB_NAME},
    {CONST_MEDIA_DATA_DB_NAME, CONST_MEDIA_DATA_DB_DATE_TAKEN},
    {CONST_MEDIA_DATA_DB_SIZE, CONST_MEDIA_DATA_DB_NAME},
    {"hidden_time", CONST_MEDIA_DATA_DB_NAME}
};
}
mutex AlbumRefreshExecution::albumRefreshMtx_;

std::unordered_map<PhotoAlbumSubType, SystemAlbumInfoCalculation>
    AlbumRefreshExecution::systemTypeAlbumCalculations_ = {
    { PhotoAlbumSubType::FAVORITE, { FavoriteAssetHelper::IsAsset, FavoriteAssetHelper::IsVideoAsset,
        FavoriteAssetHelper::IsHiddenAsset, FavoriteAssetHelper::IsNewerAsset,
        FavoriteAssetHelper::IsNewerHiddenAsset, PhotoAlbumSubType::FAVORITE }},
    { PhotoAlbumSubType::VIDEO, { VideoAssetHelper::IsAsset, VideoAssetHelper::IsVideoAsset,
        VideoAssetHelper::IsHiddenAsset, VideoAssetHelper::IsNewerAsset,
        VideoAssetHelper::IsNewerHiddenAsset, PhotoAlbumSubType::VIDEO }},
    { PhotoAlbumSubType::HIDDEN, { HiddenAssetHelper::IsAsset, HiddenAssetHelper::IsVideoAsset,
        HiddenAssetHelper::IsHiddenAsset, HiddenAssetHelper::IsNewerAsset,
        HiddenAssetHelper::IsNewerHiddenAsset, PhotoAlbumSubType::HIDDEN }},
    { PhotoAlbumSubType::TRASH, { TrashAssetHelper::IsAsset, TrashAssetHelper::IsVideoAsset,
        TrashAssetHelper::IsHiddenAsset, TrashAssetHelper::IsNewerAsset,
        TrashAssetHelper::IsNewerHiddenAsset, PhotoAlbumSubType::TRASH }},
    { PhotoAlbumSubType::IMAGE, { ImageAssetHelper::IsAsset, ImageAssetHelper::IsVideoAsset,
        ImageAssetHelper::IsHiddenAsset, ImageAssetHelper::IsNewerAsset,
        ImageAssetHelper::IsNewerHiddenAsset, PhotoAlbumSubType::IMAGE }},
    { PhotoAlbumSubType::CLOUD_ENHANCEMENT, { CloudEnhancementAssetHelper::IsAsset,
        CloudEnhancementAssetHelper::IsVideoAsset, CloudEnhancementAssetHelper::IsHiddenAsset,
        CloudEnhancementAssetHelper::IsNewerAsset, CloudEnhancementAssetHelper::IsNewerHiddenAsset,
        PhotoAlbumSubType::CLOUD_ENHANCEMENT }}
};

std::unordered_map<int32_t, SystemAlbumInfoCalculation> AlbumRefreshExecution::systemAlbumCalculations_;

int32_t AlbumRefreshExecution::RefreshAlbum(const vector<PhotoAssetChangeData> &assetChangeDatas,
    NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified)
{
    // 计算影响相册和refresh info，结果放入systemAlbumInfos_和ownerAlbumInfos_
    CalRefreshInfos(assetChangeDatas);
    CHECK_AND_RETURN_RET_INFO_LOG(!albumRefreshInfos_.empty(), ACCURATE_REFRESH_RET_OK, "no refresh album");

    // 相册执行逻辑只能串行执行
    lock_guard<mutex> lock(albumRefreshMtx_);
    isRefreshWithDateModified_ = isRefreshWithDateModified;

    // 计算相册信息
    CalAlbumsInfos();

    // 更新相册
    UpdateAllAlbums(notifyAlbumType);

    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumRefreshExecution::CalRefreshInfos(const vector<PhotoAssetChangeData> &assetChangeDatas)
{
    CheckInitSystemCalculation();
    // 计算系统相册信息
    {
        MediaLibraryTracer tracer;
        tracer.Start("AlbumRefreshExecution::CalRefreshInfos system");
        for (auto &item : systemAlbumCalculations_) {
            for (auto const &assetChangeData : assetChangeDatas) {
                const auto &albumId = item.first;
                SystemAlbumInfoCalculation &calculation = item.second;
                auto subTypeInfoIter = systemAlbumRefreshInfos_.find(albumId);
                if (subTypeInfoIter != systemAlbumRefreshInfos_.end()) {
                    calculation.CalAlbumRefreshInfo(assetChangeData, subTypeInfoIter->second, albumId);
                    continue;
                }
                AlbumRefreshInfo refreshInfo;
                if (calculation.CalAlbumRefreshInfo(assetChangeData, refreshInfo, albumId)) {
                    systemAlbumRefreshInfos_.emplace(albumId, refreshInfo);
                }
            }
        }
    }

    // 计算用户和来源相册
    {
        MediaLibraryTracer tracer;
        tracer.Start("AlbumRefreshExecution::CalRefreshInfos owner albumId");
        ownerAlbumRefreshInfos_ = OwnerAlbumInfoCalculation::CalOwnerAlbumRefreshInfo(assetChangeDatas);
        albumRefreshInfos_.insert(systemAlbumRefreshInfos_.begin(), systemAlbumRefreshInfos_.end());
        albumRefreshInfos_.insert(ownerAlbumRefreshInfos_.begin(), ownerAlbumRefreshInfos_.end());
        if (IS_ACCURATE_DEBUG) {
            for (auto const &albumInfo : albumRefreshInfos_) {
                MEDIA_DEBUG_LOG("albumId: %{public}d, refreshInfo: %{public}s", albumInfo.first,
                    albumInfo.second.ToString().c_str());
            }
        }
    }

    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumRefreshExecution::CalAlbumsInfos()
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumRefreshExecution::CalAlbumsInfos");
    // 查询相册数据
    albumRefresh_.Init(GetAlbumIds());
    initAlbumInfos_ = albumRefresh_.GetInitAlbumInfos();
    if (initAlbumInfos_.empty()) {
        ACCURATE_DEBUG("empty album Info, insert.");
        return ACCURATE_REFRESH_RET_OK;
    }

    // 修改数据
    for (auto item : initAlbumInfos_) {
        auto &albumInfo = item.second;
        auto albumId = albumInfo.albumId_;
        auto calAlbumInfoIter = albumRefreshInfos_.find(static_cast<PhotoAlbumSubType>(albumId));
        if (calAlbumInfoIter == albumRefreshInfos_.end()) {
            MEDIA_ERR_LOG("no album[%{public}d] found", albumInfo.albumId_);
            continue;
        }
        AlbumRefreshInfo &refreshInfo = calAlbumInfoIter->second;
        auto albumInfoBefore = albumInfo;
        auto subType = albumInfo.albumSubType_;
        bool isAccurateRefresh = CalAlbumInfos(albumInfo, refreshInfo, subType);
        ACCURATE_INFO("albumInfo: %{public}s", albumInfoBefore.ToString(true).c_str());
        if (isAccurateRefresh) {
            ACCURATE_INFO("refresh info: %{public}s", refreshInfo.ToString().c_str());
            ACCURATE_INFO("cal albumInfo: %{public}s", albumInfo.ToString(true).c_str());
        }
    }

    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumRefreshExecution::Notify()
{
    albumRefresh_.Notify(dfxRefreshManager_);
    return ACCURATE_REFRESH_RET_OK;
}

vector<int32_t> AlbumRefreshExecution::GetAlbumIds()
{
    vector<int32_t> albumIds;
    for (auto const &item : albumRefreshInfos_) {
        albumIds.push_back(item.first);
    }
    return albumIds;
}

void AlbumRefreshExecution::CheckNotifyAlbum()
{
    affectedAlbumCount_ ++;
    if (affectedAlbumCount_ == MAX_AFFECTED_ALBUM_LENGTH) {
        affectedAlbumCount_ = 0;
        albumRefresh_.Notify();
        albumRefresh_.ClearChangeInfos();
    }
}

int32_t AlbumRefreshExecution::UpdateAllAlbums(NotifyAlbumType notifyAlbumType)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumRefreshExecution::UpdateAllAlbums");
    for (auto &albumId : forceRefreshAlbums_) {
        // 封装刷新处理；全量刷新指定的相册（系统相册、用户相册、来源相册）
        ForceUpdateAlbums(albumId, false, notifyAlbumType);
    }

    for (auto &albumId : forceRefreshHiddenAlbums_) {
        // 封装刷新处理；全量刷新指定的隐藏相册
        ForceUpdateAlbums(albumId, true, notifyAlbumType);
    }

    AccurateUpdateAlbums(notifyAlbumType);
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumRefreshExecution::ForceUpdateAlbums(int32_t albumId, bool isHidden, NotifyAlbumType notifyAlbumType)
{
    DfxRefreshHander::SetOperationStartTimeHander(dfxRefreshManager_);
    const auto &iter = initAlbumInfos_.find(albumId);
    if (iter == initAlbumInfos_.end()) {
        MEDIA_WARN_LOG("no album info.");
        return ACCURATE_REFRESH_ALBUM_INFO_NULL;
    }
    AlbumRefreshTimestampRecord refreshRecord(albumId, isHidden);
    MediaLibraryTracer tracer;
    tracer.Start("AlbumRefreshExecution::ForceUpdateAlbums " + to_string(albumId) + (isHidden ? " hidden" : ""));
    ACCURATE_DEBUG("force update albumId[%{public}d], isHidden[%{public}d]", albumId, isHidden);
    auto &albumInfo = iter->second;
    PhotoAlbumSubType subtype = static_cast<PhotoAlbumSubType>(albumInfo.albumSubType_);
    ValuesBucket values;
    OHOS::Media::NotifyType type = OHOS::Media::NotifyType::NOTIFY_INVALID;
    GetUpdateValues(values, albumInfo, isHidden, type);
    if (values.IsEmpty()) {
        MEDIA_ERR_LOG("albumId[%{public}d], subType[%{public}d], hidden[%{public}d] no need update.", albumId, subtype,
            isHidden);
        refreshRecord.ClearRecord();
        return ACCURATE_REFRESH_RET_OK;
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(subtype));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumInfo.albumId_));
    int32_t changedRows = 0;
    auto ret = albumRefresh_.Update(changedRows, values, predicates);
    ACCURATE_DEBUG("ret: %{public}d, changed: %{public}d, type: %{public}d, albumId: %{public}d, isHidden: %{public}d",
        ret, changedRows, albumInfo.albumSubType_, albumInfo.albumId_, isHidden);
    if (ret == ACCURATE_REFRESH_RET_OK) {
        if (!isHidden) {
            CheckNotifyOldNotification(notifyAlbumType, albumInfo, type);
        }
        refreshRecord.RefreshAlbumEnd();
    }
    DfxRefreshHander::SetAlbumIdAndOptTimeHander(albumId, isHidden, dfxRefreshManager_);
    CheckNotifyAlbum();
    return ACCURATE_REFRESH_RET_OK;
}

int32_t AlbumRefreshExecution::GetUpdateValues(ValuesBucket &values, const AlbumChangeInfo &albumInfo,
    bool isHidden, OHOS::Media::NotifyType &type)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumRefreshExecution::GetUpdateValues");
    PhotoAlbumSubType subtype = static_cast<PhotoAlbumSubType>(albumInfo.albumSubType_);
    struct UpdateAlbumData data;
    data.albumSubtype = albumInfo.albumSubType_;
    data.albumId = albumInfo.albumId_;
    data.hiddenCount = albumInfo.hiddenCount_;
    data.albumCount = albumInfo.count_;
    data.albumImageCount = albumInfo.imageCount_;
    data.albumVideoCount = albumInfo.videoCount_;
    data.hiddenCover = albumInfo.hiddenCoverUri_;
    data.albumCoverUri = albumInfo.coverUri_;
    data.coverDateTime = albumInfo.coverDateTime_;
    data.coverUriSource = albumInfo.coverUriSource_;
    data.hiddenCoverDateTime = albumInfo.hiddenCoverDateTime_;
    data.coverOrderKey = albumInfo.coverOrderKey_;
    data.coverOrderSubKey = albumInfo.coverOrderSubKey_;
    data.coverOrderType = albumInfo.coverOrderType_;
    data.hiddenCoverOrderKey = albumInfo.hiddenCoverOrderKey_;
    data.hiddenCoverOrderSubKey = albumInfo.hiddenCoverOrderSubKey_;
    data.hiddenCoverOrderType = albumInfo.hiddenCoverOrderType_;
    if (!isHidden && (subtype == PhotoAlbumSubType::USER_GENERIC ||
        (subtype >= PhotoAlbumSubType::SOURCE_START && subtype <= PhotoAlbumSubType::SOURCE_END))
        && isRefreshWithDateModified_) {
        data.shouldUpdateDateModified = true; // 非隐藏全量刷新时，说明相册封面有变化，需要设置
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, ACCURATE_REFRESH_RDB_NULL, "rdbStore null");
    int32_t ret = MediaLibraryRdbUtils::GetUpdateValues(rdbStore, data, values, isHidden);
    type = data.albumCount < data.newTotalCount ? NOTIFY_ALBUM_ADD_ASSET :
        (data.albumCount > data.newTotalCount ? NOTIFY_ALBUM_REMOVE_ASSET : NOTIFY_UPDATE);
    return ret;
}

void AlbumRefreshExecution::CheckUpdateValues(const AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo,
    ValuesBucket &values)
{
    if ((albumInfo.albumSubType_ == PhotoAlbumSubType::USER_GENERIC ||
        albumInfo.albumSubType_ == PhotoAlbumSubType::SOURCE_GENERIC) && refreshInfo.assetModifiedCnt_ > 0 &&
        isRefreshWithDateModified_) {
        values.PutLong(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
        ACCURATE_DEBUG("album date modified.");
    }
}

int32_t AlbumRefreshExecution::SetForceSelectCoverValues(ValuesBucket &values, const AlbumChangeInfo &albumInfo,
    bool isHidden)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumRefreshExecution::SetForceSelectCoverValues " +
        to_string(albumInfo.albumId_) + (isHidden ? " hidden" : ""));
    PhotoAlbumSubType subtype = static_cast<PhotoAlbumSubType>(albumInfo.albumSubType_);
    struct UpdateAlbumData data;
    data.albumSubtype = albumInfo.albumSubType_;
    data.albumId = albumInfo.albumId_;
    data.hiddenCount = albumInfo.hiddenCount_;
    data.albumCount = albumInfo.count_;
    data.albumImageCount = albumInfo.imageCount_;
    data.albumVideoCount = albumInfo.videoCount_;
    data.hiddenCover = albumInfo.hiddenCoverUri_;
    data.albumCoverUri = albumInfo.coverUri_;
    data.coverDateTime = albumInfo.coverDateTime_;
    data.coverUriSource = albumInfo.coverUriSource_;
    data.hiddenCoverDateTime = albumInfo.hiddenCoverDateTime_;
    data.coverOrderKey = albumInfo.coverOrderKey_;
    data.coverOrderSubKey = albumInfo.coverOrderSubKey_;
    data.coverOrderType = albumInfo.coverOrderType_;
    data.hiddenCoverOrderKey = albumInfo.hiddenCoverOrderKey_;
    data.hiddenCoverOrderSubKey = albumInfo.hiddenCoverOrderSubKey_;
    data.hiddenCoverOrderType = albumInfo.hiddenCoverOrderType_;
    if (!isHidden && (subtype == PhotoAlbumSubType::USER_GENERIC ||
        subtype == PhotoAlbumSubType::SOURCE_GENERIC) && isRefreshWithDateModified_) {
        data.shouldUpdateDateModified = true; // 非隐藏全量刷新时，说明相册封面有变化，需要设置
    }
    int32_t ret = MediaLibraryRdbUtils::SetUpdateCoverValues(data, values, isHidden);
    return ret;
}

void AlbumRefreshExecution::GetUpdateValuesAndNotifyType(const AccurateRefresh::AlbumChangeInfo &initAlbumInfo,
    AccurateRefresh::AlbumChangeInfo &albumInfo, ValuesBucket &values, NotifyType &type)
{
    values = albumInfo.GetUpdateValues(initAlbumInfo, type);
    int countValue = 0;
    int hiddenCountValue = 0;
    MediaValuesBucketUtils::GetInt(values, PhotoAlbumColumns::ALBUM_COUNT, countValue);
    MediaValuesBucketUtils::GetInt(values, PhotoAlbumColumns::HIDDEN_COUNT, hiddenCountValue);
    if (countValue < 0 || hiddenCountValue < 0) {
        MEDIA_WARN_LOG("Something went wrong. Caculated count: %{public}d, hiddenCount: %{public}d."
            " Start scanning entire album %{public}d", countValue, hiddenCountValue, initAlbumInfo.albumId_);
        values.Clear();
        AlbumRefreshExecution::GetUpdateValues(values, initAlbumInfo, true, type);
        AlbumRefreshExecution::GetUpdateValues(values, initAlbumInfo, false, type);
    } else {
        if (albumInfo.needForceSelectCover) {
            SetForceSelectCoverValues(values, initAlbumInfo, false);
        }
        if (albumInfo.needForceSelectHiddenCover) {
            SetForceSelectCoverValues(values, initAlbumInfo, true);
        }
    }
}

int32_t AlbumRefreshExecution::AccurateUpdateAlbums(NotifyAlbumType notifyAlbumType)
{
    ACCURATE_INFO("AccurateUpdateAlbums notifyAlbumType[0x%{public}x]", notifyAlbumType);
    MediaLibraryTracer tracer;
    tracer.Start("AlbumRefreshExecution::AccurateUpdateAlbums");
    for (auto &iter : refreshAlbums_) {
        DfxRefreshHander::SetOperationStartTimeHander(dfxRefreshManager_);
        auto &albumInfo = iter.second.second;
        MEDIA_DEBUG_LOG("## Update type: %{public}d, albumId: %{public}d start", albumInfo.albumSubType_,
            albumInfo.albumId_);
        const auto &initIter = initAlbumInfos_.find(albumInfo.albumId_);
        if (initIter == initAlbumInfos_.end()) {
            MEDIA_WARN_LOG("no album info.");
            return ACCURATE_REFRESH_ALBUM_INFO_NULL;
        }

        auto &initAlbumInfo = initIter->second;
        OHOS::Media::NotifyType type = OHOS::Media::NotifyType::NOTIFY_INVALID;
        ValuesBucket values {};
        GetUpdateValuesAndNotifyType(initAlbumInfo, albumInfo, values, type);
        CheckUpdateValues(albumInfo, iter.second.first, values);
        if (values.IsEmpty()) {
            MEDIA_ERR_LOG("no need update.");
            continue;
        }
        RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumInfo.albumId_));
        int32_t changedRows = 0;

        albumRefresh_.Update(changedRows, values, predicates);
        auto &albumChangeInfo = iter.second.first;
        if (albumChangeInfo.IsAlbumInfoRefresh()) {
            CheckNotifyOldNotification(notifyAlbumType, albumInfo, type);
        }
        MEDIA_INFO_LOG("## Update type: %{public}d, albumId: %{public}d end", albumInfo.albumSubType_,
            albumInfo.albumId_);
        DfxRefreshHander::SetOptEndTimeHander(predicates, dfxRefreshManager_);
        DfxRefreshHander::SetAlbumIdHander(albumInfo.albumId_, dfxRefreshManager_);
        CheckNotifyAlbum();
    }
    return ACCURATE_REFRESH_RET_OK;
}

void AlbumRefreshExecution::CheckHiddenAlbumInfo(ValuesBucket &values, stringstream &ss)
{
    int32_t hiddenCount;
    ValueObject value;
    if (values.GetObject(PhotoAlbumColumns::HIDDEN_COUNT, value)) {
        value.GetInt(hiddenCount);
        ss << " hiddenCount: " << hiddenCount;
    }
    string hiddenCover;
    if (values.GetObject(PhotoAlbumColumns::HIDDEN_COVER, value)) {
        value.GetString(hiddenCover);
        ss << " hiddenCover: " << hiddenCover;
    }
    int64_t hiddenCoverDateTime;
    if (values.GetObject(PhotoAlbumColumns::HIDDEN_COVER_DATE_TIME, value)) {
        value.GetLong(hiddenCoverDateTime);
        ss << " hiddenCoverDateTime: " << hiddenCoverDateTime;
    }
    int32_t containHidden;
    if (values.GetObject(PhotoAlbumColumns::CONTAINS_HIDDEN, value)) {
        value.GetInt(containHidden);
        ss << " containHidden: " << containHidden;
    }
}

void AlbumRefreshExecution::CheckAlbumInfo(ValuesBucket &values, stringstream &ss)
{
    int32_t count;
    ValueObject value;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_COUNT, value)) {
        value.GetInt(count);
        ss << " count: " << count;
    }
    string cover;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_COVER_URI, value)) {
        value.GetString(cover);
        ss << " cover: " << cover;
    }
    int64_t coverDateTime;
    if (values.GetObject(PhotoAlbumColumns::COVER_DATE_TIME, value)) {
        value.GetLong(coverDateTime);
        ss << " coverDateTime: " << coverDateTime;
    }

    int32_t videoCount;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, value)) {
        value.GetInt(videoCount);
        ss << " videoCount: " << videoCount;
    }

    int32_t imageCount;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, value)) {
        value.GetInt(imageCount);
        ss << " imageCount: " << imageCount;
    }
}

void AlbumRefreshExecution::CheckUpdateAlbumInfo(const AlbumChangeInfo &albumInfo, bool isHidden)
{
    ValuesBucket values;
    NotifyType type;
    GetUpdateValues(values, albumInfo, isHidden, type);
    if (values.IsEmpty()) {
        return;
    }
    stringstream ss;
    ValueObject value;
    if (isHidden) {
        CheckHiddenAlbumInfo(values, ss);
    } else {
        CheckAlbumInfo(values, ss);
    }

    int64_t dateModified;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_DATE_MODIFIED, value)) {
        value.GetLong(dateModified);
        ss << "dateModified: " << dateModified;
    }

    ACCURATE_INFO("album cal wrong, albumId[%{public}d], isHiden[%{public}d]: %{public}s", albumInfo.albumId_,
        isHidden, ss.str().c_str());
}

bool AlbumRefreshExecution::CalAlbumHiddenCount(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo)
{
    albumInfo.hiddenCount_ += refreshInfo.deltaHiddenCount_;
    return refreshInfo.deltaHiddenCount_ != 0;
}

bool AlbumRefreshExecution::CalAlbumCount(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo)
{
    // count 更新
    albumInfo.count_ += refreshInfo.deltaCount_;
    albumInfo.videoCount_ += refreshInfo.deltaVideoCount_;
    albumInfo.imageCount_ += (refreshInfo.deltaCount_ - refreshInfo.deltaVideoCount_);
    return refreshInfo.deltaCount_ != 0 || refreshInfo.deltaVideoCount_ != 0;
}

bool AlbumRefreshExecution::IsValidCover(const PhotoAssetChangeInfo &assetInfo)
{
    return assetInfo.fileId_ != INVALID_INT32_VALUE;
}

bool AlbumRefreshExecution::CalAlbumInfos(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo,
    int32_t subType)
{
    bool needRefreshAblum = CalAlbumInfo(albumInfo, refreshInfo, subType);
    bool needRefreshHiddenAlbum = CalHiddenAlbumInfo(albumInfo, refreshInfo, subType);
    // 相册的普通信息和隐藏信息在同一个albumInfo中，只刷新其中一类信息时，另一类信息为无效值
    if (needRefreshAblum || needRefreshHiddenAlbum) {
        refreshAlbums_.insert(make_pair(albumInfo.albumId_, make_pair(refreshInfo, albumInfo)));
        return true;
    }
    ACCURATE_INFO("accurate refresh album_info[%{public}d], hidden_album_info[%{public}d]", needRefreshAblum,
        needRefreshHiddenAlbum);
    return false;
}

bool AlbumRefreshExecution::CalAlbumInfo(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo,
    int32_t subType)
{
    // 计算结果需要forceRefresh
    if (refreshInfo.isForceRefresh_) {
        forceRefreshAlbums_.insert(albumInfo.albumId_);
        ClearAlbumInfo(albumInfo);
        ACCURATE_DEBUG("force refresh album[%{public}d] info", albumInfo.albumId_);
        return false;
    }
    // 增量计算后相册有变化
    if (refreshInfo.IsAlbumInfoRefresh()) {
        bool isTimestampMatch = AlbumAccurateRefreshManager::GetInstance().IsRefreshTimestampMatch(albumInfo.albumId_,
            false, refreshInfo.albumRefreshTimestamp_);
        bool isAccurateRefresh = AlbumAccurateRefreshManager::GetInstance().IsAlbumAccurateRefresh(albumInfo.albumId_,
            false);
        // 增量计算时间戳不匹配 || 不能增量刷新（需要全量刷新）
        if (!isTimestampMatch || !isAccurateRefresh) {
            forceRefreshAlbums_.insert(albumInfo.albumId_);
            ClearAlbumInfo(albumInfo);
            ACCURATE_DEBUG("force refresh album[%{public}d] info, isTimestampMatch[%{public}d], "
                "isAccurateRefresh[%{public}d]", albumInfo.albumId_, isTimestampMatch, isAccurateRefresh);
            return false;
        }
    }

    // 更新count
    bool isRefreshAlbumCount = CalAlbumCount(albumInfo, refreshInfo);
    // 更新uri、dateTimeForCover
    bool isRefreshAlbumCover = CalAlbumCover(albumInfo, refreshInfo, subType);

    // 增量刷新场景：不强制刷新 && count/video count/cover无变化
    return forceRefreshAlbums_.find(albumInfo.albumId_) == forceRefreshAlbums_.end() &&
        (isRefreshAlbumCover || isRefreshAlbumCount);
}

bool AlbumRefreshExecution::CalHiddenAlbumInfo(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo,
    int32_t subType)
{
    if (CheckSetHiddenAlbumInfo(albumInfo)) {
        return false;
    }
        // 计算结果需要forceRefresh || 增量计算后相册有变化（增量计算时间戳不匹配 || 不能增量刷新)
    if (refreshInfo.isHiddenForceRefresh_) {
        forceRefreshHiddenAlbums_.insert(albumInfo.albumId_);
        ClearHiddenAlbumInfo(albumInfo);
        ACCURATE_DEBUG("force update album[%{public}d] hidden info", albumInfo.albumId_);
        return false;
    }
    if (refreshInfo.IsAlbumHiddenInfoRefresh()) {
        bool isTimestampMatch = AlbumAccurateRefreshManager::GetInstance().IsRefreshTimestampMatch(albumInfo.albumId_,
            true, refreshInfo.albumHiddenRefreshTimestamp_);
        bool isAccurateRefresh = AlbumAccurateRefreshManager::GetInstance().IsAlbumAccurateRefresh(albumInfo.albumId_,
            true);
        if (!isTimestampMatch || !isAccurateRefresh) {
            forceRefreshHiddenAlbums_.insert(albumInfo.albumId_);
            ClearHiddenAlbumInfo(albumInfo);
            ACCURATE_DEBUG("force update album[%{public}d] hidden info, isTimestampMatch[%{public}d], "
                "isAccurateRefresh[%{public}d]", albumInfo.albumId_, isTimestampMatch, isAccurateRefresh);
            return false;
        }
    }
    bool isRefreshHiddenAlbumCount = CalAlbumHiddenCount(albumInfo, refreshInfo);
    // 更新hiddenCoverUri、dateTimeForHiddenCover
    bool isRefreshHiddenAlbumCover = CalAlbumHiddenCover(albumInfo, refreshInfo);

    // 增量刷新场景：不强制刷新 && hidden count/cover无变化
    return forceRefreshHiddenAlbums_.find(albumInfo.albumId_) ==
        forceRefreshHiddenAlbums_.end() && (isRefreshHiddenAlbumCover || isRefreshHiddenAlbumCount);
}

bool AlbumRefreshExecution::CalCoverSetCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo)
{
    bool isCoverSet = albumInfo.coverUriSource_ > 0;
    if (!isCoverSet) {
        return false;
    }
    auto coverFileId = MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(albumInfo.coverUri_);
    if (coverFileId <= 0 || refreshInfo.removeFileIds.find(coverFileId) != refreshInfo.removeFileIds.end()) {
        forceRefreshAlbums_.insert(albumInfo.albumId_);
        ClearAlbumInfo(albumInfo);
        ACCURATE_DEBUG("cover already set, refresh album for setcover.");
    } else {
        ACCURATE_DEBUG("cover already set, no need cal album cover.");
    }
    return true;
}

static bool RefreshByCoverOrder(const AlbumRefreshInfo &refreshInfo, AlbumChangeInfo &albumInfo,
    int32_t currentFileId, int64_t coverDateTime)
{
    CHECK_AND_RETURN_RET_INFO_LOG(validOrderKeys.count(albumInfo.coverOrderKey_) > 0, false,
        "[fm] no valid order set[%{public}d]", albumInfo.albumId_);

    auto isNewer = refreshInfo.deltaAddCover_.fileId_ > currentFileId;
    auto compare = [&isNewer, &albumInfo](int64_t before, int64_t after, const std::string &beforeSub,
        const std::string &afterSub) -> std::pair<bool, bool> {
            return { before == INVALID_INT64_VALUE, albumInfo.coverOrderType_ == 0 ?
                (before == after ? (beforeSub == afterSub ? isNewer : beforeSub < afterSub) : before < after) :
                (before == after ? (beforeSub == afterSub ? isNewer : beforeSub > afterSub) : before > after) };
    };

    auto compareName = [&isNewer, &albumInfo](const std::string &before, const std::string &after,
        int64_t beforeSub, int64_t afterSub) -> std::pair<bool, bool> {
            return { before == EMPTY_STR, albumInfo.coverOrderType_ == 0 ?
                (before == after ? (beforeSub == afterSub ? isNewer : beforeSub < afterSub) : before < after) :
                (before == after ? (beforeSub == afterSub ? isNewer : beforeSub > afterSub) : before > after) };
    };

    // 是否强制刷新，本次是否需要刷新
    std::pair<bool, bool> result = { false, false };
    const auto &current = albumInfo.coverInfo_;
    const auto &refresh = refreshInfo.deltaAddCover_;
    if (albumInfo.coverOrderKey_ == CONST_MEDIA_DATA_DB_DATE_TAKEN) {
        result = compare(current.dateTakenMs_, refresh.dateTakenMs_, current.displayName_, refresh.displayName_);
    } else if (albumInfo.coverOrderKey_ == CONST_MEDIA_DATA_DB_SIZE) {
        result = compare(current.size_, refresh.size_, current.displayName_, refresh.displayName_);
    } else if (albumInfo.coverOrderKey_ == CONST_MEDIA_DATA_DB_DATE_ADDED) {
        result = compare(current.dateAddedMs_, refresh.dateAddedMs_, current.displayName_, refresh.displayName_);
    } else {
        result = compareName(current.displayName_, refresh.displayName_, current.dateTakenMs_, refresh.dateTakenMs_);
    }

    if (result.first) {
        albumInfo.needForceSelectCover = true;
        albumInfo.coverUri_ = EMPTY_STR;
        albumInfo.coverDateTime_ = INVALID_INT64_VALUE;
        return true;
    }
    if (result.second) {
        albumInfo.coverInfo_ = refreshInfo.deltaAddCover_;
        albumInfo.coverDateTime_ = coverDateTime;
        albumInfo.coverUri_ = refreshInfo.deltaAddCover_.uri_;
    }
    return true;
}

static bool RefreshByCoverOrderHidden(const AlbumRefreshInfo &refreshInfo, AlbumChangeInfo &albumInfo,
    int32_t currentFileId)
{
    CHECK_AND_RETURN_RET_INFO_LOG(validHiddenOrderKeys.count(albumInfo.hiddenCoverOrderKey_) > 0, false,
        "[fm] no valid hidden order set[%{public}d]", albumInfo.albumId_);

    auto isNewer = refreshInfo.deltaAddHiddenCover_.fileId_ > currentFileId;
    auto compare = [&isNewer, &albumInfo](int64_t before, int64_t after, const std::string &beforeSub,
        const std::string &afterSub) -> std::pair<bool, bool> {
            return { before == INVALID_INT64_VALUE, albumInfo.hiddenCoverOrderType_ == 0 ?
                (before == after ? (beforeSub == afterSub ? isNewer : beforeSub < afterSub) : before < after) :
                (before == after ? (beforeSub == afterSub ? isNewer : beforeSub > afterSub) : before > after) };
    };
    auto compareName = [&isNewer, &albumInfo](const std::string &before, const std::string &after,
    int64_t beforeSub, int64_t afterSub) -> std::pair<bool, bool> {
        return { before == EMPTY_STR, albumInfo.hiddenCoverOrderType_ == 0 ?
            (before == after ? (beforeSub == afterSub ? isNewer : beforeSub < afterSub) : before < after) :
            (before == after ? (beforeSub == afterSub ? isNewer : beforeSub > afterSub) : before > after) };
    };

    // 是否强制刷新，本次是否需要刷新
    std::pair<bool, bool> result = { false, false };
    const auto &current = albumInfo.hiddenCoverInfo_;
    const auto &refresh = refreshInfo.deltaAddHiddenCover_;
    if (albumInfo.hiddenCoverOrderKey_ == CONST_MEDIA_DATA_DB_DATE_TAKEN) {
        result = compare(current.dateTakenMs_, refresh.dateTakenMs_, current.displayName_, refresh.displayName_);
    } else if (albumInfo.hiddenCoverOrderKey_ == CONST_MEDIA_DATA_DB_SIZE) {
        result = compare(current.size_, refresh.size_, current.displayName_, refresh.displayName_);
    } else if (albumInfo.hiddenCoverOrderKey_ == CONST_MEDIA_DATA_DB_DATE_ADDED) {
        result = compare(current.dateAddedMs_, refresh.dateAddedMs_, current.displayName_, refresh.displayName_);
    } else if (albumInfo.hiddenCoverOrderKey_ == CONST_MEDIA_DATA_DB_NAME) {
        result = compareName(current.displayName_, refresh.displayName_, current.dateTakenMs_, refresh.dateTakenMs_);
    } else {
        result = compare(current.hiddenTime_, refresh.hiddenTime_, current.displayName_, refresh.displayName_);
    }

    if (result.first) {
        albumInfo.needForceSelectHiddenCover = true;
        albumInfo.hiddenCoverUri_ = EMPTY_STR;
        albumInfo.hiddenCoverDateTime_ = INVALID_INT64_VALUE;
        return true;
    }
    if (result.second) {
        albumInfo.hiddenCoverInfo_ = refreshInfo.deltaAddHiddenCover_;
        albumInfo.hiddenCoverDateTime_ = refreshInfo.deltaAddHiddenCover_.hiddenTime_;
        albumInfo.hiddenCoverUri_ = refreshInfo.deltaAddHiddenCover_.uri_;
    }
    return true;
}

bool AlbumRefreshExecution::CalAlbumCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo,
    int32_t subType)
{
    if (CalCoverSetCover(albumInfo, refreshInfo)) {
        return false;
    }
    bool isRefreshAlbum = false;
    // 系统相册、用户、来源资产对比默认为date_taken
    auto dateTimeForAddCover = refreshInfo.deltaAddCover_.dateTakenMs_;
    if (subType == static_cast<int32_t>(PhotoAlbumSubType::VIDEO) ||
        subType == static_cast<int32_t>(PhotoAlbumSubType::IMAGE)) {
        // date_added
        dateTimeForAddCover = refreshInfo.deltaAddCover_.dateAddedMs_;
    } else if (subType == static_cast<int32_t>(PhotoAlbumSubType::HIDDEN)) {
        // hidden_time
        dateTimeForAddCover = refreshInfo.deltaAddCover_.hiddenTime_;
    }

    // 普通cover 更新
    auto coverFileId = MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(albumInfo.coverUri_);
    if (IsValidCover(refreshInfo.deltaAddCover_) && refreshInfo.removeFileIds.size() == 0) { // 新增场景
        CHECK_AND_RETURN_RET(!RefreshByCoverOrder(refreshInfo, albumInfo, coverFileId, dateTimeForAddCover), true);
        bool isRefresh = dateTimeForAddCover > albumInfo.coverDateTime_ ||
            (dateTimeForAddCover == albumInfo.coverDateTime_ && refreshInfo.deltaAddCover_.fileId_ > coverFileId);
        if (isRefresh) {
            albumInfo.coverInfo_ = refreshInfo.deltaAddCover_;
            albumInfo.coverDateTime_ = dateTimeForAddCover;
            // 更新coverUri
            albumInfo.coverUri_ = refreshInfo.deltaAddCover_.uri_;
            isRefreshAlbum = true;
        }
        ACCURATE_DEBUG("Add[%{public}d], refresh[%{public}d], albumInfo cover id: %{public}d, addCover id: %{public}d",
            albumInfo.albumId_, isRefresh, albumInfo.coverInfo_.fileId_, refreshInfo.deltaAddCover_.fileId_);
    } else if (!IsValidCover(refreshInfo.deltaAddCover_) && refreshInfo.removeFileIds.size() > 0) { // 删除场景
        // 当前cover没有removeCover新
        bool isForceRefresh = refreshInfo.removeFileIds.find(coverFileId) != refreshInfo.removeFileIds.end();
        if (coverFileId <= 0 || isForceRefresh) {
            albumInfo.needForceSelectCover = true;
            isRefreshAlbum = true;
            ClearAlbumCoverInfo(albumInfo);
        }
        ACCURATE_DEBUG("Del[%{public}d], forceSelectCover[%{public}d], old cover fileId:%{public}d", albumInfo.albumId_,
            isForceRefresh, coverFileId);
    } else if (IsValidCover(refreshInfo.deltaAddCover_) && refreshInfo.removeFileIds.size() > 0) {
        // 异常场景：同一个相册中cover既有新增又有删除;同一个相册中没有新增也没有删除
        // 全量刷新指定的系统相册
        albumInfo.needForceSelectCover = true;
        isRefreshAlbum = true;
        ClearAlbumCoverInfo(albumInfo);
        ACCURATE_ERR("Abnormal[%{public}d], forceSelectCover, addCover: %{public}s, remove size: %{public}zu",
            albumInfo.albumId_, refreshInfo.deltaAddCover_.ToString().c_str(),
            refreshInfo.removeFileIds.size());
    } else {
        CHECK_AND_RETURN_RET(!RefreshByCoverOrder(refreshInfo, albumInfo, coverFileId, dateTimeForAddCover), true);
    }
    return isRefreshAlbum;
}

bool AlbumRefreshExecution::CalAlbumHiddenCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo)
{
    bool isRefreshHiddenAlbum = false;
    auto coverFileId = MediaLibraryDataManagerUtils::GetFileIdNumFromPhotoUri(albumInfo.hiddenCoverUri_);
    if (IsValidCover(refreshInfo.deltaAddHiddenCover_) && refreshInfo.removeHiddenFileIds.size() == 0) {
        CHECK_AND_RETURN_RET(!RefreshByCoverOrderHidden(refreshInfo, albumInfo, coverFileId), true);
        // 新增场景
        bool isRefresh = refreshInfo.deltaAddHiddenCover_.hiddenTime_ > albumInfo.hiddenCoverDateTime_ ||
            (refreshInfo.deltaAddHiddenCover_.hiddenTime_ == albumInfo.hiddenCoverDateTime_ &&
            refreshInfo.deltaAddHiddenCover_.fileId_ > coverFileId);
        if (isRefresh) {
            albumInfo.hiddenCoverInfo_ = refreshInfo.deltaAddHiddenCover_;
            albumInfo.hiddenCoverDateTime_ = refreshInfo.deltaAddHiddenCover_.hiddenTime_;
            // 更新coverUri
            albumInfo.hiddenCoverUri_ = refreshInfo.deltaAddHiddenCover_.uri_;
            isRefreshHiddenAlbum = true;
        }
        ACCURATE_DEBUG("Add hidden[%{public}d], refresh[%{public}d] album hidden cover id[%{public}d, %{public}d]",
            albumInfo.albumId_, isRefresh, albumInfo.hiddenCoverInfo_.fileId_,
            refreshInfo.deltaAddHiddenCover_.fileId_);
    } else if (!IsValidCover(refreshInfo.deltaAddHiddenCover_) && refreshInfo.removeHiddenFileIds.size() > 0) {
        // 删除场景
        // 当前cover没有removeCover新
        bool isRefresh = refreshInfo.removeHiddenFileIds.find(coverFileId) != refreshInfo.removeHiddenFileIds.end();
        if (coverFileId <= 0 || isRefresh) {
            albumInfo.needForceSelectHiddenCover = true;
            ClearHiddenAlbumCoverInfo(albumInfo);
        }
        ACCURATE_DEBUG("Del hidden[%{public}d], force select cover[%{public}d], coverFileId[%{public}d]",
            albumInfo.albumId_, isRefresh, coverFileId);
    } else if (IsValidCover(refreshInfo.deltaAddHiddenCover_) && refreshInfo.removeHiddenFileIds.size() > 0) {
        // 异常场景：同一个相册中cover既有新增又有删除
        // 全量刷新指定的系统相册
        albumInfo.needForceSelectHiddenCover = true;
        ClearHiddenAlbumCoverInfo(albumInfo);
        ACCURATE_DEBUG("Abnormal hidden[%{public}d], forceSelectCover: %{public}s, add/remove[%{public}s/%{public}zu]",
            albumInfo.albumId_, albumInfo.ToString().c_str(), refreshInfo.deltaAddHiddenCover_.ToString().c_str(),
            refreshInfo.removeHiddenFileIds.size());
    } else {
        CHECK_AND_RETURN_RET(!RefreshByCoverOrderHidden(refreshInfo, albumInfo, coverFileId), true);
    }
    return isRefreshHiddenAlbum;
}

void AlbumRefreshExecution::ClearAlbumInfo(AlbumChangeInfo &albumInfo)
{
    albumInfo.imageCount_ = INVALID_INT32_VALUE;
    albumInfo.videoCount_ = INVALID_INT32_VALUE;
    albumInfo.count_ = INVALID_INT32_VALUE;
    albumInfo.coverUri_ = EMPTY_STR;
    albumInfo.coverDateTime_ = INVALID_INT64_VALUE;
}

void AlbumRefreshExecution::ClearHiddenAlbumInfo(AlbumChangeInfo &albumInfo)
{
    albumInfo.hiddenCount_ = INVALID_INT32_VALUE;
    albumInfo.hiddenCoverUri_ = EMPTY_STR;
    albumInfo.hiddenCoverDateTime_ = INVALID_INT64_VALUE;
}

void AlbumRefreshExecution::ClearAlbumCoverInfo(AlbumChangeInfo &albumInfo)
{
    albumInfo.coverUri_ = EMPTY_STR;
    albumInfo.coverDateTime_ = INVALID_INT64_VALUE;
}

void AlbumRefreshExecution::ClearHiddenAlbumCoverInfo(AlbumChangeInfo &albumInfo)
{
    albumInfo.hiddenCoverUri_ = EMPTY_STR;
    albumInfo.hiddenCoverDateTime_ = INVALID_INT64_VALUE;
}

void AlbumRefreshExecution::CheckNotifyOldNotification(NotifyAlbumType notifyAlbumType,
    const AlbumChangeInfo &albumInfo, OHOS::Media::NotifyType type)
{
    if (notifyAlbumType == NotifyAlbumType::NO_NOTIFY || notifyAlbumType == NotifyAlbumType::ANA_ALBUM) {
        MEDIA_DEBUG_LOG("no need old notification");
        return;
    }
    if (type == OHOS::Media::NotifyType::NOTIFY_INVALID) {
        ACCURATE_ERR("invalid notify type.");
        return;
    }

    if (albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::TRASH) ||
        albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::HIDDEN)) {
        ACCURATE_DEBUG("subtype[%{public}d] no need old notification.", albumInfo.albumId_);
        return;
    }

    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        ACCURATE_ERR("watch nullptr no old notification");
        return;
    }
    bool needSystemNotify = notifyAlbumType & NotifyAlbumType::SYS_ALBUM;
    bool isSystemAlbum = albumInfo.albumType_ == static_cast<int32_t>(PhotoAlbumType::SYSTEM) &&
        (albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::FAVORITE) ||
        albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::VIDEO) ||
        albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::IMAGE) ||
        albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::CLOUD_ENHANCEMENT));

    bool needUserNotify = notifyAlbumType & NotifyAlbumType::USER_ALBUM;
    bool isUserAlbum = albumInfo.albumType_ == static_cast<int32_t>(PhotoAlbumType::USER) &&
        albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);

    bool needSourceNotify = notifyAlbumType & NotifyAlbumType::SOURCE_ALBUM;
    bool isSourceAlbum = albumInfo.albumType_ == static_cast<int32_t>(PhotoAlbumType::SOURCE) &&
        albumInfo.albumSubType_ == static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC);

    CHECK_AND_RETURN_LOG(hasSentOldNotifications_.find(albumInfo.albumId_) == hasSentOldNotifications_.end(),
        "an old notification has been sent: type[%{public}d], albumId[%{public}d], notifyAlbumType[0x%{public}x]",
        type, albumInfo.albumId_, notifyAlbumType);
    bool needNotify = (needSystemNotify && isSystemAlbum) || (needUserNotify && isUserAlbum)
        || (needSourceNotify && isSourceAlbum);
    if (needNotify) {
        hasSentOldNotifications_[albumInfo.albumId_] = true;
        watch->Notify(PhotoColumn::PHOTO_URI_PREFIX, type, albumInfo.albumId_);
        ACCURATE_DEBUG("old notification: type[%{public}d], albumId[%{public}d], notifyAlbumType[0x%{public}x]",
            type, albumInfo.albumId_, notifyAlbumType);
    } else {
        ACCURATE_DEBUG("no notification albumType[%{public}d], subType[%{public}d], notifyAlbumType[0x%{public}x]",
            albumInfo.albumType_, albumInfo.albumSubType_, notifyAlbumType);
    }
}

int32_t AlbumRefreshExecution::RefreshAllAlbum(std::unordered_set<int32_t> albumIds,
    NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, ACCURATE_REFRESH_RDB_NULL, "rdbStore null");
    ACCURATE_DEBUG("force update all albums");
    lock_guard<mutex> lock(albumRefreshMtx_);
    isRefreshWithDateModified_ = isRefreshWithDateModified;
    std::vector<std::string> albumIdList;
    albumIdList.reserve(albumIds.size());
    for (int32_t albumId : albumIds) {
        albumIdList.push_back(std::to_string(albumId));
    }
    MediaLibraryRdbUtils::UpdateSystemAlbumsByUris(rdbStore, AlbumOperationType::DEFAULT, {}, notifyAlbumType);
    MediaLibraryRdbUtils::UpdateCommonAlbumInternal(rdbStore, albumIdList, (notifyAlbumType & USER_ALBUM) ||
        (notifyAlbumType & SOURCE_ALBUM), isRefreshWithDateModified);
    MediaLibraryRdbUtils::UpdateCommonAlbumHiddenState(rdbStore, albumIdList);
    return ACCURATE_REFRESH_RET_OK;
}

bool AlbumRefreshExecution::CheckSetHiddenAlbumInfo(AlbumChangeInfo &albumInfo)
{
    if (albumInfo.albumSubType_ == PhotoAlbumSubType::HIDDEN) {
        albumInfo.hiddenCount_ = albumInfo.count_;
        albumInfo.hiddenCoverInfo_ = albumInfo.coverInfo_;
        albumInfo.hiddenCoverUri_ = albumInfo.coverUri_;
        albumInfo.hiddenCoverDateTime_ = albumInfo.coverDateTime_;
        ACCURATE_DEBUG("hiddenCount_[%{public}d], hiddenCover[%{public}s], dateTime[%{public}" PRId64 "]",
            albumInfo.count_, MediaFileUtils::DesensitizeUri(albumInfo.coverUri_).c_str(), albumInfo.coverDateTime_);
        return true;
    }
    return false;
}

void AlbumRefreshExecution::CheckInitSystemCalculation()
{
    if (!systemAlbumCalculations_.empty()) {
        return;
    }
    lock_guard<mutex> lock(albumRefreshMtx_);
    if (!systemAlbumCalculations_.empty()) {
        return;
    }
    vector<string> subTypes;
    for (auto calculation : systemTypeAlbumCalculations_) {
        auto albumId = MediaLibraryRdbUtils::GetAlbumIdBySubType(calculation.first);
        if (albumId <= 0) {
            MEDIA_ERR_LOG("no subType[%{public}d] calculation.", calculation.first);
            continue;
        }
        systemAlbumCalculations_.insert_or_assign(albumId, calculation.second);
    }
}

} // namespace Media
} // namespace OHOS