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

#ifndef OHOS_MEDIALIBRARY_ALBUM_ACCURATE_REFRESH_H
#define OHOS_MEDIALIBRARY_ALBUM_ACCURATE_REFRESH_H

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "abs_rdb_predicates.h"

#include "accurate_refresh_base.h"
#include "album_change_info.h"
#include "album_data_manager.h"
#include "album_change_notify_execution.h"

namespace OHOS {
namespace Media::AccurateRefresh {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT AlbumAccurateRefresh : public AccurateRefreshBase {
public:
    AlbumAccurateRefresh() : AccurateRefreshBase() {};
    AlbumAccurateRefresh(std::shared_ptr<TransactionOperations> trans);
    AlbumAccurateRefresh(const std::string &targetBusiness) : AccurateRefreshBase(targetBusiness, nullptr) {}
    AlbumAccurateRefresh(const std::string &targetBusiness, std::shared_ptr<TransactionOperations> trans);
    virtual ~AlbumAccurateRefresh() {}
    // init的查询语句
    int32_t Init() override;
    int32_t Init(const NativeRdb::AbsRdbPredicates &predicates) override;
    int32_t Init(const std::string &sql, const std::vector<NativeRdb::ValueObject> bindArgs) override;
    int32_t Init(const std::vector<int32_t> &albumIds) override;
    
    // 更新modified数据信息；数据库操作只是缓存数据，需要执行这个函数触发对比修改前后的数据
    int32_t UpdateModifiedDatas();

    // notify album change infos based on init datas and modified datas.
    int32_t Notify(std::shared_ptr<DfxRefreshManager> dfxRefreshManager = nullptr);

    // 根据传递的assetChangeDatas进行通知，不需要dataManager_处理
    int32_t Notify(std::vector<AlbumChangeData> albumChangeDatas,
        std::shared_ptr<DfxRefreshManager> dfxRefreshManager = nullptr);

    int32_t NotifyAddAlbums(const std::vector<std::string> &albumIdsStr);

    std::unordered_map<int32_t, AlbumChangeInfo> GetInitAlbumInfos();
    
    using AccurateRefreshBase::LogicalDeleteReplaceByUpdate;
    int32_t LogicalDeleteReplaceByUpdate(MediaLibraryCommand &cmd, int32_t &deletedRows) override;
    int32_t LogicalDeleteReplaceByUpdate(const NativeRdb::AbsRdbPredicates &predicates, int32_t &deletedRows) override;
    static int32_t NotifyForReCheck();
    bool IsCoverContentChange(const string &fileId);
    void NotifyAlbumsCoverChange(const string &fileId, vector<int32_t> &albumIds);
    vector<AlbumChangeData> GetAlbumChangeDatas();
    void ClearChangeInfos();

protected:
    int32_t AddAlbumIdForMoveOperation(const NativeRdb::AbsRdbPredicates &predicates) override;
    int32_t UpdateModifiedDatasInner(const std::vector<int> &albumIds, RdbOperation operation,
        PendingInfo pendingInfo = PendingInfo()) override;
    std::string GetReturningKeyName() override;
    bool IsValidTable(std::string tableName) override;

private:
    int32_t DeleteCommon(std::function<int32_t(NativeRdb::ValuesBucket &)> updateExe);

private:
    AlbumDataManager dataManager_;
    AlbumChangeNotifyExecution notifyExe_;
};

} // namespace Media
} // namespace OHOS

#endif