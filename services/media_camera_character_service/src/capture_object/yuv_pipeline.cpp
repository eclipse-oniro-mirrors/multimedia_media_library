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

#define MLOG_TAG "YuvPipeline"

#include "yuv_pipeline.h"

#include <unordered_map>

#include "camera_mapper.h"
#include "camera_path_utils.h"
#include "exif_metadata.h"
#include "file_utils.h"
#include "high_quality_scan_file_callback.h"
#include "media_change_effect.h"
#include "media_edit_utils.h"
#include "media_exif.h"
#include "media_log.h"
#include "media_string_utils.h"
#include "media_values_bucket_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_tracer.h"
#include "multistages_camera_capture_manager.h"
#include "multistages_capture_dfx_capture_fault.h"
#include "multistages_capture_dfx_save_camera_photo.h"
#include "multistages_capture_notify.h"
#include "multistages_photo_capture_manager.h"
#include "picture_manager_thread.h"
#include "refresh_business_name.h"
#include "thumbnail_utils.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::Notification;

namespace OHOS {
namespace Media {
constexpr int32_t HIGH_PIXEL_SIDE = 12000;

enum ImageFileType : int32_t {
    JPEG = 1,
    HEIF = 2
};

const std::string MIME_TYPE_JPEG = "image/jpeg";
const std::string EXTENSION_JPEG = "jpg";

const std::string MIME_TYPE_HEIF = "image/heic";
const std::string EXTENSION_HEIF = "heic";

// <ImageFileType, vector<mimeType, extension>>
const int32_t MIMETYPE_INDEX = 0;
const int32_t EXTENSION_INDEX = 1;
static const std::unordered_map<ImageFileType, std::vector<std::string>> IMAGE_FILE_TYPE_MAP = {
    {ImageFileType::JPEG, { MIME_TYPE_JPEG, EXTENSION_JPEG }},
    {ImageFileType::HEIF, { MIME_TYPE_HEIF, EXTENSION_HEIF }},
};

constexpr int32_t ORIENTATION_0 = 1;
constexpr int32_t ORIENTATION_90 = 6;
constexpr int32_t ORIENTATION_180 = 3;
constexpr int32_t ORIENTATION_270 = 8;

static const std::unordered_map<int, int> ORIENTATION_MAP = {
    {0, ORIENTATION_0},
    {90, ORIENTATION_90},
    {180, ORIENTATION_180},
    {270, ORIENTATION_270}
};

YuvPipeline::YuvPipeline()
{
    SetPipelineType(CameraPipelineType::YUV);
}

// 一阶段上报
bool YuvPipeline::CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
    const CameraPathType& pathType)
{
    return true;
}

void YuvPipeline::OnDelivery(std::shared_ptr<Media::Picture> picture)
{
    // YuvPipeline 把 picture 存入缓存
    if (picture == nullptr || picture->GetMainPixel() == nullptr) {
        MEDIA_ERR_LOG("picture is nullptr.");
        return;
    }
    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();

    MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddCaptureTime(photoId, AddCaptureTimeStat::END);
    MultiStagesPhotoCaptureManager::GetInstance().DealLowQualityPicture(photoId, fileId, std::move(picture), false);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} save low quality image end",
        MLOG_TAG, __FUNCTION__, __LINE__);
}

static int32_t GetPicture(const std::string& photoId, std::shared_ptr<Media::Picture>& picture,
    bool& isHighQualityPicture, bool isCleanImmediately = false)
{
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_RETURN_RET_LOG(pictureManagerThread != nullptr, E_FILE_EXIST, "pictureManagerThread is nullptr.");

    bool isTakeEffect = false;
    picture = pictureManagerThread->GetDataWithImageId(photoId, isHighQualityPicture, isTakeEffect, isCleanImmediately);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, E_FILE_EXIST, "picture is not exists!");
    
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "photoId: %{public}s, picture use: %{public}d, picture point to addr: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, photoId.c_str(), static_cast<int32_t>(picture.use_count()),
        std::to_string(reinterpret_cast<long long>(picture.get())).c_str());
    return E_OK;
}

