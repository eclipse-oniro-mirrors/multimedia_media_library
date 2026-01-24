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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_SAVE_CAMERA_PHOTO_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MULTISTAGES_CAPTURE_DFX_SAVE_CAMERA_PHOTO_H

#include <string>
#include <unordered_map>
#include <mutex>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
enum class AddAssetTimeStat : int32_t {
    START,
    SET_FILE_ASSET,
    UPDATE_DB,
    END,
};

enum class AddCaptureTimeStat : int32_t {
    START,
    END,
};

enum class AddSaveTimeStat : int32_t {
    START,
    SAVE_PICTURE,
    GET_FILE_ASSET,
    DEAL_PICTURE,
    TAKE_EFFECT,
    SAVE_EFFECT,
    UPDATE_DB,
    END,
};

class MultiStagesCaptureDfxSaveCameraPhoto {
public:
    EXPORT static MultiStagesCaptureDfxSaveCameraPhoto &GetInstance();
    EXPORT void AddAssetTime(const std::string &photoId, AddAssetTimeStat stat);
    EXPORT void AddCaptureTime(const std::string &photoId, AddCaptureTimeStat stat);
    EXPORT void AddSaveTime(const std::string &photoId, AddSaveTimeStat stat);
    EXPORT void RemoveTime(const std::string &photoId);
    EXPORT void Report(const std::string &photoId, const int32_t isDecoding, const int32_t mediaSubtype);
private:
    MultiStagesCaptureDfxSaveCameraPhoto();
    ~MultiStagesCaptureDfxSaveCameraPhoto();
    bool GetCreateAssetTime(const std::string &photoId, std::string &createAssetTime);
    bool GetPhotoCaptureTime(const std::string &photoId, std::string &photoCaptureTime);
    bool GetSaveCameraTime(const std::string &photoId, std::string &saveCameraTime);
    bool GetResultString(const std::string &photoId,
        std::string &createAssetTime, std::string &photoCaptureTime, std::string &saveCameraTime);
    // <photoId, <stat, <subStat, time>>>
    std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<int32_t, int64_t>>> dfxTimes_;
    std::mutex addTimeMutex_;
    const std::unordered_map<int32_t, std::string> AddAssetTimeStatMap = {
        {static_cast<int32_t>(AddAssetTimeStat::START), "start"},
        {static_cast<int32_t>(AddAssetTimeStat::SET_FILE_ASSET), "setFileAsset"},
        {static_cast<int32_t>(AddAssetTimeStat::UPDATE_DB), "updateDB"},
        {static_cast<int32_t>(AddAssetTimeStat::END), "total"}
    };
    const std::unordered_map<int32_t, std::string> AddSaveTimeStatMap = {
        {static_cast<int32_t>(AddSaveTimeStat::START), "start"},
        {static_cast<int32_t>(AddSaveTimeStat::SAVE_PICTURE), "savePicture"},
        {static_cast<int32_t>(AddSaveTimeStat::GET_FILE_ASSET), "getFileAsset"},
        {static_cast<int32_t>(AddSaveTimeStat::DEAL_PICTURE), "dealPicture"},
        {static_cast<int32_t>(AddSaveTimeStat::TAKE_EFFECT), "takeEffect"},
        {static_cast<int32_t>(AddSaveTimeStat::SAVE_EFFECT), "saveEffect"},
        {static_cast<int32_t>(AddSaveTimeStat::UPDATE_DB), "updateDb"},
        {static_cast<int32_t>(AddSaveTimeStat::END), "total"}
    };
};
}
}
#endif