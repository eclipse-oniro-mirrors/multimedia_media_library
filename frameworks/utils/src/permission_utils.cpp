/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include "permission_utils.h"

#include <unordered_set>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "el5_filekey_manager_kit.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "privacy_kit.h"
#include "system_ability_definition.h"
#include "tokenid_kit.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::AppExecFwk::Constants;

const int32_t CAPACITY = 50;
const int32_t HDC_SHELL_UID = 2000;
const int32_t BASE_USER_RANGE = 200000;

std::mutex PermissionUtils::uninstallMutex_;
std::list<std::pair<int32_t, BundleInfo>> PermissionUtils::bundleInfoList_ = {};
std::unordered_map<int32_t, std::list<std::pair<int32_t, BundleInfo>>::iterator> PermissionUtils::bundleInfoMap_ = {};

bool g_isDelayTask;
std::mutex addPhotoPermissionRecordLock_;
std::thread delayTask_;
std::vector<Security::AccessToken::AddPermParamInfo> infos_;

sptr<AppExecFwk::IBundleMgr> PermissionUtils::bundleMgr_ = nullptr;
mutex PermissionUtils::bundleMgrMutex_;
sptr<AppExecFwk::IBundleMgr> PermissionUtils::GetSysBundleManager()
{
    if (bundleMgr_ != nullptr) {
        return bundleMgr_;
    }

    lock_guard<mutex> lock(bundleMgrMutex_);
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

void PermissionUtils::GetBundleNameFromCache(int uid, string &bundleName)
{
    lock_guard<mutex> lock(uninstallMutex_);
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end() && !iter->second->second.bundleName.empty()) {
        bundleInfoList_.splice(bundleInfoList_.begin(), bundleInfoList_, iter->second);
        bundleName = iter->second->second.bundleName;
        return;
    }
    bundleMgr_ = GetSysBundleManager();
    if (bundleMgr_ == nullptr) {
        bundleName = "";
        return;
    }
    auto result = bundleMgr_->GetBundleNameForUid(uid, bundleName);
    if (!result) {
        bundleName = "";
        return;
    }

    UpdateBundleNameInCache(uid, bundleName);
}

void PermissionUtils::GetPackageNameFromCache(int uid, const string &bundleName, string &packageName)
{
    lock_guard<mutex> lock(uninstallMutex_);
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end() && !iter->second->second.packageName.empty()) {
        bundleInfoList_.splice(bundleInfoList_.begin(), bundleInfoList_, iter->second);
        packageName = iter->second->second.packageName;
        return;
    }

    int32_t userId = uid / BASE_USER_RANGE;
    MEDIA_DEBUG_LOG("uid:%{private}d, userId:%{private}d", uid, userId);

    AAFwk::Want want;
    auto bundleMgr = GetSysBundleManager();
    if (bundleMgr == nullptr) {
        MEDIA_ERR_LOG("Get BundleManager failed");
        packageName = "";
        return;
    }
    int ret = bundleMgr->GetLaunchWantForBundle(bundleName, want, userId);
    if (ret != ERR_OK) {
        MEDIA_ERR_LOG("Can not get bundleName by want, err=%{public}d, userId=%{private}d", ret, userId);
        packageName = "";
        return;
    }
    string abilityName = want.GetOperation().GetAbilityName();
    packageName = bundleMgr->GetAbilityLabel(bundleName, abilityName);

    UpdatePackageNameInCache(uid, packageName);
}

void PermissionUtils::GetAppIdFromCache(int uid, const string &bundleName, string &appId)
{
    lock_guard<mutex> lock(uninstallMutex_);
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end() && !iter->second->second.appId.empty()) {
        bundleInfoList_.splice(bundleInfoList_.begin(), bundleInfoList_, iter->second);
        appId = iter->second->second.appId;
        return;
    }
    int32_t userId = uid / BASE_USER_RANGE;
    MEDIA_DEBUG_LOG("uid:%{private}d, userId:%{private}d", uid, userId);

    auto bundleMgr_ = GetSysBundleManager();
    if (bundleMgr_ == nullptr) {
        MEDIA_ERR_LOG("Get BundleManager failed");
        return;
    }

    appId = bundleMgr_->GetAppIdByBundleName(bundleName, userId);

    UpdateAppIdInCache(uid, appId);
}

void PermissionUtils::UpdateLatestBundleInfo(int uid, const BundleInfo &bundleInfo)
{
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end()) {
        bundleInfoList_.erase(iter->second);
    }
    bundleInfoList_.push_front(make_pair(uid, bundleInfo));
    bundleInfoMap_[uid] = bundleInfoList_.begin();
    if (bundleInfoMap_.size() > CAPACITY) {
        int32_t deleteKey = bundleInfoList_.back().first;
        bundleInfoMap_.erase(deleteKey);
        bundleInfoList_.pop_back();
    }
}