static void UpdateEditDataPath(const std::string& oldFilePath, const std::string& extension)
{
    MEDIA_INFO_LOG("UpdateEditDataPath enter, oldFilePath: %{public}s, extension: %{public}s.",
        oldFilePath.c_str(), extension.c_str());

    std::string editDataPath = MediaEditUtils::GetEditDataDir(oldFilePath);
    std::string tempOutputPath = editDataPath;
    size_t pos = tempOutputPath.find_last_of('.');
    if (pos == string::npos) {
        MEDIA_ERR_LOG("Failed to UpdateEditDataPath.");
        return;
    }

    tempOutputPath.erase(pos + 1);
    tempOutputPath.append(extension);
    int32_t ret = rename(editDataPath.c_str(), tempOutputPath.c_str());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("rename failed, ret: %{public}d, errno: %{public}d.", ret, errno);
    }

    MEDIA_INFO_LOG("rename, src: %{public}s, dest: %{public}s", editDataPath.c_str(), tempOutputPath.c_str());
}

static void GetModityExtensionPath(const std::string& path, const std::string& extension, std::string& modifyFilePath)
{
    size_t pos = path.find_last_of('.');
    CHECK_AND_RETURN_LOG(pos != string::npos, "Failed to parse the path");
    modifyFilePath = path.substr(0, pos + 1) + extension;
}

static int32_t UpdataExtension(const CameraAssetInfo& assetInfo, const SaveCameraPhotoDto& dto,
    TempModifiedDataForYuv& tempData)
{
    ImageFileType type = static_cast<ImageFileType>(dto.imageFileType);
    auto itr = IMAGE_FILE_TYPE_MAP.find(type);
    CHECK_AND_RETURN_RET_LOG(itr != IMAGE_FILE_TYPE_MAP.end(), E_INVALID_ARGUMENTS,
        "fileType : %{public} is not support", dto.imageFileType);

    std::string mimetype = (itr->second).at(MIMETYPE_INDEX);
    if (mimetype == assetInfo.GetMimeType()) {
        // 与默认格式一致, 则无需修改
        MEDIA_INFO_LOG("No format need to change, fileId: %{public}d.", assetInfo.GetFileId());
        return E_OK;
    }
    tempData.isModified = true;
    tempData.modifyMimeType = mimetype;
    tempData.modifyExtension = (itr->second).at(EXTENSION_INDEX);

    GetModityExtensionPath(assetInfo.GetPath(), tempData.modifyExtension, tempData.modifyFilePath);
    CHECK_AND_RETURN_RET_LOG(!tempData.modifyFilePath.empty(), E_ERR, "modifyFilePath is empty.");
    GetModityExtensionPath(assetInfo.GetDisplayName(), tempData.modifyExtension, tempData.modifyDisplayName);
    CHECK_AND_RETURN_RET_LOG(!tempData.modifyDisplayName.empty(), E_ERR, "modifyDisplayName is empty.");

    MEDIA_INFO_LOG("modifyFilePath: %{public}s, modifyDisplayName: %{public}s.",
        tempData.modifyFilePath.c_str(), tempData.modifyDisplayName.c_str());
    return E_OK;
}

// 一阶段落盘
void YuvPipeline::SaveImageForStageInternal(const SaveCameraPhotoDto& dto)
{
    MediaLibraryTracer tracer;
    tracer.Start("YuvPipeline::SaveImageForStageInternal");
    MEDIA_INFO_LOG("YuvPipeline::SaveImageForStageInternal, imageFileType: %{public}d.", dto.imageFileType);

    auto assetInfo = GetAssetInfo();

    // 确认: 是否要更新路径
    int32_t ret = UpdataExtension(assetInfo, dto, tempData_);
    if (tempData_.isModified && ret == E_OK) {
        // 更新editData路径, 避免水印信息找不到
        UpdateEditDataPath(assetInfo.GetPath(), tempData_.modifyExtension);
    }

    SavePictureForFirstStage(true);
}

static void UpdateQualityAndDirty(const int32_t fileId, const std::string& photoId, const MediaDpsMetadata& metadata,
    bool isHighQualityPicture, bool needDfx)
{
    if (!isHighQualityPicture) {
        MEDIA_WARN_LOG("no need UpdateQualityAndDirty");
        return;
    }

    MultiStagesCaptureDao::UpdateHighQualityInfo(fileId, metadata, false);
    if (needDfx) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(photoId, AddSaveTimeStat::UPDATE_DB);
    }
}

