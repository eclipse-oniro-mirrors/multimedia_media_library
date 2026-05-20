/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CameraAssetPipeline"

#include "camera_asset_pipeline.h"

#include "camera_path_utils.h"
#include "high_quality_scan_file_callback.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_privacy_manager.h"
#include "media_values_bucket_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_tracer.h"
#include "multistages_capture_dfx_capture_fault.h"
#include "multistages_capture_dfx_capture_times.h"
#include "multistages_capture_dfx_result.h"
#include "multistages_capture_dfx_save_camera_photo.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_notify.h"
#include "multistages_capture_request_task_manager.h"
#include "multistages_moving_photo_capture_manager.h"
#include "multistages_photo_capture_manager.h"
#include "refresh_business_name.h"
#include "rdb_predicates.h"
#include "thumbnail_const.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace NativeRdb;
using namespace OHOS::Media::Notification;

const std::string HIGH_TEMPERATURE = "high_temperature";
constexpr int32_t PHONE_8G_CPU_NUMS = 8;

bool CameraAssetPipeline::IsValid()
{
    return assetInfo_.GetFileId() > 0;
}

const CameraAssetInfo CameraAssetPipeline::GetAssetInfo() const
{
    return assetInfo_;
}

CameraPipelineType CameraAssetPipeline::GetPipelineType() const
{
    return pipelineType_;
}

void CameraAssetPipeline::SetPipelineType(const CameraPipelineType& pipelineType)
{
    pipelineType_ = pipelineType;
}

void CameraAssetPipeline::SetActiveType(const CameraInfoActiveType& activeType)
{
    assetInfo_.SetActiveType(activeType);
}

MediaDpsMetadata CameraAssetPipeline::GetMediaDpsMetadata() const
{
    return metadata_;
}

void CameraAssetPipeline::SetMediaDpsMetadata(const MediaDpsMetadata& metadata)
{
    metadata_ = metadata;
}

void CameraAssetPipeline::SetTakeEffectStatus(const TakeEffectStatus& takeEffectStatus)
{
    assetInfo_.SetTakeEffectStatus(takeEffectStatus);
}

void CameraAssetPipeline::SetEffectiveFileSaved(bool effectiveFileSaved)
{
    assetInfo_.SetEffectiveFileSaved(effectiveFileSaved);
}

void CameraAssetPipeline::SetSourceFileSaved(bool sourceFileSaved)
{
    assetInfo_.SetSourceFileSaved(sourceFileSaved);
}

bool CameraAssetPipeline::IsLifeFinished()
{
    return assetInfo_.IsLifeFinished();
}

void CameraAssetPipeline::SaveCameraPhotoFinished()
{
    assetInfo_.SetFirstStageFinished(true);
}

void CameraAssetPipeline::OnProcessFinished()
{
    assetInfo_.SetSecondStageFinished(true);
}

// 数据初始化
void CameraAssetPipeline::Init(const CameraAssetInfo& assetInfo)
{
    assetInfo_ = assetInfo;
    InitCachePath();
}

void CameraAssetPipeline::InitCachePath()
{
    std::string path = assetInfo_.GetPath();
    CHECK_AND_RETURN_LOG(!path.empty(), "Failed to get asset path");

    std::string cacheDirPath = CameraPathUtils::GetCacheDir(path);
    CHECK_AND_RETURN_LOG(!cacheDirPath.empty(), "Can not get cache dir path");
    CHECK_AND_RETURN_LOG(MediaFileUtils::CreateDirectory(cacheDirPath),
        "Can not create dir %{private}s", cacheDirPath.c_str());
    MEDIA_INFO_LOG("InitCachePath success, fileId: %{public}d", assetInfo_.GetFileId());
}

// 保存水印
void CameraAssetPipeline::SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string& bundleName,
    const std::string& editData)
{
    const NativeRdb::ValuesBucket& values = cmd.GetValueBucket();

    std::string compatibleFormat;
    MediaValuesBucketUtils::GetString(values, CONST_COMPATIBLE_FORMAT, compatibleFormat);
    compatibleFormat = compatibleFormat.empty() ? bundleName : compatibleFormat;
    std::string formatVersion;
    MediaValuesBucketUtils::GetString(values, CONST_FORMAT_VERSION, formatVersion);
    std::string data;
    MediaValuesBucketUtils::GetString(values, CONST_EDIT_DATA, data);

    int32_t ret = CameraPathUtils::SaveEditDataCameraByStruct(
        assetInfo_.GetPath(), compatibleFormat, formatVersion, data, bundleName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to SaveEditDataCameraByStruct.");
        SetTakeEffectStatus(TakeEffectStatus::NO_NEED_TAKE_EFFECT);
        return;
    }
    SetTakeEffectStatus(TakeEffectStatus::NEED_TAKE_EFFECT);
    MEDIA_INFO_LOG("SaveEditDataCamera finish.");
}

