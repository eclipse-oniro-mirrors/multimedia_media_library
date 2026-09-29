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

#define MLOG_TAG "HeifTranscodingCheckUtils"

#include "heif_transcoding_check_utils.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <sys/stat.h>

#include "heif_bundle_info_cache.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "bundle_constants.h"
#include "transcode_compatible_info_operations.h"
#include "directory_ex.h"
#include "permission_whitelist_utils.h"

using std::string;
using std::unordered_map;
using std::mutex;
using namespace nlohmann;
namespace OHOS {
namespace Media {
const std::string HEIF_TRANSCODING_CHECKLIST_NAME = "heif_transcoding_checklist.json";
const std::string HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH =
    "/system/etc/com.ohos.medialibrary.medialibrarydata/heif_transcoding/" + HEIF_TRANSCODING_CHECKLIST_NAME;
const string DUE_INSTALL_DIR =
    "/data/service/el1/public/update/param_service/install/system/etc/"
    "com.ohos.medialibrary.medialibrarydata/heif_transcoding/";
const string COTA_UPDATE_EVENT = "usual.event.DUE_HAP_CFG_UPDATED";
const string RECEIVE_UPDATE_MESSAGE = "ohos.permission.RECEIVE_UPDATE_MESSAGE";
const string COTA_EVENT_INFO_TYPE = "type";
const string COTA_EVENT_INFO_SUBTYPE = "subtype";
const string COTA_EVENT_INFO_SUBTYPE_VALUE = "heif_transcoding";
const string CLOUD_EVENT_INFO_TYPE_VALUE = "medialibrary_kit_whitelist";
const string LIST_STRATEGY = "listStrategy";
const string LIST_STRATEGY_WHITELIST = "whiteList";
const string LIST_STRATEGY_DENYLIST = "denyList";
const string PIXEL_50_STRATEGY = "50-Strategy";
const string PIXEL_200_STRATEGY = "200-Strategy";
const string PIXEL_50_WHITELIST = "50-whiteList";
const string PIXEL_50_DENYLIST = "50-denyList";
const string PIXEL_200_WHITELIST = "200-whiteList";
const string PIXEL_200_DENYLIST = "200-denyList";
const string JSON_VERSION = "version";

const int CONFIG_EVENT_SUBSCRIBE_DELAY_TIME = 300;

using WhiteList = std::unordered_map<std::string, std::string>;
using DenyList = std::unordered_set<std::string>;

// Forward declaration for version comparison
bool CompareVersion(const std::string& v1, const std::string& v2);

WhiteList HeifTranscodingCheckUtils::whiteList_;
DenyList HeifTranscodingCheckUtils::denyList_;
bool HeifTranscodingCheckUtils::isUseWhiteList_ = false;

WhiteList HeifTranscodingCheckUtils::whiteList50_;
WhiteList HeifTranscodingCheckUtils::denyList50_;
bool HeifTranscodingCheckUtils::isUseWhiteList50_ = true;
WhiteList HeifTranscodingCheckUtils::whiteList200_;
WhiteList HeifTranscodingCheckUtils::denyList200_;
bool HeifTranscodingCheckUtils::isUseWhiteList200_ = true;

sptr<AppExecFwk::IBundleMgr> HeifTranscodingCheckUtils::bundleMgr_ = nullptr;
mutex HeifTranscodingCheckUtils::bundleMgrMutex_;
std::shared_ptr<EventFwk::CommonEventSubscriber> HeifTranscodingCheckUtils::cotaUpdateSubscriber_{};
std::shared_mutex HeifTranscodingCheckUtils::checkListMutex_;
class HeifTranscodingCheckUtils::CotaUpdateReceiver : public EventFwk::CommonEventSubscriber {
public:
    explicit CotaUpdateReceiver(const EventFwk::CommonEventSubscribeInfo &subscribeInfo);
    ~CotaUpdateReceiver() {}
    void OnReceiveEvent(const EventFwk::CommonEventData &data) override;
};

// LCOV_EXCL_START
HeifTranscodingCheckUtils::CotaUpdateReceiver::CotaUpdateReceiver(
    const EventFwk::CommonEventSubscribeInfo &subscribeInfo)
    : EventFwk::CommonEventSubscriber(subscribeInfo)
{
}

void HeifTranscodingCheckUtils::CotaUpdateReceiver::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    std::string action = data.GetWant().GetAction();
    std::string type = data.GetWant().GetStringParam(COTA_EVENT_INFO_TYPE);
    std::string subtype = data.GetWant().GetStringParam(COTA_EVENT_INFO_SUBTYPE);
    MEDIA_INFO_LOG("CotaUpdateReceiver: action[%{public}s], type[%{public}s], subType[%{public}s]", action.c_str(),
        type.c_str(), subtype.c_str());

