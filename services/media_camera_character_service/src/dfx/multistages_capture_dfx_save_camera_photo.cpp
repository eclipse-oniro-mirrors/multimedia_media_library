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

#define MLOG_TAG "MultiStagesCaptureDfxCameraPhoto"

#include "multistages_capture_dfx_save_camera_photo.h"
#include "media_log.h"
#include "post_event_utils.h"
#include "media_file_utils.h"

namespace OHOS {
namespace Media {
const int32_t createAssetBaseTime = 50;
const int32_t captureBaseTime = 200;
const int32_t saveBaseTime = 350;
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
    MEDIA_INFO_LOG("AddAssetTime photoId : %{public}s, stat %{public}d",
        photoId.c_str(), static_cast<int32_t>(stat));
    if (stat != AddAssetTimeStat::START &&
        (dfxTimes_.empty() || dfxTimes_.find(photoId) == dfxTimes_.end() ||
        dfxTimes_[photoId].find(KEY_CREATE_ASSET_TIME) == dfxTimes_[photoId].end() ||
        dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].find(static_cast<int32_t>(AddAssetTimeStat::START)) ==
        dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].end())) {
        return;
    }
    
    if (!dfxTimes_.empty() && (dfxTimes_.find(photoId) != dfxTimes_.end())) {
        if (dfxTimes_[photoId].find(KEY_CREATE_ASSET_TIME) != dfxTimes_[photoId].end()) {
            dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].emplace(
                static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds());
                return;
        }
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        dfxTimes_[photoId].emplace(KEY_CREATE_ASSET_TIME, stats);
    } else {
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>> times =  {
            {KEY_CREATE_ASSET_TIME, stats}
        };
        dfxTimes_.emplace(photoId, times);
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::AddCaptureTime(const std::string &photoId, AddCaptureTimeStat stat)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    MEDIA_INFO_LOG("AddCaptureTime photoId : %{public}s, stat %{public}d",
        photoId.c_str(), static_cast<int32_t>(stat));
    if (stat != AddCaptureTimeStat::START &&
        (dfxTimes_.empty() || dfxTimes_.find(photoId) == dfxTimes_.end() ||
        dfxTimes_[photoId].find(KEY_PHOTO_CAPTURE_TIME) == dfxTimes_[photoId].end() ||
        dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].find(static_cast<int32_t>(AddCaptureTimeStat::START)) ==
        dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].end())) {
        return;
    }
    if (!dfxTimes_.empty() && (dfxTimes_.find(photoId) != dfxTimes_.end())) {
        if (dfxTimes_[photoId].find(KEY_PHOTO_CAPTURE_TIME) != dfxTimes_[photoId].end()) {
            dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].emplace(
                static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds());
                return;
        }
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        dfxTimes_[photoId].emplace(KEY_PHOTO_CAPTURE_TIME, stats);
    } else {
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>> times = {
            {KEY_PHOTO_CAPTURE_TIME, stats}
        };
        dfxTimes_.emplace(photoId, times);
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::AddSaveTime(const std::string &photoId, AddSaveTimeStat stat)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    MEDIA_INFO_LOG("AddSaveTime photoId : %{public}s, stat %{public}d",
        photoId.c_str(), static_cast<int32_t>(stat));
    if (stat != AddSaveTimeStat::START &&
        (dfxTimes_.empty() || dfxTimes_.find(photoId) == dfxTimes_.end() ||
        dfxTimes_[photoId].find(KEY_SAVE_CAMERA_TIME) == dfxTimes_[photoId].end() ||
        dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].find(static_cast<int32_t>(AddSaveTimeStat::START)) ==
        dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].end())) {
        return;
    }
    if (!dfxTimes_.empty() && (dfxTimes_.find(photoId) != dfxTimes_.end())) {
        if (dfxTimes_[photoId].find(KEY_SAVE_CAMERA_TIME) != dfxTimes_[photoId].end()) {
            dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].emplace(
                static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds());
                return;
        }
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        dfxTimes_[photoId].emplace(KEY_SAVE_CAMERA_TIME, stats);
    } else {
        std::unordered_map<int32_t, int64_t> stats = {
            {static_cast<int32_t>(stat), MediaFileUtils::UTCTimeMilliSeconds()}
        };
        std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>> times = {
            {KEY_SAVE_CAMERA_TIME, stats}
        };
        dfxTimes_.emplace(photoId, times);
    }
}

void MultiStagesCaptureDfxSaveCameraPhoto::RemoveTime(const std::string &photoId)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (dfxTimes_.empty() || dfxTimes_.find(photoId) == dfxTimes_.end()) {
        MEDIA_ERR_LOG("dfxTimes_ is empty or photoId is not in dfxTimes_");
        return;
    }
    dfxTimes_.erase(photoId);
}