static void ClearPictureData(const std::string& photoId, std::shared_ptr<Media::Picture>& resultPicture)
{
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_RETURN_LOG(pictureManagerThread != nullptr, "pictureManager is nullptr, There may be a memory leak.");

    // 取消 GetPicture 的标记
    pictureManagerThread->FinishAccessingPicture(photoId);

    // 删除拖尾
    std::string lastPhotoId = MultistagesCameraCaptureManager::GetInstance().GetLastSavePhotoId();
    pictureManagerThread->DeleteDataWithImageId(lastPhotoId, LOW_QUALITY_PICTURE);

    if (resultPicture != nullptr) {
        auto pixelMap = resultPicture->GetMainPixel();
        CHECK_AND_RETURN_LOG(pixelMap != nullptr, "pixelMap is nullptr.");
        int32_t height = pixelMap->GetHeight();
        int32_t width = pixelMap->GetWidth();
        if (height > HIGH_PIXEL_SIDE && width > HIGH_PIXEL_SIDE) {
            // 200M 不需要拖尾
            pictureManagerThread->DeleteDataWithImageId(photoId, LOW_QUALITY_PICTURE);
            pictureManagerThread->SetLast200mImageId("default");
        }
    }

    // 保存拖尾
    MultistagesCameraCaptureManager::GetInstance().SetLastSavePhotoId(photoId);

    // 通知内存变化
    MultistagesCaptureNotify::NotifyLowQualityMemoryCount();
}

int32_t YuvPipeline::SavePictureForFirstStage(bool needDfx)
{
    MEDIA_INFO_LOG("SavePictureForFirstStage enter.");

    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();
    if (needDfx) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(photoId, AddSaveTimeStat::SAVE_PICTURE);
    }

    // 1.落盘信息
    std::string path = tempData_.isModified ? tempData_.modifyFilePath : assetInfo.GetPath();
    std::string mimetype = tempData_.isModified ? tempData_.modifyMimeType : assetInfo.GetMimeType();

    // 2.判断是否存在水印信息
    std::string editData;
    int32_t ret = CameraPathUtils::ReadEditdataCameraFromFile(path, true, editData);
    if (editData.empty()) {
        SaveOnePictureForFirstStage(path, mimetype, needDfx);          // 仅存在原图
    } else {
        SaveTwoPictureForFirstStage(path, mimetype, editData, needDfx);  // 原图 + 效果图
    }

    // 3.若落盘高质量, 需要更新photo_quality && dirty
    UpdateQualityAndDirty(fileId, photoId, GetMediaDpsMetadata(), isHighForEffective_, needDfx);
    
    // 4.清理数据
    ClearPictureData(photoId, resultPictureForFirstStage_);
    return E_OK;
}

int32_t YuvPipeline::CheckSaveImageForYuv()
{
    if (isSaveDirectly_ || isHighForEffective_) {
        MEDIA_INFO_LOG("no need to CheckSaveImageForYuv, saveDirectly: %{public}d, isHighForEffective: %{public}d",
            isSaveDirectly_, isHighForEffective_);
        return E_OK;
    }
    MEDIA_INFO_LOG("CheckSaveImageForYuv enter.");
    return SavePictureForFirstStage(false);
}

static bool CheckAndReport(bool cond, const CameraAssetInfo& assetInfo, CaptureFaultType faultType,
    const std::string& reason)
{
    if (!cond) {
        MultiStagesCaptureDfxCaptureFault::Report(assetInfo.GetPhotoId(), assetInfo.GetSubtype(), faultType,
            reason);
    }
    return cond;
}

int32_t YuvPipeline::SaveOnePictureForFirstStage(const std::string& filePath, const std::string& mimeType, bool needDfx)
{
    MEDIA_INFO_LOG("SaveOnePictureForFirstStage enter.");
    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();

    // 获取picture
    std::shared_ptr<Media::Picture> picture = nullptr;
    int32_t ret = GetPicture(photoId, picture, isHighForEffective_, false);
    CHECK_AND_RETURN_RET_LOG(
        CheckAndReport(ret == E_OK && picture != nullptr, assetInfo, CaptureFaultType::ASSET_FILE_CHECK_ERROR,
        "Failed get picture"), E_FILE_EXIST, "Failed get picture");

    FileUtils::DealPicture(mimeType, filePath, picture, isHighForEffective_);
    if (needDfx) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(photoId, AddSaveTimeStat::DEAL_PICTURE);
    }

    // 确认需要扫描的picture
    resultPictureForFirstStage_ = picture;

    MEDIA_INFO_LOG("SaveOnePictureForFirstStage end.");
    return E_OK;
}