    if (action != COTA_UPDATE_EVENT) {
        MEDIA_ERR_LOG("other action, ignore.");
        return;
    }

    if (type == COTA_EVENT_INFO_SUBTYPE_VALUE) {
        HeifTranscodingCheckUtils::ReadCheckList();
    } else if (type == CLOUD_EVENT_INFO_TYPE_VALUE) {
        PermissionWhitelistUtils::OnReceiveEvent();
    } else {
        MEDIA_INFO_LOG("unmatched type[%{public}s], ignore.", type.c_str());
    }
}

bool IsFileExists(const std::string &fileName)
{
    struct stat statInfo {};
    return ((stat(fileName.c_str(), &statInfo)) == 0);
}

static std::string ReadVersionFromJson(const std::string &jsonPath)
{
    string realPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(jsonPath, realPath),
        "", "check path failed");
    std::ifstream file(realPath);
    if (!file.is_open()) {
        MEDIA_WARN_LOG("Failed to open json file: %{public}s", jsonPath.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jStr = buffer.str();
    file.close();
    if (jStr.empty() || !nlohmann::json::accept(jStr)) {
        return "";
    }
    nlohmann::json j = nlohmann::json::parse(jStr, nullptr, false);
    if (j.is_discarded() || !j.contains(JSON_VERSION) || !j[JSON_VERSION].is_string()) {
        return "";
    }
    std::string version = j[JSON_VERSION].get<std::string>();
    MEDIA_INFO_LOG("Version from %{public}s: %{public}s", jsonPath.c_str(), version.c_str());
    return version;
}

bool HeifTranscodingCheckUtils::IsDUEVersionHigher()
{
    std::string dueVersion = ReadVersionFromJson(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME);
    std::string localVersion = ReadVersionFromJson(HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH);
    if (dueVersion.empty() || localVersion.empty()) {
        MEDIA_WARN_LOG("Cannot compare versions: due[%{public}s], local[%{public}s]",
            dueVersion.c_str(), localVersion.c_str());
        return false;
    }
    // CompareVersion(a, b) returns true when a >= b
    return CompareVersion(dueVersion, localVersion);
}

sptr<AppExecFwk::IBundleMgr> HeifTranscodingCheckUtils::GetSysBundleManager()
{
    if (bundleMgr_ != nullptr) {
        return bundleMgr_;
    }

    std::lock_guard<mutex> lock(bundleMgrMutex_);
    if (bundleMgr_ != nullptr) {
        return bundleMgr_;
    }

    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to get SystemAbilityManager.");
        return nullptr;
    }

    auto bundleObj = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleObj == nullptr) {
        MEDIA_ERR_LOG("Remote object is nullptr.");
        return nullptr;
    }

    auto bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    if (bundleMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to iface_cast");
        return nullptr;
    }
    bundleMgr_ = bundleMgr;

    return bundleMgr_;
}

static bool CheckListDUE(nlohmann::json& json)
{
    bool dueFileHasPixelStrategy = false;
    if (!IsFileExists(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME)) {
        return false;
    }
    if (!HeifTranscodingCheckUtils::IsDUEVersionHigher()) {
        MEDIA_INFO_LOG("DUE version is not higher than local, skip DUE checklist");
        return false;
    }
    string realPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME, realPath),
        false, "check path failed");
    std::ifstream dueFile;
    dueFile.open(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME);
    if (!dueFile.is_open()) {
        return false;
    }
    std::stringstream dueBuffer;
    dueBuffer << dueFile.rdbuf();
    std::string dueJStr = dueBuffer.str();
    dueFile.close();
    if (dueJStr.empty() || !nlohmann::json::accept(dueJStr)) {
        return false;
    }
    nlohmann::json dueCheckListJson = nlohmann::json::parse(dueJStr, nullptr, false);
    if (dueCheckListJson.is_discarded()) {
        return false;
    }
    dueFileHasPixelStrategy = dueCheckListJson.contains(PIXEL_50_STRATEGY) ||
                            dueCheckListJson.contains(PIXEL_200_STRATEGY);
    if (dueFileHasPixelStrategy) {
        json = dueCheckListJson;
        return true;
    }
    return false;
}

int32_t HeifTranscodingCheckUtils::ParsePixelWhiteListFromFile()
{
    nlohmann::json dueCheckListJson;
    if (CheckListDUE(dueCheckListJson)) {
            ParseHighPixelCheckList(dueCheckListJson, DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME);
        return E_OK;
    }
    string realPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH, realPath),
        E_ERR, "check path failed");
    std::ifstream jFile;
    jFile.open(HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH);
    if (!jFile.is_open()) {
        MEDIA_WARN_LOG("Failed to open pixel whitelist file: %{public}s",
            HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH.c_str());
        return E_FAIL;
    }

    std::stringstream buffer;
    buffer << jFile.rdbuf();
    std::string jStr = buffer.str();
    jFile.close();

    if (!jStr.empty() && nlohmann::json::accept(jStr)) {
        nlohmann::json checkListJson = nlohmann::json::parse(jStr, nullptr, false);
        if (!checkListJson.is_discarded()) {
            ParseHighPixelCheckList(checkListJson, HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH);
            return E_OK;
        }
    }
    return E_FAIL;
}

int32_t HeifTranscodingCheckUtils::ReadCheckList()
{
    ParsePixelWhiteListFromFile();

    // Determine which file to use for standard lists based on version comparison
    std::string checkListPath;
    if (IsFileExists(DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME) && IsDUEVersionHigher()) {
        checkListPath = DUE_INSTALL_DIR + HEIF_TRANSCODING_CHECKLIST_NAME;
        MEDIA_INFO_LOG("Using DUE checklist (higher version)");
    } else {
        checkListPath = HEIF_TRANSCODING_CHECK_LIST_JSON_LOCAL_PATH;
        MEDIA_INFO_LOG("Using local checklist");
    }

    std::ifstream jFile;
    string realPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(checkListPath, realPath), E_ERR, "check path failed");
    jFile.open(checkListPath);
    if (!jFile.is_open()) {
        MEDIA_ERR_LOG("Failed to open file, Error: %{public}s", std::strerror(errno));
        return E_FAIL;
    }

    std::stringstream buffer;
    buffer << jFile.rdbuf();
    std::string jStr = buffer.str();
    nlohmann::json checkListJson;
    jFile.close();
    if (!jStr.empty() && nlohmann::json::accept(jStr)) {
        checkListJson = nlohmann::json::parse(jStr, nullptr, false);
        if (checkListJson.is_discarded()) {
            MEDIA_ERR_LOG("JSON parse error");
            return E_FAIL;
        }
    } else {
        MEDIA_ERR_LOG("Failed to get json, Error: %{public}s", std::strerror(errno));
        return E_FAIL;
    }

    ClearCheckList();
    ClearBundleInfoInCache();

    if (!checkListJson.contains(LIST_STRATEGY) || !checkListJson[LIST_STRATEGY].is_string()) {
        MEDIA_ERR_LOG("Invalid or missing 'listStrategy' in json");
        return E_FAIL;
    }
    string listStrategy = checkListJson[LIST_STRATEGY].get<std::string>();
    isUseWhiteList_ = (listStrategy == LIST_STRATEGY_WHITELIST);
    if (isUseWhiteList_) {
        return ParseWhiteList(checkListJson);
    }
    return ParseDenyList(checkListJson);
}

void HeifTranscodingCheckUtils::ClearCheckList()
{
    std::unique_lock<std::shared_mutex> lock(checkListMutex_);
    whiteList_.clear();
    denyList_.clear();
    isUseWhiteList_ = false;
}

int32_t HeifTranscodingCheckUtils::SubscribeCotaUpdatedEvent()
{
    CHECK_AND_RETURN_RET_WARN_LOG(cotaUpdateSubscriber_ == nullptr, E_OK, "cota update event already subscribed.");

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(COTA_UPDATE_EVENT);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetPermission(RECEIVE_UPDATE_MESSAGE);
    cotaUpdateSubscriber_ = std::make_shared<CotaUpdateReceiver>(subscribeInfo);
    CHECK_AND_RETURN_RET_LOG(cotaUpdateSubscriber_ != nullptr, E_FAIL, "cota update subscriber nullptr.");

    if (EventFwk::CommonEventManager::SubscribeCommonEvent(cotaUpdateSubscriber_)) {
        MEDIA_INFO_LOG("Subscribe cota update event successed");
        return E_OK;
    }
    MEDIA_ERR_LOG("Subscribe cota update event fail");
    return E_FAIL;
}

