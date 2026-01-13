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

#ifndef OHOS_ANALYSIS_DATA_ALBUM_DAO_H
#define OHOS_ANALYSIS_DATA_ALBUM_DAO_H

#include <string>
#include "set_highlight_user_action_data_dto.h"

namespace OHOS::Media::AnalysisData {
class AnalysisDataAlbumDao {
public:
    AnalysisDataAlbumDao() = default;
    ~AnalysisDataAlbumDao() = default;

    int32_t GetFaceIdByAlbumId(int32_t albumId, std::string& groupTag);
    int32_t SetHighlightUserActionData(const SetHighlightUserActionDataDto& dto);
    int32_t DeleteHighlightAlbums(const std::vector<std::string>& albumIds);
};
} // OHOS::Media::AnalysisData
#endif // OHOS_ANALYSIS_DATA_ALBUM_DAO_H