int32_t YuvPipeline::SaveTwoPictureForFirstStage(const std::string& filePath, const std::string& mimeType,
    std::string& editData, bool needDfx)
{
    MEDIA_INFO_LOG("SaveTwoPictureForFirstStage enter.");
    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();

    // 获取 picture
    std::shared_ptr<Media::Picture> sourcePicture = nullptr;
    int32_t ret = GetPicture(photoId, sourcePicture, isHighForSource_);
    CHECK_AND_RETURN_RET_LOG(
        CheckAndReport(ret == E_OK && sourcePicture != nullptr, assetInfo, CaptureFaultType::ASSET_FILE_CHECK_ERROR,
        "Failed get sourcePicture"), E_FILE_EXIST, "Failed get sourcePicture");
    
    // 先落盘原图
    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, filePath, editDataSourcePath);
    FileUtils::DealPicture(mimeType, editDataSourcePath, sourcePicture, isHighForSource_);
    if (needDfx) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(photoId, AddSaveTimeStat::DEAL_PICTURE);
    }

    // 重新获取 picture
    std::shared_ptr<Media::Picture> editedPicture = nullptr;
    ret = GetPicture(photoId, editedPicture, isHighForEffective_);
    CHECK_AND_RETURN_RET_LOG(
        CheckAndReport(ret == E_OK && editedPicture != nullptr, assetInfo, CaptureFaultType::ASSET_FILE_CHECK_ERROR,
        "Failed get editedPicture"), E_FILE_EXIST, "Failed get editedPicture");
    
    if (!isHighForSource_ && isHighForEffective_) {
        // 需要重新落盘原图
        MEDIA_INFO_LOG("Save the original image again, cause it is high.");
        FileUtils::DealPicture(mimeType, editDataSourcePath, sourcePicture, isHighForEffective_);
        isHighForSource_ = true;
    }

    // 落盘效果图
    ret = MediaChangeEffect::TakeEffectForPicture(editedPicture, editData);
    MEDIA_INFO_LOG("TakeEffectForPicture end, ret: %{public}d.", ret);
    if (needDfx) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(photoId, AddSaveTimeStat::TAKE_EFFECT);
    }
    FileUtils::DealPicture(mimeType, filePath, editedPicture, isHighForEffective_);
    if (needDfx) {
        MultiStagesCaptureDfxSaveCameraPhoto::GetInstance().AddSaveTime(photoId, AddSaveTimeStat::SAVE_EFFECT);
    }

    // 确认需要扫描的picture
    resultPictureForFirstStage_ = editedPicture;

    MEDIA_INFO_LOG("SaveTwoPictureForFirstStage end.");
    return E_OK;
}

bool YuvPipeline::UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, ValuesBucket &values,
    CameraAssetInfo& modifyAssetInfo)
{
    if (!tempData_.isModified) {
        MEDIA_WARN_LOG("no need UpdateExtValuesForStageInternal, yuv save type: %{public}d.", dto.imageFileType);
        return false;
    }

    // YUV 保存为 jpg 或者 heif, 需要重新修正数据
    values.Put(MediaColumn::MEDIA_FILE_PATH, tempData_.modifyFilePath);
    values.Put(MediaColumn::MEDIA_NAME, tempData_.modifyDisplayName);
    values.Put(MediaColumn::MEDIA_MIME_TYPE, tempData_.modifyMimeType);
    values.Put(PhotoColumn::PHOTO_MEDIA_SUFFIX, tempData_.modifyExtension);

    modifyAssetInfo.SetPath(tempData_.modifyFilePath);
    modifyAssetInfo.SetDisplayName(tempData_.modifyDisplayName);
    modifyAssetInfo.SetMimeType(tempData_.modifyMimeType);

    return true;
}

