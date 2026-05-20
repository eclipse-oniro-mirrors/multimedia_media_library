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

#define MLOG_TAG "MediaCameraTestUtils"

#include "camera_test_utils.h"

#include <fcntl.h>
#include <fstream>

#include "camera_character_types.h"
#include "directory_ex.h"
#include "image_source.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_command.h"
#include "medialibrary_errno.h"
#define private public
#define protected public
#include "camera_asset_pipeline.h"
#include "mock_camera_pipeline.h"
#include "multistages_camera_capture_manager.h"
#undef protected
#undef private
#include "userfilemgr_uri.h"

#define O_RDONLY 00

namespace OHOS::Media {
static const std::string JPG_FILE_PATH = "/data/local/tmp/test_jpeg.jpg";

std::shared_ptr<Picture> CameraTestUtils::CreatePictureByPixelMap(
    const std::string& srcFormat, const std::string& srcPathName)
{
    uint32_t errorCode = -1;
    SourceOptions opts;
    opts.formatHint = srcFormat;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(srcPathName.c_str(), opts, errorCode);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "imageSource is nullptr.");

    DecodeOptions dstOpts;
    dstOpts.desiredPixelFormat = PixelFormat::NV21;
    std::shared_ptr<PixelMap> pixelMap = imageSource->CreatePixelMapEx(0, dstOpts, errorCode);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "pixelMap is nullptr.");

    std::unique_ptr<Picture> tmpPicture = Picture::Create(pixelMap);
    CHECK_AND_RETURN_RET_LOG(tmpPicture != nullptr, nullptr, "tmpPicture is nullptr.");
    std::shared_ptr<Picture> picture = std::move(tmpPicture);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, nullptr, "tmpPicture is nullptr.");
    return picture;
}

uint8_t* CameraTestUtils::CreateFileAddr(int32_t& bytes)
{
    MEDIA_INFO_LOG("CreateFileAddr begin: %{public}s.", JPG_FILE_PATH.c_str());

    CHECK_AND_RETURN_RET_LOG(!JPG_FILE_PATH.empty() && JPG_FILE_PATH.size() < PATH_MAX, nullptr, "File path too long.");
    std::string absFilePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(JPG_FILE_PATH, absFilePath), nullptr, "file is not real path.");
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), nullptr, "Failed to obtain the canonical path for source path.");

    int32_t srcFd = open(absFilePath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(srcFd > 0, nullptr, "Failed to open source file.");

    struct stat fileStat;
    if (fstat(srcFd, &fileStat) < 0) {
        close(srcFd);
        MEDIA_ERR_LOG("Failed to get file size");
        return nullptr;
    }
    bytes = fileStat.st_size;
    if (bytes == 0) {
        close(srcFd);
        MEDIA_ERR_LOG("File size is 0");
        return nullptr;
    }

    uint8_t* addr = new uint8_t[bytes];
    if (!addr) {
        MEDIA_ERR_LOG("Failed to allocate memory");
        close(srcFd);
        return nullptr;
    }

    read(srcFd, addr, bytes);
    close(srcFd);

    MEDIA_INFO_LOG("CreateFileAddr success: %{public}s.", JPG_FILE_PATH.c_str());
    return addr;
}

void CameraTestUtils::CreateFileAsset(const FileAssetInfo& info, FileAsset& fileAsset)
{
    fileAsset.SetId(info.fileId);
    fileAsset.SetPhotoId(info.photoId);
    fileAsset.SetPath(info.path);
    fileAsset.SetDisplayName(info.displayName);
    fileAsset.SetMimeType(info.mimeType);
    fileAsset.SetMediaType(info.mediaType);
    fileAsset.SetPhotoSubType(info.subtype);
    fileAsset.SetBurstCoverLevel(info.burstCoverLevel);
}

std::vector<CameraStandard::ImageFd> CameraTestUtils::CreateImageFdVec(bool needOriginFd)
{
    MEDIA_INFO_LOG("CreateImageFdVec begin.");

    int32_t bytes = 0;
    uint8_t* addrEdited = CreateFileAddr(bytes);
    CameraStandard::ImageFd imageFdEdited(nullptr, CameraStandard::EFFECTIVE_IMAGE, bytes);
    imageFdEdited.addr = addrEdited;

    if (needOriginFd) {
        uint8_t* addrOrigin = CreateFileAddr(bytes);
        CameraStandard::ImageFd imageFdOrigin(nullptr, CameraStandard::ORIGINAL_IMAGE, bytes);
        imageFdOrigin.addr = addrOrigin;
        return { imageFdEdited, imageFdOrigin };
    }

    return { imageFdEdited };
}

bool CameraTestUtils::InsertPipelineForConfirmType(const std::string& type, const FileAssetInfo& info)
{
    if (info.fileId <= 0) {
        MEDIA_ERR_LOG("fileId is invalid.");
        return false;
    }

    std::string uri = CONST_PAH_CREATE_PHOTO;
    MediaUriUtils::AppendKeyValue(uri, CAMERA_PIPELINE_TYPE, type);
    Uri createUri(uri);

    MediaLibraryCommand cmd(createUri);
    FileAsset fileAsset;
    CreateFileAsset(info, fileAsset);
    size_t count = MultistagesCameraCaptureManager::GetInstance().InsertCaptureData(cmd, fileAsset);
    if (count != 1) {
        MEDIA_ERR_LOG("count is %{public}zu.", count);
        return false;
    }
    return true;
}

bool CameraTestUtils::PrepareMockPipeline(const CameraPipelineType& pipelineType, const FileAssetInfo& assetInfo)
{
    auto mockPipeline = std::make_shared<MockCameraPipeline>();
    CHECK_AND_RETURN_RET_LOG(mockPipeline != nullptr, false, "mockPipeline is nullptr.");
    mockPipeline->SetPipelineType(pipelineType);

    FileAsset fileAsset;
    CameraTestUtils::CreateFileAsset(assetInfo, fileAsset);
    CameraAssetInfo cameraAssetInfo(fileAsset);
    mockPipeline->Init(cameraAssetInfo);

    size_t count = MultistagesCameraCaptureManager::GetInstance().InsertCaptureData(
        assetInfo.fileId, assetInfo.photoId, mockPipeline);
    if (count != 1) {
        MEDIA_ERR_LOG("count is %{public}zu.", count);
        return false;
    }
    return true;
}
} // namespace OHOS::Media