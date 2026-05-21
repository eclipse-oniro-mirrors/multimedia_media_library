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

#define MLOG_TAG "ImagePipeline"

#include "image_pipeline.h"

#include "camera_mapper.h"
#include "camera_path_utils.h"
#include "file_utils.h"
#include "high_quality_scan_file_callback.h"
#include "media_change_effect.h"
#include "media_log.h"
#include "media_string_utils.h"
#include "media_values_bucket_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_tracer.h"
#include "userfile_manager_types.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
const std::string MIME_TYPE_HEIF = "image/heif";
const std::string MIME_TYPE_HEIC = "image/heic";
const uint8_t PACKOPTION_QUALITY = 90;
const uint8_t PACKOPTION_QUALITY_HEIF = 95;

ImagePipeline::ImagePipeline()
{
    SetPipelineType(CameraPipelineType::IMAGE);
}

// 一阶段上报
bool ImagePipeline::CloseCameraFileFdWithMutex(const std::string& realPath, const std::string& tempPath,
    const CameraPathType& pathType)
{
    std::unique_lock<mutex> locker(fileMutex_);
    MEDIA_INFO_LOG("CloseCameraFileFdWithMutex begin.");

    auto assetInfo = GetAssetInfo();
    bool fileSaved = false;
    if (pathType == CameraPathType::EDITED_PATH) {
        fileSaved = assetInfo.GetEffectiveFileSaved();
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

void ImagePipeline::OnDelivery(std::shared_ptr<Media::Picture> picture)
{
}

// 一阶段落盘
bool ImagePipeline::UpdateExtValuesForStageInternal(const SaveCameraPhotoDto &dto, ValuesBucket &values,
    CameraAssetInfo& modifyAssetInfo)
{
    MEDIA_INFO_LOG("ImagePipeline no need UpdateExtValues.");
    return false;
}

int32_t ImagePipeline::AddFiltersToPhoto(const std::string& sourcePath, const std::string& tempFilterPath,
    const std::string &editData)
{
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty() && !tempFilterPath.empty(), E_ERR, "path is invalid.");

    MediaLibraryTracer tracer;
    tracer.Start("ImagePipeline::AddFiltersToPhoto");
    MEDIA_INFO_LOG("AddFiltersToPhoto sourcePath: %{private}s, tempFilterPath: %{private}s",
        MediaFileUtils::DesensitizePath(sourcePath).c_str(), MediaFileUtils::DesensitizePath(tempFilterPath).c_str());

    auto assetInfo = GetAssetInfo();
    std::string info = editData;

    // 1.创建落盘文件
    int32_t ret = MediaFileUtils::CreateAsset(tempFilterPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_ERR, "Failed to CreateAsset, ret: %{public}d, errno: %{public}d.",
        ret, errno);

    // 2.添加水印
    tracer.Start("MediaChangeEffect::TakeEffect");
    int32_t quality = assetInfo.GetMimeType() == MIME_TYPE_HEIF ? PACKOPTION_QUALITY_HEIF : PACKOPTION_QUALITY;
    ret = MediaChangeEffect::TakeEffect(sourcePath, tempFilterPath, info, quality);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "Failed to TakeEffect, ret: %{public}d.", ret);

    MEDIA_INFO_LOG("AddFiltersToPhoto success, save to tempFilterPath: %{private}s.", tempFilterPath.c_str());
    return E_OK;
}

int32_t ImagePipeline::AddFiltersExecute(const std::string& filePath, bool executeForLowImage,
    std::string& tempSourcePath, std::string& tempFiltersPath)
{
    MediaLibraryTracer tracer;
    tracer.Start("AddFiltersExecute");
    // 1.Photo目录文件: 复制到.editdata目录的 temp_source.jpg
    CameraPathType tempSourcePathType = executeForLowImage ? CameraPathType::TEMP_LOW_EDIT_DATA_SOURCE_PATH
                                                           : CameraPathType::TEMP_HIGH_EDIT_DATA_SOURCE_PATH;
    CameraPathUtils::GetCameraPath(tempSourcePathType, filePath, tempSourcePath);
    MediaFileUtils::CopyFileUtil(filePath, tempSourcePath);

    // 2.基于 temp_source.jpg 添加水印, 保存到 temp_filters 临时文件
    std::string editData;
    CameraPathUtils::ReadEditdataCameraFromFile(filePath, true, editData);
    CameraPathType tempFiltersPathType = executeForLowImage ? CameraPathType::TEMP_LOW_FILTERS_PATH
                                                            : CameraPathType::TEMP_HIGH_FILTERS_PATH;
    CameraPathUtils::GetCameraPath(tempFiltersPathType, filePath, tempFiltersPath);
    int32_t ret = AddFiltersToPhoto(tempSourcePath, tempFiltersPath, editData);
    return ret;
}

