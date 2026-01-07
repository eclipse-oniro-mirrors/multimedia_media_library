/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 
#ifndef MEDIA_ANALYSIS_DATA_SERVICE_H
#define MEDIA_ANALYSIS_DATA_SERVICE_H
 
#include <stdint.h>
#include <string>
 
#include "get_asset_analysis_data_dto.h"
#include "start_asset_analysis_dto.h"
#include "start_asset_analysis_vo.h"
#include "change_request_set_order_position_dto.h"
#include "get_order_position_dto.h"
#include "get_order_position_vo.h"
#include "get_relationship_vo.h"
#include "get_analysis_process_vo.h"
#include "analysis_data_album_dao.h"
#include "query_result_vo.h"
 
namespace OHOS::Media::AnalysisData {
class MediaAnalysisDataService {
public:
    static MediaAnalysisDataService &GetInstance();
    MediaAnalysisDataService(const MediaAnalysisDataService&) = delete;
    MediaAnalysisDataService& operator=(const MediaAnalysisDataService&) = delete;
 
    int32_t GetAssetAnalysisData(GetAssetAnalysisDataDto &dto);
    int32_t GetIndexConstructProgress(std::string &indexProgress);
    int32_t StartAssetAnalysis(const StartAssetAnalysisDto &dto, StartAssetAnalysisRespBody &respBody);
    int32_t SetOrderPosition(ChangeRequestSetOrderPositionDto &setOrderPositionDto);
    int32_t GetOrderPosition(const GetOrderPositionDto& getOrderPositionDto, GetOrderPositionRespBody& resp);
    int32_t SetPortraitRelationship(const int32_t albumId, const std::string& relationship, const int32_t isMe);
    int32_t GetPortraitRelationship(const int32_t albumId, GetRelationshipRespBody& respBody);
    int32_t GetAnalysisProcess(GetAnalysisProcessReqBody &reqBody, QueryResultRespBody &respBody);
    int32_t GetFaceId(int32_t albumId, std::string& groupTag);

private:
    MediaAnalysisDataService() = default;
    AnalysisDataAlbumDao albumDao_;
};
} // namespace OHOS::Media::AnalysisData
#endif  // MEDIA_ANALYSIS_DATA_SERVICE_H