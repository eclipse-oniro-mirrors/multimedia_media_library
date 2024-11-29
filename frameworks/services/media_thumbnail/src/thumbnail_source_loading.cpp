/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "thumbnail_source_loading.h"

#include <fcntl.h>

#include "dfx_manager.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "image_source.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"
#include "post_proc.h"
#include "thumbnail_utils.h"
#include "thumbnail_const.h"

using namespace std;

namespace OHOS {
namespace Media {

const std::string LOCAL_MEDIA_PATH = "/storage/media/local/files/";

const std::unordered_map<SourceState, SourceState> SourceLoader::LOCAL_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::CLOUD_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::CLOUD_THUMB },
    { SourceState::CLOUD_THUMB, SourceState::CLOUD_LCD },
    { SourceState::CLOUD_LCD, SourceState::CLOUD_ORIGIN },
    { SourceState::CLOUD_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::ALL_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_THUMB },
    { SourceState::LOCAL_THUMB, SourceState::LOCAL_LCD },
    { SourceState::LOCAL_LCD, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::CLOUD_LCD },
    { SourceState::CLOUD_LCD, SourceState::CLOUD_ORIGIN },
    { SourceState::CLOUD_ORIGIN, SourceState::FINISH },
};

// For cloud video, generating thumbnail foreground in LOCAL_ORIGIN state may download its source video.
const std::unordered_map<SourceState, SourceState> SourceLoader::ALL_SOURCE_LOADING_CLOUD_VIDEO_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_THUMB },
    { SourceState::LOCAL_THUMB, SourceState::LOCAL_LCD },
    { SourceState::LOCAL_LCD, SourceState::CLOUD_LCD },
    { SourceState::CLOUD_LCD, SourceState::CLOUD_ORIGIN },
    { SourceState::CLOUD_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::CLOUD_LCD },
    { SourceState::CLOUD_LCD, SourceState::CLOUD_ORIGIN },
    { SourceState::CLOUD_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::LOCAL_LCD_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_LCD },
    { SourceState::LOCAL_LCD, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::UPGRADE_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::CLOUD_ORIGIN },
    { SourceState::CLOUD_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::UPGRADE_VIDEO_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::CLOUD_LCD },
    { SourceState::CLOUD_LCD, SourceState::FINISH },
};

std::string GetLocalThumbnailPath(const std::string &path, const std::string &key)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "") ? "" : "/" + key + ".jpg";
    return LOCAL_MEDIA_PATH + ((key == "") ? "" : ".thumbs/") + path.substr(ROOT_MEDIA_DIR.length()) + suffix;
}

std::string GetLcdExPath(const std::string &path)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = "/THM_EX/" + THUMBNAIL_LCD_SUFFIX + ".jpg";
    return ROOT_MEDIA_DIR + ".thumbs/" + path.substr(ROOT_MEDIA_DIR.length()) + suffix;
}

bool IsLocalSourceAvailable(const std::string& path)
{
    char tmpPath[PATH_MAX] = { 0 };
    if (realpath(path.c_str(), tmpPath) == nullptr) {
        // it's alright if source loading fails here, just move on to next source
        MEDIA_ERR_LOG("SourceLoader path to realPath is nullptr: errno: %{public}d, %{public}s",
            errno, DfxUtils::GetSafePath(path).c_str());
        return false;
    }

    FILE* filePtr = fopen(tmpPath, "rb");
    if (filePtr == nullptr) {
        MEDIA_ERR_LOG("SourceLoader open local file fail: errno: %{public}d, %{public}s",
            errno, DfxUtils::GetSafePath(path).c_str());
        return false;
    }
    if (fclose(filePtr) != E_OK) {
        MEDIA_ERR_LOG("SourceLoader close filePtr fail: errno: %{public}d, %{public}s",
            errno, DfxUtils::GetSafePath(path).c_str());
        return false;
    }
    return true;
}

bool IsCloudSourceAvailable(const std::string& path)
{
    string absFilePath;
    if (!PathToRealPath(path, absFilePath)) {
        MEDIA_ERR_LOG("Not real file path: errno: %{public}d, %{public}s", errno, DfxUtils::GetSafePath(path).c_str());
        return false;
    }

    int fd = open(absFilePath.c_str(), O_RDONLY);
    if (fd < 0) {
        MEDIA_ERR_LOG("open cloud file fail: errno: %{public}d, %{public}s",
            errno, DfxUtils::GetSafePath(path).c_str());
        return false;
    }
    close(fd);
    return true;
}