void HeifTranscodingCheckUtils::UnsubscribeCotaUpdatedEvent()
{
    CHECK_AND_RETURN_WARN_LOG(cotaUpdateSubscriber_ != nullptr, "cota update event not subscribed.");

    bool subscribeResult = EventFwk::CommonEventManager::UnSubscribeCommonEvent(cotaUpdateSubscriber_);
    MEDIA_INFO_LOG("subscribeResult = %{public}d", subscribeResult);
    cotaUpdateSubscriber_ = nullptr;
}

int32_t HeifTranscodingCheckUtils::InitCheckList()
{
    MediaLibraryTracer tracer;
    tracer.Start("InitCheckList Excute");
    CHECK_AND_RETURN_RET_INFO_LOG(ReadCheckList() == E_OK, E_FAIL, "ReadCheckList failed");
    CHECK_AND_RETURN_RET_INFO_LOG(SubscribeCotaUpdatedEvent() == E_OK, E_FAIL, "SubscribeCotaUpdatedEvent failed");
    ClearBundleInfoInCache();
    return E_OK;
}

std::vector<int> SplitVersion(const std::string& version)
{
    std::vector<int> parts;
    std::stringstream ss(version);
    std::string item;
    while (std::getline(ss, item, '.')) {
        if (MediaFileUtils::IsValidInteger(item)) {
            parts.push_back(std::stoi(item));
        }
    }
    return parts;
}

bool CompareVersion(const std::string& v1, const std::string& v2)
{
    std::vector<int> parts1 = SplitVersion(v1);
    std::vector<int> parts2 = SplitVersion(v2);

    size_t n = std::max(parts1.size(), parts2.size());
    parts1.resize(n, 0);
    parts2.resize(n, 0);

    for (size_t i = 0; i < n; ++i) {
        if (parts1[i] > parts2[i]) {
            return true;
        }
        if (parts1[i] < parts2[i]) {
            return false;
        }
    }
    return true;
}

int32_t HeifTranscodingCheckUtils::ParseWhiteList(const nlohmann::json &checkListJson)
{
    if (!checkListJson.contains(LIST_STRATEGY_WHITELIST) || !checkListJson[LIST_STRATEGY_WHITELIST].is_array()) {
        MEDIA_ERR_LOG("Invalid or missing 'whiteList' in json");
        return E_FAIL;
    }
    std::unique_lock<std::shared_mutex> lock(checkListMutex_);
    for (const auto &item : checkListJson[LIST_STRATEGY_WHITELIST]) {
        if (item.is_string()) {
            std::string str = item.get<std::string>();
            size_t atPos = str.find('@');
            if (atPos == std::string::npos || atPos == 0 || atPos == str.size() - 1) {
                MEDIA_ERR_LOG("Invalid format in whiteList item: %{public}s", str.c_str());
                continue;
            }
            std::string bundleName = str.substr(0, atPos);
            std::string version = str.substr(atPos + 1);
            MEDIA_DEBUG_LOG("Bundle %{public}s with version %{public}s added to whiteList",
                bundleName.c_str(), version.c_str());
            auto it = whiteList_.find(bundleName);
            if (it == whiteList_.end()) {
                whiteList_[bundleName] = version;
                continue;
            }
            // If the bundle already exists, keep the higher version
            if (CompareVersion(version, it->second)) {
                MEDIA_DEBUG_LOG("Bundle %{public}s version updated from %{public}s to %{public}s",
                    bundleName.c_str(), it->second.c_str(), version.c_str());
                it->second = version;
            }
        }
    }
    return E_OK;
}

