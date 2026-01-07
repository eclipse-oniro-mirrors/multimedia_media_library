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

#define MLOG_TAG "PictureDataOperations"

#include "picture_data_operations.h"

#include "file_utils.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_unistore_manager.h"
#include "parameter.h"
#include "parameters.h"
#include "result_set_utils.h"
#include "medialibrary_tracer.h"

using namespace std;
namespace OHOS {
namespace Media {
int32_t PictureDataOperations::taskSize = 0;
const int32_t SAVE_PICTURE_TIMEOUT_SEC = 20;

PictureDataOperations::PictureDataOperations() {}

PictureDataOperations::~PictureDataOperations()
{
    lowQualityPictureMap_.clear();
    highQualityPictureMap_.clear();
    highQualityPictureImageId.clear();
}

static int32_t IsPictureTempAndEdited(const string &photoId, bool &isTemp, bool &isEdited)
{
    MediaLibraryTracer tracer;
    tracer.Start("IsPictureTempAndEdited " + photoId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "Failed to get rdbStore");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_ID, photoId);
    vector<string> columns { PhotoColumn::PHOTO_IS_TEMP, PhotoColumn::PHOTO_EDIT_TIME };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_ERR, "resultSet is empty");
    isTemp = (GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet) == 1);
    isEdited = (GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet) > 0);
    return E_OK;
}

void PictureDataOperations::CleanPictureMapData(std::map<std::string, sptr<PicturePair>>& pictureMap,
    PictureType pictureType)
{
    MEDIA_INFO_LOG("enter CleanPictureMapData, pictureMap size: %{public}d, pictureType: %{public}d",
        static_cast<int32_t>(pictureMap.size()), static_cast<int32_t>(pictureType));

    auto iter = pictureMap.begin();
    while (iter != pictureMap.end()) {
        time_t now = time(nullptr);
        bool isNeedDeletePicture = ((iter->second)->expireTime_ < now) && ((iter->second)->isCleanImmediately_);
        if (isNeedDeletePicture || ((iter->second)->expireTime_ + SAVE_PICTURE_TIMEOUT_SEC) < now) {
            bool isTemp = false;
            bool isEdited = false;
            IsPictureTempAndEdited(iter->first, isTemp, isEdited);
            bool isLowQualityPicture = (pictureType != HIGH_QUALITY_PICTURE);
            if (isTemp) {
                FileUtils::SavePicture(iter->first, (iter->second)->picture_, isEdited, isLowQualityPicture);
                MEDIA_INFO_LOG("end SavePicture, photoId: %{public}s, isEdited: %{public}d",
                    (iter->first).c_str(), static_cast<int32_t>(isEdited));
            }
            MEDIA_INFO_LOG("enter CleanDateByPictureMap %{public}s enter", (iter->first).c_str());
            iter->second = nullptr;
            iter = pictureMap.erase(iter);
        } else {
            iter++;
        }
    }
    MEDIA_INFO_LOG("end CleanPictureMapData, pictureMap size: %{public}d, pictureType: %{public}d",
        static_cast<int32_t>(pictureMap.size()), static_cast<int32_t>(pictureType));
}

void PictureDataOperations::CleanDateForPeriodical()
{
    MEDIA_INFO_LOG("enter CleanDateForPeriodical.");
    lock_guard<mutex> lock(pictureMapMutex_);
    enum PictureType pictureType;
    for (pictureType = LOW_QUALITY_PICTURE; pictureType <= HIGH_QUALITY_PICTURE;
        pictureType = (PictureType)(pictureType + 1)) {
        switch (pictureType) {
            case LOW_QUALITY_PICTURE:
                CleanPictureMapData(lowQualityPictureMap_, pictureType);
                break;
            case HIGH_QUALITY_PICTURE:
                CleanPictureMapData(highQualityPictureMap_, pictureType);
                break;
            default:
                break;
        }
    }
    MEDIA_INFO_LOG("end CleanDateForPeriodical.");
}