// 一阶段上报
int32_t CameraAssetPipeline::CreateCameraFileFd(const CreateCameraFileFdDto &dto)
{
    CHECK_AND_RETURN_RET_LOG(IsValid(), E_ERR, "pipeline is invalid.");

    MEDIA_INFO_LOG("CameraAssetPipeline::CreateCameraFileFd enter");

    std::string unifyMode = dto.mode;
    transform(unifyMode.begin(), unifyMode.end(), unifyMode.begin(), ::tolower);
    // 当前仅接受"rw", 如果修改需要注意权限
    CHECK_AND_RETURN_RET_LOG(unifyMode == MEDIA_FILEMODE_READWRITE, E_INVALID_MODE,
        "Invalid mode: %{public}s.", unifyMode.c_str());

    // (校验)CheckPermissionToOpenFileAsset: 回收站数据 + 隐藏数据的处理策略。
    // 当前仅考虑新通路场景, 获取fd的时机都在一阶段，不存在该类问题

    // 1.处理路径
    CameraPathType type = static_cast<CameraPathType>(dto.pathType);
    std::string filePath;
    CameraPathUtils::GetCameraPath(type, assetInfo_.GetPath(), filePath);
    // 该接口需要拦截: 打开已存在的fd

    // 2.优先创建空文件
    int32_t errCode = MediaFileUtils::CreateAsset(filePath);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_ERR, "Create asset failed, path: %{private}s", filePath.c_str());

    int32_t fd = MediaPrivacyManager(filePath, unifyMode, std::to_string(assetInfo_.GetFileId())).Open();
    MEDIA_INFO_LOG("CameraAssetPipeline::CreateCameraFileFd success, fd: %{public}d.", fd);
    return fd;
}

int32_t CameraAssetPipeline::CloseCameraFileFd(const ScanCameraFileDto &dto)
{
    CHECK_AND_RETURN_RET_LOG(IsValid(), E_ERR, "pipeline is invalid.");

    // 1.文件转正(如果是临时文件, 则需要转为正式文件)
    std::string tempPath;
    CameraPathUtils::GetCameraPath(static_cast<CameraPathType>(dto.pathType), assetInfo_.GetPath(), tempPath);

    CameraPathType realPathType = CameraPathType::UNDEFINED;
    std::string realPath = CameraPathUtils::GetRealPathFromTempPath(assetInfo_.GetPath(),
        static_cast<CameraPathType>(dto.pathType), realPathType);

    bool ret = CloseCameraFileFdWithMutex(realPath, tempPath, realPathType);
    CHECK_AND_RETURN_RET_LOG(ret, E_ERR, "Failed to CloseCameraFileFdWithMutex.");
    
    // 仅效果图目录, 需要扫描
    if (realPathType == CameraPathType::EDITED_PATH) {
        // 按需扫描: 不更新相册、不生成缩略图
        if (!dto.needUpdateAlbum && !dto.needGenerateThumbnail) {
            MEDIA_INFO_LOG("ScanFileWithoutAlbumUpdateAndThumbGeneration");
            MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdateAndThumbGeneration(
                assetInfo_.GetPath(), true, assetInfo_.GetFileId());
        }
    }
    return E_OK;
}

// 一阶段落盘
static void UpdateValuesForCommon(const SaveCameraPhotoDto &dto, ValuesBucket &values)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateValuesForCommon");

    if (dto.supportedWatermarkType != INT32_MIN) {
        values.Put(PhotoColumn::SUPPORTED_WATERMARK_TYPE, dto.supportedWatermarkType);
    }
    if (dto.cameraShotKey != "NotSet") {
        values.Put(PhotoColumn::CAMERA_SHOT_KEY, dto.cameraShotKey);
    }

    long cpuNums = sysconf(_SC_NPROCESSORS_CONF);
    MEDIA_INFO_LOG("device cpuNums is %{public}ld", cpuNums);
    // For device For devices with <= 8 cores, the thumbnail generated by capturing does not take the ready process,
    // it is directly set to visible, and the ready status is set to retry generating the thumbnail
    if (cpuNums > 0 && cpuNums <= PHONE_8G_CPU_NUMS) {
        values.Put(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 1);
        values.Put(PhotoColumn::PHOTO_THUMBNAIL_READY, static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_RETRY));
    }
}

