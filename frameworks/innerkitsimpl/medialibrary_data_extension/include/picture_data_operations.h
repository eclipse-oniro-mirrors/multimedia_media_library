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

#ifndef OHOS_PHOTO_PICTURE_DATA_OPERATIONS_H
#define OHOS_PHOTO_PICTURE_DATA_OPERATIONS_H

#include <iostream>
#include <sstream>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <list>
#include <thread>
#include "picture.h"
#include "media_log.h"
#include "medialibrary_async_worker.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum PictureType {
    LOW_QUALITY_PICTURE = 0,
    HIGH_QUALITY_PICTURE
};

class PicturePair : public RefBase {
public:
    std::shared_ptr<Media::Picture> picture_;
    std::string photoId_ = "default";
    time_t expireTime_;
    bool isCleanImmediately_ = true;
    bool isEdited_ = false;
    bool isTakeEffect_ = false;
    explicit PicturePair(std::shared_ptr<Media::Picture> picture, std::string photoId,
        time_t expireTime, bool isCleanImmediately, bool isEdited)
    {
        picture_ = std::move(picture);
        photoId_ = std::move(photoId);
        expireTime_ = expireTime;
        isCleanImmediately_ = isCleanImmediately;
        isEdited_ = isEdited;
        MEDIA_INFO_LOG("Constructor picturePair: %{public}s.", this->ToString().c_str());
    }

    PicturePair(const PicturePair& other)
    {
        picture_ = std::move(other.picture_);
        photoId_ = std::move(other.photoId_);
        expireTime_ = other.expireTime_;
        isCleanImmediately_ = other.isCleanImmediately_;
        isEdited_ = other.isEdited_;
        MEDIA_INFO_LOG("Copy constructor picturePair: %{public}s.", this->ToString().c_str());
    }

    PicturePair& operator=(const PicturePair& other)
    {
        if (this != &other) {
            picture_ = std::move(other.picture_);
            photoId_ = std::move(other.photoId_);
            expireTime_ = other.expireTime_;
            isCleanImmediately_ = other.isCleanImmediately_;
            isEdited_ = other.isEdited_;
        }
        MEDIA_INFO_LOG("Assignment constructor picturePair: %{public}s.", this->ToString().c_str());
        return *this;
    }

    ~PicturePair()
    {
        MEDIA_INFO_LOG("~PicturePair: %{public}s.", this->ToString().c_str());
        if (picture_) {
            picture_ = nullptr;
        }
    }

    std::string ToString()
    {
        std::stringstream ss;
        ss << "{"
           << "\"photoId\": " << this->photoId_
           << "; \"picture use\": " << static_cast<int32_t>(this->picture_.use_count())
           << "; \"picture point to addr\": " << std::to_string(reinterpret_cast<long long>(this->picture_.get()))
           << "}";
        return ss.str();
    }
    
    void SetTakeEffect(bool isTakeEffect)
    {
        isTakeEffect_ = isTakeEffect;
    }
};

class SavePictureData : public AsyncTaskData {
public:
    SavePictureData(sptr<PicturePair> picturePair) : picturePair_(picturePair){};
    ~SavePictureData() override = default;

    sptr<PicturePair> picturePair_;
};
class PictureDataOperations : public RefBase {
public:
    EXPORT explicit PictureDataOperations();
    ~PictureDataOperations();
    EXPORT void CleanDateForPeriodical();
    EXPORT void InsertPictureData(const std::string& imageId, sptr<PicturePair>& picturePair, PictureType pictureType);
    std::shared_ptr<Media::Picture> GetDataWithImageId(const std::string& imageId,
        bool &isHighQualityPicture, bool &isTakeEffect, bool isCleanImmediately = true);
    void DeleteDataWithImageId(const std::string& imageId, PictureType pictureType);
    bool IsExsitDataForPictureType(PictureType pictureType);
    bool IsExsitDataForPictureType(const std::string& imageId, PictureType pictureType);
    void SaveLowQualityPicture(const std::string& imageId = "default");
    void FinishAccessingPicture(const std::string& imageId, PictureType pictureType);
    void SavePictureWithImageId(const std::string& imageId);
    static void SavePictureExecutor(AsyncTaskData *data);
    int32_t AddSavePictureTask(sptr<PicturePair>& picturePair);
    int32_t GetPendingTaskSize();
    int32_t GetLowPendingTaskSize();

private:
    bool SavePicture(const std::string& imageId, std::map<std::string, sptr<PicturePair>>& pictureMap,
        bool isLowQualityPicture);
    void CleanHighQualityPictureDataInternal(const std::string& imageId, sptr<PicturePair>& picturePair,
        std::list<std::string>& pictureImageIdList);
    std::shared_ptr<Media::Picture> GetDataWithImageIdAndPictureType(const std::string& imageId,
        PictureType pictureType, bool &isTakeEffect, bool isCleanImmediately = true);
    void CleanPictureMapData(std::map<std::string, sptr<PicturePair>>& pictureMap, PictureType pictureType);

    const int MAX_PICTURE_CAPBILITY = 3;
    int max_capibilty = MAX_PICTURE_CAPBILITY;
    std::mutex pictureMapMutex_;
    std::map<std::string, sptr<PicturePair>> lowQualityPictureMap_;
    std::map<std::string, sptr<PicturePair>> highQualityPictureMap_;
    std::list<std::string> highQualityPictureImageId;
    std::list<std::string> pendingSavedPicturelist_;
    static int32_t taskSize;
}; // class PictureDataOperation
} // namespace Media
}  // namespace OHOS
#endif // OHOS_PHOTO_PICTURE_DATA_OPERATION_H