void PictureDataOperations::InsertPictureData(const std::string& imageId, sptr<PicturePair>& picturePair,
    PictureType pictureType)
{
    MEDIA_INFO_LOG("enter InsertPictureData, imageId: %{public}s, pictureType: %{public}d", imageId.c_str(),
        static_cast<int32_t>(pictureType));
    switch (pictureType) {
        case LOW_QUALITY_PICTURE:{
            lock_guard<mutex>  lock(pictureMapMutex_);
            auto iter = lowQualityPictureMap_.find(imageId);
            if (iter != lowQualityPictureMap_.end()) {
                iter->second = nullptr;
                lowQualityPictureMap_.erase(iter);
            }
            lowQualityPictureMap_[imageId] = picturePair;
            }
            break;
        case HIGH_QUALITY_PICTURE:
            if (highQualityPictureMap_.find(imageId) ==  highQualityPictureMap_.end()) {
                highQualityPictureImageId.push_back(imageId);
            }
            CleanHighQualityPictureDataInternal(imageId, picturePair, highQualityPictureImageId);
            break;
        default:
            break;
    }

    MEDIA_INFO_LOG("end InsertPictureData, lowQualityPictureMap: %{public}d, highQualityPictureMap: %{public}d",
        static_cast<int32_t>(lowQualityPictureMap_.size()), static_cast<int32_t>(highQualityPictureMap_.size()));
}

void PictureDataOperations::CleanHighQualityPictureDataInternal(const std::string& imageId,
    sptr<PicturePair>& picturePair,
    std::list<std::string>& pictureImageIdList)
{
    MEDIA_INFO_LOG("enter CleanHighQualityPictureDataInternal, %{public}zu, %{public}zu",
        lowQualityPictureMap_.size(), highQualityPictureMap_.size());
    lock_guard<mutex>  lock(pictureMapMutex_);
    // 清理低质量图
    auto iterPicture = lowQualityPictureMap_.find(imageId);
    if (iterPicture != lowQualityPictureMap_.end() && (iterPicture->second)->isCleanImmediately_) {
        lowQualityPictureMap_.erase(iterPicture);
    }
    // 存储高质量图
    iterPicture = highQualityPictureMap_.find(imageId);
    if (iterPicture != highQualityPictureMap_.end() && (iterPicture->second)->isCleanImmediately_) {
        highQualityPictureMap_.erase(iterPicture);
    }
    highQualityPictureMap_[imageId] = picturePair;

    // 删除至最大值,高质量不用落盘
    for (auto iter = pictureImageIdList.begin(); iter != pictureImageIdList.end();) {
        if ((int)(highQualityPictureMap_.size()) <= max_capibilty) {
            return;
        }
        std::string imageId = *iter;
        std::map<std::string, sptr<PicturePair>>::iterator iterPicture = highQualityPictureMap_.find(imageId);
        if (iterPicture != highQualityPictureMap_.end() && (iterPicture->second)->isCleanImmediately_) {
            bool isTemp = false;
            bool isEdited = false;
            IsPictureTempAndEdited(iterPicture->first, isTemp, isEdited);
            if (isTemp) {
                FileUtils::SavePicture(iterPicture->first, (iterPicture->second)->picture_, isEdited, false);
                MEDIA_INFO_LOG("end SavePicture, photoId: %{public}s, isEdited: %{public}d",
                    (iterPicture->first).c_str(), static_cast<int32_t>(isEdited));
            }
            highQualityPictureMap_.erase(iterPicture);
            iter = pictureImageIdList.erase(iter);
        } else {
            iter++;
        }
    }
    MEDIA_DEBUG_LOG("end");
}

std::shared_ptr<Media::Picture> PictureDataOperations::GetDataWithImageId(const std::string& imageId,
    bool &isHighQualityPicture, bool &isTakeEffect, bool isCleanImmediately)
{
    lock_guard<mutex>  lock(pictureMapMutex_);
    MEDIA_INFO_LOG("enter %{public}s enter", imageId.c_str());
    enum PictureType pictureType;
    std::shared_ptr<Media::Picture> picture;
    isHighQualityPicture = false;
    for (pictureType = HIGH_QUALITY_PICTURE; pictureType >= LOW_QUALITY_PICTURE;
        pictureType = (PictureType)(pictureType - 1)) {
        picture = GetDataWithImageIdAndPictureType(imageId, pictureType, isTakeEffect, isCleanImmediately);
        if (picture != nullptr && picture->GetMainPixel() != nullptr) {
            MEDIA_INFO_LOG("GetDataWithImageId is founded, pictureType:%{public}d", static_cast<int32_t>(pictureType));
            isHighQualityPicture = (pictureType == HIGH_QUALITY_PICTURE);
            return picture;
        } else {
            MEDIA_WARN_LOG("GetDataWithImageId not found, pictureType:%{public}d", static_cast<int32_t>(pictureType));
        }
    }
    return picture;
}


