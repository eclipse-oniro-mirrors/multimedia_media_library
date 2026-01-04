/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureDfxCameraPhoto"

#include "multistages_capture_dfx_save_camera_photo.h"

#include "media_log.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
MultiStagesCaptureDfxSaveCameraPhoto::MultiStagesCaptureDfxSaveCameraPhoto() {}
MultiStagesCaptureDfxSaveCameraPhoto::~MultiStagesCaptureDfxSaveCameraPhoto() {}

MultiStagesCaptureDfxSaveCameraPhoto& MultiStagesCaptureDfxSaveCameraPhoto::GetInstance()
{
    static MultiStagesCaptureDfxSaveCameraPhoto instance;
    return instance;
}

void MultiStagesCaptureDfxSaveCameraPhoto::AddAssetTime(const std::string &photoId, AddAssetTimeStat stat)
{   
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (stat != AddAssetTimeStat::START && 
        (times_.empty() || times_.find(photoId) == times_.end() || 
        times_[photoId].find(KEY_CREATE_ASSET_TIME) == times_[photoId].end() || 
        times_[photoId][KEY_CREATE_ASSET_TIME].find(static_cast<int32_t>(AddAssetTimeStat::START)) == 
        times_[photoId][KEY_CREATE_ASSET_TIME].end())) {
        return;
    }
    
    if (!times_.empty() && (times_.find(photoId) != times_.end())) {
        if (times_[photoId].find(KEY_CREATE_ASSET_TIME) != times_[photoId].end()) {
            times_[photoId][KEY_CREATE_ASSET_TIME].emplace(
                static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds());
                return;
        }
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        times_[photoId].emplace(KEY_CREATE_ASSET_TIME, stats);
    } else {
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>> times =  {
            {KEY_CREATE_ASSET_TIME, stat}
        };
        times_.emplace(photoId, times);
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::AddCaptureTime(const std::string &photoId, AddCaptureTimeStat stat)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (stat != AddCaptureTimeStat::START && 
        (times_.empty() || times_.find(photoId) == times_.end() || 
        times_[photoId].find(KEY_PHOTO_CAPTURE_TIME) == times_[photoId].end() || 
        times_[photoId][KEY_PHOTO_CAPTURE_TIME].find(static_cast<int32_t>(AddCaptureTimeStat::START)) == 
        times_[photoId][KEY_PHOTO_CAPTURE_TIME].end())) {
        return;
    }
    if (!times_.empty() && (times_.find(photoId) != times_.end())) {
        if (times_[photoId].find(KEY_PHOTO_CAPTURE_TIME) != times_[photoId].end()) {
            times_[photoId][KEY_PHOTO_CAPTURE_TIME].emplace(
                static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds());
                return;
        }
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        times_[photoId].emplace(KEY_PHOTO_CAPTURE_TIME, stats);
    } else {
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>> times = {
            {KEY_PHOTO_CAPTURE_TIME, stat}
        };
        times_.emplace(photoId, times);
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::AddSaveTime(const std::string &photoId, AddSaveTimeStat stat)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (stat != AddSaveTimeStat::START && 
        (times_.empty() || times_.find(photoId) == times_.end() || 
        times_[photoId].find(KEY_SAVE_CAMERA_TIME) == times_[photoId].end() || 
        times_[photoId][KEY_SAVE_CAMERA_TIME].find(static_cast<int32_t>(AddSaveTimeStat::START)) == 
        times_[photoId][KEY_SAVE_CAMERA_TIME].end())) {
        return;
    }
    if (!times_.empty() && (times_.find(photoId) != times_.end())) {
        if (times_[photoId].find(KEY_SAVE_CAMERA_TIME) != times_[photoId].end()) {
            times_[photoId][KEY_SAVE_CAMERA_TIME].emplace(
                static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds());
                return;
        }
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        times_[photoId].emplace(KEY_SAVE_CAMERA_TIME, stats);
    } else {
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>> times = {
            {KEY_SAVE_CAMERA_TIME, stat}
        };
        times_.emplace(photoId, times);
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::RemoveTime(const std::string &photoId)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (times_.empty() || times_.find(photoId) == times_.end()) {
        MEDIA_ERR_LOG("times_ is empty or photoId is not in times_");
        return;
    }
    times_.erase(photoId);
}

void MultiStagesCaptureDfxSaveCameraPhoto::GetResultString(const std::string &photoId,
    std::string &createAssetTime, std::string &photoCaptureTime, std::string &saveCameraTime)
{
if (times_[photoId].find(KEY_CREATE_ASSET_TIME) != times_[photoId].end() &&
        times_[photoId][KEY_CREATE_ASSET_TIME].find(static_cast<int32_t>(AddAssetTimeStat::END)) !=
        times_[photoId][KEY_CREATE_ASSET_TIME].end() &&
        times_[photoId][KEY_CREATE_ASSET_TIME].find(static_cast<int32_t>(AddAssetTimeStat::START)) !=
        times_[photoId][KEY_CREATE_ASSET_TIME].end()) {
        auto stats = times_[photoId][KEY_CREATE_ASSET_TIME];
        createAssetTime = createAssetTime + "total : " + to_string(stats[static_cast<int32_t>(AddAssetTimeStat::END)]
            - stats[static_cast<int32_t>(AddAssetTimeStat::START)]) + ",";
        for (int32_t i = static_cast<int32_t>(AddAssetTimeStat::START) + 1;
            i < static_cast<int32_t>(AddAssetTimeStat::END); ++i) {
            if (stats.find(i) != stats.end()) {
                auto it = AddAssetTimeStatMap.find(i);
                createAssetTime = createAssetTime + it->second + " : "
                    + to_string(stats[i] - stats[static_cast<int32_t>(AddAssetTimeStat::START)]) + ",";
            }
        }
    }

    if (times_[photoId].find(KEY_PHOTO_CAPTURE_TIME) != times_[photoId].end() &&
        times_[photoId][KEY_PHOTO_CAPTURE_TIME].find(static_cast<int32_t>(AddCaptureTimeStat::END)) !=
        times_[photoId][KEY_PHOTO_CAPTURE_TIME].end() &&
        times_[photoId][KEY_PHOTO_CAPTURE_TIME].find(static_cast<int32_t>(AddCaptureTimeStat::START)) !=
        times_[photoId][KEY_PHOTO_CAPTURE_TIME].end()) {
        auto stats = times_[photoId][KEY_PHOTO_CAPTURE_TIME];
        photoCaptureTime = photoCaptureTime + "total : " +
            to_string(stats[static_cast<int32_t>(AddCaptureTimeStat::END)]
            - stats[static_cast<int32_t>(AddCaptureTimeStat::START)]) + ",";
    }

    if (times_[photoId].find(KEY_SAVE_CAMERA_TIME) != times_[photoId].end() &&
        times_[photoId][KEY_SAVE_CAMERA_TIME].find(static_cast<int32_t>(AddSaveTimeStat::END)) !=
        times_[photoId][KEY_SAVE_CAMERA_TIME].end() &&
        times_[photoId][KEY_SAVE_CAMERA_TIME].find(static_cast<int32_t>(AddSaveTimeStat::START)) !=
        times_[photoId][KEY_SAVE_CAMERA_TIME].end()) {
        auto stats = times_[photoId][KEY_SAVE_CAMERA_TIME];
        saveCameraTime = saveCameraTime + "total : " + to_string(stats[static_cast<int32_t>(AddSaveTimeStat::END)]
            - stats[static_cast<int32_t>(AddSaveTimeStat::START)]) + ",";
        for (int32_t i = static_cast<int32_t>(AddSaveTimeStat::START) + 1;
            i < static_cast<int32_t>(AddSaveTimeStat::END); ++i) {
            if (stats.find(i) != stats.end()) {
                auto it = AddSaveTimeStatMap.find(i);
                saveCameraTime = saveCameraTime + it->second + " : "
                    + to_string(stats[i] - stats[static_cast<int32_t>(AddSaveTimeStat::START)]) + ",";
            }
        }
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::Report(const std::string &photoId, const int32_t isDecoding,
    const int32_t mediaSubtype)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (times_.empty() || times_.find(photoId) == times_.end()) {
        MEDIA_INFO_LOG("times_ is empty or photoId is not in times_");
        return;
    }
    std::string createAssetTime = "";
    std::string photoCaptureTime = "";
    std::string saveCameraTime = "";
    GetResultString(photoId, createAssetTime, photoCaptureTime, saveCameraTime);
    times_.erase(photoId);
    VariantMap map = {
        {KEY_PHOTO_ID, photoId},
        {KEY_CREATE_ASSET_TIME, createAssetTime},
        {KEY_PHOTO_CAPTURE_TIME, photoCaptureTime},
        {KEY_SAVE_CAMERA_TIME, saveCameraTime},
        {KEY_IS_DECODING, isDecoding},
        {KEY_MEDIA_SUBTYPE, mediaSubtype}};
    PostEventUtils::GetInstance().PostStatProcess(StatType::MSC_SAVE_CAMERA_PHOTO_STAT, map);
}
}
}