int32_t CameraAssetPipeline::DoAccurateRefresh(const SaveCameraPhotoDto &dto,
    AccurateRefresh::AssetAccurateRefresh &assetRefresh, ValuesBucket &values, RdbPredicates &predicates)
{
    MediaLibraryTracer tracer;
    tracer.Start("CameraAssetPipeline::DoAccurateRefresh");
    MEDIA_DEBUG_LOG("CameraAssetPipeline::DoAccurateRefresh begin.");

    int32_t updateRows = 0;
    if (dto.discardHighQualityPhoto) {
        MultiStagesPhotoCaptureManager::GetInstance().CancelProcessRequest(assetInfo_.GetPhotoId());
        MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(assetInfo_.GetPhotoId(), false);

        // Only third-party app save photo, it will bring dirty flag
        // The photo saved by third-party apps, whether of low or high quality, should set dirty to TYPE_NEW
        // Every subtype of photo saved by third-party apps, should set dirty to TYPE_NEW
        // Need to change the quality to high quality before updating
        values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
        // 必须更新success
        CHECK_AND_RETURN_RET_LOG(updateRows > 0, E_ERR, "failed to update third party photo temp.");
        return updateRows;
    }

    if (assetInfo_.GetSubtype() == static_cast<int32_t>(PhotoSubType::BURST)) {
        predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
        predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::BURST)));

        // 同时更新 is_temp 和 dirty
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
        // 必须更新success
        CHECK_AND_RETURN_RET_LOG(updateRows > 0, E_ERR, "failed to update burst photo update temp.");
        return updateRows;
    }

    updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
    // 必须更新success
    CHECK_AND_RETURN_RET_LOG(updateRows > 0, E_ERR, "update temp flag fail.");
    if (assetInfo_.GetSubtype() != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
        predicates.NotEqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));

        ValuesBucket valuesBucketDirty;
        valuesBucketDirty.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        int32_t updateDirtyRows = MediaLibraryRdbStore::UpdateWithDateTime(valuesBucketDirty, predicates);
        // 允许更新失败
        CHECK_AND_RETURN_RET_LOG(updateDirtyRows >= 0, E_ERR, "update dirty flag fail.");
    }
    return updateRows;
}

int32_t CameraAssetPipeline::UpdateIsTempAndDirty(const SaveCameraPhotoDto &dto)
{
    MediaLibraryTracer tracer;
    tracer.Start("CameraAssetPipeline::UpdateIsTempAndDirty");
    MEDIA_INFO_LOG("UpdateIsTempAndDirty begin");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, assetInfo_.GetFileId());
    // 1.文件转正
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_TEMP, false);

    // 2.需要合并更新的公共数据（客户端添加）
    UpdateValuesForCommon(dto, values);

    // 3.多态：需要额外更新的业务数据（各业务侧添加, 可能涉及到 assetInfo 的变更）
    CameraAssetInfo modifyAssetInfo = assetInfo_;
    bool isUpdate = UpdateExtValuesForStageInternal(dto, values, modifyAssetInfo);
    MEDIA_INFO_LOG("modifyAssetInfo: %{public}s, assetInfo_: %{public}s.",
        modifyAssetInfo.ToString().c_str(), assetInfo_.ToString().c_str());

    // 4.实际更新
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SAVE_CAMERA_PHOTO_BUSSINESS_NAME);
    int32_t updateRows = DoAccurateRefresh(dto, assetRefresh, values, predicates);
    CHECK_AND_RETURN_RET(updateRows > 0, E_ERR);
    if (isUpdate) {
        assetInfo_ = modifyAssetInfo;
    }

    // 5.刷新相册
    assetRefresh.RefreshAlbum(static_cast<NotifyAlbumType>(SYS_ALBUM | USER_ALBUM | SOURCE_ALBUM));
    assetRefresh.Notify();

    MEDIA_INFO_LOG("UpdateIsTempAndDirty end");
    return updateRows;
}

