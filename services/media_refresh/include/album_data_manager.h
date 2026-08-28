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

#ifndef OHOS_MEDIALIBRARY_ALBUM_DATA_MANAGER_H
#define OHOS_MEDIALIBRARY_ALBUM_DATA_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "accurate_refresh_data_manager.h"
#include "abs_rdb_predicates.h"
#include "result_set.h"

#include "album_change_info.h"

namespace OHOS {
namespace Media::AccurateRefresh {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT AlbumDataManager : public AccurateRefreshDataManager<AlbumChangeInfo, AlbumChangeData> {
public:
    AlbumDataManager() : AlbumDataManager(nullptr) {}
    AlbumDataManager(std::shared_ptr<TransactionOperations> trans)
        : AccurateRefreshDataManager<AlbumChangeInfo, AlbumChangeData>(trans) {}
    virtual ~AlbumDataManager() {}
    // 增删场景下初始化数据
    int32_t InitAlbumInfos(const std::vector<int> &albumIds);
    int32_t UpdateModifiedDatas() override;
    int32_t PostProcessModifiedDatas(const std::vector<int32_t> &keys) override;
    std::unordered_map<int32_t, AlbumChangeInfo> GetInitAlbumInfos();
    std::vector<int32_t> GetInitKeys() override;
    static std::vector<AlbumChangeData> GetAlbumDatasFromAddAlbum(const std::vector<std::string> &albumIdsStr);
    // 共享相册成员表变更场景：按 albumId 查当前状态构造 UPDATE 变更数据（before==after==当前值）
    static std::vector<AlbumChangeData> GetAlbumDatasForUpdateNoChange(const std::vector<int32_t> &albumIds);
    void ClearChangeInfos();
    bool CheckIsForRecheck() override;
    bool CheckIsExceedInMultiThread(const vector<int32_t>& keys) override {return false;};

private:
    int32_t GetChangeInfoKey(const AlbumChangeInfo &changeInfo) override;
    std::vector<AlbumChangeInfo> GetInfoByKeys(const std::vector<int32_t> &albumIds) override;
    std::vector<AlbumChangeInfo> GetInfosByPredicates(const NativeRdb::AbsRdbPredicates &predicates) override;
    std::vector<AlbumChangeInfo> GetInfosByResult(
        const std::shared_ptr<NativeRdb::ResultSet> &resultSet) override;
    int32_t SetAlbumIdsByPredicates(const NativeRdb::AbsRdbPredicates &predicates) override;
    int32_t SetAlbumIdsBySql(const std::string &sql, const std::vector<NativeRdb::ValueObject> &bindArgs) override;
    int32_t SetAlbumIdsByFileds(const std::vector<int32_t> &fileIds) override;

    std::vector<AlbumChangeInfo> GetAlbumInfos(const std::vector<int32_t> &albumIds,
        const std::vector<std::string> systemTypes = {});
    PhotoAssetChangeInfo GetPhotoAssetInfo(int32_t fileId);
protected:
    bool CheckIsExceed(const NativeRdb::AbsRdbPredicates &predicates, bool isLengthChanged = false) override;
    bool CheckIsExceed(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs, bool isLengthChanged = false) override;
    bool CheckIsExceed(const std::vector<int32_t> &keys) override;
    bool CheckIsExceed(bool isLengthChanged = false) override;
    bool CheckIsExceed(size_t length) override;
};

} // namespace Media
} // namespace OHOS

#endif