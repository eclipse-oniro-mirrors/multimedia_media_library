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

#include <charconv>
#include <fcntl.h>

#include "dfx_manager.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "image_source.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_image_framework_utils.h"
#include "medialibrary_tracer.h"
#include "post_proc.h"
#include "thumbnail_image_framework_utils.h"
#include "thumbnail_utils.h"
#include "thumbnail_const.h"
#include "lake_file_utils.h"

using namespace std;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
const std::string LOCAL_MEDIA_PATH = "/storage/media/local/files/";

const std::unordered_map<SourceState, SourceState> SourceLoader::LOCAL_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::LOCAL_THUMB_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::LOCAL_THUMB },
    { SourceState::LOCAL_THUMB, SourceState::LOCAL_LCD },
    { SourceState::LOCAL_LCD, SourceState::LOCAL_ORIGIN },
    { SourceState::LOCAL_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::CLOUD_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::CLOUD_THUMB },
    { SourceState::CLOUD_THUMB, SourceState::CLOUD_LCD },
    { SourceState::CLOUD_LCD, SourceState::CLOUD_ORIGIN },
    { SourceState::CLOUD_ORIGIN, SourceState::FINISH },
};

const std::unordered_map<SourceState, SourceState> SourceLoader::CLOUD_ORIGIN_SOURCE_LOADING_STATES = {
    { SourceState::BEGIN, SourceState::CLOUD_ORIGIN },
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

std::string GetLocalKeyFrameThumbnailPath(const std::string &path, const std::string &key, const std::string &timeStamp)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "") ? "" : "/" + key + ".jpg";
    return LOCAL_MEDIA_PATH + ((key == "") ? "" : ".thumbs/") + path.substr(ROOT_MEDIA_DIR.length()) +
        "/beginTimeStamp" + timeStamp + "/" + suffix;
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
    Size thumbDecodeSize = thumbDesiredSize;
    if (sourceScale - desiredScale <= EPSILON) {
        scale = min((float)thumbDesiredSize.height / sourceSize.height, 1.0f);
        thumbDecodeSize.width = static_cast<int32_t> (scale * sourceSize.width);
    } else {
        scale = min((float)thumbDesiredSize.width / sourceSize.width, 1.0f);
        thumbDecodeSize.height = static_cast<int32_t> (scale * sourceSize.height);
    }

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

int32_t ParseDesiredMinSide(const ThumbnailType &type)
{
    switch (type) {
        case ThumbnailType::THUMB:
            return SHORT_SIDE_THRESHOLD;
            break;
        case ThumbnailType::MTH_ASTC:
            return MONTH_SHORT_SIDE_THRESHOLD;
            break;
        default:
            break;
    }
    return std::numeric_limits<int32_t>::max();
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

int64_t SafeStoll(const std::string &str)
{
    int64_t value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc()) {
        MEDIA_ERR_LOG("Failed to convert string to int64_t: %{public}s", str.c_str());
        return 0;
    }
    return value;
}

bool SourceLoader::CreateVideoFramePixelMap()
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateVideoFramePixelMap");
    int64_t timeStamp = AV_FRAME_TIME;
    if (!data_.tracks.empty()) {
        timeStamp = SafeStoll(data_.timeStamp) * MS_TRANSFER_US;
    }
    if (state_ == SourceState::CLOUD_ORIGIN && timeStamp != AV_FRAME_TIME) {
        MEDIA_ERR_LOG("Avoid reading specific frame from cloud video, path %{public}s",
            DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    return ThumbnailUtils::LoadVideoFrame(data_, desiredSize_, timeStamp);
}

void SourceLoader::SetCurrentStateFunction()
{
    StateFunc stateFunc = STATE_FUNC_MAP.at(state_);
    GetSourcePath = stateFunc.GetSourcePath;
    IsSizeLargeEnough = stateFunc.IsSizeLargeEnough;
}

bool SourceLoader::SetImageInfo(const unique_ptr<ImageSource> &imageSource)
{
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, false, "ImageSource is null");
    uint32_t err = imageSource->GetImageInfo(0, imageInfo_);
    CHECK_AND_EXECUTE(err == E_OK, {
        MEDIA_ERR_LOG("ImageSource::GetImageInfo err: %{public}d", err);
        DfxManager::GetInstance()->HandleThumbnailError(data_.path, DfxType::IMAGE_SOURCE_GET_INFO, err);
        error_ = E_ERR;
        return false;
    });
    return true;
}

