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
#define MLOG_TAG "DfxAnalyzer"

#include "dfx_analyzer.h"

#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "preferences.h"
#include "preferences_helper.h"

namespace OHOS {
namespace Media {

using namespace std;

DfxAnalyzer::DfxAnalyzer()
{
}

DfxAnalyzer::~DfxAnalyzer()
{
}

void DfxAnalyzer::FlushThumbnail(std::unordered_map<std::string, ThumbnailErrorInfo> &thumbnailErrorMap)
{
    if (thumbnailErrorMap.empty()) {
        return;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(THUMBNAIL_ERROR_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    for (auto entry: thumbnailErrorMap) {
        string key = entry.first + SPLIT_CHAR + to_string(entry.second.method) + SPLIT_CHAR +
            to_string(entry.second.errCode);
        if (!prefs->GetString(key).empty()) {
            continue;
        }
        string value = to_string(entry.second.time);
        prefs->PutString(key, value);
    }
    prefs->FlushSync();
    MEDIA_INFO_LOG("flush %{public}zu itmes", thumbnailErrorMap.size());
}

void DfxAnalyzer::FlushCommonBehavior(std::unordered_map<string, CommonBehavior> &commonBehaviorMap)
{
    if (commonBehaviorMap.empty()) {
        return;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(COMMON_BEHAVIOR_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    string behaviors;
    for (auto entry: commonBehaviorMap) {
        string bundleName = entry.first;
        if (bundleName == "") {
            continue;
        }
        int32_t times = entry.second.times;
        int32_t oldValue = prefs->GetInt(bundleName, 0);
        prefs->PutInt(bundleName, times + oldValue);
        behaviors += "{" + bundleName + ": " + to_string(times) + "}";
    }
    prefs->FlushSync();
    if (!behaviors.empty()) {
        MEDIA_INFO_LOG("common behavior: %{public}s", behaviors.c_str());
    }
}

void DfxAnalyzer::FlushDeleteBehavior(std::unordered_map<string, int32_t> &deleteBehaviorMap, int32_t type)
{
    if (deleteBehaviorMap.empty()) {
        return;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DELETE_BEHAVIOR_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    for (auto entry: deleteBehaviorMap) {
        string bundleName = entry.first;
        int32_t times = entry.second;
        int32_t oldValue = prefs->GetInt(bundleName, 0);
        prefs->PutInt(bundleName + SPLIT_CHAR + to_string(type), times + oldValue);
    }
    prefs->FlushSync();
}

void DfxAnalyzer::FlushAdaptationToMovingPhoto(AdaptationToMovingPhotoInfo& newAdaptationInfo)
{
    if (newAdaptationInfo.unadaptedAppPackages.empty() && newAdaptationInfo.adaptedAppPackages.empty()) {
        return;
    }
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ADAPTATION_TO_MOVING_PHOTO_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    string unadaptedAppsStr = prefs->GetString(MOVING_PHOTO_KEY_UNADAPTED_PACKAGE);
    string adaptedAppsStr = prefs->GetString(MOVING_PHOTO_KEY_ADAPTED_PACKAGE);
    unordered_set<string> allUnadaptedApps = DfxUtils::SplitString(unadaptedAppsStr, ';');
    unordered_set<string> allAdaptedApps = DfxUtils::SplitString(adaptedAppsStr, ';');
    for (const auto& app : newAdaptationInfo.unadaptedAppPackages) {
        allUnadaptedApps.insert(app);
    }
    for (const auto& app : newAdaptationInfo.adaptedAppPackages) {
        allAdaptedApps.insert(app);
    }

    prefs->PutInt(MOVING_PHOTO_KEY_UNADAPTED_NUM, allUnadaptedApps.size());
    prefs->PutString(MOVING_PHOTO_KEY_UNADAPTED_PACKAGE, DfxUtils::JoinStrings(allUnadaptedApps, ';'));
    prefs->PutInt(MOVING_PHOTO_KEY_ADAPTED_NUM, allAdaptedApps.size());
    prefs->PutString(MOVING_PHOTO_KEY_ADAPTED_PACKAGE, DfxUtils::JoinStrings(allAdaptedApps, ';'));

    prefs->FlushSync();
    MEDIA_INFO_LOG("flush adaptation to moving photo, unadapted num: %{private}zu, adapted num: %{private}zu",
        allUnadaptedApps.size(), allAdaptedApps.size());
}

int32_t DfxAnalyzer::CalculateAvgWaitTime(CinematicWaitType waitType, const CinematicVideoInfo& cinematicVideoInfo,
    const int32_t oldNum, const int32_t oldWaitAvgTime)
{
    std::unordered_map<std::string, TimeRange> waitTimeMap;
    if (waitType == CinematicWaitType::CANCEL_CINEMATIC) {
        waitTimeMap = cinematicVideoInfo.cancelWaitTimeMap;
    } else {
        waitTimeMap = cinematicVideoInfo.processWaitTimeMap;
    }

    uint64_t totalWaitTime = 0;
    for (const auto& [videoId, TimeRange] : waitTimeMap) {
        if (TimeRange.endTime >= TimeRange.startTime) {
            totalWaitTime += (TimeRange.endTime - TimeRange.startTime);
        }
    }
    totalWaitTime = static_cast<int64_t>(oldWaitAvgTime) * static_cast<int64_t>(oldNum) +
        totalWaitTime;
    int64_t AvgWaitTime = totalWaitTime /
        static_cast<int64_t>(static_cast<int32_t>(waitTimeMap.size()) + oldNum);
    return static_cast<int32_t>(AvgWaitTime);
}

static bool CheckDfxFile(const string& dfxFileName, shared_ptr<NativePreferences::Preferences> &prefs)
{
    int32_t errCode;
    prefs = NativePreferences::PreferencesHelper::GetPreferences(dfxFileName, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return false;
    }
    return true;
}

void DfxAnalyzer::FlushTranscodeAccessTimes(const TranscodeAccessType type)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ALIB_HEIF_DUPLICATE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    const char* accessKey = nullptr;
    switch (type) {
        case ACCESS_MEDIALIB:
            accessKey = TRANSCODE_ACCESS_MEDIALIB.c_str();
            break;
        case ACCESS_LIBC:
            accessKey = TRANSCODE_ACCESS_LIBC.c_str();
            break;
        default:
            MEDIA_ERR_LOG("get TranscodeAccessType error: %{public}d", type);
            return;
    }

    int32_t useTimes = prefs->GetInt(TRANSCODE_ACCESS_TIMES, 0);
    int32_t accessTimes = prefs->GetInt(accessKey, 0);
    prefs->PutInt(TRANSCODE_ACCESS_TIMES, useTimes + 1);
    prefs->PutInt(accessKey, accessTimes + 1);
    prefs->FlushSync();
}

void DfxAnalyzer::FlushTranscodeFailed(const TranscodeErrorType type)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ALIB_HEIF_DUPLICATE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    const char* typeKey = nullptr;
    switch (type) {
        case INNER_FAILED:
            typeKey = INNER_FAILED_TIMES.c_str();
            break;
        case CODEC_FAILED:
            typeKey = CODEC_FAILED_TIMES.c_str();
            break;
        default:
            MEDIA_ERR_LOG("get TranscodeFailedType error: %{public}d", type);
            return;
    }
    int32_t failedAllTimes = prefs->GetInt(TRANSCODE_FAILED_TIMES, 0);
    int32_t failedTimes = prefs->GetInt(typeKey, 0);
    prefs->PutInt(TRANSCODE_FAILED_TIMES, failedAllTimes + 1);
    prefs->PutInt(typeKey, failedTimes + 1);
    prefs->FlushSync();
}

void DfxAnalyzer::FlushTranscodeCostTime(const int32_t costTime)
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(ALIB_HEIF_DUPLICATE_XML, errCode);
    if (!prefs) {
        MEDIA_ERR_LOG("get preferences error: %{public}d", errCode);
        return;
    }
    
    int32_t alreadyCostTime = prefs->GetInt(TRANSCODE_AVG_TIME, 0);
    int32_t transcodeTime = prefs->GetInt(TRANSCODE_TIMES, 0);
    alreadyCostTime = alreadyCostTime + costTime;
    prefs->PutInt(TRANSCODE_AVG_TIME, alreadyCostTime);
    prefs->PutInt(TRANSCODE_TIMES, transcodeTime + 1);
    prefs->FlushSync();
}

void DfxAnalyzer::FlushCinematicVideoInfo(CinematicVideoInfo& newCinematicVideoInfo)
{
    MEDIA_INFO_LOG("Refresh CinematicVideoInfo into file");
    shared_ptr<NativePreferences::Preferences> prefs = nullptr;
    CHECK_AND_RETURN(CheckDfxFile(DFX_CINEMATIC_VIDEO_XML, prefs));

    int32_t oldCancelNum = prefs->GetInt(CINEMATIC_VIDEO_KEY_CANCEL_NUM);
    int32_t oldCancelAvgTime = prefs->GetInt(CINEMATIC_VIDEO_KEY_CANCEL_WAIT_AVG_TIME);
    int32_t newCancelNum = oldCancelNum + static_cast<int32_t>(newCinematicVideoInfo.cancelWaitTimeMap.size());
    int32_t newCancelWaitAvgTime = 0;
    if (newCancelNum != 0) {
        newCancelWaitAvgTime = CalculateAvgWaitTime(CinematicWaitType::CANCEL_CINEMATIC,
            newCinematicVideoInfo, oldCancelNum, oldCancelAvgTime);
    }

    int32_t oldProcessNum = prefs->GetInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_SUCCESS_TIMES);
    int32_t oldProcessAvgTime = prefs->GetInt(CINEMATIC_VIDEO_PROCESS_AVG_TIME);
    int32_t newProcessNum = oldProcessNum + static_cast<int32_t>(newCinematicVideoInfo.processWaitTimeMap.size());
    int32_t newProcessWaitAvgTime = 0;
    if (newProcessNum != 0) {
        newProcessWaitAvgTime = CalculateAvgWaitTime(CinematicWaitType::PROCESS_CINEMATIC,
            newCinematicVideoInfo, oldProcessNum, oldProcessAvgTime);
    }

    int32_t lowQualityAccessTimes =
        prefs->GetInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES) + newCinematicVideoInfo.accessTimesLow;
    int32_t highQualityAccessTimes =
        prefs->GetInt(CINEMATIC_VIDEO_KEY_HIGH_QUALITY_ACCESS_TIMES) + newCinematicVideoInfo.accessTimesHigh;
    int32_t lowQualityUriAccessTimes =
        prefs->GetInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_URI_ACCESS_TIMES) + newCinematicVideoInfo.uriAccessTimesLow;
    int32_t highQualityUriAccessTimes =
        prefs->GetInt(CINEMATIC_VIDEO_KEY_HIGH_QUALITY_URI_ACCESS_TIMES) + newCinematicVideoInfo.uriAccessTimesHigh;
    int32_t multistageSuccessTime = prefs->GetInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_SUCCESS_TIMES) +
        newCinematicVideoInfo.multistageSuccessTimes;
    int32_t multistageFailedTime = prefs->GetInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_FAILED_TIMES) +
        newCinematicVideoInfo.multistageFailedTimes;

    prefs->PutInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_ACCESS_TIMES, lowQualityAccessTimes);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_HIGH_QUALITY_ACCESS_TIMES, highQualityAccessTimes);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_LOW_QUALITY_URI_ACCESS_TIMES, lowQualityUriAccessTimes);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_HIGH_QUALITY_URI_ACCESS_TIMES, highQualityUriAccessTimes);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_CANCEL_NUM, newCancelNum);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_CANCEL_WAIT_AVG_TIME, newCancelWaitAvgTime);
    prefs->PutInt(CINEMATIC_VIDEO_PROCESS_AVG_TIME, newProcessWaitAvgTime);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_SUCCESS_TIMES, multistageSuccessTime);
    prefs->PutInt(CINEMATIC_VIDEO_KEY_MULTISTAGE_FAILED_TIMES, multistageFailedTime);

    prefs->FlushSync();
}

} // namespace Media
} // namespace OHOS