void PictureDataOperations::SavePictureWithImageId(const std::string& imageId)
{
    MEDIA_DEBUG_LOG("enter ");
    enum PictureType pictureType;
    bool isSuccess = false;
    for (pictureType = HIGH_QUALITY_PICTURE; pictureType >= LOW_QUALITY_PICTURE;
        pictureType = (PictureType)(pictureType - 1)) {
        switch (pictureType) {
            case LOW_QUALITY_PICTURE:
                isSuccess = SavePicture(imageId, lowQualityPictureMap_, true);
                break;
            case HIGH_QUALITY_PICTURE:
                isSuccess = SavePicture(imageId, highQualityPictureMap_, false);
                break;
            default:
                break;
        }
    }
    if (isSuccess) { // 高质量提前返回
        return;
    }
    MEDIA_DEBUG_LOG("end ");
}

std::shared_ptr<Media::Picture> PictureDataOperations::GetDataWithImageIdAndPictureType(const std::string& imageId,
    PictureType pictureType, bool &isTakeEffect, bool isCleanImmediately)
{
    MEDIA_DEBUG_LOG("enter ");
    std::map<std::string, sptr<PicturePair>>::iterator iter;
    std::shared_ptr<Media::Picture> picture;
    switch (pictureType) {
        case LOW_QUALITY_PICTURE:
            iter = lowQualityPictureMap_.find(imageId);
            if (iter != lowQualityPictureMap_.end()) {
                (iter->second)->isCleanImmediately_ = isCleanImmediately;
                picture = (iter->second)->picture_;
            }
            break;
        case HIGH_QUALITY_PICTURE:
            iter = highQualityPictureMap_.find(imageId);
            if (iter != highQualityPictureMap_.end()) {
                (iter->second)->isCleanImmediately_ = isCleanImmediately;
                picture = (iter->second)->picture_;
                isTakeEffect = (iter->second)->isTakeEffect_;
            }
            break;
        default:
            break;
    }
    return picture;
}

bool PictureDataOperations::IsExsitDataForPictureType(PictureType pictureType)
{
    MEDIA_DEBUG_LOG("enter ");
    lock_guard<mutex> lock(pictureMapMutex_);
    bool isExsit = false;
    switch (pictureType) {
        case LOW_QUALITY_PICTURE:
            isExsit = lowQualityPictureMap_.size() >= 1;
            break;
        case HIGH_QUALITY_PICTURE:
            isExsit = highQualityPictureMap_.size() >= 1;
            break;
        default:
            break;
    }
    return isExsit;
}

bool PictureDataOperations::IsExsitDataForPictureType(const std::string& imageId, PictureType pictureType)
{
    MEDIA_DEBUG_LOG("enter ");
    lock_guard<mutex> lock(pictureMapMutex_);
    bool isExsit = false;
    switch (pictureType) {
        case LOW_QUALITY_PICTURE:
            isExsit = lowQualityPictureMap_.size() >= 0 &&
                lowQualityPictureMap_.find(imageId) != lowQualityPictureMap_.end();
            break;
        case HIGH_QUALITY_PICTURE:
            isExsit = highQualityPictureMap_.size() > 0 &&
                highQualityPictureMap_.find(imageId) != highQualityPictureMap_.end();
            break;
        default:
            break;
    }
    return isExsit;
}

// 落盘低质量图，包括低质量裸图/低质量
void PictureDataOperations::SaveLowQualityPicture(const std::string& imageId)
{
    MEDIA_DEBUG_LOG("enter ");
    enum PictureType pictureType;
    bool isSuccess = SavePicture(imageId, lowQualityPictureMap_, true);
}

// 落盘低质量图，包括低质量裸图
bool PictureDataOperations::SavePicture(const std::string& imageId,
    std::map<std::string, sptr<PicturePair>>& pictureMap, bool isLowQualityPicture)
{
    MEDIA_INFO_LOG("enter photoId: %{public}s, isLowQualityPicture: %{public}d", imageId.c_str(), isLowQualityPicture);
    lock_guard<mutex> lock(pictureMapMutex_);
    bool isSuccess = false;
    CHECK_AND_RETURN_RET_LOG(pictureMap.size() != 0, false, "pictureMap is null.");

    std::map<std::string, sptr<PicturePair>>::iterator iter;
    if (imageId == "default") {
        iter = pictureMap.begin();
    } else {
        iter = pictureMap.find(imageId);
    }
    if (iter != pictureMap.end()) {
        FileUtils::SavePicture(iter->first, (iter->second)->picture_, false, isLowQualityPicture);
        MEDIA_INFO_LOG("SavePicture, photoId: %{public}s, isLowQualityPicture: %{public}d",
            imageId.c_str(), isLowQualityPicture);
        // 落盘后清除缓存数据
        pictureMap.erase(iter);
        isSuccess = true;
    }
    MEDIA_INFO_LOG("SavePicture end, isSuccess: %{public}d, map size: %{public}zu", isSuccess, pictureMap.size());
    return isSuccess;
}

