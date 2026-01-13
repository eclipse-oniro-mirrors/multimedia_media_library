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

#ifndef OHOS_PHOTO_PICTURE_MANAGER_THREAD_H
#define OHOS_PHOTO_PICTURE_MANAGER_THREAD_H

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <list>
#include <thread>
#include "picture.h"
#include "picture_data_operations.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum State {
    STOPPED = 0, // 停止状态，包括从未启动过和启动后被停止
    RUNNING, // 运行状态
    PAUSED, // 暂停状态
};

class PictureManagerThread : public RefBase {
public:
    EXPORT static PictureManagerThread* GetInstance();
    PictureManagerThread();
    ~PictureManagerThread();

    State State() const;
    EXPORT void Start();
    void Stop();
    void Pause();
    void Resume();
    EXPORT void InsertPictureData(const std::string& imageId, sptr<PicturePair>& picturePair, PictureType pictureType);
    EXPORT std::shared_ptr<Media::Picture> GetDataWithImageId(const std::string& imageId,
        bool &isHighQualityPicture, bool &isTakeEffect, bool isCleanImmediately = false);
    EXPORT bool IsExsitDataForPictureType(PictureType pictureType);
    EXPORT bool IsExsitPictureByImageId(const std::string& imageId);
    EXPORT void SaveLowQualityPicture(const std::string& imageId = "default");
    EXPORT void FinishAccessingPicture(const std::string& imageId);
    EXPORT void SavePictureWithImageId(const std::string& imageId);
    EXPORT int32_t AddSavePictureTask(sptr<PicturePair>& picturePair);
    EXPORT int32_t GetPendingTaskSize();
    EXPORT int32_t GetLowPendingTaskSize();
    EXPORT void DeleteDataWithImageId(const std::string& imageId, PictureType pictureType);
private:
    void Run();
    std::unique_ptr<std::thread> thread_ = nullptr;
    std::mutex threadMutex_;
    std::mutex runningMutex_;
    std::condition_variable condition_;
    std::atomic_bool pauseFlag_; // 暂停标识
    std::atomic_bool stopFlag_; // 停止标识
    enum State state_;
    static std::unique_ptr<PictureManagerThread> instance_;
    static std::mutex mutex_;
    sptr<PictureDataOperations> pictureDataOperations_;
    int32_t lastPendingTaskSize_ = 0;
}; // class PictureManagerThread
} // namespace Media
}  // namespace OHOS
#endif // OHOS_PHOTO_PICTURE_MANAGER_THREAD_H