// true: 表示需要落盘; false: 表示不需要落盘
static bool CheckFileSavedStatus(bool effectiveFileSaved, bool sourceFileSaved, bool& executeForLowImage)
{
    if (effectiveFileSaved && sourceFileSaved) {
        // 高质量图已添加水印, 且落盘, 无需执行一阶段
        MEDIA_INFO_LOG("High-quality images have been saved to disk, no need to execute.");
        return false;
    } else if (effectiveFileSaved && !sourceFileSaved) {
        // 高质量图仅裸图落盘, 没有添加水印
        executeForLowImage = false;
    } else if (!effectiveFileSaved && !sourceFileSaved) {
        // 高质量图还没有落盘
        executeForLowImage = true;
    }
    // 其余情况: 一律按照低质量图落盘处理
    MEDIA_INFO_LOG("effectiveFileSaved: %{public}d, sourceFileSaved: %{public}d, executeForLowImage: %{public}d.",
        effectiveFileSaved, sourceFileSaved, executeForLowImage);
    return true;
}

void ImagePipeline::SaveImageForStageInternal(const SaveCameraPhotoDto& dto)
{
    // ImagePipeline 仅添加水印时, 需要落盘
    auto assetInfo = GetAssetInfo();
    std::string assetPath = assetInfo.GetPath();
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, assetPath, editDataCameraPath);
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        MEDIA_INFO_LOG("No need SaveImageForStageInternal, editDataCameraPath not exist.");
        return;
    }

    MediaLibraryTracer tracer;
    tracer.Start("ImagePipeline::SaveImageForStageInternal");
    MEDIA_INFO_LOG("ImagePipeline::SaveImageForStageInternal.");

    // 基于文件状态判断, 是否高质量图已处理完。
    bool effectiveFileSaved = assetInfo.GetEffectiveFileSaved();
    bool sourceFileSaved = assetInfo.GetSourceFileSaved();
    bool executeForLowImage = true;
    if (!CheckFileSavedStatus(effectiveFileSaved, sourceFileSaved, executeForLowImage)) {
        return;
    }

    std::string tempSourcePath;
    std::string tempFiltersPath;
    int32_t ret = AddFiltersExecute(assetPath, executeForLowImage, tempSourcePath, tempFiltersPath);
    if (executeForLowImage) {
        // 处理低质量图添加水印, 存在一二阶段并发, 需要加锁
        ExecuteLowImageWithMutex(assetPath, ret, tempSourcePath, tempFiltersPath);
    } else {
        // 处理高质量图添加水印, 表示二阶段已执行完, 不用加锁, 直接落盘即可
        ExecuteImageWithoutMutex(assetPath, ret, tempSourcePath, tempFiltersPath);
    }
}

void ImagePipeline::ExecuteImageWithoutMutex(const std::string& path, int32_t errcode,
    const std::string& tempSourcePath, const std::string& tempFiltersPath)
{
    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);
    CameraPathUtils::SaveTemporaryImage(editDataSourcePath, tempSourcePath);

    if (errcode != E_OK) {
        // 添加水印失败: 原图落盘, 并且原图作为效果图
        MEDIA_ERR_LOG("Failed to AddFilters Image.");
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempFiltersPath),
            "Failed to delete tempFiltersPath, errno: %{public}d", errno);
        // copy(editDataSourcePath->path)
        MediaFileUtils::CopyFileSafe(editDataSourcePath, path);
        return;
    }
    // 添加水印成功: 双图落盘
    CameraPathUtils::SaveTemporaryImage(path, tempFiltersPath);
}

void ImagePipeline::ExecuteLowImageWithMutex(const std::string& path, int32_t& errcode, std::string& tempSourcePath,
    std::string& tempFiltersPath)
{
    std::unique_lock<mutex> locker(fileMutex_);
    
    int32_t ret = errcode;
    // 再次校验低质量图落盘场景, 防止出现覆盖高质量图的问题
    auto assetInfo = GetAssetInfo();
    bool effectiveFileSaved = assetInfo.GetEffectiveFileSaved();
    bool sourceFileSaved = assetInfo.GetSourceFileSaved();
    if (effectiveFileSaved && sourceFileSaved) {
        // 高质量图已落盘, 需要丢弃低质量图
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempSourcePath),
            "Failed to delete tempSourcePath, errno: %{public}d", errno);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempFiltersPath),
            "Failed to delete tempFiltersPath, errno: %{public}d", errno);
        return;
    } else if (effectiveFileSaved && !sourceFileSaved) {
        // 高质量图未添加水印落盘, 需要丢弃低质量图, 重新对高质量图添加水印
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempSourcePath),
            "Failed to delete tempSourcePath, errno: %{public}d", errno);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempFiltersPath),
            "Failed to delete tempFiltersPath, errno: %{public}d", errno);

        // 该接口耗时, 但此场景中, 一定不存在二阶段流程, 不会导致其他流程被锁住
        ret = AddFiltersExecute(path, false, tempSourcePath, tempFiltersPath);
    }
    // 其余status, 一律按照低质量图落盘处理
    ExecuteImageWithoutMutex(path, ret, tempSourcePath, tempFiltersPath);
    MEDIA_INFO_LOG("ExecuteLowImageWithMutex success, effectiveFileSaved: %{public}d, sourceFileSaved: %{public}d.",
        effectiveFileSaved, sourceFileSaved);
}