int32_t HeifTranscodingCheckUtils::ParseDenyList(const nlohmann::json &checkListJson)
{
    if (!checkListJson.contains(LIST_STRATEGY_DENYLIST) || !checkListJson[LIST_STRATEGY_DENYLIST].is_array()) {
        MEDIA_ERR_LOG("Invalid or missing 'denyList' in json");
        return E_FAIL;
    }
    std::unique_lock<std::shared_mutex> lock(checkListMutex_);
    for (const auto &item : checkListJson[LIST_STRATEGY_DENYLIST]) {
        if (item.is_string()) {
            std::string str = item.get<std::string>();
            MEDIA_DEBUG_LOG("Bundle %{public}s added to denyList", str.c_str());
            denyList_.insert(str);
        }
    }
    return E_OK;
}

static void ParseHighPixelList(const nlohmann::json &checkListJson,
    const string &listStr, std::unordered_map<std::string, std::string> &list)
{
    for (const auto &item : checkListJson[listStr]) {
        if (item.is_string()) {
            std::string str = item.get<std::string>();
            size_t atPos = str.find('@');
            if (atPos == 0 || atPos == str.size() - 1) {
                MEDIA_ERR_LOG("Invalid format in 50-whiteList item: %{public}s", str.c_str());
                continue;
            }
            std::string bundleName = str.substr(0, atPos);
            std::string version;
            if (atPos == std::string::npos) {
                version = "0";
            } else {
                version = str.substr(atPos + 1);
            }
            MEDIA_DEBUG_LOG("Bundle %{public}s with version %{public}s added to 50-whiteList",
                bundleName.c_str(), version.c_str());
            auto it = list.find(bundleName);
            if (it == list.end()) {
                list[bundleName] = version;
                continue;
            }
            if (CompareVersion(version, it->second)) {
                it->second = version;
            }
        }
    }
}

int32_t HeifTranscodingCheckUtils::ParseHighPixelCheckList(const nlohmann::json &checkListJson,
    const std::string &path)
{
    MEDIA_INFO_LOG("ParseHighPixelCheckList from path: %{public}s", path.c_str());

    std::unique_lock<std::shared_mutex> lock(checkListMutex_);
    whiteList50_.clear();
    denyList50_.clear();
    isUseWhiteList50_ = true;
    whiteList200_.clear();
    denyList200_.clear();
    isUseWhiteList200_ = true;

    if (checkListJson.contains(PIXEL_50_STRATEGY) && checkListJson[PIXEL_50_STRATEGY].is_string()) {
        string strategy50 = checkListJson[PIXEL_50_STRATEGY].get<std::string>();
        isUseWhiteList50_ = (strategy50 == LIST_STRATEGY_WHITELIST);
        MEDIA_INFO_LOG("50 pixel strategy: %{public}s, isUseWhiteList: %{public}d",
            strategy50.c_str(), isUseWhiteList50_);
    }

    if (checkListJson.contains(PIXEL_200_STRATEGY) && checkListJson[PIXEL_200_STRATEGY].is_string()) {
        string strategy200 = checkListJson[PIXEL_200_STRATEGY].get<std::string>();
        isUseWhiteList200_ = (strategy200 == LIST_STRATEGY_WHITELIST);
        MEDIA_INFO_LOG("200 pixel strategy: %{public}s, isUseWhiteList: %{public}d",
            strategy200.c_str(), isUseWhiteList200_);
    }

    if (checkListJson.contains(PIXEL_50_WHITELIST) && checkListJson[PIXEL_50_WHITELIST].is_array()) {
        ParseHighPixelList(checkListJson, PIXEL_50_WHITELIST, whiteList50_);
    }

    if (checkListJson.contains(PIXEL_50_DENYLIST) && checkListJson[PIXEL_50_DENYLIST].is_array()) {
        ParseHighPixelList(checkListJson, PIXEL_50_DENYLIST, denyList50_);
    }

    if (checkListJson.contains(PIXEL_200_WHITELIST) && checkListJson[PIXEL_200_WHITELIST].is_array()) {
        ParseHighPixelList(checkListJson, PIXEL_200_WHITELIST, whiteList200_);
    }

    if (checkListJson.contains(PIXEL_200_DENYLIST) && checkListJson[PIXEL_200_DENYLIST].is_array()) {
        ParseHighPixelList(checkListJson, PIXEL_200_DENYLIST, denyList200_);
    }

    return E_OK;
}

