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
#include "cover_picker_strategy.h"

#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "medialibrary_restore.h"
#include "medialibrary_tracer.h"
#include "rdb_predicates.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

/**
 * 通用封面挑选逻辑
 */
bool CoverPickerStrategyBase::PickCover(const UpdateAlbumData &baseInfo,
    AnalysisAlbumRefreshInfo &info)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "no rdbStore");

    info.refreshCover_ = QueryCover(rdbStore, baseInfo);

    MEDIA_INFO_LOG("CoverPickerStrategyBase: album:%{public}d, albumSubType:%{public}d, newCover:%{public}s",
        baseInfo.albumId, baseInfo.albumSubtype, MediaFileUtils::GetUriWithoutDisplayname(info.refreshCover_).c_str());

    return true;
}

static shared_ptr<NativeRdb::ResultSet> QueryGoToFirst(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const NativeRdb::RdbPredicates &predicates, const vector<string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryGoToFirst");
    auto resultSet = rdbStore->QueryByStepWithoutCount(predicates, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, nullptr);

    MediaLibraryTracer goToFirst;
    goToFirst.Start("GoToFirstRow");
    int32_t err = resultSet->GoToFirstRow();
    MediaLibraryRestore::GetInstance().CheckRestore(err);
    return resultSet;
}

std::string CoverPickerStrategyBase::QueryCover(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, const UpdateAlbumData &baseInfo)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    MediaLibraryRdbUtils::GetAlbumCountAndCoverPredicates(baseInfo, predicates, false, true);

    std::vector<std::string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME,
    };

    MediaLibraryRdbUtils::DetermineQueryOrder(predicates, baseInfo, false, columns);
    predicates.Limit(1);

    auto resultSet = QueryGoToFirst(rdbStore, predicates, columns);
    if (!resultSet) {
        return ""; // 无封面
    }

    return MediaLibraryRdbUtils::GetCover(resultSet); // 旧逻辑维持不变
}

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS
