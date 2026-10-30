/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "geography_city_strategies.h"

#include "media_column.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
namespace AccurateRefresh {

std::string GeographyCityCoverPickerStrategy::QueryCover(
    const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, const UpdateAlbumData &baseInfo)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, "", "rdbStore is nullptr, geography city cover");

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    MediaLibraryRdbUtils::GetAlbumCountAndCoverPredicates(baseInfo, predicates, false, true);

    std::vector<std::string> columns = {
        PhotoColumn::MEDIA_ID,
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME,
    };

    predicates.SetOrder(MediaColumn::MEDIA_DATE_MODIFIED + " DESC, " + PhotoColumn::MEDIA_NAME + " DESC");
    predicates.Limit(1);

    auto resultSet = rdbStore->QueryByStepWithoutCount(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, "", "Query geography city cover failed");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return "";
    }

    std::string cover = MediaLibraryRdbUtils::GetCover(resultSet);
    resultSet->Close();
    return cover;
}

static const auto GeographyCityStrategyReg =
    AnalysisStrategyRegistry::Register(PhotoAlbumSubType::GEOGRAPHY_CITY)
        .Effective<GeographyCityAlbumEffectiveStrategy>()
        .Count<GeographyCityCountStrategy>()
        .Cover<GeographyCityCoverStrategy>()
        .Picker<GeographyCityCoverPickerStrategy>()
        .UseDefaultPipeline()
        .Build();

} // namespace AccurateRefresh
} // namespace Media
} // namespace OHOS