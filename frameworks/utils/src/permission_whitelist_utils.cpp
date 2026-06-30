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

#define MLOG_TAG "PermissionWhitelistUtils"

#include "permission_whitelist_utils.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <sys/stat.h>

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "bundle_constants.h"
#include "parameter.h"
#include "ipc_skeleton.h"
#include "accesstoken_kit.h"

#include "permission_utils.h"

using std::string;
using std::unordered_map;
using std::mutex;
using namespace nlohmann;
using namespace OHOS::Security::AccessToken;
namespace OHOS {
namespace Media {
const std::string MEDIA_KIT_WHITE_LIST_NAME = "medialibrary_kit_whitelist.json";
const std::string MEDIA_KIT_WHITE_LIST_JSON_LOCAL_DIR =
    "/system/etc/com.ohos.medialibrary.medialibrarydata/medialibrary_kit_whitelist/";
const std::string MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH =
    MEDIA_KIT_WHITE_LIST_JSON_LOCAL_DIR + MEDIA_KIT_WHITE_LIST_NAME;
const string DUE_INSTALL_DIR =
    "/data/service/el1/public/update/param_service/install/system/etc/"
    "com.ohos.medialibrary.medialibrarydata/medialibrary_kit_whitelist/";
const string LIST_VERSION = "version";
const string LIST_APPLICATIONS = "applications";
const string LIST_APPIDENTIFIER = "appIdentifier";
const string LIST_ALLOW_API_VERSION = "allowedApiVersion";

using WhiteList = std::unordered_map<std::string, int>;

sptr<AppExecFwk::IBundleMgr> PermissionWhitelistUtils::bundleMgr_ = nullptr;
mutex PermissionWhitelistUtils::bundleMgrMutex_;
mutex PermissionWhitelistUtils::whiteListMutex_;
WhiteList PermissionWhitelistUtils::whiteList_;
std::atomic_bool PermissionWhitelistUtils::isLoadWhiteList_{false};

void PermissionWhitelistUtils::OnReceiveEvent()
{
    CHECK_AND_RETURN_INFO_LOG(isLoadWhiteList_.load(), "Not LoadWhiteList");
    PermissionWhitelistUtils::LoadWhiteList();
}

static std::vector<int> SplitApiVersion(const std::string &version)
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

static bool IsLeftVersionHigher(const std::string &v1, const std::string &v2)
{
    std::vector<int> parts1 = SplitApiVersion(v1);
    std::vector<int> parts2 = SplitApiVersion(v2);

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
    return false;
}

static nlohmann::json LoadJsonFile(const std::string &jsonPath, bool &isLoad)
{
    bool isJsonPathValid = (jsonPath == MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH ||
        jsonPath == DUE_INSTALL_DIR + MEDIA_KIT_WHITE_LIST_NAME);
    CHECK_AND_RETURN_RET_LOG(isJsonPathValid, {}, "invalid whitelist json path");

    std::ifstream jFile(jsonPath);
    CHECK_AND_RETURN_RET_LOG(jFile.is_open(), {}, "Failed to open file, Error: %{public}s", std::strerror(errno));

    std::stringstream buffer;
    buffer << jFile.rdbuf();
    std::string jStr = buffer.str();
    jFile.close();
    CHECK_AND_RETURN_RET_LOG((!jStr.empty() && nlohmann::json::accept(jStr)), {}, "Content is empty or not valid JSON");

    nlohmann::json jsonFile = nlohmann::json::parse(jStr, nullptr, false);
    CHECK_AND_RETURN_RET_LOG(!jsonFile.is_discarded(), {}, "JSON parse error");
    CHECK_AND_RETURN_RET_LOG((jsonFile.contains(LIST_VERSION) && jsonFile[LIST_VERSION].is_string() &&
        !jsonFile[LIST_VERSION].get<std::string>().empty()), {}, "Invalid, missing or empty 'version' in json");
    CHECK_AND_RETURN_RET_LOG((jsonFile.contains(LIST_APPLICATIONS) && jsonFile[LIST_APPLICATIONS].is_array()), {},
        "Invalid or missing 'applications' in json");
    isLoad = true;
    return jsonFile;
}

static std::string GetClientBundleName()
{
    std::string bundleName;
    int32_t uid = IPCSkeleton::GetCallingUid();
    PermissionUtils::GetClientBundle(uid, bundleName);
    CHECK_AND_WARN_LOG(!bundleName.empty(), "bundleName is empty");
    return bundleName;
}

int32_t PermissionWhitelistUtils::InitWhiteList()
{
    MediaLibraryTracer tracer;
    tracer.Start("InitWhiteList Excute");
    CHECK_AND_RETURN_RET_INFO_LOG(LoadWhiteList() == E_OK, E_FAIL, "LoadWhiteList failed");
    return E_OK;
}

int32_t PermissionWhitelistUtils::LoadWhiteList()
{
    isLoadWhiteList_.store(true);
    std::lock_guard<mutex> lock(whiteListMutex_);

    const std::string dueFile = DUE_INSTALL_DIR + MEDIA_KIT_WHITE_LIST_NAME;
    const std::string localFile = MEDIA_KIT_WHITE_LIST_JSON_LOCAL_PATH;
    bool dueIsLoad = false;
    bool localIsLoad = false;
    nlohmann::json dueJson = LoadJsonFile(dueFile, dueIsLoad);
    nlohmann::json localJson = LoadJsonFile(localFile, localIsLoad);
    CHECK_AND_RETURN_RET_LOG((dueIsLoad || localIsLoad), E_FAIL, "Both dueJson and localJson are missing or broken");

    nlohmann::json *higherVerFile = nullptr;
    if (dueIsLoad && localIsLoad) {
        std::string dueVer = dueJson[LIST_VERSION].get<std::string>();
        std::string localVer = localJson[LIST_VERSION].get<std::string>();
        higherVerFile = IsLeftVersionHigher(dueVer, localVer) ? &dueJson : &localJson;
    } else {
        higherVerFile = dueIsLoad ? &dueJson : &localJson;
    }
    CHECK_AND_RETURN_RET_LOG(higherVerFile != nullptr, E_FAIL, "higherVerFile is nullptr");
    return ParseWhiteList(*higherVerFile);
}

int32_t PermissionWhitelistUtils::ParseWhiteList(const nlohmann::json &higherVerFile)
{
    std::unordered_map<std::string, int> tmpWhiteList;
    for (const auto &app : higherVerFile[LIST_APPLICATIONS]) {
        CHECK_AND_CONTINUE_ERR_LOG((app.contains(LIST_APPIDENTIFIER) && app.contains(LIST_ALLOW_API_VERSION)),
            "Missing appIdentifier or allowedApiVersion");

        std::string appIdentifier = app[LIST_APPIDENTIFIER].get<std::string>();
        int allowedApiVersion = app[LIST_ALLOW_API_VERSION].get<int>();

        auto [it, inserted] = tmpWhiteList.emplace(appIdentifier, allowedApiVersion);
        if (!inserted && allowedApiVersion > it->second) {
            MEDIA_DEBUG_LOG("appIdentifier's allowedApiVersion updated from %{public}d to %{public}d",
                it->second, allowedApiVersion);
            it->second = allowedApiVersion;
        } else if (!inserted) {
            MEDIA_DEBUG_LOG("appIdentifier is already in whiteList");
        }
    }
    tmpWhiteList.swap(whiteList_);
    return E_OK;
}

sptr<AppExecFwk::IBundleMgr> PermissionWhitelistUtils::GetSysBundleManager()
{
    std::lock_guard<mutex> lock(bundleMgrMutex_);
    CHECK_AND_RETURN_RET_INFO_LOG(bundleMgr_ == nullptr, bundleMgr_, "return bundleMgr_");

    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(systemAbilityMgr != nullptr, nullptr, "Failed to get SystemAbilityManager.");

    auto bundleObj = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    CHECK_AND_RETURN_RET_LOG(bundleObj != nullptr, nullptr, "Remote object is nullptr.");

    auto bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    CHECK_AND_RETURN_RET_LOG(bundleMgr != nullptr, nullptr, "Failed to iface_cast");
    bundleMgr_ = bundleMgr;

    return bundleMgr_;
}

int32_t PermissionWhitelistUtils::CheckWhiteList()
{
    CHECK_AND_RETURN_RET_WARN_LOG(!PermissionUtils::IsSystemApp(), E_SUCCESS, "current is system app, no need check");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    CHECK_AND_RETURN_RET_WARN_LOG(tokenType == ATokenTypeEnum::TOKEN_HAP, E_SUCCESS,
        "current is not normal hap, no need check");

    static std::once_flag loadFlag;
    std::call_once(loadFlag, []() {
        if (InitWhiteList() != E_OK) {
            MEDIA_ERR_LOG("whitelist init fail, all auth will deny");
        }
    });

    CHECK_AND_RETURN_RET_WARN_LOG(!whiteList_.empty(), E_SUCCESS, "whiteList_ is empty");

    std::string hapBundleName = GetClientBundleName();
    CHECK_AND_RETURN_RET_LOG(!hapBundleName.empty(), E_PERMISSION_DENIED, "get caller hapBundleName name fail");

    auto bundleManager = GetSysBundleManager();
    CHECK_AND_RETURN_RET_LOG(bundleManager != nullptr, E_PERMISSION_DENIED, "GetSysBundleManager() returned nullptr");

    AppExecFwk::BundleInfo bundleInfo;
    ErrCode state = bundleManager->GetBundleInfoV9(
        hapBundleName, static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION),
        bundleInfo, OHOS::AppExecFwk::Constants::START_USERID);
    CHECK_AND_RETURN_RET_LOG(state == 0, E_PERMISSION_DENIED,
        "Failed to get bundle info for %{public}s, Error: %{public}d", hapBundleName.c_str(), state);

    std::string appIdentifier = bundleInfo.signatureInfo.appIdentifier;
    auto it = whiteList_.find(appIdentifier);
    CHECK_AND_RETURN_RET_LOG((it != whiteList_.end()), E_PERMISSION_DENIED, "appIdentifier not in whiteList");

    const int32_t whiteListApiVersion = it->second;
    CHECK_AND_RETURN_RET_WARN_LOG(whiteListApiVersion != 0, E_SUCCESS, "the whiteListApiVersion of appIdentifier is 0");

    int32_t systemApiVersion = GetSdkApiVersion();
    CHECK_AND_RETURN_RET_LOG(systemApiVersion > 0, E_PERMISSION_DENIED, "systemApiVersion <= 0");
    CHECK_AND_RETURN_RET_LOG(!(whiteListApiVersion > 0 && systemApiVersion > whiteListApiVersion), E_PERMISSION_DENIED,
        "systemApiVersion=%{public}d > whiteListApiVersion=%{public}d", systemApiVersion, whiteListApiVersion);
    return E_SUCCESS;
}
}
}