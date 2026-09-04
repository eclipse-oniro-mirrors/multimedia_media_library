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

#include <fstream>
#include <list>

#include "dfx_utils.h"
#include "dfx_database_utils.h"
#include "file_utils.h"
#include "medialibrary_astc_stat.h"
#include "medialibrary_type_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "thumbnail_const.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_generate_worker_manager.h"

namespace OHOS {
namespace Media {

MediaLibraryAstcStat &MediaLibraryAstcStat::GetInstance()
{
    static MediaLibraryAstcStat instance;
    return instance;
}

template <typename T>
auto enum_to_value(T enumValue) -> std::underlying_type_t<T>
{
    return static_cast<std::underlying_type_t<T>>(enumValue);
}

static nlohmann::json ConvertSceneStatToJson(const SceneStat &sceneStat, AstcGenScene sceneType)
{
    nlohmann::json jsonSceneStat;
    jsonSceneStat["scene"] = enum_to_value(sceneType);
    jsonSceneStat["duration"] = sceneStat.duration_;
    jsonSceneStat["astc"] = sceneStat.astcCount_;
    return jsonSceneStat;
}

static nlohmann::json ConvertPhaseStatToJson(const PhaseStat &phaseStat, AstcPhase phaseType)
{
    nlohmann::json jsonPhaseStat;
    jsonPhaseStat["phase"] = enum_to_value(phaseType);
    jsonPhaseStat["phase_start_time"] = phaseStat.startTime_;
    jsonPhaseStat["phase_end_time"] = phaseStat.endTime_;
    jsonPhaseStat["interruptArr"] = nlohmann::json::array();
    for (const auto &[retValue, retCount] : phaseStat.retValues_) {
        nlohmann::json jsonRetStat;
        jsonRetStat[retValue] = retCount;
        jsonPhaseStat["interruptArr"].emplace_back(jsonRetStat);
    }
    nlohmann::json scenesJson;
    for (const auto &[sceneType, sceneStat] : phaseStat.scenes_) {
        std::string key = "scene" + std::to_string(enum_to_value(sceneType));
        jsonPhaseStat[key] = ConvertSceneStatToJson(sceneStat, sceneType);
    }
    return jsonPhaseStat;
}

bool MediaLibraryAstcStat::ConvertToJson(nlohmann::json& jsonPhasesStat, const PhasesStat& phasesStat,
    int32_t totalAstcCount, int64_t updateDuration, const std::string& uidType)
{
    jsonPhasesStat["totalAstcCount"] = totalAstcCount;
    if (updateDuration > 0 && !uidType.empty()) {
        jsonPhasesStat["updateDuration"] = updateDuration;
        jsonPhasesStat["uid"] = uidType;
    }
    for (const auto &[phaseType, phaseStat] : phasesStat.phases_) {
        std::string key = "phase" + std::to_string(enum_to_value(phaseType));
        jsonPhasesStat[key] = ConvertPhaseStatToJson(phaseStat, phaseType);
    }
    return true;
}

static bool ConvertRetStatToStruct(const nlohmann::json &jsonPhaseStat, PhaseStat &phaseStat)
{
    for (const auto& jsonRetStat : jsonPhaseStat["interruptArr"]) {
        for (const auto& [key, value] : jsonRetStat.items()) {
            phaseStat.retValues_[key] = 0;
            if (value.is_number_integer()) {
                phaseStat.retValues_[key] = value.get<int32_t>();
            }
        }
    }
    return true;
}

static bool ConvertSceneStatToStruct(const nlohmann::json &jsonSceneStat, SceneStat &sceneStat, AstcGenScene& sceneType)
{
    if (jsonSceneStat.contains("scene") && jsonSceneStat["scene"].is_number_integer()) {
        sceneType = static_cast<AstcGenScene>(jsonSceneStat["scene"].get<int32_t>());
        sceneStat.sceneKey_ = sceneType;
    }
    if (jsonSceneStat.contains("duration") && jsonSceneStat["duration"].is_number_integer()) {
        sceneStat.duration_ = jsonSceneStat["duration"].get<int64_t>();
    }
    if (jsonSceneStat.contains("astc") && jsonSceneStat["astc"].is_number_integer()) {
        sceneStat.astcCount_ = static_cast<uint32_t>(jsonSceneStat["astc"].get<int32_t>());
    }

    return true;
}

static bool ConvertPhaseStatToStruct(const nlohmann::json &jsonPhaseStat, PhaseStat &phaseStat,
    AstcPhase& photoSyncPhase)
{
    if (jsonPhaseStat.contains("phase") && jsonPhaseStat["phase"].is_number_integer()) {
        photoSyncPhase = static_cast<AstcPhase>(jsonPhaseStat["phase"].get<int32_t>());
        phaseStat.phase_ = photoSyncPhase;
    }
    if (jsonPhaseStat.contains("phase_start_time") && jsonPhaseStat["phase_start_time"].is_number_integer()) {
        phaseStat.startTime_ = jsonPhaseStat["phase_start_time"].get<int64_t>();
    }
    if (jsonPhaseStat.contains("phase_end_time") && jsonPhaseStat["phase_end_time"].is_number_integer()) {
        phaseStat.endTime_ = jsonPhaseStat["phase_end_time"].get<int64_t>();
    }
    if (jsonPhaseStat.contains("interruptArr") && jsonPhaseStat["interruptArr"].is_array()) {
        ConvertRetStatToStruct(jsonPhaseStat, phaseStat);
    }
    for (int i = 0; i <= static_cast<int>(AstcGenScene::CHARGING_SCREENOFF); i++) {
        std::string key = "scene" + std::to_string(i);
        if (jsonPhaseStat.contains(key)) {
            const auto& jsonSceneStat = jsonPhaseStat[key];
            SceneStat sceneStat{};
            AstcGenScene sceneType{AstcGenScene::DEFAULT};
            ConvertSceneStatToStruct(jsonSceneStat, sceneStat, sceneType);
            if (phaseStat.scenes_.count(sceneType) == 0) {
                phaseStat.scenes_[sceneType] = sceneStat;
            } else {
                phaseStat.scenes_[sceneType] += sceneStat;
            }
        }
    }
    return true;
}

bool MediaLibraryAstcStat::ConvertToStruct(const nlohmann::json &jsonPhasesStat, PhasesStat &phasesStat,
    int32_t& totalAstcCount)
{
    if (jsonPhasesStat.contains("totalAstcCount") && jsonPhasesStat["totalAstcCount"].is_number_integer()) {
        totalAstcCount = jsonPhasesStat["totalAstcCount"].get<int32_t>();
    }
    for (int i = static_cast<int>(AstcPhase::PHASE1); i <= static_cast<int>(AstcPhase::PHASE5); i++) {
        std::string key = "phase" + std::to_string(i);
        if (jsonPhasesStat.contains(key)) {
            const auto& jsonPhaseStat = jsonPhasesStat[key];
            PhaseStat phaseStat{};
            AstcPhase phaseType{AstcPhase::DEFAULT};
            ConvertPhaseStatToStruct(jsonPhaseStat, phaseStat, phaseType);
            if (phasesStat.phases_.count(phaseType) == 0) {
                phasesStat.phases_[phaseType] = phaseStat;
            } else {
                phasesStat.phases_[phaseType] += phaseStat;
            }
        }
    }
    return true;
}

bool MediaLibraryAstcStat::ReadAstcInfoFromJsonFile(PhasesStat& phasesStat, int32_t& totalAstcCount)
{
    nlohmann::json jsonPhaseStat{};
    if (not ReadJsonFile(ASTC_JSON_FILE_PATH, jsonPhaseStat)) {
        MEDIA_ERR_LOG("ReadJsonFile failed!");
        return false;
    }
    if (not ConvertToStruct(jsonPhaseStat, phasesStat, totalAstcCount)) {
        MEDIA_ERR_LOG("convert from json to struct failed!");
        return false;
    }
    return true;
}

bool MediaLibraryAstcStat::WriteAstcInfoToJsonFile(const PhasesStat& phasesStat, int32_t totalAstcCount)
{
    nlohmann::json jsonPhaseStat{};
    if (not ConvertToJson(jsonPhaseStat, phasesStat, totalAstcCount)) {
        MEDIA_ERR_LOG("convert from struct to json failed!");
        return false;
    }
    if (not WriteJsonFile(ASTC_JSON_FILE_PATH, jsonPhaseStat)) {
        MEDIA_ERR_LOG("WriteJsonFile failed!");
        return false;
    }
    return true;
}

bool MediaLibraryAstcStat::WriteJsonFile(const std::string &filePath, const nlohmann::json &j)
{
    const std::string parentDir = MediaFileUtils::GetParentPath(filePath);
    if (!MediaFileUtils::CreateDirectory(parentDir)) {
        MEDIA_ERR_LOG("CreateDirectory failed, dir = %{public}s", DfxUtils::GetSafePath(parentDir).c_str());
        return false;
    }

    std::ofstream outFile(filePath, std::ofstream::out | std::ofstream::trunc);
    CHECK_AND_RETURN_RET_LOG(outFile.is_open(), false, "open filePath: %{private}s failed", filePath.c_str());
    outFile << j << std::endl;
    outFile.close();
    return true;
}

bool MediaLibraryAstcStat::ReadJsonFile(const std::string &filePath, nlohmann::json &j)
{
    std::ifstream inFile(filePath);
    CHECK_AND_RETURN_RET_LOG(inFile.is_open(), false, "open filePath: %{private}s failed", filePath.c_str());
    
    std::string buffer = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    j = nlohmann::json::parse(buffer, nullptr, false);
    inFile.close();
    return !j.is_discarded();
}

AstcPhase MediaLibraryAstcStat::GetAstcPhase(int32_t totalAstcCount, GenerateScene genScene)
{
    AstcPhase phaseKey = AstcPhase::DEFAULT;
    constexpr int32_t phase1MaxCount = 100;
    constexpr int32_t phase2MaxCount = 2000;
    constexpr int32_t phase3MaxCount = 20000;
    constexpr int32_t phase4MaxCount = 200000;
    if (totalAstcCount == phase1MaxCount || totalAstcCount == phase2MaxCount ||
        totalAstcCount == phase3MaxCount || totalAstcCount == phase4MaxCount) {
        GetJsonStr();
    }

    if (totalAstcCount <= phase1MaxCount) {
        phaseKey = AstcPhase::PHASE1;
    } else if (totalAstcCount <= phase2MaxCount && totalAstcCount > phase1MaxCount) {
        phaseKey = AstcPhase::PHASE2;
    } else if (totalAstcCount <= phase3MaxCount && totalAstcCount > phase2MaxCount) {
        phaseKey = AstcPhase::PHASE3;
    } else if (totalAstcCount <= phase4MaxCount && totalAstcCount > phase3MaxCount) {
        phaseKey = AstcPhase::PHASE4;
    } else {
        phaseKey = AstcPhase::PHASE5;
    }

    return phaseKey;
}

bool MediaLibraryAstcStat::IsBackupGroundTaskEmpty()
{
    std::shared_ptr<ThumbnailGenerateWorker> thumbnailWorker =
        ThumbnailGenerateWorkerManager::GetInstance().GetThumbnailWorker(ThumbnailTaskType::BACKGROUND);
    CHECK_AND_RETURN_RET_LOG(thumbnailWorker != nullptr, true, "thumbnailWorker is null");
    return thumbnailWorker->IsLowerQueueEmpty();
}

void MediaLibraryAstcStat::GetInterruptInfo(bool isScreenOff, bool isCharging,
    bool isPowerSufficient, bool isThermalLow)
{
    if (IsBackupGroundTaskEmpty()) {
        return;
    }
    std::string screenOn = "screenOn";
    std::string notCharging = "notCharging";
    std::string powerNotSufficient = "powerNotSufficient";
    std::string thermalHigh = "thermalHigh";

    std::lock_guard<std::mutex> lock(mutex_);
    AstcPhase phaseKey = GetAstcPhase(totalAstcCount_ + 1, GenerateScene::BACKGROUND);
    if (!phasesStat_.phases_.count(phaseKey)) {
        PhaseStat phase;
        phasesStat_.phases_[phaseKey] = phase;
    }
    auto &interruptStat = phasesStat_.phases_[phaseKey].retValues_;
    if (!isScreenOff) {
        if (interruptStat.count(screenOn)) {
            interruptStat[screenOn]++;
        } else {
            interruptStat[screenOn] = 1;
        }
    }
    if (!isCharging) {
        if (interruptStat.count(notCharging)) {
            interruptStat[notCharging]++;
        } else {
            interruptStat[notCharging] = 1;
        }
    }
    if (!isPowerSufficient) {
        if (interruptStat.count(powerNotSufficient)) {
            interruptStat[powerNotSufficient]++;
        } else {
            interruptStat[powerNotSufficient] = 1;
        }
    }
    if (!isThermalLow) {
        if (interruptStat.count(thermalHigh)) {
            interruptStat[thermalHigh]++;
        } else {
            interruptStat[thermalHigh] = 1;
        }
    }
}

static SceneStat GetScene(int64_t duration, AstcGenScene sceneKey)
{
    SceneStat scene;
    scene.sceneKey_ = sceneKey;
    scene.duration_ = duration;
    scene.astcCount_ = 1;
    return scene;
}

void MediaLibraryAstcStat::TryToReadAstcInfoFromJsonFile()
{
    if (FileUtils::IsFileExist(ASTC_JSON_FILE_PATH)) {
        ReadAstcInfoFromJsonFile(phasesStat_, totalAstcCount_);
    } else {
        totalAstcCount_ = DfxDatabaseUtils::QueryASTCThumb(true) + DfxDatabaseUtils::QueryASTCThumb(false);
    }
}

bool MediaLibraryAstcStat::CheckId(const std::string &id)
{
    static std::list<std::string> idList;
    constexpr int32_t maxSize = 2000;
    constexpr int32_t removeCount = 100;
    if (id == "") {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(idList.begin(), idList.end(), id) != idList.end()) {
        MEDIA_DEBUG_LOG("have statted id %{public}s", id.c_str());
        return true;
    }
    if (idList.size() >= maxSize) {
        for (size_t i = 0; i < removeCount; ++i) {
            if (idList.empty()) {
                break;
            }
            idList.pop_front();
        }
    }
    idList.push_back(id);
    return false;
}

void MediaLibraryAstcStat::AddAstcInfo(int64_t startTime, GenerateScene genScene, AstcGenScene sceneKey,
    const std::string &id)
{
    if (CheckId(id)) {
        return;
    }
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    AstcPhase phaseKey = GetAstcPhase(totalAstcCount_ + 1, genScene);
    MEDIA_DEBUG_LOG("phaseKey %{public}d GenerateScene %{public}d sceneKey %{public}d", static_cast<int32_t>(phaseKey),
        static_cast<int32_t>(genScene), static_cast<int32_t>(sceneKey));

    PhaseStat phase;
    phase.phase_ = phaseKey;
    phase.scenes_[sceneKey] = GetScene(endTime - startTime, sceneKey);
    phase.startTime_ = startTime;
    phase.endTime_ = endTime;
    std::lock_guard<std::mutex> lock(mutex_);
    if (totalAstcCount_ == 0) {
        TryToReadAstcInfoFromJsonFile();
    }
    totalAstcCount_++;
    if (phasesStat_.phases_.count(phaseKey)) {
        PhaseStat &phaseStat = phasesStat_.phases_[phaseKey];
        phaseStat += phase;
    } else {
        phasesStat_.phases_[phaseKey] = phase;
    }
    int64_t currentTime = MediaFileUtils::UTCTimeSeconds();
    constexpr int64_t oneHour = 3600;
    if (currentTime - lastReportTime_ > oneHour) {
        WriteAstcInfoToJsonFile(phasesStat_, totalAstcCount_);
        lastReportTime_ = currentTime;
    }
}

std::string MediaLibraryAstcStat::GetJson()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return GetJsonStr();
}

std::string MediaLibraryAstcStat::GetJsonStr()
{
    nlohmann::json jsn;
    ConvertToJson(jsn, phasesStat_, totalAstcCount_, updateDuration_, uidType_);
    MEDIA_INFO_LOG("json %{public}s", jsn.dump().c_str());
    return jsn.dump();
}

void MediaLibraryAstcStat::ClearOldData()
{
    std::lock_guard<std::mutex> lock(mutex_);
    phasesStat_.phases_.clear();
    totalAstcCount_ = 0;
    updateDuration_ = 0;
    uidType_ = "";
    if (FileUtils::IsFileExist(ASTC_JSON_FILE_PATH)) {
        FileUtils::DeleteFile(ASTC_JSON_FILE_PATH);
    }
}

void MediaLibraryAstcStat::SetDoubleUpgradeInfo(int64_t updateDuration, const std::string& uidType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    updateDuration_ = updateDuration;
    uidType_ = uidType;
    MEDIA_INFO_LOG("SetDoubleUpgradeInfo: updateDuration=%{public}" PRId64 ", uidType=%{public}s",
        updateDuration, uidType.c_str());
}
} // namespace Media
} // namespace OHOS
