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

#define MLOG_TAG "MediaCameraCharacterService"

#include "media_camera_character_service.h"

#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "cloud_enhancement_dfx_get_count.h"
#endif
#include "local_enhancement_manager.h"
#include "media_column.h"
#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "multistages_camera_capture_manager.h"
#include "multistages_capture_dao.h"
#include "multistages_photo_capture_manager.h"
#include "multistages_video_capture_manager.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

using namespace std;

namespace OHOS::Media {

constexpr int32_t ENHANCEMENT_PROGRESS_COMPLETE = 100;

MediaCameraCharacterService &MediaCameraCharacterService::GetInstance()
{
    static MediaCameraCharacterService service;
    return service;
}

int32_t MediaCameraCharacterService::AddProcessVideo(const AddProcessVideoDto &dto)
{
    MultiStagesVideoCaptureManager::GetInstance().AddVideo(dto);
    return E_OK;
}

static bool DoCancelRequestForLocalEnhancemnet(const CancelRequestDto &dto)
{
    CameraPipelineType type = CameraPipelineType::UNDEFINED;
    auto pipeline = LocalEnhancementManager::GetInstance().GetLocalEnhancementPipeline(dto.photoId, type);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("pipeline is nullptr, photoId: %{public}s, pipelineType: %{public}d.",
            dto.photoId.c_str(), static_cast<int32_t>(type));
        return false;
    }

    MultiStagesPhotoCaptureManager::GetInstance().CancelProcessImageEnhancement(dto.photoId);
    auto assetInfo = pipeline->GetAssetInfo();
    int32_t localEnhancementAbility = assetInfo.GetLocalEnhancementAbility();
    if (localEnhancementAbility == static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL_AUTO)) {
        pipeline->SetLocalEnhancementAbility(static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL));
    }
    MultiStagesCaptureDao::UpdateLocalEnhancementAbility(
        assetInfo.GetFileId(), static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL),
        static_cast<int32_t>(LocalEnhancementStatus::IDEL));
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    CloudEnhancementGetCount::GetInstance().RemoveStartTime(dto.photoId);
#endif
    return true;
}

int32_t MediaCameraCharacterService::CancelRequest(const CancelRequestDto &dto)
{
    if (DoCancelRequestForLocalEnhancemnet(dto)) {
        return E_OK;
    }
    switch (dto.mediaType) {
        case static_cast<int32_t>(MEDIA_TYPE_IMAGE): {
            MultiStagesPhotoCaptureManager::GetInstance().CancelProcessRequest(dto.photoId);
            break;
        }
        case static_cast<int32_t>(MEDIA_TYPE_VIDEO): {
            MultiStagesVideoCaptureManager::GetInstance().CancelProcessRequest(dto.photoId);
            break;
        }
        default: {
            MEDIA_ERR_LOG("unsupported media_type");
            return E_ERR;
        }
    }
    return E_OK;
}

int32_t MediaCameraCharacterService::ProcessVideo(const ProcessVideoDto &dto)
{
    MultiStagesVideoCaptureManager::GetInstance().ProcessVideo(dto);
    return E_OK;
}

int32_t MediaCameraCharacterService::GetProgressCallback(GetProgressCallbackRespBody &respbody)
{
    MultiStagesVideoCaptureManager::GetInstance().GetProgressCallback(respbody);
    return E_OK;
}

int32_t MediaCameraCharacterService::CreateCameraFileFd(const CreateCameraFileFdDto &dto,
    CreateCameraFileFdRespBody &respbody)
{
    CameraPipelineType type;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(dto.fileId, type);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("invalid type: %{public}d.", static_cast<int32_t>(type));
        return E_ERR;
    }

    int32_t fd = pipeline->CreateCameraFileFd(dto);
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_ERR, "invalid fd: %{public}d.", fd);
    respbody.fd = fd;
    MEDIA_DEBUG_LOG("respbody: %{public}s.", respbody.ToString().c_str());
    return E_OK;
}

int32_t MediaCameraCharacterService::ScanCameraFile(const ScanCameraFileDto &dto)
{
    CameraPipelineType type;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileId(dto.fileId, type);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("invalid type: %{public}d.", static_cast<int32_t>(type));
        return E_ERR;
    }

    return pipeline->CloseCameraFileFd(dto);
}

int32_t MediaCameraCharacterService::GetDeferredPictureInfo(
    const GetDeferredPictureInfoDto& dto, GetDeferredPictureInfoRespBody& respbody)
{
    CameraPipelineType type = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(dto.photoId, type);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("failed, photoId: %{public}s.", dto.photoId.c_str());
        return E_ERR;
    }
    pipeline->GetDeferredPictureInfo(respbody);
    return E_OK;
}