int32_t CameraAssetPipeline::SaveCameraPhoto(const SaveCameraPhotoDto &dto)
{
    CHECK_AND_RETURN_RET_LOG(IsValid(), E_ERR, "pipeline is invalid.");
    MediaLibraryTracer tracer;
    tracer.Start("CameraAssetPipeline::SaveCameraPhoto");
    MEDIA_DEBUG_LOG("SaveCameraPhoto enter");

    MultiStagesCaptureDfxCaptureTimes::GetInstance().AddCaptureTimes(CaptureMessageType::SAVE_ASSET);
    MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(assetInfo_.GetPhotoId(), AddSaveTimeStat::START);

    // save
    int32_t ret = HandleSaveCameraPhoto(dto);
    if (ret <= 0) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().RemoveTime(assetInfo_.GetPhotoId());
        return ret;
    }

    MultiStagesCaptureDfxCaptureTimes::GetInstance().AddCaptureTimes(CaptureMessageType::CAPTURE_IMAGE_TIMES_SUCCESS);
    MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(assetInfo_.GetPhotoId(), AddSaveTimeStat::END);
    MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().Report(assetInfo_.GetPhotoId(), false, assetInfo_.GetSubtype());
    MEDIA_DEBUG_LOG("SaveCameraPhoto end");
    return ret;
}

int32_t CameraAssetPipeline::HandleSaveCameraPhoto(const SaveCameraPhotoDto &dto)
{
    MediaLibraryTracer tracer;
    tracer.Start("CameraAssetPipeline::HandleSaveCameraPhoto");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} start save assetInfo: %{public}s.",
        MLOG_TAG, __FUNCTION__, __LINE__, assetInfo_.ToString().c_str());

    // 1.确定水印状态: 二阶段需要使用
    RecheckEffectStatus();

    // 2.文件落盘
    SaveImageForStageInternal(dto);

    // 3.临时文件转正
    int32_t ret = 0;
    {
        std::unique_lock<mutex> locker(dbMutex_);
        ret = UpdateIsTempAndDirty(dto);
    }
    if (ret <= 0) {
        MultiStagesCaptureDfxCaptureFault::Report(assetInfo_.GetPhotoId(), assetInfo_.GetSubtype(),
            CaptureFaultType::UPDATE_DB_TIMEOUT, "UpdateIsTempAndDirty failed");
        MEDIA_ERR_LOG("UpdateIsTempAndDirty failed, ret: %{public}d", ret);
        return ret;
    }

    // 3.1 YUV场景需要校验: 是否存在高质量图并发
    CheckSaveImageForYuv();

    // 4.扫描业务
    if (dto.containsAddResource) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
            "MultistagesCapture Success, no need scanfile, assetInfo: %{public}s, ret: %{public}d.",
            MLOG_TAG, __FUNCTION__, __LINE__, assetInfo_.ToString().c_str(), ret);
        return ret;
    }
    ScanFileForStageInternal();
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture Success, assetInfo: %{public}s, ret: %{public}d.",
        MLOG_TAG, __FUNCTION__, __LINE__, assetInfo_.ToString().c_str(), ret);
    return ret;
}

void CameraAssetPipeline::RecheckEffectStatus()
{
    auto effectStatus = assetInfo_.GetTakeEffectStatus();
    if (effectStatus == TakeEffectStatus::UNDEFINED) {
        // 当前无水印
        SetTakeEffectStatus(TakeEffectStatus::NO_NEED_TAKE_EFFECT);
    }
    MEDIA_INFO_LOG("EffectStatus: %{public}d.", static_cast<int32_t>(assetInfo_.GetTakeEffectStatus()));
}

int32_t CameraAssetPipeline::CheckSaveImageForYuv()
{
    // 仅 YuvPipeline 需要实现
    MEDIA_INFO_LOG("pipeline: %{public}d no need check.", static_cast<int32_t>(pipelineType_));
    return E_OK;
}

