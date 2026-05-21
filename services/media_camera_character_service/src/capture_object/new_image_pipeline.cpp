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

#define MLOG_TAG "NewImagePipeline"

#include "new_image_pipeline.h"

#include "camera_mapper.h"
#include "camera_path_utils.h"
#include "file_utils.h"
#include "high_quality_scan_file_callback.h"
#include "media_log.h"
#include "media_string_utils.h"
#include "media_values_bucket_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_tracer.h"
#include "multistages_capture_dfx_save_camera_photo.h"
#include "multistages_photo_capture_manager.h"
#include "picture_manager_thread.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
// 落盘类型
constexpr size_t SAVE_ONE_IMAGE = 1;    // 仅包含原图（包含动态照片）
constexpr size_t SAVE_TWO_IMAGE = 2;    // 原图+效果图（包含动态照片）

NewImagePipeline::NewImagePipeline()
{
    SetPipelineType(CameraPipelineType::NEW_IMAGE);
}

// 保存水印
void NewImagePipeline::SaveEditDataCamera(MediaLibraryCommand &cmd, const std::string& bundleName,
    const std::string& editData)
{
    if (editData.empty()) {
        MEDIA_INFO_LOG("editData is empty, no need SaveEditDataCamera, maybe ImagePipeline.");
        return;
    }

    auto assetInfo = GetAssetInfo();
    int32_t ret = CameraPathUtils::SaveEditDataCameraByString(assetInfo.GetPath(), editData, bundleName);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to SaveEditDataCameraByString.");
        SetTakeEffectStatus(TakeEffectStatus::NO_NEED_TAKE_EFFECT);
        return;
    }
    SetTakeEffectStatus(TakeEffectStatus::NEED_TAKE_EFFECT);
}

// 一阶段上报
bool NewImagePipeline::CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
    const CameraPathType& pathType)
{
    std::unique_lock<mutex> locker(fileMutex_);

    auto assetInfo = GetAssetInfo();
    bool fileSaved = false;
    if (pathType == CameraPathType::EDITED_PATH) {
        fileSaved = assetInfo.GetEffectiveFileSaved();
    } else if (pathType == CameraPathType::EDIT_DATA_SOURCE_PATH) {
        fileSaved = assetInfo.GetSourceFileSaved();
    }
    MEDIA_INFO_LOG("CloseCameraFileFdWithMutex pathType: %{public}d, fileSaved: %{public}d.",
        static_cast<int32_t>(pathType), fileSaved);

    if (fileSaved) {
        // 100分图已保存, 清理临时文件即可
        bool ret = MediaFileUtils::DeleteFile(tempPath);
        if (!ret) {
            MEDIA_ERR_LOG("Failed to delete temp file, errno: %{public}d", errno);
        }
        return true;
    }
    return CameraPathUtils::SaveTemporaryImage(realPath, tempPath);
}

