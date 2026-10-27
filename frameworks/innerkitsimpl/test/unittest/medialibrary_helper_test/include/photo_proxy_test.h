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

#ifndef MEDIA_LIBRAYR_HELPER_TEST_PHOTO_PROXY_TEST_H
#define MEDIA_LIBRAYR_HELPER_TEST_PHOTO_PROXY_TEST_H

#include <ctime>
#include <iostream>

#include "media_photo_asset_proxy.h"

namespace OHOS {
namespace Media {
const int32_t WIDTH = 3072;
const int32_t HEIGHT = 4096;
const int32_t PADDING_WIDTH = 2;
const char FILL_CHAR = '0';
const int32_t BASE_YEAR = 1900;
const int32_t MONTH_GAP = 1;

class PhotoProxyTest : public PhotoProxy {
public:
    PhotoProxyTest()
    {
        time_t nowTime = time(NULL);
        if (nowTime == -1) {
            return;
        }
        tm *localTime = localtime(&nowTime);
        if (localTime == nullptr) {
            return;
        }
        std::ostringstream streamObj;
        // 设置宽度并使用 '0' 填充未使用的位置
        streamObj << std::setw(PADDING_WIDTH) << std::setfill(FILL_CHAR) <<
            std::to_string(localTime->tm_mon + MONTH_GAP) << std::to_string(localTime->tm_mday) << "_" <<
            std::to_string(localTime->tm_hour) << std::to_string(localTime->tm_min) <<
            std::to_string(localTime->tm_sec);
        displayName_ = "IMG_" + std::to_string(localTime->tm_year + BASE_YEAR) + streamObj.str() + ".jpg";

        std::ostringstream streamObj2;
        // 设置宽度并使用 '0' 填充未使用的位置
        streamObj2 << std::setw(PADDING_WIDTH) << std::setfill(FILL_CHAR) <<
            std::to_string(localTime->tm_mon + MONTH_GAP) << std::to_string(localTime->tm_mday) <<
            std::to_string(localTime->tm_hour) << std::to_string(localTime->tm_min) <<
            std::to_string(localTime->tm_sec);
        photoId_ = std::to_string(localTime->tm_year + BASE_YEAR) + streamObj2.str();
    }

    virtual std::string GetDisplayName() override
    {
        return displayName_;
    }

    // 图片后缀，例如：jpg/png
    virtual std::string GetExtension() override
    {
        return "jpg";
    }

    // 分段式图片id
    virtual std::string GetPhotoId() override
    {
        return photoId_;
    }

    // 分段式拍照类型，相机框架写入，通过媒体库直接透传回相机框架
    virtual DeferredProcType GetDeferredProcType() override
    {
        return deferredProcType_;
    }

    void SetDeferredProcType(DeferredProcType deferredProcType)
    {
        deferredProcType_ = deferredProcType;
    }

    virtual int32_t GetWidth() override
    {
        return WIDTH;
    }

    virtual int32_t GetHeight() override
    {
        return HEIGHT;
    }

    virtual void* GetFileDataAddr() override
    {
        return fileDataAddr_;
    }

    void SetFileDataAddr(void *addr)
    {
        fileDataAddr_ = addr;
    }

    virtual size_t GetFileSize() override
    {
        return fileSize_;
    }

    void SetFileSize(int32_t size)
    {
        fileSize_ = size;
    }

    virtual void Release() override
    {
    }

    // RGBA、JPG、YUV
    virtual PhotoFormat GetFormat() override
    {
        return PhotoFormat::RGBA;
    }

    // 后续相机框架可能通过AddPhotoProxy传入高质量图
    virtual PhotoQuality GetPhotoQuality() override
    {
        return PhotoQuality::HIGH;
    }

    double GetLatitude() override
    {
        return 0.0;
    }

    double GetLongitude() override
    {
        return 0.0;
    }

    int32_t GetShootingMode() override
    {
        return 0;
    }

    uint32_t GetCloudImageEnhanceFlag() override
    {
        return 0;
    }

    LocalEnhancementAbilityType GetLocalEnhancementType() override
    {
        return LocalEnhancementAbilityType::NOT_SUPPORT;
    }
private:
    void *fileDataAddr_ = nullptr;
    int32_t fileSize_ = 0;
    std::string displayName_;
    std::string photoId_;
    DeferredProcType deferredProcType_ = DeferredProcType::OFFLINE;
};
} // Media
} // OHOS
#endif // MEDIA_LIBRAYR_HELPER_TEST_PHOTO_PROXY_TEST_H