// 二阶段落盘
int32_t CameraAssetPipeline::OnProcessImageDone(const OnProcessImageWrapper &wrapper)
{
    CHECK_AND_RETURN_RET_LOG(IsValid(), E_ERR, "pipeline is invalid.");
    MediaLibraryTracer tracer;
    tracer.Start("CameraAssetPipeline::OnProcessImageDone");
    MEDIA_INFO_LOG("CameraAssetPipeline::OnProcessImageDone: %{public}s.", assetInfo_.ToString().c_str());

    int32_t fileId = assetInfo_.GetFileId();
    std::string photoId = assetInfo_.GetPhotoId();

    // 1.各业务侧的入参校验, 初始化
    CHECK_AND_RETURN_RET_LOG(InitForOnProcessInternal(wrapper), E_ERR, "failed to InitForOnProcess.");

    // 2.查询数据, 并确定一阶段与二阶段的中间业务影响
    std::shared_ptr<FileAsset> fileAsset;
    {
        std::unique_lock<mutex> locker(dbMutex_);
        fileAsset = MultiStagesCaptureDao::QueryForOnProcess(fileId, photoId, metadata_);
        CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr.");
        // 2.1 校验is_temp字段的处理
        bool ret = CheckCanSaveDirectlyInternal(fileAsset);
        CHECK_AND_RETURN_RET(ret, E_OK);
    }

    // 3 二阶段落盘(不允许失败)
    ProcessMultistagesPhoto(fileAsset);

    // 4.更新二阶段相关信息(不允许失败)
    MultiStagesCaptureDao::UpdateHighQualityInfo(assetInfo_.GetFileId(), metadata_, true);

    // 5.异步扫描, 生成缩略图
    ScanFileForOnProcessInternal();

    // 6.通知(可优化)
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo_, MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE);
    NotifyImageIfTempFile(false);     // 不能删除，ani等接口尚未适配

    // 7.动态照片需要添加视频二阶段
    CHECK_AND_EXECUTE(!assetInfo_.IsMovingPhoto(),
        MultiStagesMovingPhotoCaptureManager::AddVideoFromMovingPhoto(fileId));

    // 8.打点(耗时 + 行为)
    MultiStagesCaptureDfxTotalTime::GetInstance().Report(photoId, static_cast<int32_t>(metadata_.dfxMediaType));
    MultiStagesCaptureDfxResult::Report(photoId,
        static_cast<int32_t>(MultiStagesCaptureResultErrCode::SUCCESS), static_cast<int32_t>(metadata_.dfxMediaType));

    // 9.落盘后: 删除raw图
    MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(photoId, false);

    // 结束标记
    OnProcessFinished();
    MEDIA_INFO_LOG("CameraAssetPipeline::OnProcessImageDone success: %{public}s.", assetInfo_.ToString().c_str());
    return E_OK;
}

void CameraAssetPipeline::ProcessMultistagesPhoto(const std::shared_ptr<FileAsset> &fileAsset)
{
    // 各业务侧实现落盘逻辑, 原则: 100分效果图落盘失败, 用100分裸图替代
    int32_t ret = ProcessMultistagesPhotoInternal(fileAsset);
    if (ret != E_OK) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} Save high quality image failed. ret: %{public}d.",
            MLOG_TAG, __FUNCTION__, __LINE__, ret);

        MultiStagesCaptureDfxResult::Report(assetInfo_.GetPhotoId(),
            static_cast<int32_t>(MultiStagesCaptureResultErrCode::SAVE_IMAGE_FAIL),
            static_cast<int32_t>(metadata_.dfxMediaType));
    }
}

// 不可恢复错误码
void CameraAssetPipeline::HandleIrrecoverableErrImage(const CameraAssetInfo& assetInfo)
{
    // 1.清理raw图
    MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(assetInfo.GetPhotoId(), false);
 
    // 2.强转: 80->100
    MultiStagesCaptureDao::UpdatePhotoQuality(assetInfo.GetFileId());
    MultiStagesCaptureDao().UpdatePhotoDirtyNew(assetInfo.GetFileId());

    // 3.结束标记
    OnProcessFinished();
}
 
// 可恢复错误码: 高温场景
void CameraAssetPipeline::HandleHighTemperatureImage(const CameraAssetInfo& assetInfo)
{
    // 1.通知客户端, 之前的高优先级请求失败
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo, MultistagesCaptureNotifyType::ON_ERROR_IMAGE);
    NotifyImageIfTempFile(true);     // 不能删除，ani等接口尚未适配
 
    // 2.清理高优先级请求, 避免后续无法请求
    MultiStagesCaptureRequestTaskManager::ClearPhotoInProcessRequestCount(assetInfo.GetPhotoId());
}
 
