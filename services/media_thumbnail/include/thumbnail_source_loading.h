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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SOURCE_LOADING_H
#define FRAMEWORKS_SERVICES_THUMBNAIL_SOURCE_LOADING_H

#include <unordered_map>

#include "image_packer.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "thumbnail_const.h"
#include "thumbnail_data.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

EXPORT std::string GetLocalThumbnailPath(const std::string &path, const std::string &key);
EXPORT std::string GetLocalKeyFrameThumbnailPath(const std::string &path, const std::string &key,
    const std::string &timeStamp);
EXPORT Size ConvertDecodeSize(ThumbnailData &data, const Size &sourceSize, Size &desiredSize);
EXPORT bool GenDecodeOpts(const Size &sourceSize, const Size &targetSize, DecodeOptions &decodeOpts);
EXPORT std::unique_ptr<ImageSource> LoadImageSource(const std::string &path, uint32_t &err);
EXPORT bool NeedAutoResize(const Size &size);
EXPORT int32_t ParseDesiredMinSide(const ThumbnailType &type);

const std::unordered_map<SourceState, std::string> STATE_NAME_MAP = {
    { SourceState::BEGIN, "BEGIN" },
    { SourceState::LOCAL_THUMB, "LOCAL_THUMB" },
    { SourceState::LOCAL_LCD, "LOCAL_LCD" },
    { SourceState::LOCAL_ORIGIN, "LOCAL_ORIGIN" },
    { SourceState::CLOUD_THUMB, "CLOUD_THUMB" },
    { SourceState::CLOUD_LCD, "CLOUD_LCD" },
    { SourceState::CLOUD_ORIGIN, "CLOUD_ORIGIN" },
    { SourceState::ERROR, "ERROR" },
    { SourceState::FINISH, "FINISH" },
};

class BeginSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error)
        { return ""; }
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize) { return false; }
};

class LocalThumbSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error);
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize);
};

class LocalLcdSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error);
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize);
};

class LocalOriginSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error);
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize);
};

class CloudThumbSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error);
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize);
};

class CloudLcdSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error);
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize);
};

class CloudOriginSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error);
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize);
};

class ErrorSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error)
        { return ""; }
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize) { return false; }
};

class FinishSource {
public:
    EXPORT static std::string GetSourcePath(ThumbnailData &data, int32_t &error)
        { return ""; }
    EXPORT static bool IsSizeLargeEnough(ThumbnailData &data, int32_t &minSize) { return false; }
};

struct StateFunc {
    std::string (*GetSourcePath)(ThumbnailData &data, int32_t &error);
    bool (*IsSizeLargeEnough)(ThumbnailData &data, int32_t &minSize);
};

const std::unordered_map<SourceState, StateFunc> STATE_FUNC_MAP = {
    { SourceState::BEGIN, { BeginSource::GetSourcePath, BeginSource::IsSizeLargeEnough } },
    { SourceState::LOCAL_THUMB, { LocalThumbSource::GetSourcePath, LocalThumbSource::IsSizeLargeEnough } },
    { SourceState::LOCAL_LCD, { LocalLcdSource::GetSourcePath, LocalLcdSource::IsSizeLargeEnough } },
    { SourceState::LOCAL_ORIGIN, { LocalOriginSource::GetSourcePath, LocalOriginSource::IsSizeLargeEnough } },
    { SourceState::CLOUD_THUMB, { CloudThumbSource::GetSourcePath, CloudThumbSource::IsSizeLargeEnough } },
    { SourceState::CLOUD_LCD, { CloudLcdSource::GetSourcePath, CloudLcdSource::IsSizeLargeEnough } },
    { SourceState::CLOUD_ORIGIN, { CloudOriginSource::GetSourcePath, CloudOriginSource::IsSizeLargeEnough } },
    { SourceState::ERROR, { ErrorSource::GetSourcePath, ErrorSource::IsSizeLargeEnough } },
    { SourceState::FINISH, { FinishSource::GetSourcePath, FinishSource::IsSizeLargeEnough } },
};

class EXPORT SourceLoader {
public:
    /*
     * Define source loading states sequence for creating thumbnails from local photo.
     */
    static const std::unordered_map<SourceState, SourceState> LOCAL_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for creating year&month thumbnails from local thumbnails.
     */
    static const std::unordered_map<SourceState, SourceState> LOCAL_THUMB_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for creating thumbnails from cloud photo.
     */
    static const std::unordered_map<SourceState, SourceState> CLOUD_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for creating thumbnails from cloud origin photo.
     */
    static const std::unordered_map<SourceState, SourceState> CLOUD_ORIGIN_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for creating thumbnails on demand.
     */
    static const std::unordered_map<SourceState, SourceState> ALL_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for creating cloud video thumbnails on demand.
     */
    static const std::unordered_map<SourceState, SourceState> ALL_SOURCE_LOADING_CLOUD_VIDEO_STATES;

    /*
     * Define source loading states sequence for creating thumbnails resolved cloud LCD.
     */
    static const std::unordered_map<SourceState, SourceState> CLOUD_LCD_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for creating thumbnails resolved local LCD.
     */
    static const std::unordered_map<SourceState, SourceState> LOCAL_LCD_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for upgrading thumbnails.
     */
    static const std::unordered_map<SourceState, SourceState> UPGRADE_SOURCE_LOADING_STATES;

    /*
     * Define source loading states sequence for upgrading video thumbnails.
     */
    static const std::unordered_map<SourceState, SourceState> UPGRADE_VIDEO_SOURCE_LOADING_STATES;

    SourceLoader(Size &desiredSize, ThumbnailData &data) : data_(data), desiredSize_(desiredSize)
    {
        GetSourcePath = nullptr;
        IsSizeLargeEnough = nullptr;
    };
    ~SourceLoader() = default;
    EXPORT bool RunLoading();

private:
    EXPORT void SetCurrentStateFunction();
    EXPORT bool IsSizeAcceptable();
    EXPORT bool CreateSourcePixelMap();
    EXPORT bool CreateImagePixelMap(const std::string &sourcePath);
    EXPORT bool CreateVideoFramePixelMap();
    EXPORT bool GeneratePictureSource(std::unique_ptr<ImageSource> &imageSource, const Size &targetSize);
    EXPORT bool GeneratePixelMapSource(std::unique_ptr<ImageSource> &imageSource, const Size &sourceSize,
        const Size &targetSize);
    EXPORT bool CreateSourceFromOriginalPhotoPicture();
    EXPORT bool CreateSourceWithWholeOriginalPicture();
    EXPORT bool CreateSourceWithOriginalPictureMainPixel();
    EXPORT bool IsLoadingOriginalSource();
    EXPORT bool SetImageInfo(const std::unique_ptr<ImageSource> &imageSource);
    EXPORT bool LoadHdrSource(const std::unique_ptr<ImageSource> &imageSource, const Size &targetSize);

    bool IsFinal();

    int32_t error_ { E_OK };
    std::string (*GetSourcePath)(ThumbnailData &data, int32_t &error);
    bool (*IsSizeLargeEnough)(ThumbnailData &data, int32_t &minSize);

    ThumbnailData &data_;
    Size &desiredSize_;
    SourceState state_ { SourceState::BEGIN };
    ImageInfo imageInfo_;
};

} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_SERVICES_THUMBNAIL_SOURCE_LOADING_H