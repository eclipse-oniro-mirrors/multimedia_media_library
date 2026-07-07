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

#ifndef OHOS_ANALYSIS_DATA_WATERMARK_DAO_H
#define OHOS_ANALYSIS_DATA_WATERMARK_DAO_H

#include <string>

namespace OHOS::Media::AnalysisData {
#define CONST_WATERMARK_STATUS "status"
#define CONST_WATERMARK_TYPE "type"
#define CONST_WATERMARK_VALID_REGION_X "valid_region_x"
#define CONST_WATERMARK_VALID_REGION_Y "valid_region_y"
#define CONST_WATERMARK_VALID_REGION_WIDTH "valid_region_width"
#define CONST_WATERMARK_VALID_REGION_HEIGHT "valid_region_height"
#define CONST_WATERMARK_ALGO_VERSION "algo_version"

class AnalysisDataWatermarkDao {
private:
    static inline const std::string DELETE_WATERMARK_STATUS_BY_FILE_ID = "\
        UPDATE tab_analysis_watermark \
        SET status = 2, valid_region_x = 0, valid_region_y = 0, \
        valid_region_width = 1, valid_region_height = 1 \
        WHERE file_id = ?;";

public:

    AnalysisDataWatermarkDao() = default;
    ~AnalysisDataWatermarkDao() = default;

    static void DeleteAnalysisWatermark(const std::string &fileId);
};

} // namespace OHOS::Media::AnalysisData

#endif // OHOS_ANALYSIS_DATA_WATERMARK_DAO_H