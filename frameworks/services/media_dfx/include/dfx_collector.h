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

#ifndef OHOS_MEDIA_DFX_COLLECTOR_H
#define OHOS_MEDIA_DFX_COLLECTOR_H

#include <map>
#include <mutex>
#include <string>

#include "dfx_const.h"

namespace OHOS {
namespace Media {
class DfxCollector {
public:
    DfxCollector();
    ~DfxCollector();
    void CollectThumbnailError(const std::string &path, int32_t method, int32_t errorCode);
    std::unordered_map<std::string, ThumbnailErrorInfo> GetThumbnailError();
    std::unordered_map<std::string, CommonBehavior> GetCommonBehavior();
    void AddCommonBahavior(std::string bundleName, int32_t type);
    void CollectDeleteBehavior(std::string bundleName, int32_t type, int32_t size);
    std::unordered_map<std::string, int32_t> GetDeleteBehavior(int32_t type);
    void CollectAdaptationToMovingPhotoInfo(const std::string &appName, bool adapted);
    AdaptationToMovingPhotoInfo GetAdaptationToMovingPhotoInfo();
    void CollectCinematicVideoAccessTimes(bool isByUri, bool isHighQualityRequest);
    void CollectCinematicVideoAddStartTime(const CinematicWaitType waitType, const std::string videoId);
    void CollectCinematicVideoAddEndTime(const CinematicWaitType waitType, const std::string videoId);
    void CollectCinematicVideoMultistageResult(bool multistageResult);
    CinematicVideoInfo GetCinematicVideoInfo();

private:
    std::mutex thumbnailErrorLock_;
    std::mutex commonBehaviorLock_;
    std::mutex deleteToTrashLock_;
    std::mutex deleteFromDiskLock_;
    std::mutex removeLock_;
    std::mutex adaptationToMovingPhotoLock_;
    std::mutex cinematicVideoStaticLock_;
    std::unordered_map<std::string, ThumbnailErrorInfo> thumbnailErrorMap_;
    std::unordered_map<std::string, CommonBehavior> commonBehaviorMap_;
    std::unordered_map<std::string, int32_t> deleteToTrashMap_;
    std::unordered_map<std::string, int32_t> deleteFromDiskMap_;
    std::unordered_map<std::string, int32_t> removeMap_;
    AdaptationToMovingPhotoInfo adaptationToMovingPhotoInfo_;
    CinematicVideoInfo cinematicVideoInfo_;
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_COLLECTOR_H