void ImagePipeline::ScanFileForStageInternal()
{
    // ImagePipeline 仅支持 path 生成缩略图, ImagePipeline 没有连拍场景
    MediaLibraryTracer tracer;
    tracer.Start("ImagePipeline::ScanFileForStageInternal");

    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();
    std::string path = assetInfo.GetPath();
    MEDIA_INFO_LOG("scan file start, assetInfo: %{public}s", assetInfo.ToString().c_str());

    MediaLibraryAssetOperations::ScanFile(path, false, true, true, fileId);
}

// 二阶段落盘
bool ImagePipeline::InitForOnProcessInternal(const OnProcessImageWrapper &wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("ImagePipeline::InitForOnProcessInternal");
    MEDIA_DEBUG_LOG("InitForOnProcessInternal enter");

    CHECK_AND_RETURN_RET_LOG(wrapper.image.IsValid(), false, "wrapper is inValid");
    image_ = std::move(wrapper.image);

    auto assetInfo = GetAssetInfo();
    auto metadata = wrapper.metadata;
    metadata.dfxMediaType = assetInfo.IsMovingPhoto() ? MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE
                                                      : MultiStagesCaptureMediaType::IMAGE;
    SetMediaDpsMetadata(std::move(metadata));
    MEDIA_INFO_LOG("InitForOnProcessInternal end.");
    return true;
}

bool ImagePipeline::CheckCanSaveDirectlyInternal(const std::shared_ptr<FileAsset> &fileAsset)
{
    MEDIA_INFO_LOG("ImagePipeline no need check is_temp.");
    return true;
}

int32_t ImagePipeline::ProcessMultistagesPhotoInternal(const std::shared_ptr<FileAsset> &fileAsset)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr.");

    MediaLibraryTracer tracer;
    tracer.Start("ImagePipeline::ProcessMultistagesPhotoInternal");
    MEDIA_DEBUG_LOG("ProcessMultistagesPhotoInternal enter");

    auto assetInfo = GetAssetInfo();
    std::string path = assetInfo.GetPath();

    std::string editDataSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);

    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    ImageFileMapper sourceImage = image_.file;
    if (isEdited) {
        // 图片编辑过了只替换低质量裸图, 此时不存在一、二阶段并发问题, 不用设置 OnProcessPackerStatus
        return FileUtils::SaveImage(editDataSourcePath, sourceImage.addr, sourceImage.bytes);
    }

    // 图片没编辑过
    std::string editDataCameraPath;
    CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, path, editDataCameraPath);
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        // 没有editdata_camera，只落盘在Photo目录
        return SaveOneImageForOnProcess(sourceImage, path);
    } else {
        // 有editdata_camera
        return SaveTwoImageForOnProcess(sourceImage, path);
    }
}

int32_t ImagePipeline::SaveOneImageForOnProcess(const ImageFileMapper& sourceImage, const std::string& path)
{
    CHECK_AND_RETURN_RET_LOG((sourceImage.addr != nullptr) && (sourceImage.bytes > 0), E_ERR,
        "addr is nullptr or bytes is zero.");
    
    MediaLibraryTracer tracer;
    tracer.Start("SaveOneImageForOnProcess");
    MEDIA_INFO_LOG("SaveOneImageForOnProcess, path: %{private}s.", path.c_str());
    // 优先落盘 high.jpg
    std::string tempHighPath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_PATH, path, tempHighPath);
    int32_t ret = FileUtils::SaveImage(tempHighPath, sourceImage.addr, sourceImage.bytes);

    // 有可能水印文件未落盘, 重新校验是否需要添加水印
    auto assetInfo = GetAssetInfo();
    auto effectStatusRecheck = assetInfo.GetTakeEffectStatus();
    if (effectStatusRecheck == TakeEffectStatus::UNDEFINED ||
        effectStatusRecheck == TakeEffectStatus::NO_NEED_TAKE_EFFECT) {
        // 如果不确定是否需要添加水印 或 明确不需要添加水印, 统一保存 OneImage
        {
            std::unique_lock<mutex> locker(fileMutex_);
            bool errCode = CameraPathUtils::SaveTemporaryImage(path, tempHighPath);
            CHECK_AND_RETURN_RET_LOG(errCode, E_ERR, "Failed to ProcessSaveOneImage, errno: %{public}d", errno);
            SetEffectiveFileSaved(true);
        }
        MEDIA_INFO_LOG("SaveOneImageForOnProcess success without filters, effectStatus: %{public}d.",
            static_cast<int32_t>(effectStatusRecheck));
        return E_OK;
    }

    // 需要对高质量图重新添加水印, 此时高质量图依然保存在 high.jpg
    AddFiltersForRecheckHighImage(path, tempHighPath);
    MEDIA_INFO_LOG("SaveOneImageForOnProcess success with filters.");
    return E_OK;
}

