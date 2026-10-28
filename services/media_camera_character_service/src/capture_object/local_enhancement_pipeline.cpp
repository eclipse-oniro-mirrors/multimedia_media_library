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

#define MLOG_TAG "LocalEnhancementPipeline"

#include "camera_path_utils.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "cloud_enhancement_dfx_get_count.h"
#endif
#include "local_enhancement_pipeline.h"
#include "media_edit_utils.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "multistages_capture_dao.h"
#include "multistages_capture_notify.h"
#include "multistages_photo_capture_manager.h"

#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace Notification;
static const std::string MAMUAL_LOCAL_ENHANCEMENT_EXIF = "1";
static const std::string AUTO_LOCAL_ENHANCEMENT_EXIF = "2";
static const std::map<LocalEnhancementType, std::map<std::string, std::string>> ENHANCEMENT_EXIF_VALUE_MAP = {
    { LocalEnhancementType::MANUAL, {{PHOTO_DATA_CLOUD_ENHANCE_MODE, MAMUAL_LOCAL_ENHANCEMENT_EXIF}} },
    { LocalEnhancementType::AUTO, {{PHOTO_DATA_CLOUD_ENHANCE_MODE, AUTO_LOCAL_ENHANCEMENT_EXIF}} },
};

static const std::map<MediaDpsErrorCode, MultistagesCaptureNotifyType> LOCAL_ENHANCEMENT_ERRCODE_TO_NOTIFYTYPE = {
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INVALID_PHOTO_ID,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_INVALID_PHOTO_ID },
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_FAILED,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_FAILED },
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_TIMEOUT,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_TIMEOUT },
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_ABNORMAL,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_ABNORMAL },
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INTERRUPTED,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_INTERRUPTED },
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_MODEL_UNAVAILABLE,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_MODEL_UNAVAILABLE },
    { MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_RESOURCE_REMOVED,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_ERROR_RESOURCE_REMOVED },
};

bool LocalEnhancementPipeline::GetEnhancementExifValue(LocalEnhancementType type,
    std::map<std::string, std::string> &values)
{
    auto iter = ENHANCEMENT_EXIF_VALUE_MAP.find(type);
    if (iter == ENHANCEMENT_EXIF_VALUE_MAP.end()) {
        MEDIA_ERR_LOG("invalid local enhancement type, %{public}d.", static_cast<int32_t>(type));
        return false;
    }
    values = iter->second;
    return true;
}

static MultistagesCaptureNotifyType ErrCode2NotifyType(const MediaDpsErrorCode error)
{
    if (LOCAL_ENHANCEMENT_ERRCODE_TO_NOTIFYTYPE.find(error) == LOCAL_ENHANCEMENT_ERRCODE_TO_NOTIFYTYPE.end()) {
        MEDIA_ERR_LOG("errorCode is not in map, %{public}d.", static_cast<int32_t>(error));
        return MultistagesCaptureNotifyType::UNDEFINED;
    }
    return LOCAL_ENHANCEMENT_ERRCODE_TO_NOTIFYTYPE.at(error);
}

#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
static int32_t GetEnhancementDfxFinishType(LocalEnhancementType enhancementType)
{
    if (enhancementType == LocalEnhancementType::MANUAL) {
        return static_cast<int32_t>(CloudEnhancementTaskFinishType::MANUAL_LOCAL_ENHANCEMENT);
    } else {
        return static_cast<int32_t>(CloudEnhancementTaskFinishType::AUTO_LOCAL_ENHANCEMENT);
    }
}
#endif

// 不可恢复错误码
void LocalEnhancementPipeline::OnEnhancementIrrecoverableErrImage(const CameraAssetInfo& assetInfo,
    const MediaDpsErrorCode error)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    CloudEnhancementGetCount::GetInstance().Report(
        "FailedType", assetInfo.GetPhotoId(), GetEnhancementDfxFinishType(assetInfo.GetLocalEnhancementType()), 0);
#endif

    // 1.清理raw图
    MultiStagesPhotoCaptureManager::GetInstance().RemoveImageEnhancement(assetInfo.GetPhotoId(), false);

    // 2.强转(ability): 1 和 2 -> 0
    MultiStagesCaptureDao::UpdateLocalEnhancementAbility(assetInfo.GetFileId(),
        static_cast<int32_t>(LocalEnhancementAbilityType::NOT_SUPPORT),
        static_cast<int32_t>(LocalEnhancementStatus::IDEL));

    // 3.通知
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo, ErrCode2NotifyType(error));
}

// 可恢复错误码
void LocalEnhancementPipeline::OnEnhancementRecoverableErrImage(const CameraAssetInfo& assetInfo,
    const MediaDpsErrorCode error)
{
    // 1.强转(ability): 2 -> 1
    int32_t localEnhancementAbility = assetInfo.GetLocalEnhancementAbility();
    if (localEnhancementAbility == static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL_AUTO)) {
        MultiStagesCaptureDao::UpdateLocalEnhancementAbility(assetInfo.GetFileId(),
            static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL_AUTO),
            static_cast<int32_t>(LocalEnhancementStatus::PROCESSING_AUTO));
        assetInfo_.SetLocalEnhancementType(LocalEnhancementType::AUTO);
    } else if (localEnhancementAbility == static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL)) {
        MultiStagesCaptureDao::UpdateLocalEnhancementAbility(assetInfo.GetFileId(),
            static_cast<int32_t>(LocalEnhancementAbilityType::SUPPORT_MANUAL),
            static_cast<int32_t>(LocalEnhancementStatus::IDEL));
        MultiStagesPhotoCaptureManager::GetInstance().CancelProcessImageEnhancement(assetInfo.GetPhotoId());
    }

    // 2.通知
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo, ErrCode2NotifyType(error));
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    CloudEnhancementGetCount::GetInstance().RemoveStartTime(assetInfo.GetPhotoId());
#endif
}