bool NeedAutoResize(const Size &size)
{
    // Only small thumbnails need to be scaled after decoding, others should resized while decoding.
    return size.width > SHORT_SIDE_THRESHOLD && size.height > SHORT_SIDE_THRESHOLD;
}

bool GenDecodeOpts(const Size &sourceSize, const Size &targetSize, DecodeOptions &decodeOpts)
{
    if (targetSize.width == 0) {
        MEDIA_ERR_LOG("Failed to generate decodeOpts, scale size contains zero");
        return false;
    }
    decodeOpts.desiredPixelFormat = PixelFormat::RGBA_8888;
    if (NeedAutoResize(targetSize)) {
        decodeOpts.desiredSize = targetSize;
        return true;
    }

    int32_t decodeScale = 1;
    int32_t scaleFactor = sourceSize.width / targetSize.width;
    while (scaleFactor /= DECODE_SCALE_BASE) {
        decodeScale *= DECODE_SCALE_BASE;
    }
    decodeOpts.desiredSize = {
        std::ceil(sourceSize.width / decodeScale),
        std::ceil(sourceSize.height / decodeScale),
    };
    return true;
}

Size ConvertDecodeSize(ThumbnailData &data, const Size &sourceSize, Size &desiredSize)
{
    int width = sourceSize.width;
    int height = sourceSize.height;
    if (!ThumbnailUtils::ResizeThumb(width, height)) {
        MEDIA_ERR_LOG("ResizeThumb failed");
        return {0, 0};
    }
    Size thumbDesiredSize = {width, height};
    data.thumbDesiredSize = thumbDesiredSize;
    float desiredScale = static_cast<float>(thumbDesiredSize.height) / static_cast<float>(thumbDesiredSize.width);
    float sourceScale = static_cast<float>(sourceSize.height) / static_cast<float>(sourceSize.width);
    float scale = 1.0f;
    if (sourceScale - desiredScale <= EPSILON) {
        scale = (float)thumbDesiredSize.height / sourceSize.height;
    } else {
        scale = (float)thumbDesiredSize.width / sourceSize.width;
    }
    scale = scale < 1.0f ? scale : 1.0f;
    Size thumbDecodeSize = {
        static_cast<int32_t> (scale * sourceSize.width),
        static_cast<int32_t> (scale * sourceSize.height),
    };

    width = sourceSize.width;
    height = sourceSize.height;
    if (!ThumbnailUtils::ResizeLcd(width, height)) {
        MEDIA_ERR_LOG("ResizeLcd failed");
        return {0, 0};
    }
    Size lcdDesiredSize = {width, height};
    data.lcdDesiredSize = lcdDesiredSize;

    int lcdMinSide = std::min(lcdDesiredSize.width, lcdDesiredSize.height);
    int thumbMinSide = std::min(thumbDesiredSize.width, thumbDesiredSize.height);
    Size lcdDecodeSize = lcdMinSide < thumbMinSide ? thumbDecodeSize : lcdDesiredSize;

    if (data.loaderOpts.decodeInThumbSize) {
        desiredSize = thumbDesiredSize;
        return thumbDecodeSize;
    } else if (data.needResizeLcd) {
        desiredSize = lcdDesiredSize;
        return lcdDecodeSize;
    } else {
        desiredSize = lcdDesiredSize;
        return lcdDesiredSize;
    }
}

void SwitchToNextState(ThumbnailData &data, SourceState &state)
{
    if (state == SourceState::FINISH) {
        MEDIA_INFO_LOG("SwitchToNextState current state is FINISH.");
        return;
    }
    auto iter = data.loaderOpts.loadingStates.find(state);
    if (iter == data.loaderOpts.loadingStates.end()) {
        MEDIA_WARN_LOG("SwitchToNextState find next state error, current state:%{public}d",
            static_cast<int32_t>(state));
        return;
    }
    state = iter->second;
}

unique_ptr<ImageSource> LoadImageSource(const std::string &path, uint32_t &err)
{
    MediaLibraryTracer tracer;
    tracer.Start("LoadImageSource");

    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, err);
    if (err != E_OK || !imageSource) {
        MEDIA_ERR_LOG("Failed to LoadImageSource, pixelmap path: %{public}s exists: %{public}d",
            path.c_str(), MediaFileUtils::IsFileExists(path));
        DfxManager::GetInstance()->HandleThumbnailError(path, DfxType::IMAGE_SOURCE_CREATE, err);
        return imageSource;
    }
    return imageSource;
}