void NewImagePipeline::OnDelivery(std::shared_ptr<Media::Picture> picture)
{
    // NewImagePipeline 把 picture 存入缓存, 用于生成缩略图
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

// 一阶段落盘
bool NewImagePipeline::UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, ValuesBucket &values,
    CameraAssetInfo& modifyAssetInfo)
{
    MEDIA_INFO_LOG("NewImagePipeline no need UpdateExtValues.");
    return false;
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

void NewImagePipeline::SaveImageForStageInternal(const SaveCameraPhotoDto& dto)
{
    // NewImagePipeline 仅获取缓存 lcdPicture, 用于生成缩略图
    MediaLibraryTracer tracer;
    tracer.Start("NewImagePipeline::SaveImageForStageInternal");
    MEDIA_INFO_LOG("NewImagePipeline::SaveImageForStageInternal.");

    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();

    // 获取 lcdPicture
    bool isHighQualityPicture = false;
    int32_t ret = GetPicture(photoId, resultPictureForFirstStage_, isHighQualityPicture, false);

    // 清理缓存
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_RETURN_LOG(pictureManagerThread != nullptr, "pictureManager is nullptr, There may be a memory leak.");
    pictureManagerThread->FinishAccessingPicture(photoId);
    pictureManagerThread->DeleteDataWithImageId(photoId, LOW_QUALITY_PICTURE);
}

void NewImagePipeline::ScanFileForStageInternal()
{
    // NewImagePipeline 支持基于 YUV 直出缩略图
    MediaLibraryTracer tracer;
    tracer.Start("NewImagePipeline::ScanFileForStageInternal");

    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string photoId = assetInfo.GetPhotoId();
    std::string path = assetInfo.GetPath();

    MEDIA_INFO_LOG("scan file start, assetInfo: %{public}s, isYuv: %{public}d.", assetInfo.ToString().c_str(),
        resultPictureForFirstStage_ != nullptr);

    if (assetInfo.GetBurstCoverLevel() == static_cast<int32_t>(BurstCoverLevelType::COVER)) {
        MediaLibraryAssetOperations::ScanFile(path, false, true, true, fileId, resultPictureForFirstStage_);
    } else {
        MediaLibraryAssetOperations::ScanFileWithoutAlbumUpdate(path, false, true, true, fileId);
    }

    // 清理缓存数据
    resultPictureForFirstStage_.reset();
}

// 二阶段落盘
bool NewImagePipeline::InitForOnProcessInternal(const OnProcessImageWrapper &wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("NewImagePipeline::InitForOnProcessInternal");
    MEDIA_DEBUG_LOG("InitForOnProcessInternal enter");

    CHECK_AND_RETURN_RET_LOG(wrapper.newImage.IsValid(), false, "wrapper is inValid");
    newImage_ = std::move(wrapper.newImage);

    auto assetInfo = GetAssetInfo();
    auto metadata = wrapper.metadata;
    metadata.dfxMediaType = assetInfo.IsMovingPhoto() ? MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE
                                                      : MultiStagesCaptureMediaType::IMAGE;
    if (!metadata.editData.empty()) {
        CameraPathUtils::SaveEditDataCameraByString(assetInfo.GetPath(), metadata.editData, "");
        // 保存完, 清空
        metadata.editData.clear();
    }
    SetMediaDpsMetadata(std::move(metadata));

    // 缩略图picture，如果为null，则按需生成缩略图
    std::shared_ptr<Media::Picture> lcdPicture = newImage_.lcdImage;
    if (lcdPicture == nullptr || lcdPicture->GetMainPixel() == nullptr) {
        MEDIA_INFO_LOG("lcdPicture is null, generate thumbnails may take a long time.");
    }

    MEDIA_INFO_LOG("InitForOnProcessInternal end, newImage: %{public}s.", newImage_.ToString().c_str());
    return true;
}

bool NewImagePipeline::CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset)
{
    MEDIA_INFO_LOG("NewImagePipeline no need check is_temp.");
    return true;
}

int32_t NewImagePipeline::ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr.");

    MediaLibraryTracer tracer;
    tracer.Start("NewImagePipeline::ProcessMultistagesPhotoInternal");
    MEDIA_DEBUG_LOG("ProcessMultistagesPhotoInternal enter");

    // 1.落盘图片的数量
    auto assetInfo = GetAssetInfo();
    size_t fileCount = newImage_.files.size();
    if (fileCount == SAVE_ONE_IMAGE) {
        return ProcessSaveOneImage(assetInfo, fileAsset, newImage_.files);
    } else if (fileCount == SAVE_TWO_IMAGE) {
        return ProcessSaveTwoImage(assetInfo, fileAsset, newImage_.files);
    }

    MEDIA_DEBUG_LOG("param is invalid, fileCount: %{public}zu.", fileCount);
    return E_ERR;
}