bool SourceLoader::LoadHdrSource(const unique_ptr<ImageSource> &imageSource, const Size &targetSize)
{
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, false, "ImageSource is null");
    CHECK_AND_RETURN_RET_LOG(targetSize.width > 0 && targetSize.height > 0, false,
        "Invalid target size, weith: %{public}d height: %{public}d", targetSize.width, targetSize.height);
    ImageHdrType hdrType = imageSource->CheckHdrType();
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::IsAdaptedHdrType(hdrType),
        false, "Not adapted hdr type: %{public}d", hdrType);
    if (ThumbnailImageFrameWorkUtils::IsSinglePixelMapHdrType(hdrType)) {
        MEDIA_INFO_LOG("Single pixel map hdr type: %{public}d", hdrType);
        uint32_t err = E_OK;
        DecodeOptions decodeOpts;
        decodeOpts.desiredDynamicRange = DecodeDynamicRange::HDR;
        decodeOpts.desiredSize = targetSize;
        decodeOpts.desiredPixelFormat = PixelFormat::RGBA_1010102;
        auto ret = ThumbnailImageFrameWorkUtils::CreatePixelMap(imageSource, decodeOpts, err);
        CHECK_AND_EXECUTE(ret != nullptr, {
            MEDIA_ERR_LOG("Create hdr pixel map failed");
            DfxManager::GetInstance()->HandleThumbnailError(data_.path, DfxType::IMAGE_SOURCE_CREATE_PIXELMAP, err);
            return false;
        });
        data_.source.SetPixelMap(ret);
        return true;
    }
    auto ret = ThumbnailImageFrameWorkUtils::CreatePicture(imageSource, targetSize);
    CHECK_AND_RETURN_RET_LOG(ret != nullptr, false, "Create picture failed");
    data_.source.SetPicture(ret);
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

bool SourceLoader::IsLoadingOriginalSource()
{
    return state_ == SourceState::LOCAL_ORIGIN || state_ == SourceState::CLOUD_ORIGIN;
}

bool SourceLoader::CreateImagePixelMap(const std::string &sourcePath)
{
    MediaLibraryTracer tracer;
    tracer.Start("SourceLoader CreateImagePixelMap");
    uint32_t err = 0;
    std::unique_ptr<ImageSource> imageSource = LoadImageSource(sourcePath, err);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "LoadImageSource err: %{public}d", err);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, false, "LoadImageSource err: imageSource is nullptr");

    CHECK_AND_RETURN_RET_LOG(SetImageInfo(imageSource), false, "Set image info faild");

    CHECK_AND_RETURN_RET_LOG(IsSizeAcceptable(), false, "Size not acceptable, status:%{public}s, path:%{public}s",
        STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());

    Size targetSize = ConvertDecodeSize(data_, imageInfo_.size, desiredSize_);
    Size sourceSize = imageInfo_.size;

    if (data_.loaderOpts.isHdr && imageSource->IsHdrImage()) {
        CHECK_AND_RETURN_RET_LOG(LoadHdrSource(imageSource, targetSize), false, "LoadHdrSource failed");
    } else {
        CHECK_AND_RETURN_RET_LOG(GeneratePixelMapSource(imageSource, sourceSize, targetSize),
            false, "Create sdr pixel map failed");
    }
    tracer.Finish();

    if (data_.exifRotate < 0 && IsLoadingOriginalSource()) {
        data_.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
        CHECK_AND_WARN_LOG(MediaImageFrameWorkUtils::GetExifRotate(imageSource, data_.exifRotate) == E_OK,
            "SourceLoader Failed to get exif rotate, path: %{public}s", DfxUtils::GetSafePath(data_.path).c_str());
    }
    DfxManager::GetInstance()->HandleHighMemoryThumbnail(data_.path, MEDIA_TYPE_IMAGE, imageInfo_.size.width,
        imageInfo_.size.height);
    return true;
}