void PictureDataOperations::SavePictureExecutor(AsyncTaskData *data)
{
    auto *taskData = static_cast<SavePictureData *>(data);
    auto picturePair = taskData->picturePair_;

    MEDIA_DEBUG_LOG("SavePictureExecutor %{public}d ", taskSize);
    FileUtils::SavePicture(picturePair->photoId_, picturePair->picture_, false, true);
    picturePair->isCleanImmediately_ = true;
    taskSize --;
}

int32_t PictureDataOperations::AddSavePictureTask(sptr<PicturePair>& picturePair)
{
    auto asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_DEBUG_LOG("Failed to get async worker instance!");
        return -1;
    }

    auto *taskData = new (std::nothrow) SavePictureData(picturePair);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, -1,
        "Failed to alloc async data for downloading cloud files!");

    auto asyncTask = std::make_shared<MediaLibraryAsyncTask>(SavePictureExecutor, taskData);
    asyncWorker->AddTask(asyncTask, true);
    taskSize ++;
    return 0;
}

int32_t PictureDataOperations::GetPendingTaskSize()
{
    lock_guard<mutex> lock(pictureMapMutex_);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "GetPendingTaskSize, lowQualityPictureMap: %{public}d, highQualityPictureMap: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        static_cast<int32_t>(lowQualityPictureMap_.size()), static_cast<int32_t>(highQualityPictureMap_.size()));
    return lowQualityPictureMap_.size() + highQualityPictureMap_.size();
}

int32_t PictureDataOperations::GetLowPendingTaskSize()
{
    lock_guard<mutex> lock(pictureMapMutex_);
    MEDIA_ERR_LOG("GetLowPendingTaskSize, lowQualityPictureMap: %{public}d.",
        static_cast<int32_t>(lowQualityPictureMap_.size()));
    return lowQualityPictureMap_.size();
}

void PictureDataOperations::DeleteDataWithImageId(const std::string& imageId, PictureType pictureType)
{
    MEDIA_DEBUG_LOG("enter ");
    lock_guard<mutex> lock(pictureMapMutex_);
    MEDIA_INFO_LOG("DeleteDataWithImageId start, imageId: %{public}s, pictureType: %{public}d",
        imageId.c_str(), static_cast<int32_t>(pictureType));
    std::map<std::string, sptr<PicturePair>>::iterator iter;
    switch (pictureType) {
        case LOW_QUALITY_PICTURE:
            iter = lowQualityPictureMap_.find(imageId);
            if (iter != lowQualityPictureMap_.end()) {
                (iter->second)->picture_ = nullptr;
                lowQualityPictureMap_.erase(iter);
            }
            break;
        case HIGH_QUALITY_PICTURE:
            iter = highQualityPictureMap_.find(imageId);
            if (iter != highQualityPictureMap_.end()) {
                (iter->second)->picture_ = nullptr;
                highQualityPictureMap_.erase(iter);
            }
            break;
        default:
            break;
    }
    MEDIA_DEBUG_LOG("DeleteDataWithImageId end: %{public}s", imageId.c_str());
}

void PictureDataOperations::FinishAccessingPicture(const std::string& imageId, PictureType pictureType)
{
    lock_guard<mutex> lock(pictureMapMutex_);
    MEDIA_INFO_LOG("FinishAccessingPicture start, imageId: %{public}s, pictureType: %{public}d",
        imageId.c_str(), static_cast<int32_t>(pictureType));
    std::map<std::string, sptr<PicturePair>>::iterator iter;
    switch (pictureType) {
        case LOW_QUALITY_PICTURE:
            iter = lowQualityPictureMap_.find(imageId);
            if (iter != lowQualityPictureMap_.end()) {
                (iter->second)->isCleanImmediately_ = true;
            }
            break;
        case HIGH_QUALITY_PICTURE:
            iter = highQualityPictureMap_.find(imageId);
            if (iter != highQualityPictureMap_.end()) {
                (iter->second)->isCleanImmediately_ = true;
            }
            break;
        default:
            break;
    }
    MEDIA_DEBUG_LOG("FinishAccessingPicture end: %{public}s", imageId.c_str());
}
} // namespace Media
} // namespace OHOS