// 三阶段失败分发
bool LocalEnhancementPipeline::OnEnhancementImageError(const MediaDpsErrorCode error)
{
    MEDIA_INFO_LOG("OnEnhancement error: %{public}d, fileId: %{public}d, photoid: %{public}s",
        static_cast<int32_t>(error), assetInfo_.GetFileId(), assetInfo_.GetPhotoId().c_str());
    switch (error) {
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INVALID_PHOTO_ID:
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_FAILED:
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_RESOURCE_REMOVED:
            OnEnhancementIrrecoverableErrImage(assetInfo_, error);
            break;
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_TIMEOUT:
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_ABNORMAL:
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INTERRUPTED:
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_MODEL_UNAVAILABLE:
            OnEnhancementRecoverableErrImage(assetInfo_, error);
            break;
        default:
            MEDIA_ERR_LOG("error: %{public}d is invalid.", static_cast<int32_t>(error));
            return false;
    }
    return true;
}

void LocalEnhancementPipeline::SetLocalEnhancementType(const LocalEnhancementType& localEnhancementType)
{
    assetInfo_.SetLocalEnhancementType(localEnhancementType);
}

int32_t LocalEnhancementPipeline::OnProcessImageEnhancementDone(const OnProcessImageWrapper &wrapper)
{
    CHECK_AND_RETURN_RET_LOG(IsValid(), E_ERR, "pipeline is invalid.");
    MediaLibraryTracer tracer;
    tracer.Start("LocalEnhancementPipeline::OnProcessImageEnhancementDone");
    MEDIA_INFO_LOG("OnProcessImageEnhancementDone: %{public}s.", assetInfo_.ToString().c_str());

    int32_t fileId = assetInfo_.GetFileId();
    std::string photoId = assetInfo_.GetPhotoId();

    // 1.各业务侧的入参校验, 初始化
    CHECK_AND_RETURN_RET_LOG(InitForOnProcessEnhancementInternal(wrapper), E_ERR, "failed to InitForOnProcess.");

    // 2.查询数据, 并确定三阶段的中间业务影响
    auto fileAsset = MultiStagesCaptureDao::QueryForOnProcess(fileId, photoId, metadata_);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr.");

    // 3.三阶段落盘(不允许失败)
    ProcessEnhancementPhoto(fileAsset);

    // 4.更新端侧增强信息
    MultiStagesCaptureDao::UpdateLocalEnhancementInfo(fileId, static_cast<int32_t>(LocalEnhancementStatus::SUCCESS),
        (assetInfo_.GetLocalEnhancementType() == LocalEnhancementType::MANUAL),
        (fileAsset->GetPhotoEditTime() > 0));

    // 5.同步扫描, 生成缩略图
    AsyncScanFileForCamera(ScanQuality::DEFAULT);

    // 6.通知
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo_,
        MultistagesCaptureNotifyType::ON_ENHANCEMENT_IMAGE_DONE);

    // 7.打点
    LocalEnhancementType enhancementType = assetInfo_.GetLocalEnhancementType();
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    if (enhancementType == LocalEnhancementType::AUTO || enhancementType == LocalEnhancementType::MANUAL) {
        CloudEnhancementGetCount::GetInstance().Report(
            "SuccessType", photoId, GetEnhancementDfxFinishType(enhancementType), 0);
    }
#endif

    // 8.落盘后: 删除raw图
    MultiStagesPhotoCaptureManager::GetInstance().RemoveImageEnhancement(photoId, false);

    MEDIA_INFO_LOG("LocalEnhancementPipeline::OnProcessImageEnhancementDone success: %{public}s.",
        assetInfo_.ToString().c_str());
    return E_OK;
}

void LocalEnhancementPipeline::ProcessEnhancementPhoto(const std::shared_ptr<FileAsset> &fileAsset)
{
    int32_t ret = -1;
    auto assetInfo = GetAssetInfo();
    switch (assetInfo.GetLocalEnhancementType()) {
        case LocalEnhancementType::AUTO: {
            ret = ProcessEnhancementPhotoInternal(fileAsset, false);
            break;
        }
        case LocalEnhancementType::MANUAL: {
            CHECK_AND_RETURN_LOG(MediaEditUtils::CheckAndCreateEditDataDir(assetInfo.GetPath()), "create dir failed");
            ret = ProcessEnhancementPhotoInternal(fileAsset, true);
            break;
        }
        default:
            MEDIA_ERR_LOG("invalid localEnhancementType: %{public}d.",
                static_cast<int32_t>(assetInfo.GetLocalEnhancementType()));
            break;
    }

    if (ret != E_OK) {
        MEDIA_ERR_LOG("save local enhancement image failed. ret: %{public}d.", ret);
    }
}

} // namespace Media
} // namespace OHOS