bool SourceLoader::CreateSourcePixelMap()
{
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

    CHECK_AND_RETURN_RET(!((state_ == SourceState::LOCAL_ORIGIN || state_ == SourceState::CLOUD_ORIGIN) &&
        data_.mediaType == MEDIA_TYPE_VIDEO), CreateVideoFramePixelMap());

    if (!CreateImagePixelMap(sourcePath)) {
        MEDIA_ERR_LOG("SourceLoader fail to create image pixel map, status:%{public}s, path:%{public}s",
            STATE_NAME_MAP.at(state_).c_str(), DfxUtils::GetSafePath(data_.path).c_str());
        return false;
    }
    return true;
}

bool SourceLoader::CreateSourceFromOriginalPhotoPicture()
{
    CHECK_AND_RETURN_RET_LOG(data_.originalPhotoPicture != nullptr, false, "OriginalPhotoPicture is nullptr");
    if (data_.loaderOpts.decodeInThumbSize ||
        !ThumbnailImageFrameWorkUtils::IsSupportCopyPixelMap(data_.originalPhotoPicture->GetMainPixel())) {
        data_.originalPhotoPicture = nullptr;
        return false;
    }

    int32_t exifRotate = 0;
    int32_t err = MediaImageFrameWorkUtils::GetExifRotate(data_.originalPhotoPicture, exifRotate);
    CHECK_AND_WARN_LOG(err == E_OK, "SourceLoader Failed to get picture orientation, path: %{public}s",
        DfxUtils::GetSafePath(data_.path).c_str());

    bool isCreateSource = false;
    if (data_.originalPhotoPicture->GetGainmapPixelMap() == nullptr) {
        isCreateSource = CreateSourceWithOriginalPictureMainPixel();
    } else {
        isCreateSource = CreateSourceWithWholeOriginalPicture();
    }
    CHECK_AND_RETURN_RET_LOG(isCreateSource, false, "Create source failed");
    CHECK_AND_EXECUTE(!(data_.exifRotate < 0 && exifRotate > 0), data_.exifRotate = exifRotate);
    data_.lastLoadSource = SourceState::LOCAL_ORIGIN;
    return true;
}

bool SourceLoader::CreateSourceWithWholeOriginalPicture()
{
    CHECK_AND_RETURN_RET_LOG(data_.originalPhotoPicture != nullptr, false, "OriginalPhotoPicture is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("CreateSourceWithWholeOriginalPicture");

    std::shared_ptr<Picture> picture = ThumbnailImageFrameWorkUtils::CopyPictureSource(data_.originalPhotoPicture);
    data_.originalPhotoPicture = nullptr;
    CHECK_AND_RETURN_RET_LOG(picture != nullptr, false, "CopyPictureSource failed");
    auto pixelMap = picture->GetMainPixel();
    auto gainMap = picture->GetGainmapPixelMap();
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr && gainMap != nullptr, false,
        "PixelMap or gainMap is nullptr");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetWidth() * pixelMap->GetHeight() != 0, false,
        "Picture is invalid");

    Size originSize = { .width = pixelMap->GetWidth(),
                        .height = pixelMap->GetHeight() };
    Size targetSize = ConvertDecodeSize(data_, originSize, desiredSize_);

    float widthScale = (1.0f * targetSize.width) / pixelMap->GetWidth();
    float heightScale = (1.0f * targetSize.height) / pixelMap->GetHeight();
    pixelMap->resize(widthScale, heightScale);
    gainMap->resize(widthScale, heightScale);
    data_.source.SetPicture(picture);

    DfxManager::GetInstance()->HandleHighMemoryThumbnail(data_.path, data_.mediaType, originSize.width,
        originSize.height);
    return true;
}

