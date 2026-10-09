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

#ifndef OHOS_MEDIALIBRARY_ASTCSTAT_H
#define OHOS_MEDIALIBRARY_ASTCSTAT_H

#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "media_log.h"
#include "thumbnail_const.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

enum class AstcPhase : int32_t {
    DEFAULT = 0,
    PHASE1,
    PHASE2,
    PHASE3,
    PHASE4,
    PHASE5
};

enum class AstcGenScene : int32_t {
    DEFAULT = 0,
    SCREEN_ON,
    NOCHARGING_SCREENOFF,
    CHARGING_SCREENOFF
};

struct SceneStat {
    AstcGenScene sceneKey_;
    int64_t duration_{0};
    uint32_t astcCount_{0};
    SceneStat& operator+=(SceneStat& sceneStat)
    {
        if (sceneKey_ != sceneStat.sceneKey_) {
            MEDIA_ERR_LOG("scene not the same\n");
            return *this;
        }
        duration_ += sceneStat.duration_;
        astcCount_ += sceneStat.astcCount_;
        return *this;
    }
};

struct PhaseStat {
    AstcPhase phase_;
    using RetValue = std::string;
    using RetCount = int32_t;
    int64_t startTime_{0};
    int64_t endTime_{0};
    std::unordered_map<AstcGenScene, SceneStat> scenes_;
    std::unordered_map<RetValue, RetCount> retValues_;
    PhaseStat& operator+=(PhaseStat &phaseStat)
    {
        if (phaseStat.startTime_ != 0 && startTime_ > phaseStat.startTime_) {
            startTime_ = phaseStat.startTime_;
        }
        if (endTime_ < phaseStat.endTime_) {
            endTime_ = phaseStat.endTime_;
        }
        for (auto [_, s] : phaseStat.scenes_) {
            if (scenes_.count(s.sceneKey_)) {
                scenes_[s.sceneKey_] += s;
            } else {
                scenes_[s.sceneKey_] = s;
            }
        }
        return *this;
    }
};

struct PhasesStat {
    std::unordered_map<AstcPhase, PhaseStat> phases_;
};

class MediaLibraryAstcStat {
public:
    EXPORT static MediaLibraryAstcStat &GetInstance();
    EXPORT void AddAstcInfo(int64_t startTime, GenerateScene genScene, AstcGenScene sceneKey,
        const std::string &id = "");
    EXPORT AstcPhase GetAstcPhase(int32_t totalAstcCount, GenerateScene genScene);
    EXPORT void GetInterruptInfo(bool isScreenOff, bool power, bool thermal, bool charging);
    EXPORT std::string GetJson();
    EXPORT void ClearOldData();
    EXPORT static bool ConvertToJson(nlohmann::json& jsonPhasesStat, const PhasesStat& phasesStat,
        int32_t totalAstcCount, int64_t updateDuration = 0, const std::string& uidType = "");
    EXPORT static bool ConvertToStruct(const nlohmann::json& jsonPhasesStat, PhasesStat& phasesStat,
        int32_t& totalAstcCount);
    EXPORT void TryToReadAstcInfoFromJsonFile();
    EXPORT bool ReadAstcInfoFromJsonFile(PhasesStat& phasesStat, int32_t& totalAstcCount);
    EXPORT bool WriteAstcInfoToJsonFile(const PhasesStat& phasesStat, int32_t totalAstcCount);
    EXPORT bool IsBackupGroundTaskEmpty();
    EXPORT void SetDoubleUpgradeInfo(int64_t updateDuration, const std::string& uidType);
private:
    std::string GetJsonStr();
    bool CheckId(const std::string& id);
    bool WriteJsonFile(const std::string &filePath, const nlohmann::json &j);
    bool ReadJsonFile(const std::string &filePath, nlohmann::json &j);
    PhasesStat phasesStat_{};
    int32_t totalAstcCount_{0};
    int64_t lastReportTime_{0};
    int64_t updateDuration_{0};
    std::string uidType_;
    std::mutex mutex_;
};

} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_PHOTOSYNCSTAT_H