int32_t MediaCameraCharacterService::QueryLocalEnhancementStatus(
    const QueryLocalEnhancementStatusReqBody &reqBody, QueryLocalEnhancementStatusRespBody &respBody)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is null");

    auto fileAsset = MultiStagesCaptureDao::QueryForLocalEnhancementStatus(reqBody.fileId);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_CAMERA_ASSET_NOT_EXIST, "fileAsset is nullptr");

    int32_t localEnhancementAbility = fileAsset->GetLocalEnhancementAbility();
    int32_t localEnhancementStatus = fileAsset->GetLocalEnhancementStatus();
    if (localEnhancementAbility ==  static_cast<int32_t>(LocalEnhancementAbilityType::NOT_SUPPORT) ||
        localEnhancementStatus == static_cast<int32_t>(LocalEnhancementStatus::EDIT)) {
        MEDIA_ERR_LOG("Asset not support local enhancement.");
        return E_NOT_SUPPORT_LOCAL_ENHANCEMENT;
    }

    respBody.localEnhancementAbility = localEnhancementAbility;
    respBody.localEnhancementStatus = localEnhancementStatus;
    respBody.photoId = fileAsset->GetPhotoId();

    MEDIA_INFO_LOG("QueryLocalEnhancementStatus: %{public}s.", respBody.ToString().c_str());
    return E_OK;
}

int32_t MediaCameraCharacterService::ProcessImageEnhancement(const ProcessImageEnhancementReqBody &reqBody)
{
    MultiStagesPhotoCaptureManager::GetInstance().ProcessImageEnhancement(reqBody.fileId);
    MultiStagesCaptureDao::UpdateLocalEnhancementInfo(reqBody.fileId,
        static_cast<int32_t>(LocalEnhancementStatus::PROCESSING_MANUAL), false, false);
    return E_OK;
}

int32_t MediaCameraCharacterService::QueryLocalEnhancementTaskState(
    const QueryLocalEnhancementTaskStateReqBody &reqBody, QueryLocalEnhancementTaskStateRespBody &respBody)
{
    int32_t fileId = MediaUriUtils::GetFileId(reqBody.photoUri);
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_HAS_DB_ERROR, "Invalid uri: %{public}s", reqBody.photoUri.c_str());

    MEDIA_INFO_LOG("QueryLocalEnhancementTaskState: fileId=%{public}d", fileId);
    auto fileAsset = MultiStagesCaptureDao::QueryForLocalEnhancementStatus(fileId);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_CAMERA_ASSET_NOT_EXIST, "fileAsset is nullptr");
    
    int32_t localEnhancementAbility = fileAsset->GetLocalEnhancementAbility();
    int32_t localEnhancementStatus = fileAsset->GetLocalEnhancementStatus();

    if (localEnhancementAbility ==  static_cast<int32_t>(LocalEnhancementAbilityType::NOT_SUPPORT)) {
        MEDIA_ERR_LOG("Asset not support local enhancement.");
        return E_NOT_SUPPORT_LOCAL_ENHANCEMENT;
    }

    if (localEnhancementStatus == static_cast<int32_t>(LocalEnhancementStatus::IDEL) ||
        localEnhancementStatus == static_cast<int32_t>(LocalEnhancementStatus::EDIT)) {
        MEDIA_INFO_LOG("Asset is not in process local enhancement, %{public}d.", localEnhancementStatus);
        respBody.progress = 0;
        return E_OK;
    } else if (localEnhancementStatus == static_cast<int32_t>(LocalEnhancementStatus::SUCCESS)) {
        MEDIA_INFO_LOG("Asset localEnhancement success.");
        respBody.progress = ENHANCEMENT_PROGRESS_COMPLETE;
        return E_OK;
    }

    MEDIA_INFO_LOG("Asset is in process local enhancement.");
    int32_t progress = 0;
    bool found = LocalEnhancementManager::GetInstance().GetEnhancementProgress(fileId, progress);
    if (!found) {
        MEDIA_INFO_LOG("Progress not found for fileId=%{public}d, no active enhancement task", fileId);
        respBody.progress = -1;  // 表示没有正在处理的增强任务
        return E_OK;
    }

    respBody.progress = progress;
    MEDIA_INFO_LOG("QueryLocalEnhancementTaskState: progress=%{public}d", progress);
    return E_OK;
}
} // namespace OHOS::Media