bool SourceLoader::CreateSourceWithOriginalPictureMainPixel()
{
    CHECK_AND_RETURN_RET_LOG(data_.originalPhotoPicture != nullptr, false, "OriginalPhotoPicture is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("CreateSourceWithOriginalPictureMainPixel");
    auto mainPixel = data_.originalPhotoPicture->GetMainPixel();
    CHECK_AND_RETURN_RET_LOG(mainPixel != nullptr, false, "Main pixel is nullptr");
    MEDIA_INFO_LOG("Main pixelMap format:%{public}d isHdr:%{public}d allocatorType:%{public}d, "
        "size: %{public}d * %{public}d", mainPixel->GetPixelFormat(), mainPixel->IsHdr(),
        mainPixel->GetAllocatorType(), mainPixel->GetWidth(), mainPixel->GetHeight());

    std::shared_ptr<PixelMap> pixelMap = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(mainPixel);
    mainPixel = nullptr;
    data_.originalPhotoPicture = nullptr;
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, false, "Copy main pixel failed");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetWidth() * pixelMap->GetHeight() != 0, false,
        "PixelMap is invalid");

    Size originSize = { .width = pixelMap->GetWidth(),
                        .height = pixelMap->GetHeight() };
    Size targetSize = ConvertDecodeSize(data_, originSize, desiredSize_);

    float widthScale = (1.0f * targetSize.width) / pixelMap->GetWidth();
    float heightScale = (1.0f * targetSize.height) / pixelMap->GetHeight();
    pixelMap->resize(widthScale, heightScale);
    data_.source.SetPixelMap(pixelMap);

    DfxManager::GetInstance()->HandleHighMemoryThumbnail(data_.path, data_.mediaType, originSize.width,
        originSize.height);
    return true;
}

bool SourceLoader::RunLoading()
{
    if (data_.originalPhotoPicture != nullptr && CreateSourceFromOriginalPhotoPicture()) {
        return true;
    }

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
            data_.lastLoadSource = state_;
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

bool SourceLoader::IsSizeAcceptable()
{
    if (IsSizeLargeEnough == nullptr) {
        MEDIA_ERR_LOG("IsSizeLargeEnough is nullptr.");
        return false;
    }

    int32_t minSize = imageInfo_.size.width < imageInfo_.size.height ? imageInfo_.size.width : imageInfo_.size.height;
    if (!IsSizeLargeEnough(data_, minSize)) {
        MEDIA_DEBUG_LOG("SourceLoader size not acceptable, width:%{public}d, height:%{public}d", imageInfo_.size.width,
            imageInfo_.size.height);
        return false;
    }

    data_.stats.sourceWidth = imageInfo_.size.width;
    data_.stats.sourceHeight = imageInfo_.size.height;
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
    CHECK_AND_RETURN_RET_WARN_LOG(!(ThumbnailUtils::IsImageWithRotate(data) && data.needGenerateExThumbnail),
        "", "Need generate Ex thumbnail, can not use rotated thumbnail");
    CHECK_AND_RETURN_RET_WARN_LOG(data.loaderOpts.decodeInThumbSize, "", "Can not use thumb");
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
    CHECK_AND_RETURN_RET_WARN_LOG(!(ThumbnailUtils::IsImageWithRotate(data) && data.needGenerateExThumbnail),
        "", "Need generate Ex thumbnail, can not use rotated thumbnail");
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
    std::string tmpPath = LakeFileUtils::GetAssetRealPath(data.path);
    if (tmpPath.find(ROOT_MEDIA_DIR) != std::string::npos) {
        tmpPath = GetLocalThumbnailPath(tmpPath, "");
    }
    MEDIA_DEBUG_LOG("File path is %{private}s", tmpPath.c_str());
    if (!IsLocalSourceAvailable(tmpPath)) {
        MEDIA_ERR_LOG("IsLocalSourceAvailable failed, path:%{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
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
    CHECK_AND_RETURN_RET_WARN_LOG(data.loaderOpts.decodeInThumbSize, "", "Can not use thumb");
    std::string tmpPath = ThumbnailUtils::IsExCloudThumbnail(data) ?
        GetThumbnailPath(data.path, THUMBNAIL_THUMB_EX_SUFFIX) : GetThumbnailPath(data.path, THUMBNAIL_THUMB_SUFFIX);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (!IsCloudSourceAvailable(tmpPath)) {
        return "";
    }
    int32_t totalCost = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    data.stats.openThumbCost = totalCost;
    return tmpPath;
}

bool CloudThumbSource::IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize)
{
    int minDesiredSide = ParseDesiredMinSide(data.loaderOpts.desiredType);
    if (minSize < minDesiredSide) {
        return false;
    }
    return true;
}

std::string CloudLcdSource::GetSourcePath(ThumbnailData &data, int32_t &error)
{
    std::string tmpPath;
    if (ThumbnailUtils::IsExCloudThumbnail(data)) {
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
        MEDIA_ERR_LOG("Opening cloud origin video file, path:%{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        return data.path;
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
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS