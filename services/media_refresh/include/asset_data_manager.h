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

#ifndef OHOS_MEDIALIBRARY_ASSET_DATA_MANAGER_H
#define OHOS_MEDIALIBRARY_ASSET_DATA_MANAGER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "accurate_refresh_data_manager.h"
#include "abs_rdb_predicates.h"

#include "accurate_common_data.h"
#include "photo_asset_change_info.h"
#include "multi_thread_asset_change_info_mgr.h"

namespace OHOS {
namespace Media::AccurateRefresh {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT AssetDataManager : public AccurateRefreshDataManager<PhotoAssetChangeInfo, PhotoAssetChangeData> {
public:
    AssetDataManager() : AssetDataManager(nullptr)
    {}
    AssetDataManager(std::shared_ptr<TransactionOperations> trans)
        : AccurateRefreshDataManager<PhotoAssetChangeInfo, PhotoAssetChangeData>(trans)
    {}
    virtual ~AssetDataManager();
    int32_t UpdateModifiedDatas() override;
    int32_t PostProcessModifiedDatas(const std::vector<int32_t> &keys) override;
    std::vector<int32_t> GetInitKeys() override;
    int32_t SetContentChanged(int32_t fileId, bool isChanged);
    int32_t SetThumbnailStatus(int32_t fileId, int32_t status);
    int32_t UpdateNotifyInfo();
    // 更新完对应fileId后清除
    void ClearMultiThreadChangeData(int32_t fileId);
    // 清除所有的fileId
    void ClearMultiThreadChangeDatas();
    bool CheckIsForRecheck() override;
    bool GetIsExceedStatus();

private:
    void SetAlbumIdByChangeInfos(const std::vector<PhotoAssetChangeInfo> &changeInfos) override;
    int32_t UpdateThumbnailChangeStatus(PhotoAssetChangeData &assetChangeData);
    int32_t GetChangeInfoKey(const PhotoAssetChangeInfo &changeInfo) override;
    std::vector<PhotoAssetChangeInfo> GetInfoByKeys(const std::vector<int32_t> &fileIds) override;
    std::vector<PhotoAssetChangeInfo> GetInfosByPredicates(const NativeRdb::AbsRdbPredicates &predicates) override;
    std::vector<PhotoAssetChangeInfo> GetInfosByResult(const std::shared_ptr<NativeRdb::ResultSet> &resultSet) override;
    void PostInsertBeforeData(PhotoAssetChangeData &changeData, PendingInfo &pendingInfo) override;
    void PostInsertAfterData(PhotoAssetChangeData &changeData, PendingInfo &pendingInfo, bool isAdd = false,
        int fileIdBeforeRemove = INVALID_INT32_VALUE) override;
    bool CheckUpdateDataForMultiThread(PhotoAssetChangeData &changeData) override;
    void UpdatePendingInfo(PhotoAssetChangeData &changeData, PendingInfo &pendingInfo);
    int32_t SetAlbumIdsByPredicates(const NativeRdb::AbsRdbPredicates &predicates) override;
    int32_t SetAlbumIdsBySql(const std::string &sql, const std::vector<NativeRdb::ValueObject> &bindArgs) override;
    int32_t SetAlbumIdsByFileds(const std::vector<int32_t> &fileIds) override;
    bool CheckIsExceedInMultiThread(const std::vector<int32_t> &keys) override;

private:
    std::unordered_map<int32_t, PhotoAssetContentInfo> contentInfos;
    // 用于清理MultiThreadAssetChangeInfoMgr中的资产信息
    std::unordered_set<int32_t> multiThreadAssetIds_;
protected:
    bool CheckIsExceed(const NativeRdb::AbsRdbPredicates &predicates, bool isLengthChanged = false) override;
    bool CheckIsExceed(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs, bool isLengthChanged = false) override;
    bool CheckIsExceed(const std::vector<int32_t> &keys) override;
    bool CheckIsExceed(bool isLengthChanged = false) override;
    bool CheckIsExceed(size_t length) override;
    void SetAlbumIdFromChangeDates();
};

}  // namespace Media::AccurateRefresh
}  // namespace OHOS

#endif