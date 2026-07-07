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
#ifndef MEDIALIBRARY_PERMISSION_UTILS_H
#define MEDIALIBRARY_PERMISSION_UTILS_H

#include <array>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <safe_map.h>

#include "bundle_mgr_interface.h"
#include "userfile_manager_types.h"
#include "privacy_kit.h"
#include "tokenid_kit.h"

namespace OHOS {
namespace Media {
#define COMPILE_HIDDEN __attribute__ ((visibility ("hidden")))
const std::string PERMISSION_NAME_READ_MEDIA = "ohos.permission.READ_MEDIA";
const std::string PERMISSION_NAME_WRITE_MEDIA = "ohos.permission.WRITE_MEDIA";
const std::string PERMISSION_NAME_MEDIA_LOCATION = "ohos.permission.MEDIA_LOCATION";
const std::string PERM_READ_IMAGEVIDEO = "ohos.permission.READ_IMAGEVIDEO";
const std::string CLOUD_READ_ALL_PHOTO_PERMISSION = "ohos.permission.READ_ALL_PHOTO";
const std::string PERM_READ_CLOUD_IMAGEVIDEO = "ohos.permission.READ_CLOUD_IMAGEVIDEO";
const std::string PERM_READ_AUDIO = "ohos.permission.READ_AUDIO";
const std::string PERM_READ_DOCUMENT = "ohos.permission.READ_DOCUMENT";
const std::string PERM_WRITE_IMAGEVIDEO = "ohos.permission.WRITE_IMAGEVIDEO";
const std::string CLOUD_WRITE_ALL_PHOTO_PERMISSION = "ohos.permission.WRITE_ALL_PHOTO";
const std::string PERM_WRITE_AUDIO = "ohos.permission.WRITE_AUDIO";
const std::string PERM_WRITE_DOCUMENT = "ohos.permission.WRITE_DOCUMENT";
const std::string PERM_MANAGE_PRIVATE_PHOTOS = "ohos.permission.MANAGE_PRIVATE_PHOTOS";
const std::string PERM_SHORT_TERM_WRITE_IMAGEVIDEO = "ohos.permission.SHORT_TERM_WRITE_IMAGEVIDEO";
const std::string PERM_INTERACT_ACROSS_LOCAL_ACCOUNTS = "ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS";
const std::string CONST_LOGSYSTEM_VERSIONTYPE = "const.logsystem.versiontype";
const std::string PERM_ACCESS_MEDIALIB_THUMB_DB = "ohos.permission.ACCESS_MEDIALIB_THUMB_DB";
const std::string MANAGE_RISK_PHOTOS = "ohos.permission.MANAGE_RISK_PHOTOS";
const std::string CONTROL_IMAGEVIDEO_ANALYSIS = "ohos.permission.CONTROL_IMAGEVIDEO_ANALYSIS";
const std::string E_POLICY = "E";
constexpr int SHORT_TERM_PERMISSION_DURATION_300S = 300;

enum SaveType {
    SHORT_IMAGE_PERM = 0,
};

const std::vector<std::string> WRITE_PERMS_V10 = {
    PERM_WRITE_IMAGEVIDEO,
    PERM_WRITE_AUDIO,
    PERM_WRITE_DOCUMENT
};

struct BundleInfo {
    std::string bundleName;
    std::string packageName;
    std::string appId;
    uint32_t tokenId {0};
    std::string ownerAlbumId;
};

struct OpenDataInfo {
    std::string uri;
    int32_t userId = -1;
    int32_t uid = -1;
    std::string type;
    int64_t timestamp = 0;
};

struct OpenPermissionInfo {
    Security::AccessToken::AccessTokenID token;
    std::string perm;
    bool permGranted = false;
    Security::AccessToken::PermissionUsedType type;
    OpenDataInfo openDataInfo;
};

class PermissionUtils {
public:
    static constexpr int32_t BASE_USER_RANGE = 200000;

