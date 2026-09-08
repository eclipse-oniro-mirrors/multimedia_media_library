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

#ifndef OHOS_MEDIALIBRARY_ALBUM_REFRESH_EXECUTION_H
#define OHOS_MEDIALIBRARY_ALBUM_REFRESH_EXECUTION_H

#include <functional>
#include <unordered_map>
#include <vector>
#include <set>
#include <sstream>

#include "medialibrary_rdb_utils.h"
#include "photo_asset_change_info.h"
#include "album_change_info.h"
#include "userfile_manager_types.h"
#include "album_accurate_refresh.h"
#include "system_album_info_calculation.h"
#include "dfx_refresh_manager.h"

namespace OHOS {
namespace Media::AccurateRefresh {

class AlbumRefreshExecution {
public:
    int32_t RefreshAlbum(const std::vector<PhotoAssetChangeData> &assetChangeDatas,
        NotifyAlbumType notifyAlbumType = NO_NOTIFY, bool isRefreshWithDateModified = true);
    int32_t Notify();
    int32_t RefreshAllAlbum(std::unordered_set<int32_t> albumIds,
        NotifyAlbumType notifyAlbumType, bool isRefreshWithDateModified = true);
    void SetDfxRefreshManager(std::shared_ptr<DfxRefreshManager> dfxRefreshManager)
    {
        dfxRefreshManager_ = dfxRefreshManager;
    }

private:
    std::vector<int32_t> GetAlbumIds();
    // 计算相册增量信息
    int32_t CalRefreshInfos(const std::vector<PhotoAssetChangeData> &assetChangeDatas);

    // 计算修改后相册信息
    int32_t CalAlbumsInfos();
    // 计算相册普通和隐藏信息
    bool CalAlbumInfos(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo, int32_t subType);
    // 计算相册信息
    bool CalAlbumInfo(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo, int32_t subType);
    // 计算相册隐藏信息
    bool CalHiddenAlbumInfo(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo, int32_t subType);
    // 计算相册count
    bool CalAlbumCount(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo);
    // 计算相册hidden count
    bool CalAlbumHiddenCount(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo);
    // 计算相册的封面
    bool CalAlbumCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo, int32_t subType);
    // 计算相册的隐藏封面
    bool CalAlbumHiddenCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo);
    // 相册封面手动设置场景下计算封面处理
    bool CalCoverSetCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo);
    // 计算共享相册封面（share_group DESC + date_taken DESC）
    bool CalShareAlbumCover(AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo);

    // 更新所有相册
    int32_t UpdateAllAlbums(NotifyAlbumType notifyAlbumType);
    // 相册增量更新，从refresh info中增量更新
    int32_t AccurateUpdateAlbums(NotifyAlbumType notifyAlbumType);
    // 相册强制更新，从Photos表查询获取
    int32_t ForceUpdateAlbums(int32_t albumId, bool isHidden, NotifyAlbumType notifyAlbumType);
    // 从Photos表中获取相册信息
    int32_t GetUpdateValues(NativeRdb::ValuesBucket &values, const AlbumChangeInfo &albumInfo, bool isHidden,
        OHOS::Media::NotifyType &type);
    void GetUpdateValuesAndNotifyType(const AccurateRefresh::AlbumChangeInfo &initAlbumInfo,
        AccurateRefresh::AlbumChangeInfo &albumInfo, NativeRdb::ValuesBucket &values, NotifyType &type);
    // 获取选择封面后的ValueBuckets
    int32_t SetForceSelectCoverValues(NativeRdb::ValuesBucket &values, const AlbumChangeInfo &albumInfo,
        bool isHidden);

    bool IsValidCover(const PhotoAssetChangeInfo &assetInfo);

    // 清空albumInfo中普通信息，执行后增量刷新不刷新普通信息字段
    void ClearAlbumInfo(AlbumChangeInfo &albumInfo);
    // 清空albumInfo中隐藏信息，执行后增量刷新不刷新隐藏信息字段
    void ClearHiddenAlbumInfo(AlbumChangeInfo &albumInfo);
    // 清空albumInfo中普通封面信息，执行后增量刷新不刷新普通封面信息字段
    void ClearAlbumCoverInfo(AlbumChangeInfo &albumInfo);
    // 清空albumInfo中隐藏封面信息，执行后增量刷新不刷新隐藏封面信息字段
    void ClearHiddenAlbumCoverInfo(AlbumChangeInfo &albumInfo);

    // 老通知发送
    void CheckNotifyOldNotification(NotifyAlbumType notifyAlbumType, const AlbumChangeInfo &albumInfo,
        OHOS::Media::NotifyType type);

    // 测试增量数据和强制数据是否一致
    void CheckHiddenAlbumInfo(NativeRdb::ValuesBucket &values, std::stringstream &ss);
    void CheckAlbumInfo(NativeRdb::ValuesBucket &values, std::stringstream &ss);
    void CheckUpdateAlbumInfo(const AlbumChangeInfo &albumInfo, bool isHidden);
    void CheckUpdateValues(const AlbumChangeInfo &albumInfo, const AlbumRefreshInfo &refreshInfo,
        NativeRdb::ValuesBucket &values);
    bool CheckSetHiddenAlbumInfo(AlbumChangeInfo &albumInfo);
    void CheckInitSystemCalculation();
    void CheckNotifyAlbum();

private:
    // 系统相册
    static std::unordered_map<PhotoAlbumSubType, SystemAlbumInfoCalculation> systemTypeAlbumCalculations_;
    static std::unordered_map<int32_t, SystemAlbumInfoCalculation> systemAlbumCalculations_;
    std::unordered_map<int32_t, AlbumRefreshInfo> systemAlbumRefreshInfos_;

    // 用户相册和来源相册
    std::unordered_map<int32_t, AlbumRefreshInfo> ownerAlbumRefreshInfos_;

    // 共享相册（SHARE_SUBTYPE, subType=8193）的增量刷新信息
    std::unordered_map<int32_t, AlbumRefreshInfo> shareAlbumRefreshInfos_;

    // 所有相册refreshInfo
    std::unordered_map<int32_t, AlbumRefreshInfo> albumRefreshInfos_;
    // 修改前相册信息
    std::unordered_map<int32_t, AlbumChangeInfo> initAlbumInfos_;

    AlbumAccurateRefresh albumRefresh_;

    std::shared_ptr<DfxRefreshManager> dfxRefreshManager_;

    // 需要刷新的相册信息
    std::unordered_map<int32_t, std::pair<AlbumRefreshInfo, AlbumChangeInfo>> refreshAlbums_;

    // 需要强制刷新的相册信息
    std::set<int32_t> forceRefreshAlbums_;
    std::set<int32_t> forceRefreshHiddenAlbums_;

    // 记录已发送旧通知的相册信息
    std::unordered_map<int32_t, bool> hasSentOldNotifications_;

    static std::mutex albumRefreshMtx_;

    bool isRefreshWithDateModified_ = true;

    size_t affectedAlbumCount_ = 0;
};

} // namespace Media
} // namespace OHOS

#endif