static void ResizePicture(std::shared_ptr<Media::Picture>& picture)
{
    CHECK_AND_RETURN_LOG(picture != nullptr, "picture is nullptr");

    auto pixelMap = picture->GetMainPixel();
    CHECK_AND_RETURN_LOG(pixelMap != nullptr, "pixelMap is nullptr");
    int32_t height = pixelMap->GetHeight();
    int32_t width = pixelMap->GetWidth();
    if (height <= HIGH_PIXEL_SIDE || width <= HIGH_PIXEL_SIDE) {
        MEDIA_ERR_LOG("not 200M picture");
        return;
    }

    ThumbnailUtils::ResizeLcd(width, height);
    float widthScale = (1.0f * width) / pixelMap->GetWidth();
    float heightScale = (1.0f * height) / pixelMap->GetHeight();
    pixelMap->resize(widthScale, heightScale);
    MEDIA_INFO_LOG("ResizePicture : height %{public}d, width %{public}d", pixelMap->GetHeight(), pixelMap->GetWidth());
}

void YuvPipeline::ScanFileForStageInternal()
{
    // YuvPipeline 支持基于 YUV 直出缩略图
    // 连拍照片场景不支持 YUV 直出缩略图
    MediaLibraryTracer tracer;
    tracer.Start("YuvPipeline::ScanFileForStageInternal");

    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string path = assetInfo.GetPath();
    MEDIA_INFO_LOG("scan file start, assetInfo: %{public}s", assetInfo.ToString().c_str());

    ResizePicture(resultPictureForFirstStage_);
    if (assetInfo.GetBurstCoverLevel() == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
        MediaLibraryAssetOperations::ScanFile(path, false, true, true, fileId, resultPictureForFirstStage_);
    } else {
        MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(path, false, true, true, fileId);
    }

    // 清理缓存数据
    tempData_.Clear();
    resultPictureForFirstStage_.reset();
}

// 二阶段落盘
bool YuvPipeline::InitForOnProcessInternal(const OnProcessImageWrapper &wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("NewImagePipeline::InitForOnProcessInternal");
    MEDIA_DEBUG_LOG("InitForOnProcessInternal enter");

    CHECK_AND_RETURN_RET_LOG(wrapper.yuv.IsValid(), false, "wrapper is inValid");
    yuv_ = std::move(wrapper.yuv);

    auto assetInfo = GetAssetInfo();
    auto metadata = wrapper.metadata;
    metadata.dfxMediaType = assetInfo.IsMovingPhoto() ? MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE
                                                      : MultiStagesCaptureMediaType::IMAGE;
    SetMediaDpsMetadata(std::move(metadata));
    MEDIA_DEBUG_LOG("InitForOnProcessInternal end.");
    return true;
}

static void HandleOrientation(const std::shared_ptr<FileAsset> &fileAsset, std::shared_ptr<Media::Picture> picture)
{
    CHECK_AND_RETURN_LOG(fileAsset != nullptr && picture != nullptr, "fileAsset or picture is nullptr.");

    int32_t orientation = fileAsset->GetOrientation();
    if (orientation != 0) {
        auto metadata = picture->GetExifMetadata();
        CHECK_AND_RETURN_LOG(metadata != nullptr, "metadata is null");

        auto imageSourceOrientation = ORIENTATION_MAP.find(orientation);
        CHECK_AND_RETURN_LOG(imageSourceOrientation != ORIENTATION_MAP.end(), "Orientation value is invalid.");
        metadata->SetValue(PHOTO_DATA_IMAGE_ORIENTATION, std::to_string(imageSourceOrientation->second));
    }
}

bool YuvPipeline::CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset)
{
    if (!fileAsset->GetPhotoIsTemp()) {
        MEDIA_INFO_LOG("YuvPipeline can save directly.");
        return true;
    }
    MediaLibraryTracer tracer;
    tracer.Start("ImagePipeline::CheckCanSaveDirectlyInternal");
    HILOG_COMM_WARN("%{public}s:{%{public}s:%{public}d} MultistagesCapture, this picture is temp.",
        MLOG_TAG, __FUNCTION__, __LINE__);

    auto assetInfo = GetAssetInfo();
    MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(assetInfo.GetPhotoId(), assetInfo.GetFileId(),
        std::move(yuv_.picture));
    MultiStagesPhotoCaptureManager::GetInstance()::RemoveImage(fileAsset->GetPhotoId(), false);

    // 场景1: 二阶段优先于一阶段返回
    // 场景2: 三方应用不会调用 SaveCameraPhoto 接口
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo, MultistagesCaptureNotifyType::YUV_READY);
    NotifyImageIfTempFile(false);     // 不能删除，ani等接口尚未适配
    isSaveDirectly_ = false;

    // 允许被清理
    OnProcessFinished();
    return false;
}