TranscodeMode HeifTranscodingCheckUtils::CheckTranscodeMode(const std::string &bundleName,
    bool isHighPixel, bool isHeifFile)
{
    TranscodeMode transcodeMode = TranscodeMode::DEFAULT;
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), transcodeMode, "bundleName is empty");
    CompatibleInfo compatibleInfo;
    auto ret = TranscodeCompatibleInfoOperation::QueryCompatibleInfo(bundleName, compatibleInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, transcodeMode, "QueryCompatibleInfo failed");

    transcodeMode = PreferredCompatibleModeCheckUtils::CheckTranscodeMode(compatibleInfo, isHighPixel, isHeifFile);
    return transcodeMode;
}

void HeifTranscodingCheckUtils::ClearBundleInfoInCache()
{
    HeifBundleInfoCache::ClearBundleInfoInCache();
    HighPixelBundleInfoCache::ClearBundleInfoInCache();
}

bool HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(const std::string &bundleName)
{
    std::shared_lock<std::shared_mutex> lock(checkListMutex_);
    if (isUseWhiteList_) {
        auto it = whiteList_.find(bundleName);
        if (it == whiteList_.end()) {
            MEDIA_INFO_LOG("Bundle %{public}s is not in white list", bundleName.c_str());
            return true;
        }
        bool isSupport = false;
        if (HeifBundleInfoCache::GetBundleCacheInfo(bundleName, isSupport)) {
            MEDIA_INFO_LOG("[cache] %{public}s is use jpg [%{public}d]", bundleName.c_str(), isSupport);
            return isSupport;
        }
        AppExecFwk::BundleInfo bundleInfo;
        CHECK_AND_RETURN_RET_LOG(GetSysBundleManager() != nullptr, false, "bundleMgr is null");
        ErrCode state = GetSysBundleManager()->GetBundleInfoV9(
            bundleName, static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION),
            bundleInfo, OHOS::AppExecFwk::Constants::START_USERID);
        if (state != 0) {
            MEDIA_ERR_LOG("Failed to get bundle info for %{public}s, Error: %{public}d", bundleName.c_str(), state);
            return false;
        }
        if (!CompareVersion(bundleInfo.versionName, it->second)) {
            HeifBundleInfoCache::InsertBundleCacheInfo(bundleName, true);
            MEDIA_INFO_LOG("Bundle %{public}s version %{public}s is less than white list version %{public}s",
                bundleName.c_str(), bundleInfo.versionName.c_str(), it->second.c_str());
            return true;
        }
        HeifBundleInfoCache::InsertBundleCacheInfo(bundleName, false);
        MEDIA_INFO_LOG("Bundle %{public}s version %{public}s is in white list and meets the version requirement",
            bundleName.c_str(), bundleInfo.versionName.c_str());
        return false;
    } else {
        if (denyList_.find(bundleName) != denyList_.end()) {
            MEDIA_INFO_LOG("Bundle %{public}s is in deny list", bundleName.c_str());
            return true;
        }
        MEDIA_INFO_LOG("Bundle %{public}s is not in deny list", bundleName.c_str());
        return false;
    }
}

bool HeifTranscodingCheckUtils::CheckHighPixelWhiteList(const std::string &bundleName,
    const HighPixelType &pixelType, const WhiteList* whiteList)
{
    std::shared_lock<std::shared_mutex> lock(checkListMutex_);
    auto it = whiteList->find(bundleName);
    if (it == whiteList->end()) {
        MEDIA_INFO_LOG("Bundle %{public}s is not in %{public}d pixel white list", bundleName.c_str(),
            static_cast<int>(pixelType));
        return false;
    }
    bool isSupport = false;
    if (HighPixelBundleInfoCache::GetBundleCacheInfo(bundleName, isSupport, pixelType)) {
        MEDIA_INFO_LOG("[cache] %{public}s is use highPixel [%{public}d] for %{public}d pixel",
            bundleName.c_str(), isSupport, static_cast<int>(pixelType));
        return isSupport;
    }
    AppExecFwk::BundleInfo bundleInfo;
    CHECK_AND_RETURN_RET_LOG(GetSysBundleManager() != nullptr, false, "bundleMgr is null");
    ErrCode state = GetSysBundleManager()->GetBundleInfoV9(
        bundleName, static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION),
        bundleInfo, OHOS::AppExecFwk::Constants::START_USERID);
    if (state != 0) {
        MEDIA_ERR_LOG("Failed to get bundle info for %{public}s, Error: %{public}d", bundleName.c_str(), state);
        return false;
    }
    if (!CompareVersion(bundleInfo.versionName, it->second)) {
        HighPixelBundleInfoCache::InsertBundleCacheInfo(bundleName, false, pixelType);
        MEDIA_DEBUG_LOG("Bundle %{public}s version %{public}s is less "
            "than %{public}d pixel white list version %{public}s",
            bundleName.c_str(), bundleInfo.versionName.c_str(), static_cast<int>(pixelType), it->second.c_str());
        return false;
    }
    HighPixelBundleInfoCache::InsertBundleCacheInfo(bundleName, true, pixelType);
    MEDIA_DEBUG_LOG("Bundle %{public}s version %{public}s is in "
        "%{public}d pixel white list and meets the version requirement",
        bundleName.c_str(), bundleInfo.versionName.c_str(), static_cast<int>(pixelType));
    return true;
}