int32_t ImagePipeline::AddFiltersForRecheckHighImage(const std::string& path, const std::string& tempHighPath)
{
    MEDIA_INFO_LOG("Recheck SaveOneImageForOnProcess, need addFilters.");
    std::string tempSourcePath;
    std::string tempFiltersPath;
    // 100分原图 high.jpg 拷贝到 high_source.jpg、100分效果图: high_filters
    int32_t ret = AddFiltersExecute(tempHighPath, false, tempSourcePath, tempFiltersPath);
    {
        std::unique_lock<mutex> locker(fileMutex_);
        // rename 转正
        ExecuteImageWithoutMutex(path, ret, tempSourcePath, tempFiltersPath);
        if (ret != E_OK) {
            // 添加水印失败: 原图作为效果图
            CameraPathUtils::SaveTemporaryImage(path, tempHighPath);
        } else {
            // 添加水印成功: 清理 high.jpg
            CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempHighPath),
                "Failed to delete tempHighPath, errno: %{public}d", errno);
        }
        SetEffectiveFileSaved(true);
        SetSourceFileSaved(true);
    }
    return E_OK;
}

int32_t ImagePipeline::SaveTwoImageForOnProcess(const ImageFileMapper& sourceImage, const std::string& path)
{
    CHECK_AND_RETURN_RET_LOG((sourceImage.addr != nullptr) && (sourceImage.bytes > 0), E_ERR,
        "addr is nullptr or bytes is zero.");

    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::SaveTwoImageForOnProcess");
    MEDIA_INFO_LOG("SaveTwoImageForOnProcess, path: %{private}s", path.c_str());

    // 1.优先落盘在临时目录: high_source.jpg
    std::string tempSourcePath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_EDIT_DATA_SOURCE_PATH, path, tempSourcePath);
    int32_t ret = FileUtils::SaveImage(tempSourcePath, sourceImage.addr, sourceImage.bytes);

    // 2.基于 high_source.jpg 添加水印, 保存到 high_filters 临时文件
    std::string editData;
    CameraPathUtils::ReadEditdataCameraFromFile(path, true, editData);
    std::string tempFiltersPath;
    CameraPathUtils::GetCameraPath(CameraPathType::TEMP_HIGH_FILTERS_PATH, path, tempFiltersPath);
    ret = AddFiltersToPhoto(tempSourcePath, tempFiltersPath, editData);

    // 3.落盘: high_source.jpg -> source.jpg && high_filters -> photo
    {
        std::unique_lock<mutex> locker(fileMutex_);
        // rename 转正
        ExecuteImageWithoutMutex(path, ret, tempSourcePath, tempFiltersPath);
        if (ret != E_OK) {
            // 水印失败: 100分原图当前效果保存
            std::string editDataSourcePath;
            CameraPathUtils::GetCameraPath(CameraPathType::EDIT_DATA_SOURCE_PATH, path, editDataSourcePath);
            MediaFileUtils::CopyFileSafe(editDataSourcePath, path);
        }
        SetEffectiveFileSaved(true);
        SetSourceFileSaved(true);
    }
    MEDIA_INFO_LOG("SaveTwoImageForOnProcess success.");
    return E_OK;
}

void ImagePipeline::ScanFileForOnProcessInternal()
{
    auto assetInfo = GetAssetInfo();
    int32_t fileId = assetInfo.GetFileId();

    MEDIA_INFO_LOG("ScanFileForOnProcessInternal without picture.");
    MediaLibraryObjectUtils::ScanFileAsync(assetInfo.GetPath(), to_string(fileId),
        MediaLibraryApi::API_10, assetInfo.IsMovingPhoto(), nullptr, HighQualityScanFileCallback::Create(fileId));
}
} // Media
} // OHOS