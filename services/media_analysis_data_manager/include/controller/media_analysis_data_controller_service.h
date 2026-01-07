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
 
#ifndef MEDIA_ANALYSIS_DATA_CONTROLLER_SERVICE_H
#define MEDIA_ANALYSIS_DATA_CONTROLLER_SERVICE_H
 
#include <string>
#include <map>
 
#include "message_parcel.h"
#include "user_define_ipc.h"
#include "i_media_controller_service.h"
#include "medialibrary_business_code.h"
 
namespace OHOS::Media::AnalysisData {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT MediaAnalysisDataControllerService : public IPC::IMediaControllerService {
public:
    virtual ~MediaAnalysisDataControllerService() = default;
    bool Accept(uint32_t code) override;
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override;
 
    EXPORT int32_t GetAssetAnalysisData(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetIndexConstructProgress(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StartAssetAnalysis(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetOrderPosition(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetOrderPosition(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetRelationship(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetRelationship(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAnalysisProcess(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetFaceId(MessageParcel &data, MessageParcel &reply);

private:
    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override;
    using RequestHandle = int32_t (MediaAnalysisDataControllerService::*)(MessageParcel &, MessageParcel &);
    const std::map<uint32_t, RequestHandle> HANDLERS = {
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA),
            &MediaAnalysisDataControllerService::GetAssetAnalysisData
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS),
            &MediaAnalysisDataControllerService::GetIndexConstructProgress
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_START_ASSET_ANALYSIS),
            &MediaAnalysisDataControllerService::StartAssetAnalysis
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION),
            &MediaAnalysisDataControllerService::SetOrderPosition
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ORDER_POSITION),
            &MediaAnalysisDataControllerService::GetOrderPosition
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_RELATIONSHIP),
            &MediaAnalysisDataControllerService::SetRelationship
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_RELATIONSHIP),
            &MediaAnalysisDataControllerService::GetRelationship
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS),
            &MediaAnalysisDataControllerService::GetAnalysisProcess
        },
        {
            static_cast<uint32_t>(MediaLibraryBusinessCode::GET_FACE_ID),
            &MediaAnalysisDataControllerService::GetFaceId
        },
    };
};
}  // namespace OHOS::Media::AnalysisData
#endif  // MEDIA_ANALYSIS_DATA_CONTROLLER_SERVICE_H