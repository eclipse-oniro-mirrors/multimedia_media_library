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

#ifndef PHOTO_PROXY_TEST_H
#define PHOTO_PROXY_TEST_H

#include <ctime>
#include <iostream>

#include "photo_proxy.h"

namespace OHOS {
namespace Media {
const int32_t WIDTH = 3072;
const int32_t HEIGHT = 4096;
const int32_t PADDING_WIDTH = 2;
const char FILL_CHAR = '0';
const int32_t BASE_YEAR = 1900;
const int32_t MONTH_GAP = 1;

using namespace std;
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

    // 1、title
    std::string GetTitle() override
    {
        return title_;
    }

    // 2、extension
    std::string GetExtension() override
    {
        return extension_;
    }

    // 3、photoId
    std::string GetPhotoId() override
    {
        return photoId_;
    }

    // 4、DeferredProcType
    DeferredProcType GetDeferredProcType() override
    {
        return deferredProcType_;
    }

    // 5、width
    int32_t GetWidth() override
    {
        return WIDTH;
    }

    // 6、height
    int32_t GetHeight() override
    {
        return HEIGHT;
    }

    // 7、buffer data
    void* GetFileDataAddr() override
    {
        return fileDataAddr_;
    }

    // 8、buffer size
    size_t GetFileSize() override
    {
        return fileSize_;
    }

    // 9、format
    PhotoFormat GetFormat() override
    {
        return photoFormat_;
    }

    // 10、photo_quality
    PhotoQuality GetPhotoQuality() override
    {
        return photoQuality_;
    }

    // 11、burst_key
    std::string GetBurstKey() override
    {
        return burstKey_;
    }

    // 12、burst_cover_level
    bool IsCoverPhoto() override
    {
        return isCoverPhoto_;
    }

    // 13、release
    void Release() override
    {
    }

    // 14、latitude
    double GetLatitude() override
    {
        return 0.0;
    }

    // 15、longtitude
    double GetLongitude() override
    {
        return 0.0;
    }

    // 16、shooting_mode
    int32_t GetShootingMode() override
    {
        return 0;
    }

    // 17、cloud_image_enhance_flag
    uint32_t GetCloudImageEnhanceFlag() override
    {
        return cloudImageEnhanceFlag_;
    }

    // 18、stage_video_task_status
    int32_t GetStageVideoTaskStatus() override
    {
        return stageVideoTaskStatus_;
    }

    // 19、video_enhancement_type
    int32_t GetVideoEnhancementType() override
    {
        return videoEnhancementType_;
    }

public:
    std::string title_;
    std::string extension_;
    std::string photoId_;
    DeferredProcType deferredProcType_ = DeferredProcType::OFFLINE;

    void *fileDataAddr_ = nullptr;
    int32_t fileSize_ = 0;

    PhotoFormat photoFormat_ = PhotoFormat::RGBA;
    PhotoQuality photoQuality_ = PhotoQuality::LOW;
    std::string burstKey_;
    bool isCoverPhoto_ = true;

    uint32_t cloudImageEnhanceFlag_ = 0;
    int32_t stageVideoTaskStatus_ = 0;
    int32_t videoEnhancementType_ = 0;
};
} // Media
} // OHOS
#endif // PHOTO_PROXY_TEST_H