int32_t NewImagePipeline::ProcessSaveOneImage(const CameraAssetInfo& assetInfo,
    const std::shared_ptr<FileAsset> &fileAsset, const std::map<std::string, ImageFileMapper> &files)
{
    CHECK_AND_RETURN_RET_LOG(files.find(IMAGE_FILE_EDITED_TYPE) != files.end(), E_ERR, "edited file is not exist.");

    MediaLibraryTracer tracer;
    tracer.Start("ProcessSaveOneImage");
    MEDIA_INFO_LOG("ProcessSaveOneImage enter");

    ImageFileMapper editedImage = files.at(IMAGE_FILE_EDITED_TYPE);
    std::string path = assetInfo.GetPath();

    // 1 图片编辑过了只替换低质量裸图
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    if (isEdited) {
        std::string tempSourcePath;
        CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_EDIT_DATA_SOURCE_PATH, path, tempSourcePath);
        int32_t ret = FileUtils::SaveImage(tempSourcePath, editedImage.addr, editedImage.bytes);

        std::string editDataSourcePath;
        CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);
        {
            std::unique_lock<mutex> locker(fileMutex_);
            bool errCode = CameraPathUtils::SaveTemporaryImage(editDataSourcePath, tempSourcePath);
            CHECK_AND_RETURN_RET_LOG(errCode, E_ERR, "Failed to process Edited image, errno: %{public}d", errno);
            SetSourceFileSaved(true);
        }
        return ret;
    }

    // 2 没有编辑过, 落盘在临时目录中, 再rename
    std::string tempHighPath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_PATH, path, tempHighPath);
    int32_t ret = FileUtils::SaveImage(tempHighPath, editedImage.addr, editedImage.bytes);

    std::unique_lock<mutex> locker(fileMutex_);
    bool errCode = CameraPathUtils::SaveTemporaryImage(path, tempHighPath);
    CHECK_AND_RETURN_RET_LOG(errCode, E_ERR, "Failed to ProcessSaveOneImage, errno: %{public}d", errno);
    SetEffectiveFileSaved(true);
    return ret;
}

int32_t NewImagePipeline::ProcessSaveTwoImage(const CameraAssetInfo& assetInfo,
    const std::shared_ptr<FileAsset> &fileAsset, const std::map<std::string, ImageFileMapper> &files)
{
    CHECK_AND_RETURN_RET_LOG(files.find(IMAGE_FILE_SOURCE_TYPE) != files.end(), E_ERR, "source file is not exist.");

    MediaLibraryTracer tracer;
    tracer.Start("ProcessSaveTwoImage");
    MEDIA_INFO_LOG("ProcessSaveTwoImage enter");

    ImageFileMapper sourceImage = files.at(IMAGE_FILE_SOURCE_TYPE);
    std::string path = assetInfo.GetPath();

    // 原图正常落盘(不论是否编辑)
    std::string tempSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_EDIT_DATA_SOURCE_PATH, path, tempSourcePath);
    int32_t ret = FileUtils::SaveImage(tempSourcePath, sourceImage.addr, sourceImage.bytes);

    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);
    {
        std::unique_lock<mutex> locker(fileMutex_);
        bool errCode = CameraPathUtils::SaveTemporaryImage(editDataSourcePath, tempSourcePath);
        CHECK_AND_RETURN_RET_LOG(errCode, E_ERR, "Failed to process Edited image, errno: %{public}d", errno);
        SetSourceFileSaved(true);
    }

    // 图片编辑过了只替换低质量裸图
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    CHECK_AND_RETURN_RET_LOG(!isEdited, ret, "ProcessSaveTwoImage is edited.");

    // 效果图落盘
    if (files.find(IMAGE_FILE_EDITED_TYPE) == files.end()) {
        MEDIA_ERR_LOG("edited file not exist.");
        return E_ERR;
    }
    auto editedImage = files.at(IMAGE_FILE_EDITED_TYPE);
    std::string tempEffectivePath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_PATH, path, tempEffectivePath);
    ret = FileUtils::SaveImage(tempEffectivePath, editedImage.addr, editedImage.bytes);

    std::unique_lock<mutex> locker(fileMutex_);
    bool errCode = CameraPathUtils::SaveTemporaryImage(path, tempEffectivePath);
    CHECK_AND_RETURN_RET_LOG(errCode, E_ERR, "Failed to ProcessSaveTwoImage, errno: %{public}d", errno);
    SetEffectiveFileSaved(true);
    return ret;
}

void NewImagePipeline::ScanFileForOnProcessInternal()
{
    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();

    MEDIA_INFO_LOG("ScanFileForOnProcessInternal with lcdPicture.");
    MediaLibraryObjectUtils::ScanFileAsync(assetInfo.GetPath(), to_string(fileId), MediaLibraryApi::API_10,
        assetInfo.IsMovingPhoto(), newImage_.lcdImage, HighQualityScanFileCallback::Create(fileId));

    // 清理缓存数据
    newImage_.Clear();
}
} // Media
} // OHOS