bool SourceLoader::CreateVideoFramePixelMap()
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateVideoFramePixelMap");
    return ThumbnailUtils::LoadVideoFile(data_, desiredSize_);
}

void SourceLoader::SetCurrentStateFunction()
{
    StateFunc stateFunc = STATE_FUNC_MAP.at(state_);
    GetSourcePath = stateFunc.GetSourcePath;
    IsSizeLargeEnough = stateFunc.IsSizeLargeEnough;
}

bool SourceLoader::GeneratePictureSource(std::unique_ptr<ImageSource> &imageSource, const Size &targetSize)
{
    DecodingOptionsForPicture pictureOpts;
    pictureOpts.desireAuxiliaryPictures = {AuxiliaryPictureType::GAINMAP};
    uint32_t errorCode = ERR_OK;
    auto picturePtr = imageSource->CreatePicture(pictureOpts, errorCode);
    if (errorCode != E_OK || picturePtr == nullptr) {
        MEDIA_ERR_LOG("Failed to GeneratePictureSource, CreatePicture failed, err: %{public}d, path %{public}s",
            errorCode, DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    std::shared_ptr<Picture> picture = std::move(picturePtr);
    auto pixelMap = picture->GetMainPixel();
    auto gainMap = picture->GetGainmapPixelMap();
    if (pixelMap == nullptr) {
        MEDIA_ERR_LOG("Failed to GeneratePictureSource, main pixelMap is nullptr, path %{public}s",
            DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    if (gainMap == nullptr) {
        MEDIA_ERR_LOG("Failed to GeneratePictureSource, gainmap is nullptr, path %{public}s",
            DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    if (pixelMap->GetWidth() * pixelMap->GetHeight() == 0) {
        MEDIA_ERR_LOG("Failed to GeneratePictureSource, pixelMap size invalid, width: %{public}d, height: %{public}d,"
            "path %{public}s", pixelMap->GetWidth(), pixelMap->GetHeight(), DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    float widthScale = (1.0f * targetSize.width) / pixelMap->GetWidth();
    float heightScale = (1.0f * targetSize.height) / pixelMap->GetHeight();
    pixelMap->resize(widthScale, heightScale);
    gainMap->resize(widthScale, heightScale);
    data_.source.SetPicture(picture);
    return true;
}

bool SourceLoader::GeneratePixelMapSource(std::unique_ptr<ImageSource> &imageSource, const Size &sourceSize,
    const Size &targetSize)
{
    DecodeOptions decodeOpts;
    uint32_t err = ERR_OK;
    if (!GenDecodeOpts(sourceSize, targetSize, decodeOpts)) {
        MEDIA_ERR_LOG("SourceLoader failed to generate decodeOpts, pixelmap path %{public}s",
            DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    std::shared_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    if (err != E_OK || pixelMap == nullptr) {
        MEDIA_ERR_LOG("SourceLoader failed to CreatePixelMap, err: %{public}d, pixelmap path %{public}s",
            err, DfxUtils::GetSafePath(data_.path).c_str());
        DfxManager::GetInstance()->HandleThumbnailError(data_.path, DfxType::IMAGE_SOURCE_CREATE_PIXELMAP, err);
        return false;
    }
    if (!NeedAutoResize(targetSize) && !ThumbnailUtils::ScaleTargetPixelMap(pixelMap, targetSize,
        Media::AntiAliasingOption::HIGH)) {
        MEDIA_ERR_LOG("SourceLoader fail to scaleTarget, path %{public}s", DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    data_.source.SetPixelMap(pixelMap);
    return true;
}

bool SourceLoader::CreateImagePixelMap(const std::string &sourcePath)
{
    MediaLibraryTracer tracer;
    tracer.Start("SourceLoader CreateImagePixelMap");
    uint32_t err = 0;
    std::unique_ptr<ImageSource> imageSource = LoadImageSource(sourcePath, err);
    if (err != E_OK || imageSource == nullptr) {
        MEDIA_ERR_LOG("SourceLoader LoadImageSource error:%{public}d, errorno:%{public}d", err, errno);
        return false;
    }

    ImageInfo imageInfo;
    if (!IsSizeAcceptable(imageSource, imageInfo)) {
        MEDIA_ERR_LOG("SourceLoader CreateImagePixelMap size not acceptable, status:%{public}s, path:%{public}s",
            STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }

    Size targetSize = ConvertDecodeSize(data_, imageInfo.size, desiredSize_);
    Size sourceSize = imageInfo.size;

    // When encode picture, if mainPixel width or height is odd, hardware encode would fail.
    // For the safety of encode process, only those of even desiredSize are allowed to generate througth picture.
    bool shouldGeneratePicture = data_.loaderOpts.isHdr && imageSource->IsHdrImage() &&
        data_.lcdDesiredSize.width % 2 == 0 && data_.lcdDesiredSize.height % 2 == 0;
    bool isGenerateSucceed = shouldGeneratePicture ?
        GeneratePictureSource(imageSource, targetSize) : GeneratePixelMapSource(imageSource, sourceSize, targetSize);
    if (!isGenerateSucceed && shouldGeneratePicture) {
        MEDIA_ERR_LOG("SourceLoader generate picture source failed, generate pixelmap instead, path:%{public}s",
            DfxUtils::GetSafePath(data_.path).c_str());
        std::unique_ptr<ImageSource> retryImageSource = LoadImageSource(sourcePath, err);
        if (err != E_OK || retryImageSource == nullptr) {
            MEDIA_ERR_LOG("LoadImageSource retryImageSource error:%{public}d, errorno:%{public}d", err, errno);
            return false;
        }
        isGenerateSucceed = GeneratePixelMapSource(retryImageSource, sourceSize, targetSize);
    }
    if (!isGenerateSucceed) {
        MEDIA_ERR_LOG("ImagePixelMap generate failed, path:%{public}s", DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    tracer.Finish();

    if (data_.orientation == 0) {
        err = imageSource->GetImagePropertyInt(0, PHOTO_DATA_IMAGE_ORIENTATION, data_.orientation);
        if (err != E_OK) {
            MEDIA_ERR_LOG("SourceLoader Failed to get ImageProperty, path: %{public}s",
                DfxUtils::GetSafePath(data_.path).c_str());
        }
    }

    if (data_.mediaType == MEDIA_TYPE_VIDEO) {
        data_.orientation = 0;
    }
    DfxManager::GetInstance()->HandleHighMemoryThumbnail(data_.path, MEDIA_TYPE_IMAGE, imageInfo.size.width,
        imageInfo.size.height);
    return true;
}

bool SourceLoader::CreateSourcePixelMap()
{
    if (state_ == SourceState::LOCAL_ORIGIN && data_.mediaType == MEDIA_TYPE_VIDEO) {
        return CreateVideoFramePixelMap();
    }

    if (GetSourcePath == nullptr) {
        MEDIA_ERR_LOG("GetSourcePath is nullptr.");
        return false;
    }

    std::string sourcePath = GetSourcePath(data_, error_);
    if (sourcePath.empty()) {
        MEDIA_ERR_LOG("SourceLoader source path unavailable, status:%{public}s, path:%{public}s",
            STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    if (!CreateImagePixelMap(sourcePath)) {
        MEDIA_ERR_LOG("SourceLoader fail to create image pixel map, status:%{public}s, path:%{public}s",
            STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    return true;
}

bool SourceLoader::RunLoading()
{
    if (data_.loaderOpts.loadingStates.empty()) {
        MEDIA_ERR_LOG("source loading run loading failed, the given states is empty");
        return false;
    }
    state_ = SourceState::BEGIN;
    data_.source.ClearAllSource();
    SetCurrentStateFunction();

    // always check state not final after every state switch
    while (!IsFinal()) {
        SwitchToNextState(data_, state_);
        MEDIA_DEBUG_LOG("SourceLoader new cycle status:%{public}s", STATE_NAME_MAP.at(state_).c_str());
        if (IsFinal()) {
            break;
        }
        SetCurrentStateFunction();
        do {
            if (state_ < SourceState::CLOUD_THUMB) {
                data_.stats.sourceType = LoadSourceType::LOCAL_PHOTO;
            } else if (state_ >= SourceState::CLOUD_THUMB && state_ <= SourceState::CLOUD_ORIGIN) {
                data_.stats.sourceType = static_cast<LoadSourceType>(state_);
            }

            if (!CreateSourcePixelMap()) {
                MEDIA_ERR_LOG("SourceLoader fail to create source pixel map, status:%{public}s, path:%{public}s",
                    STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());
                break;
            }
            state_ = SourceState::FINISH;
            DfxManager::GetInstance()->HandleThumbnailGeneration(data_.stats);
        } while (0);
    };
    if (state_ == SourceState::ERROR) {
        data_.source.ClearAllSource();
    }
    if (data_.source.IsEmptySource()) {
        MEDIA_ERR_LOG("SourceLoader source is nullptr, status:%{public}s, path:%{public}s",
            STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    return true;
}

bool SourceLoader::IsSizeAcceptable(std::unique_ptr<ImageSource>& imageSource, ImageInfo& imageInfo)
{
    uint32_t err = imageSource->GetImageInfo(0, imageInfo);
    if (err != E_OK) {
        DfxManager::GetInstance()->HandleThumbnailError(data_.path, DfxType::IMAGE_SOURCE_GET_INFO, err);
        error_ = E_ERR;
        return false;
    }

    if (IsSizeLargeEnough == nullptr) {
        MEDIA_ERR_LOG("IsSizeLargeEnough is nullptr.");
        return false;
    }

    int32_t minSize = imageInfo.size.width < imageInfo.size.height ? imageInfo.size.width : imageInfo.size.height;
    if (!IsSizeLargeEnough(data_, minSize)) {
        MEDIA_DEBUG_LOG("SourceLoader size not acceptable, width:%{public}d, height:%{public}d", imageInfo.size.width,
            imageInfo.size.height);
        return false;
    }

    data_.stats.sourceWidth = imageInfo.size.width;
    data_.stats.sourceHeight = imageInfo.size.height;
    return true;
}

bool SourceLoader::IsFinal()
{
    if (error_ != E_OK) {
        state_ = SourceState::ERROR;
        data_.stats.errorCode = error_;
        return true;
    }
    if (state_ == SourceState::FINISH) {
        return true;
    }
    return false;
}

std::string LocalThumbSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    std::string tmpPath = GetLocalThumbnailPath(data.path, THUMBNAIL_THUMB_SUFFIX);
    if (!IsLocalSourceAvailable(tmpPath)) {
        return "";
    }
    return tmpPath;
}

bool LocalThumbSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    if (minSize < SHORT_SIDE_THRESHOLD) {
        return false;
    }
    return true;
}

std::string LocalLcdSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    std::string tmpPath = GetLocalThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX);
    if (!IsLocalSourceAvailable(tmpPath)) {
        return "";
    }
    return tmpPath;
}

bool LocalLcdSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    if (minSize < SHORT_SIDE_THRESHOLD) {
        return false;
    }
    return true;
}

std::string LocalOriginSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    std::string tmpPath = GetLocalThumbnailPath(data.path, "");
    if (!IsLocalSourceAvailable(tmpPath)) {
        return "";
    }
    return tmpPath;
}

bool LocalOriginSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    return true;
}

std::string CloudThumbSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    std::string tmpPath = GetThumbnailPath(data.path, THUMBNAIL_THUMB_SUFFIX);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (data.orientation != 0) {
        return "";
    }
    int32_t totalCost = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    data.stats.openThumbCost = totalCost;
    return tmpPath;
}

bool CloudThumbSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    if (minSize < SHORT_SIDE_THRESHOLD) {
        return false;
    }
    return true;
}

std::string CloudLcdSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    std::string tmpPath;
    if (data.orientation != 0) {
        tmpPath = GetLcdExPath(data.path);
    } else {
        tmpPath = GetThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX);
    }
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (!IsCloudSourceAvailable(tmpPath)) {
        return "";
    }
    int32_t totalCost = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    data.stats.openLcdCost = totalCost;
    return tmpPath;
}

bool CloudLcdSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    if (data.mediaType == MEDIA_TYPE_VIDEO) {
        return true;
    }
    int photoShorterSide = data.photoWidth < data.photoHeight ? data.photoWidth : data.photoHeight;
    if (photoShorterSide != 0 && photoShorterSide < SHORT_SIDE_THRESHOLD) {
        return true;
    }
    if (minSize < SHORT_SIDE_THRESHOLD) {
        return false;
    }
    return true;
}

std::string CloudOriginSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    if (data.mediaType == MEDIA_TYPE_VIDEO) {
        // avoid opening cloud origin video file.
        MEDIA_ERR_LOG("avoid opening cloud origin video file path:%{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        return "";
    }
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (!IsCloudSourceAvailable(data.path)) {
        return "";
    }
    int32_t totalCost = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    data.stats.openOriginCost = totalCost;
    return data.path;
}

bool CloudOriginSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    return true;
}

} // namespace Media
} // namespace OHOS