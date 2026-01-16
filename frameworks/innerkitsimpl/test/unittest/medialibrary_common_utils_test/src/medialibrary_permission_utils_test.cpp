/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "medialibrary_common_utils_test.h"
#include "thumbnail_utils.h"
#define private public
#include "permission_utils.h"
#undef private

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryCommonUtilsTest, medialib_CheckCallerPermission_test_001, TestSize.Level1)
{
    string permission = "";
    bool ret = PermissionUtils::CheckCallerPermission(permission);
    EXPECT_EQ(ret, false);
    string permissions = PERM_WRITE_IMAGEVIDEO;
    ret = PermissionUtils::CheckCallerPermission(permissions);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetSysBundleManager_test_001, TestSize.Level1)
{
    auto ret = PermissionUtils::GetSysBundleManager();
    EXPECT_NE(ret, nullptr);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetBundleNameFromCache_not_in_cache_001, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    int uid = 1;
    string bundleName = "";
    PermissionUtils::GetBundleNameFromCache(uid, bundleName);
    EXPECT_EQ(bundleName, "");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetBundleNameFromCache_in_cache_002, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    int uid = 1;
    string bundleName = "com.test.demo";
    BundleInfo bundleInfo {bundleName, "", ""};
    PermissionUtils::UpdateLatestBundleInfo(1, bundleInfo);
    PermissionUtils::GetBundleNameFromCache(uid, bundleName);
    EXPECT_EQ(bundleName, "com.test.demo");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetBundleNameFromCache_in_cache_003, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    int uid1 = 1;
    string bundleName = "com.test.demo";
    BundleInfo bundleInfo {bundleName, "", ""};
    PermissionUtils::UpdateLatestBundleInfo(uid1, bundleInfo);
    int uid2 = 2;
    PermissionUtils::UpdateLatestBundleInfo(uid2, bundleInfo);
    PermissionUtils::GetBundleNameFromCache(uid1, bundleName);
    EXPECT_EQ(bundleName, "com.test.demo");
    PermissionUtils::GetBundleNameFromCache(uid2, bundleName);
    EXPECT_EQ(bundleName, "com.test.demo");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetBundleNameFromCache_in_cache_004, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo", "", ""});
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo2", "", ""});

    string bundleNameActual = "";
    PermissionUtils::GetBundleNameFromCache(1, bundleNameActual);
    EXPECT_EQ(bundleNameActual, "com.test.demo2");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetPackageNameFromCache_not_in_cache_001, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    int uid = 1;
    string packageName = "";
    PermissionUtils::GetPackageNameFromCache(uid, "", packageName);
    EXPECT_EQ(packageName, "");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetPackageNameFromCache_in_cache_002, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    int uid = 1;
    string bundleName = "com.test.demo";
    string packageName = "demo";
    BundleInfo bundleInfo {bundleName, packageName, ""};
    PermissionUtils::UpdateLatestBundleInfo(uid, bundleInfo);

    string packageNameActual = "";
    PermissionUtils::GetPackageNameFromCache(uid, "", packageNameActual);
    EXPECT_EQ(packageNameActual, packageName);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetPackageNameFromCache_in_cache_003, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo1", "demo1", ""});
    PermissionUtils::UpdateLatestBundleInfo(2, {"com.test.demo2", "demo2", ""});

    string packageNameActual = "";
    PermissionUtils::GetPackageNameFromCache(2, "com.test.demo2", packageNameActual);
    EXPECT_EQ(packageNameActual, "demo2");
    PermissionUtils::GetPackageNameFromCache(1, "com.test.demo1", packageNameActual);
    EXPECT_EQ(packageNameActual, "demo1");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetPackageNameFromCache_in_cache_004, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo1", "demo1", ""});
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo2", "demo2", ""});

    string packageNameActual = "";
    PermissionUtils::GetPackageNameFromCache(1, "com.test.demo2", packageNameActual);
    EXPECT_EQ(packageNameActual, "demo2");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetAppIdFromCache_not_in_cache_001, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    int uid = 1;
    string appId = "";
    PermissionUtils::GetAppIdFromCache(uid, "", appId);
    EXPECT_EQ(appId, "");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetAppIdFromCache_in_cache_003, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo1", "demo1", "demo1.appid"});
    PermissionUtils::UpdateLatestBundleInfo(2, {"com.test.demo2", "demo2", "demo2.appid"});

    string appIdActual = "";
    PermissionUtils::GetAppIdFromCache(1, "com.test.demo1", appIdActual);
    EXPECT_EQ(appIdActual, "demo1.appid");
    PermissionUtils::GetAppIdFromCache(2, "com.test.demo2", appIdActual);
    EXPECT_EQ(appIdActual, "demo2.appid");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetAppIdFromCache_in_cache_004, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo1", "demo1", "demo1.appid"});
    PermissionUtils::UpdateLatestBundleInfo(1, {"com.test.demo2", "demo2", "demo2.appid"});

    string appIdActual = "";
    PermissionUtils::GetAppIdFromCache(1, "com.test.demo2", appIdActual);
    EXPECT_EQ(appIdActual, "demo2.appid");
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_UpdateLatestBundleInfo_larger_than_capacity_001, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    string name = "";
    for (int i = 0; i < 55; i++) {
        name = "demo" + to_string(i);
        PermissionUtils::UpdateLatestBundleInfo(i, {"com.test." + name, name, name + ".appid"});
    }

    string packageNameActual = "";
    PermissionUtils::GetPackageNameFromCache(5, "com.test.demo5", packageNameActual);
    EXPECT_EQ(packageNameActual, "demo5");
}

HWTEST_F(MediaLibraryCommonUtilsTest, permission_utils_test_001, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    vector<string> perms = {"perm1", "perm2"};
    auto ret = PermissionUtils::CheckCallerPermission(perms);
    EXPECT_EQ(ret, false);
}

HWTEST_F(MediaLibraryCommonUtilsTest, permission_utils_test_002, TestSize.Level1)
{
    PermissionUtils::ClearBundleInfoInCache();
    vector<string> perms = {"PERMISSION_NAME_READ_MEDIA"};
    auto ret = PermissionUtils::CheckCallerPermission(perms);
    EXPECT_FALSE(ret);

    perms = {"PERMISSION_NAME_WRITE_MEDIA"};
    ret = PermissionUtils::CheckCallerPermission(perms);
    EXPECT_FALSE(ret);

    perms = {"PERMISSION_NAME_READ_MEDIA", "PERMISSION_NAME_WRITE_MEDIA"};
    ret = PermissionUtils::CheckCallerPermission(perms);
    EXPECT_FALSE(ret);
}
} // namespace Media
} // namespace OHOS
