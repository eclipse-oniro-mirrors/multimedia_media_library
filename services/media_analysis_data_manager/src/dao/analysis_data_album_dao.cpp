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

#define MLOG_TAG "Analysis_Data_Dao"

#include "analysis_data_album_dao.h"

#include "vision_column.h"
#include "media_log.h"
#include "photo_album_column.h"
#include "vision_face_tag_column.h"
#include "result_set_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_errno.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::AnalysisData {
int32_t AnalysisDataAlbumDao::GetFaceIdByAlbumId(int32_t albumId, string& groupTag)
{
    NativeRdb::RdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(albumId));
    vector<string> columns = { GROUP_TAG };
    auto resultSet = MediaLibraryRdbStore::StepQueryWithoutCheck(predicates, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, E_HAS_DB_ERROR, "Failed to query group tag!");
    groupTag = GetStringVal(GROUP_TAG, resultSet);
    return E_OK;
}
} // namespace OHOS::Media::AnalysisData