    static bool CheckCallerPermission(const std::string &permission);
    static bool CheckCallerPermission(const std::string &permission, const int &uid);
    static bool CheckCallerPermission(const std::vector<std::string> &perms);
    static bool CheckHasPermission(const std::vector<std::string> &perms);
    static void GetClientBundle(const int uid, std::string &bundleName);
    static void GetPackageName(const int uid, std::string &packageName);
    static uint32_t GetTokenId();
    static bool IsBetaVersion();
    static bool IsSystemApp();
    static bool IsSystemAppByCache(const uint64_t tokenId);
    static bool IsNativeSAApp();
    static bool IsRootShell();
    static bool IsHdcShell();
    static bool CheckIsSystemAppByUid();
    static std::string GetPackageNameByBundleName(const std::string &bundleName);
    static std::string GetAppIdByBundleName(const std::string &bundleName);
    static std::string GetAppIdByBundleName(const std::string &bundleName, int32_t uid);
    static bool CheckPhotoCallerPermission(const std::vector<std::string> &perms, OpenDataInfo info = {});
    static bool CheckPhotoCallerPermission(const std::string &permission, OpenDataInfo info = {});
    static bool CheckPhotoCallerPermission(const std::string &permission,
        const Security::AccessToken::AccessTokenID &tokenCaller, OpenDataInfo info = {});
    static bool CheckPhotoCallerPermission(const std::vector<std::string> &perms, const int &uid,
        Security::AccessToken::AccessTokenID &tokenCaller, OpenDataInfo info = {});
    static bool CheckPhotoCallerPermissionNoRecord(const std::vector<std::string> &perms, const int &uid,
        Security::AccessToken::AccessTokenID &tokenCaller);
    static bool CheckPhotoCallerPermissionNoRecord(const std::string &permission,
        const Security::AccessToken::AccessTokenID &tokenCaller);
    static void CollectPermissionInfo(const std::string &permission, const bool permGranted,
        const Security::AccessToken::PermissionUsedType type);
    static void CollectPermissionInfo(const std::string &permission, const bool permGranted,
        const Security::AccessToken::PermissionUsedType type, const int &uid);
    static void CollectPermissionInfo(const std::string &permission, const bool permGranted,
        const Security::AccessToken::PermissionUsedType type, const OpenDataInfo &openDataInfo);
    static void CollectPermissionInfo(const std::string &permission, const bool permGranted,
        const Security::AccessToken::PermissionUsedType type, const int &uid, const OpenDataInfo &openDataInfo);
    static void ClearBundleInfoInCache();
    static bool SetEPolicy();
    static int64_t GetMainTokenId(const std::string &appId, int64_t &tokenId);
    static bool GetTokenCallerForUid(const int &uid, Security::AccessToken::AccessTokenID &tokenCaller);
    static bool CheckCloudPermission();
    static bool IsSystemAppByBundleName(const std::string &bundleName);

private:
    static std::vector<Security::AccessToken::AddPermParamInfo> infos_;
    static std::vector<OpenPermissionInfo> pendingOpenPermissionInfos_;

    static sptr<AppExecFwk::IBundleMgr> GetSysBundleManager();
    COMPILE_HIDDEN static sptr<AppExecFwk::IBundleMgr> bundleMgr_;
    COMPILE_HIDDEN static std::mutex bundleMgrMutex_;
    static void GetBundleNameFromCache(int uid, std::string &bundleName);
    static void GetPackageNameFromCache(int uid, const std::string &bundleName, std::string &packageName);
    static void GetAppIdFromCache(int uid, const std::string &bundleName, std::string &appId);
    static void UpdateLatestBundleInfo(int uid, const BundleInfo &bundleInfo);
    static void UpdateBundleNameInCache(int uid, const std::string &bundleName);
    static void UpdatePackageNameInCache(int uid, const std::string &packageName);
    static void UpdateAppIdInCache(int uid, const std::string &appId);
    static std::mutex uninstallMutex_;
    static std::list<std::pair<int32_t, BundleInfo>> bundleInfoList_; // 用来快速获取使用频率最低的uid
    static std::unordered_map<int32_t, std::list<std::pair<int32_t, BundleInfo>>::iterator> bundleInfoMap_;
    static SafeMap<uint64_t, bool> systemAppCache_;
    static void DelayTaskInit();
    static std::vector<Security::AccessToken::AddPermParamInfo> GetPermissionRecord();
    static void CollectPermissionRecord(const Security::AccessToken::AccessTokenID &token, const std::string &perm,
        const bool permGranted, const Security::AccessToken::PermissionUsedType type);
    static void CollectPermissionRecord(const Security::AccessToken::AccessTokenID &token, const std::string &perm,
        const bool permGranted, const Security::AccessToken::PermissionUsedType type, const OpenDataInfo &openDataInfo);
    static void DelayAddPermissionRecord();
    static void AddToPendingOpenPermissionInfo(const Security::AccessToken::AccessTokenID &token,
        const std::string &perm, const bool permGranted, const Security::AccessToken::PermissionUsedType type,
        const OpenDataInfo &openDataInfo);
    static void HandlePendingOpenDataInfos();
    static bool HandleEmptyOpenDataInfo(const Security::AccessToken::AccessTokenID &token, const std::string &perm,
        const bool permGranted, const Security::AccessToken::PermissionUsedType type, const OpenDataInfo &openDataInfo);
    static void AddPermissionRecord();
    static void AddPermissionRecord(const Security::AccessToken::AccessTokenID &token, const std::string &perm,
        const bool permGranted);
};
}  // namespace Media
}  // namespace OHOS
#endif // MEDIALIBRARY_PERMISSION_UTILS_H
