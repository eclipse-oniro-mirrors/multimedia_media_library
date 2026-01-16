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
 
#ifndef OHOS_ANALYSIS_DATA_VIDEO_DAO_H
#define OHOS_ANALYSIS_DATA_VIDEO_DAO_H
 
#include <string>
 
namespace OHOS::Media::AnalysisData {
class AnalysisDataVideoDao {
private:
    static inline const std::string RESET_VIDEO_TOTAL_BY_FILE_ID = "\
        UPDATE tab_analysis_video_total \
        SET \
            status = 0, \
            label = 0, \
            face = 0 \
        WHERE \
            file_id = ?;";
 
    static inline const std::string CLEAR_VIDEO_FACE_BY_FILE_ID = "\
        DELETE FROM tab_analysis_video_face\
        WHERE \
            file_id = ?;";
 
    static inline const std::string CLEAR_VIDEO_LABEL_BY_FILE_ID = "\
        DELETE FROM tab_analysis_video_label\
        WHERE \
            file_id = ?;";
 
public:
    AnalysisDataVideoDao() = default;
    ~AnalysisDataVideoDao() = default;
 
    static void FixVideoAnalysisDataAfterEdit(const std::string &fileId);
};
}  // namespace OHOS::Media::AnalysisData
#endif  // OHOS_ANALYSIS_DATA_ALBUM_DAO_H