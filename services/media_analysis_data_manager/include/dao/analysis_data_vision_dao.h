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
#ifndef OHOS_ANALYSIS_DATA_VISION_DAO_H
#define OHOS_ANALYSIS_DATA_VISION_DAO_H

#include <string>

namespace OHOS::Media::AnalysisData {
class AnalysisDataVisionDao {
public:
    AnalysisDataVisionDao() = default;
    ~AnalysisDataVisionDao() = default;

    static void DeleteVisionDataAfterEdit(const std::string &fileId, bool needRefresh);
    static int32_t DeleteFromLabelByFileId(const std::string &fileId);
    static int32_t DeleteFromAestheticsScoreByFileId(const std::string &fileId);
    static int32_t DeleteFromOcrByFileId(const std::string &fileId);
    static int32_t DeleteFromSaliencyDetectByFileId(const std::string &fileId);
    static int32_t DeleteFromImageFaceByFileId(const std::string &fileId);
    static int32_t DeleteFromObjectByFileId(const std::string &fileId);
    static int32_t DeleteFromRecommendationByFileId(const std::string &fileId);
    static int32_t DeleteFromSegmentationByFileId(const std::string &fileId);
    static int32_t DeleteFromHeadByFileId(const std::string &fileId);
    static int32_t DeleteFromAffectiveByFileId(const std::string &fileId);
    static int32_t DeleteFromPoseByFileId(const std::string &fileId);
    static int32_t DeleteFromVisionProfileByFileId(const std::string &fileId);
    static int32_t DelAllStatusFromAestheticsScoreByFileId(const std::string &fileId);
    static int32_t ResetGeoByFileId(const std::string &fileId);
};
}  // namespace OHOS::Media::AnalysisData
#endif  // OHOS_ANALYSIS_DATA_VISION_DAO_H