// 可恢复错误码
void CameraAssetPipeline::HandleRecoverableErrImage(const CameraAssetInfo& assetInfo)
{
    // 仅清理高优先级请求, 避免后续无法请求
    MultiStagesCaptureRequestTaskManager::ClearPhotoInProcessRequestCount(assetInfo.GetPhotoId());
}

// 二阶段失败
bool CameraAssetPipeline::OnErrorImage(const MediaDpsErrorCode error, bool& isMovingPhoto)
{
    isMovingPhoto = assetInfo_.IsMovingPhoto();
    switch (error) {
        case MediaDpsErrorCode::UNDEFINED:
            MEDIA_ERR_LOG("errorCode is undefined.");
            return false;
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INVALID_PHOTO_ID:
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_FAILED:
            HandleIrrecoverableErrImage(assetInfo_);
            break;
        case MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_ABNORMAL:
            HandleHighTemperatureImage(assetInfo_);
            break;
        default:
            HandleRecoverableErrImage(assetInfo_);
            break;
    }
    MEDIA_INFO_LOG("error: %{public}d, fileId: %{public}d, photoid: %{public}s",
        static_cast<int32_t>(error), assetInfo_.GetFileId(), assetInfo_.GetPhotoId().c_str());
    return true;
}

// 通知
void CameraAssetPipeline::NotifyImageIfTempFile(bool isError)
{
    CHECK_AND_RETURN_LOG(IsValid(), "pipeline is invalid.");

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "get instance notify failed NotifyIfTempFile abortion");

    std::string extrUri = MediaFileUtils::GetExtraUri(assetInfo_.GetDisplayName(), assetInfo_.GetPath());
    auto notifyUri = MediaFileUtils::GetUriByExtrConditions(CONST_ML_FILE_URI_PREFIX +
        MediaFileUri::GetMediaTypeUri(assetInfo_.GetMediaType(), MEDIA_API_VERSION_V10) + "/",
        to_string(assetInfo_.GetFileId()), extrUri);
    notifyUri = MediaFileUtils::GetUriWithoutDisplayname(notifyUri);
    if (isError) {
        notifyUri += HIGH_TEMPERATURE;
    }

    MEDIA_DEBUG_LOG("MultistagesCapture notify: %{public}s", MediaFileUtils::DesensitizePath(notifyUri).c_str());
    watch->Notify(notifyUri, NOTIFY_UPDATE);
}

// 工具能力
std::string CameraAssetPipeline::CreateUri()
{
    std::string uri = "";
    
    int32_t fileId = assetInfo_.GetFileId();
    const std::string& filePath = assetInfo_.GetPath();
    const std::string& displayName = assetInfo_.GetDisplayName();
    MediaType mediaType = assetInfo_.GetMediaType();
    if (filePath.empty() || displayName.empty() || mediaType <= MediaType::MEDIA_TYPE_FILE) {
        MEDIA_ERR_LOG("filePath: %{private}s, displayName: %{private}s", filePath.c_str(), displayName.c_str());
        return uri;
    }

    std::string extrUri = MediaFileUtils::GetExtraUri(displayName, filePath);
    uri = MediaFileUtils::GetUriByExtrConditions(CONST_ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(mediaType,
        MEDIA_API_VERSION_V10) + "/", to_string(fileId), extrUri);
    return uri;
}

// 相机定制化inner接口
void CameraAssetPipeline::GetDeferredPictureInfo(GetDeferredPictureInfoRespBody& respbody)
{
    // 1.获取水印信息
    CameraPathUtils::ReadEditdataCameraFromFile(assetInfo_.GetPath(), false, respbody.editData);

    // 2.获取db数据
    auto fileAsset = MultiStagesCaptureDao::QueryForDeferredPictureInfo(assetInfo_.GetFileId());
    CHECK_AND_RETURN_LOG(fileAsset != nullptr, "fileAsset is nullptr.");
    respbody.mimeType = fileAsset->GetMimeType();
    respbody.orientation = fileAsset->GetOrientation();

    MEDIA_INFO_LOG("GetAllEditDataCamera: %{public}s.", respbody.ToString().c_str());
}
} // Media
} // OHOS