void PermissionUtils::UpdateBundleNameInCache(int uid, const string &bundleName)
{
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end()) {
        BundleInfo bundleInfo = bundleInfoMap_[uid]->second;
        bundleInfo.bundleName = bundleName;
        UpdateLatestBundleInfo(uid, bundleInfo);
        return;
    }
    BundleInfo bundleInfo { bundleName, "", "" };
    UpdateLatestBundleInfo(uid, bundleInfo);
}

void PermissionUtils::UpdatePackageNameInCache(int uid, const string &packageName)
{
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end()) {
        BundleInfo bundleInfo = bundleInfoMap_[uid]->second;
        bundleInfo.packageName = packageName;
        UpdateLatestBundleInfo(uid, bundleInfo);
        return;
    }
    BundleInfo bundleInfo { "", packageName, "" };
    UpdateLatestBundleInfo(uid, bundleInfo);
}

void PermissionUtils::UpdateAppIdInCache(int uid, const string &appId)
{
    BundleInfo bundleInfo { "", "", appId };
    auto iter = bundleInfoMap_.find(uid);
    if (iter != bundleInfoMap_.end()) {
        bundleInfo = bundleInfoMap_[uid]->second;
        bundleInfo.appId = appId;
    }
    UpdateLatestBundleInfo(uid, bundleInfo);
}

void PermissionUtils::ClearBundleInfoInCache()
{
    lock_guard<mutex> lock(uninstallMutex_);
    bundleInfoMap_.clear();
    bundleInfoList_.clear();
    MEDIA_INFO_LOG("clear all info from cache");
}

void PermissionUtils::GetClientBundle(const int uid, string &bundleName)
{
    GetBundleNameFromCache(uid, bundleName);
}

void PermissionUtils::GetPackageName(const int uid, std::string &packageName)
{
    packageName = "";
    string bundleName;
    GetClientBundle(uid, bundleName);
    if (bundleName.empty()) {
        MEDIA_ERR_LOG("Get bundle name failed");
        return;
    }

    GetPackageNameFromCache(uid, bundleName, packageName);
}

bool inline ShouldAddPermissionRecord(const AccessTokenID &token)
{
    return (AccessTokenKit::GetTokenTypeFlag(token) == TOKEN_HAP);
}

void AddPermissionRecord(const AccessTokenID &token, const string &perm, const bool permGranted)
{
    if (!ShouldAddPermissionRecord(token)) {
        return;
    }

    int res = PrivacyKit::AddPermissionUsedRecord(token, perm, !!permGranted, !permGranted, true);
    if (res != 0) {
        /* Failed to add permission used record, not fatal */
        MEDIA_WARN_LOG("Failed to add permission used record: %{public}s, permGranted: %{public}d, err: %{public}d",
            perm.c_str(), permGranted, res);
    }
}

vector<AddPermParamInfo> GetPermissionRecord()
{
    lock_guard<mutex> lock(addPhotoPermissionRecordLock_);
    vector<AddPermParamInfo> result = infos_;
    infos_.clear();
    return result;
}

void AddPermissionRecord()
{
    vector<AddPermParamInfo> infos = GetPermissionRecord();
    for (const auto &info : infos) {
        int32_t ret = PrivacyKit::AddPermissionUsedRecord(info, true);
        if (ret != 0) {
            /* Failed to add permission used record, not fatal */
            MEDIA_WARN_LOG("Failed to add permission used record: %{public}s, permGranted: %{public}d, err: %{public}d",
                info.permissionName.c_str(), info.successCount, ret);
        }
        MEDIA_DEBUG_LOG("Info: token = %{private}d, perm = %{private}s, permGranted = %{private}d, \
            !permGranted = %{private}d, type = %{public}d", info.tokenId, info.permissionName.c_str(),
            info.successCount, info.failCount, info.type);
    }
    infos.clear();
}

void DelayAddPermissionRecord()
{
    string name("DelayAddPermissionRecord");
    pthread_setname_np(pthread_self(), name.c_str());
    MEDIA_INFO_LOG("DelayTask start");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AddPermissionRecord();
    g_isDelayTask = false;
    MEDIA_INFO_LOG("DelayTask end");
}

void DelayTaskInit()
{
    if (!g_isDelayTask) {
        MEDIA_INFO_LOG("DelayTaskInit");
        delayTask_ = thread(DelayAddPermissionRecord);
        delayTask_.detach();
        g_isDelayTask = true;
    }
}

void CollectPermissionRecord(const AccessTokenID &token, const string &perm,
    const bool permGranted, const PermissionUsedType type)
{
    lock_guard<mutex> lock(addPhotoPermissionRecordLock_);
    DelayTaskInit();

    if (!ShouldAddPermissionRecord(token)) {
        return;
    }

    AddPermParamInfo info = {token, perm, permGranted, !permGranted, type};
    auto iter = find_if(infos_.begin(), infos_.end(), [&token, &perm, type](auto &info) {
        return info.tokenId == token && info.permissionName == perm && info.type == type;
    });
    if (iter == infos_.end()) {
        infos_.push_back(info);
    } else if (permGranted) {
        iter->successCount += 1;
    } else if (!permGranted) {
        iter->failCount += 1;
    }
}