bool HeifTranscodingCheckUtils::CheckHighPixelBlackList(const std::string &bundleName,
    const HighPixelType &pixelType, const WhiteList* denyList)
{
    std::shared_lock<std::shared_mutex> lock(checkListMutex_);
    auto it = denyList->find(bundleName);
    if (it != denyList->end()) {
        bool isSupport = false;
        if (HighPixelBundleInfoCache::GetBundleCacheInfo(bundleName, isSupport, pixelType)) {
            MEDIA_INFO_LOG("[cache] %{public}s is use highPixel [%{public}d] for %{public}d pixel",
                bundleName.c_str(), isSupport, static_cast<int>(pixelType));
            return isSupport;
        }
        AppExecFwk::BundleInfo bundleInfo;
        CHECK_AND_RETURN_RET_LOG(GetSysBundleManager() != nullptr, false, "bundleMgr is null");
        ErrCode state = GetSysBundleManager()->GetBundleInfoV9(
            bundleName, static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION),
            bundleInfo, OHOS::AppExecFwk::Constants::START_USERID);
        if (state != 0) {
            MEDIA_ERR_LOG("Failed to get bundle info for %{public}s, Error: %{public}d",
                bundleName.c_str(), state);
            return false;
        }
        if (it->second == "0") {
            HighPixelBundleInfoCache::InsertBundleCacheInfo(bundleName, false, pixelType);
            MEDIA_DEBUG_LOG("Bundle %{public}s include version all", bundleName.c_str());
            return false;
        }
        if (it->second == "0" || !CompareVersion(bundleInfo.versionName, it->second)) {
            HighPixelBundleInfoCache::InsertBundleCacheInfo(bundleName, false, pixelType);
            MEDIA_DEBUG_LOG("Bundle %{public}s version %{public}s is less "
                "than %{public}d pixel deny list version %{public}s",
                bundleName.c_str(), bundleInfo.versionName.c_str(), static_cast<int>(pixelType), it->second.c_str());
            return false;
        }
        HighPixelBundleInfoCache::InsertBundleCacheInfo(bundleName, true, pixelType);
        MEDIA_INFO_LOG("Bundle %{public}s is in %{public}d pixel deny list", bundleName.c_str(),
            static_cast<int>(pixelType));
        return true;
    }
    MEDIA_INFO_LOG("Bundle %{public}s is not in %{public}d pixel deny list", bundleName.c_str(),
        static_cast<int>(pixelType));
    return true;
}

bool HeifTranscodingCheckUtils::CanSupportedHighPixelPicture(const std::string &bundleName,
    const HighPixelType &pixelType)
{
    std::shared_lock<std::shared_mutex> lock(checkListMutex_);
    bool isUseWhiteList = false;
    WhiteList* whiteList = nullptr;
    WhiteList* denyList = nullptr;

    if (pixelType == HighPixelType::PIXEL_50) {
        isUseWhiteList = isUseWhiteList50_;
        whiteList = &whiteList50_;
        denyList = &denyList50_;
    } else if (pixelType == HighPixelType::PIXEL_200) {
        isUseWhiteList = isUseWhiteList200_;
        whiteList = &whiteList200_;
        denyList = &denyList200_;
    } else {
        MEDIA_ERR_LOG("Invalid TranscodeType");
        return false;
    }

    if (isUseWhiteList) {
        return CheckHighPixelWhiteList(bundleName, pixelType, whiteList);
    } else {
        return CheckHighPixelBlackList(bundleName, pixelType, denyList);
    }
}
// LCOV_EXCL_STOP
}
}