int32_t YuvPipeline::ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr.");

    MediaLibraryTracer tracer;
    tracer.Start("YuvPipeline::ProcessMultistagesPhotoInternal");
    MEDIA_DEBUG_LOG("ProcessMultistagesPhotoInternal enter");

    auto assetInfo = GetAssetInfo();
    auto picture = yuv_.picture;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, E_ERR, "picture is nullptr.");

    // 1.需要落盘的情况, 优先处理旋转角度
    HandleOrientation(fileAsset, picture);

    // 2.落盘
    std::string path = assetInfo.GetPath();
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, path, editDataCameraPath);

    // 2.1 图片编辑过了只替换低质量裸图
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    if (isEdited) {
        std::string editDataSourcePath;
        CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);
        return FileUtils::SavePicture(editDataSourcePath, picture, assetInfo.GetMimeType(), true);
    }

    // 2.2 没有编辑过, 则正常落盘覆盖
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        // 没有editdata_camera，只落盘在Photo目录
        return SaveOnePictureForOnProcess(assetInfo, picture);
    } else {
        // 有editdata_camera, 添加水印落盘
        return SaveTwoPictureForOnProcess(assetInfo, picture);
    }
}

static void EnableYuvAndNotify(const CameraAssetInfo& assetInfo, std::shared_ptr<Media::Picture> &picture)
{
    if (assetInfo.GetPhotoId().empty()) {
        return;
    }
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();

    MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(photoId, fileId, picture, false, false);
    MultistagesCaptureNotify::NotifyOnProcess(assetInfo, MultistagesCaptureNotifyType::YUV_READY);
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::YUV_READY_BUSSINESS_NAME);
    int32_t ret = assetRefresh->NotifyYuvReady(fileId);
}

int32_t YuvPipeline::SaveOnePictureForOnProcess(
    const CameraAssetInfo& assetInfo, std::shared_ptr<Media::Picture> &picture)
{
    // 1.优先落入缓存, 通知应用可以获取 YUV 对象
    EnableYuvAndNotify(assetInfo, picture);

    // 2.落盘
    int32_t ret = FileUtils::SavePicture(assetInfo.GetPath(), picture, assetInfo.GetMimeType(), true);
    resultPictureForOnProcess_ = picture;
    return ret;
}

int32_t YuvPipeline::SaveTwoPictureForOnProcess(
    const CameraAssetInfo& assetInfo, std::shared_ptr<Media::Picture> &picture)
{
    MediaLibraryTracer tracer;
    tracer.Start("SaveTwoPictureForOnProcess");
    std::string path = assetInfo.GetPath();

    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);

    // 1.先替换低质量裸图
    int ret = FileUtils::SavePicture(editDataSourcePath, picture, assetInfo.GetMimeType(), true);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    // 2.生成高质量水印滤镜图片
    std::string editData;
    CHECK_AND_RETURN_RET_LOG(CameraPathUtils::ReadEditdataCameraFromFile(path, true, editData) == E_OK,
        E_HAS_FS_ERROR, "Failed to read editdata.");

    // 3.添加水印+落盘
    FileUtils::SavePictureWithFilters(picture, path, editData, editDataSourcePath);
    EnableYuvAndNotify(assetInfo, picture);

    FileUtils::DealPicture(assetInfo.GetMimeType(), path, picture, true);
    resultPictureForOnProcess_ = picture;
    return E_OK;
}

void YuvPipeline::ScanFileForOnProcessInternal()
{
    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();

    MEDIA_INFO_LOG("ScanFileForOnProcessInternal with picture.");
    MediaLibraryObjectUtils::ScanFileAsync(assetInfo.GetPath(), to_string(fileId), MediaLibraryApi::API_10,
        assetInfo.IsMovingPhoto(), resultPictureForOnProcess_, HighQualityScanFileCallback::Create(fileId));

    // 清理缓存数据
    resultPictureForOnProcess_.reset();
}
} // Media
} // OHOS