bool MultiStagesCaptureDfxSaveCameraPhoto::GetCreateAssetTime(const std::string &photoId,
    std::string &createAssetTime)
{
    bool ret = false;
    if (dfxTimes_[photoId].find(KEY_CREATE_ASSET_TIME) != dfxTimes_[photoId].end() &&
        dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].find(static_cast<int32_t>(AddAssetTimeStat::END)) !=
        dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].end() &&
        dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].find(static_cast<int32_t>(AddAssetTimeStat::START)) !=
        dfxTimes_[photoId][KEY_CREATE_ASSET_TIME].end()) {
        auto stats = dfxTimes_[photoId][KEY_CREATE_ASSET_TIME];
        int32_t totalTime = stats[static_cast<int32_t>(AddAssetTimeStat::END)]
            - stats[static_cast<int32_t>(AddAssetTimeStat::START)];
        if (totalTime > createAssetBaseTime) {
            ret = true;
        }
        createAssetTime = createAssetTime + "total : " + std::to_string(totalTime) + ",";
        for (int32_t i = static_cast<int32_t>(AddAssetTimeStat::START) + 1;
            i < static_cast<int32_t>(AddAssetTimeStat::END); ++i) {
            if (stats.find(i) != stats.end()) {
                auto it = AddAssetTimeStatMap.find(i);
                createAssetTime = createAssetTime + it->second + " : "
                    + std::to_string(stats[i] - stats[static_cast<int32_t>(AddAssetTimeStat::START)]) + ",";
            }
        }
    }
    return ret;
}

bool MultiStagesCaptureDfxSaveCameraPhoto::GetPhotoCaptureTime(const std::string &photoId,
    std::string &photoCaptureTime)
{
    bool ret = false;
    if (dfxTimes_[photoId].find(KEY_PHOTO_CAPTURE_TIME) != dfxTimes_[photoId].end() &&
        dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].find(static_cast<int32_t>(AddCaptureTimeStat::END)) !=
        dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].end() &&
        dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].find(static_cast<int32_t>(AddCaptureTimeStat::START)) !=
        dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME].end()) {
        auto stats = dfxTimes_[photoId][KEY_PHOTO_CAPTURE_TIME];
        int32_t totalTime = stats[static_cast<int32_t>(AddCaptureTimeStat::END)]
            - stats[static_cast<int32_t>(AddCaptureTimeStat::START)];
        if (totalTime > captureBaseTime) {
            ret = true;
        }
        photoCaptureTime = photoCaptureTime + "total : " + std::to_string(totalTime) + ",";
    }
    return ret;
}

bool MultiStagesCaptureDfxSaveCameraPhoto::GetSaveCameraTime(const std::string &photoId,
    std::string &saveCameraTime)
{
    bool ret = false;
    if (dfxTimes_[photoId].find(KEY_SAVE_CAMERA_TIME) != dfxTimes_[photoId].end() &&
        dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].find(static_cast<int32_t>(AddSaveTimeStat::END)) !=
        dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].end() &&
        dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].find(static_cast<int32_t>(AddSaveTimeStat::START)) !=
        dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME].end()) {
        auto stats = dfxTimes_[photoId][KEY_SAVE_CAMERA_TIME];
        int32_t totalTime = stats[static_cast<int32_t>(AddSaveTimeStat::END)]
            - stats[static_cast<int32_t>(AddSaveTimeStat::START)];
        if (totalTime > saveBaseTime) {
            ret = true;
        }
        saveCameraTime = saveCameraTime + "total : " + std::to_string(totalTime) + ",";
        for (int32_t i = static_cast<int32_t>(AddSaveTimeStat::START) + 1;
            i < static_cast<int32_t>(AddSaveTimeStat::END); ++i) {
            if (stats.find(i) != stats.end()) {
                auto it = AddSaveTimeStatMap.find(i);
                saveCameraTime = saveCameraTime + it->second + " : "
                    + std::to_string(stats[i] - stats[static_cast<int32_t>(AddSaveTimeStat::START)]) + ",";
            }
        }
    }
    return ret;
}

bool MultiStagesCaptureDfxSaveCameraPhoto::GetResultString(const std::string &photoId,
    std::string &createAssetTime, std::string &photoCaptureTime, std::string &saveCameraTime)
{
    bool ret = false;
    bool createAsset = GetCreateAssetTime(photoId, createAssetTime);
    bool photoCapture = GetPhotoCaptureTime(photoId, photoCaptureTime);
    bool saveCamera = GetSaveCameraTime(photoId, saveCameraTime);
    if (createAsset || photoCapture || saveCamera) {
        ret = true;
    }
    
    return ret;
}

void MultiStagesCaptureDfxSaveCameraPhoto::Report(const std::string &photoId, const int32_t isDecoding,
    const int32_t mediaSubtype)
{
    std::lock_guard<std::mutex> lock(addTimeMutex_);
    if (dfxTimes_.empty() || dfxTimes_.find(photoId) == dfxTimes_.end()) {
        MEDIA_INFO_LOG("dfxTimes_ is empty or photoId is not in dfxTimes_");
        return;
    }
    std::string createAssetTime = "";
    std::string photoCaptureTime = "";
    std::string saveCameraTime = "";
    bool ret = GetResultString(photoId, createAssetTime, photoCaptureTime, saveCameraTime);
    if (!ret) {
        return;
    }
    dfxTimes_.erase(photoId);
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