void PermissionUtils::CollectPermissionInfo(const string &permission,
    const bool permGranted, const PermissionUsedType type)
{
    AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    CollectPermissionRecord(tokenCaller, permission, permGranted, type);
}

bool PermissionUtils::CheckPhotoCallerPermission(const string &permission)
{
    PermissionUsedType type = PermissionUsedTypeValue::NORMAL_TYPE;
    AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    int res = AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (res != PermissionState::PERMISSION_GRANTED) {
        MEDIA_ERR_LOG("Have no media permission: %{public}s", permission.c_str());
        CollectPermissionRecord(tokenCaller, permission, false, type);
        return false;
    }
    CollectPermissionRecord(tokenCaller, permission, true, type);
    return true;
}

bool PermissionUtils::CheckPhotoCallerPermission(const vector<string> &perms)
{
    if (perms.empty()) {
        return false;
    }

    for (const auto &perm : perms) {
        if (!CheckPhotoCallerPermission(perm)) {
            return false;
        }
    }
    return true;
}

bool PermissionUtils::CheckCallerPermission(const string &permission)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckCallerPermission");

    AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
    int res = AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (res != PermissionState::PERMISSION_GRANTED) {
        MEDIA_ERR_LOG("Have no media permission: %{public}s", permission.c_str());
        AddPermissionRecord(tokenCaller, permission, false);
        return false;
    }
    AddPermissionRecord(tokenCaller, permission, true);

    return true;
}

/* Check whether caller has at least one of @perms */
bool PermissionUtils::CheckHasPermission(const vector<string> &perms)
{
    if (perms.empty()) {
        return false;
    }

    for (const auto &perm : perms) {
        if (CheckCallerPermission(perm)) {
            return true;
        }
    }

    return false;
}

/* Check whether caller has all the @perms */
bool PermissionUtils::CheckCallerPermission(const vector<string> &perms)
{
    if (perms.empty()) {
        return false;
    }

    for (const auto &perm : perms) {
        if (!CheckCallerPermission(perm)) {
            return false;
        }
    }
    return true;
}

uint32_t PermissionUtils::GetTokenId()
{
    return IPCSkeleton::GetCallingTokenID();
}

bool PermissionUtils::IsSystemApp()
{
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    return TokenIdKit::IsSystemAppByFullTokenID(tokenId);
}

bool PermissionUtils::CheckIsSystemAppByUid()
{
    int uid = IPCSkeleton::GetCallingUid();
    bundleMgr_ = GetSysBundleManager();
    if (bundleMgr_ == nullptr) {
        MEDIA_ERR_LOG("Can not get bundleMgr");
        return false;
    }
    return bundleMgr_->CheckIsSystemAppByUid(uid);
}

bool PermissionUtils::IsNativeSAApp()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    ATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    MEDIA_DEBUG_LOG("check if native sa token, tokenId:%{public}d, tokenType:%{public}d",
        tokenId, tokenType);
    if (tokenType == ATokenTypeEnum::TOKEN_NATIVE) {
        return true;
    }
    return false;
}

bool PermissionUtils::IsRootShell()
{
    return IPCSkeleton::GetCallingUid() == 0;
}

bool PermissionUtils::IsHdcShell()
{
    return IPCSkeleton::GetCallingUid() == HDC_SHELL_UID;
}

string PermissionUtils::GetPackageNameByBundleName(const string &bundleName)
{
    const static int32_t INVALID_UID = -1;

    string packageName = "";
    int uid = IPCSkeleton::GetCallingUid();
    if (uid <= INVALID_UID) {
        MEDIA_ERR_LOG("Get INVALID_UID UID %{public}d", uid);
        return packageName;
    }
    GetPackageNameFromCache(uid, bundleName, packageName);

    return packageName;
}

string PermissionUtils::GetAppIdByBundleName(const string &bundleName)
{
    int uid = IPCSkeleton::GetCallingUid();
    return GetAppIdByBundleName(bundleName, uid);
}

string PermissionUtils::GetAppIdByBundleName(const string &bundleName, int32_t uid)
{
    if (uid <= INVALID_UID) {
        MEDIA_ERR_LOG("Get INVALID_UID UID %{public}d", uid);
        return "";
    }
    string appId = "";
    GetAppIdFromCache(uid, bundleName, appId);

    return appId;
}

bool PermissionUtils::SetEPolicy()
{
    MEDIA_INFO_LOG("SetEPolicy for directory");
    int ret = Security::AccessToken::El5FilekeyManagerKit::SetFilePathPolicy();
    if (ret != 0) {
        MEDIA_ERR_LOG("SetEPolicy fail of %{public}d", ret);
        return false;
    }
    return true;
}
}  // namespace Media
}  // namespace OHOS
