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
 
#include "analysis_data_video_dao.h"
 
#include "medialibrary_unistore_manager.h"
 
namespace OHOS::Media::AnalysisData {
void AnalysisDataVideoDao::FixVideoAnalysisDataAfterEdit(const std::string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Fix video analysis data failed, rdbStore is null.");
        return;
    }
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};
    std::string totalSql = RESET_VIDEO_TOTAL_BY_FILE_ID;
    auto totalErrCode = rdbStore->ExecuteSql(totalSql, bindArgs);
    if (totalErrCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to reset tab_analysis_video_total data for fileId: %{public}s", fileId.c_str());
    }
    std::string faceSql = CLEAR_VIDEO_FACE_BY_FILE_ID;
    auto faceErrCode = rdbStore->ExecuteSql(faceSql, bindArgs);
    if (faceErrCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to clear tab_analysis_video_face data for fileId: %{public}s", fileId.c_str());
    }
    std::string labelSql = CLEAR_VIDEO_LABEL_BY_FILE_ID;
    auto labelErrCode = rdbStore->ExecuteSql(labelSql, bindArgs);
    if (labelErrCode != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to clear tab_analysis_video_label data for fileId: %{public}s", fileId.c_str());
    }
}
}  // namespace OHOS::Media::AnalysisData