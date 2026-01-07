/*
 * Copyright (C) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_PHOTO_PROXY_H
#define INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_PHOTO_PROXY_H

#include <refbase.h>
#include <string>

namespace OHOS {
namespace Media {
// 相机拍摄类型，由相机框架传入
enum class CameraShotType : int32_t {
    IMAGE  = 0, // 图片
    VIDEO, // 视频
    MOVING_PHOTO, // 动态照片
    BURST, // 连拍照片
    CINEMATIC_VIDEO, // 电影模式
};

enum class VideoType : int32_t {
    ORIGIN_VIDEO = 1,
    EFFECT_VIDEO = 2
};

enum class PhotoFormat : int32_t {
    RGBA = 0,
    JPG,
    MP4,
    HEIF,
    YUV,
    DNG,
};

enum class PhotoQuality : int32_t {
    HIGH = 0,
    LOW,
};

enum class DeferredProcType : int32_t {
    BACKGROUND = 0,
    OFFLINE,
};

class PhotoProxy : public RefBase {
public:
    PhotoProxy() {}
    virtual ~PhotoProxy() = default;

    virtual std::string GetTitle() = 0;
    virtual std::string GetExtension() = 0; // 图片后缀，例如：jpg/png
    virtual std::string GetPhotoId() = 0; // 分段式图片id
    virtual DeferredProcType GetDeferredProcType() = 0; // 分段式拍照类型，相机框架写入，通过媒体库直接透传回相机框架
    virtual int32_t GetWidth() = 0;
    virtual int32_t GetHeight() = 0;
    virtual void *GetFileDataAddr() = 0;
    virtual size_t GetFileSize() = 0;
    virtual PhotoFormat GetFormat() = 0; // RGBA、JPG
    virtual PhotoQuality GetPhotoQuality() = 0; // 后续相机框架可能通过AddPhotoProxy传入高质量图
    virtual std::string GetBurstKey() = 0; // 一组连拍照片一个key， 32byte uuid
    virtual bool IsCoverPhoto() = 0; // 设置封面，1表示封面
    virtual void Release() = 0;
    virtual double GetLatitude() = 0;
    virtual double GetLongitude() = 0;
    virtual int32_t GetShootingMode() = 0;
    virtual uint32_t GetCloudImageEnhanceFlag() = 0;
    virtual int32_t GetStageVideoTaskStatus() // 动态照片是否需要下发分段式视频任务，返回状态枚举值
    {
        return 0;
    }

    virtual int32_t GetVideoEnhancementType()
    {
        return 0;
    }
};
} // Media
} // OHOS
#endif // INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_PHOTO_PROXY_H