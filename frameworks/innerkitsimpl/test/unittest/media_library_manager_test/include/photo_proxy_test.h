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

#ifndef PHOTO_PROXY_TEST_H
#define PHOTO_PROXY_TEST_H

#include <ctime>
#include <iostream>

#define private public
#include "media_photo_asset_proxy.h"
#undef private

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
        streamObj << std::setw(PADDING_WIDTH) << std::setfill(FILL_CHAR) << to_string(localTime->tm_mon + MONTH_GAP) <<
            to_string(localTime->tm_mday) << "_" << to_string(localTime->tm_hour) << to_string(localTime->tm_min) <<
            to_string(localTime->tm_sec);
        title_ = "IMG_" + to_string(localTime->tm_year + BASE_YEAR) + streamObj.str();

        std::ostringstream streamObj2;
        // 设置宽度并使用 '0' 填充未使用的位置
        streamObj2 << std::setw(PADDING_WIDTH) << std::setfill(FILL_CHAR) << to_string(localTime->tm_mon + MONTH_GAP) <<
            to_string(localTime->tm_mday) << to_string(localTime->tm_hour) << to_string(localTime->tm_min) <<
            to_string(localTime->tm_sec);
        photoId_ = to_string(localTime->tm_year + BASE_YEAR) + streamObj2.str();
    }

    std::string GetTitle() override
    {
        return title_;
    }

    // 图片后缀，例如：jpg/png
    std::string GetExtension() override
    {
        return "jpg";
    }

    // 分段式图片id
    std::string GetPhotoId() override
    {
        return photoId_;
    }

    // 分段式拍照类型，相机框架写入，通过媒体库直接透传回相机框架
    DeferredProcType GetDeferredProcType() override
    {
        return deferredProcType_;
    }

    void SetDeferredProcType(DeferredProcType deferredProcType)
    {
        deferredProcType_ = deferredProcType;
    }

    int32_t GetWidth() override
    {
        return WIDTH;
    }

    int32_t GetHeight() override
    {
        return HEIGHT;
    }

    void* GetFileDataAddr() override
    {
        return fileDataAddr_;
    }

    void SetFileDataAddr(void *addr)
    {
        fileDataAddr_ = addr;
    }

    size_t GetFileSize() override
    {
        return fileSize_;
    }

    void SetFileSize(int32_t size)
    {
        fileSize_ = size;
    }

    void Release() override
    {
    }

    // RGBA、JPG
    PhotoFormat GetFormat() override
    {
        return photoFormat_;
    }

    void SetFormat(PhotoFormat format)
    {
        photoFormat_ = format;
    }

    // 后续相机框架可能通过AddPhotoProxy传入高质量图
    PhotoQuality GetPhotoQuality() override
    {
        return photoQuality_;
    }

    void SetPhotoQuality(PhotoQuality quality)
    {
        photoQuality_ = quality;
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

    /**
     * @brief Get the Burst Key object
     * @return std::string 36 characters, like xxxxxxxx-xxxx-xxxx-xxxxxxxx-xxxx
     */
    std::string GetBurstKey() override
    {
        return this->burstKey_;
    }

    /**
     * @brief judge the photo is the cover photo or not
     *
     * @return true the photo is the cover photo
     * @return false the photo is not the cover photo
     */
    bool IsCoverPhoto() override
    {
        return this->isCoverPhoto_;
    }

    void SetIsCoverPhoto(bool isCoverPhoto)
    {
        this->isCoverPhoto_ = isCoverPhoto;
    }

    void SetBurstKey(std::string burstKey)
    {
        this->burstKey_ = burstKey;
    }

    void SetVideoEnhancementType(int32_t videoEnhancementType)
    {
        videoEnhancementType_ = videoEnhancementType;
    }

private:
    void *fileDataAddr_ = nullptr;
    int32_t fileSize_ = 0;
    std::string title_;
    std::string photoId_;
    PhotoFormat photoFormat_;
    PhotoQuality photoQuality_;
    DeferredProcType deferredProcType_ = DeferredProcType::OFFLINE;
    std::string burstKey_;
    bool isCoverPhoto_;
    int32_t videoEnhancementType_;
};
} // Media
} // OHOS
#endif // PHOTO_PROXY_TEST_H