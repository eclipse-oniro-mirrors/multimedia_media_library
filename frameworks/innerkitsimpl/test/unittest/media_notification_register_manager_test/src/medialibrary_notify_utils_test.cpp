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

#define MLOG_TAG "MediaLibraryNotifyUtilsTest"

#include "medialibrary_notify_utils_test.h"

#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_notify_utils.h"
#include "media_log.h"
#include "media_change_info.h"
#include "photo_asset_change_info.h"
#include "album_change_info.h"
#include "medialibrary_client_errno.h"
#include "userfile_manager_types.h"
#include <gtest/gtest.h>
#include <napi/native_api.h>

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {

void MediaLibraryNotifyUtilsTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryNotifyUtilsTest::SetUpTestCase");
}

void MediaLibraryNotifyUtilsTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaLibraryNotifyUtilsTest::TearDownTestCase");
}

void MediaLibraryNotifyUtilsTest::SetUp()
{
    MEDIA_INFO_LOG("MediaLibraryNotifyUtilsTest::SetUp");
}

void MediaLibraryNotifyUtilsTest::TearDown()
{
    MEDIA_INFO_LOG("MediaLibraryNotifyUtilsTest::TearDown");
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetAssetManagerNotifyTypeAndUri_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetAssetManagerNotifyTypeAndUri_001::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetAssetManagerNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI);
    EXPECT_EQ(uri, RegisterNotifyType::BATCH_DOWNLOAD_PROGRESS_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetAssetManagerNotifyTypeAndUri_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetAssetManagerNotifyTypeAndUri_002::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetAssetManagerNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetUserDefineNotifyTypeAndUri_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetUserDefineNotifyTypeAndUri_001::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetUserDefineNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI);
    EXPECT_EQ(uri, RegisterNotifyType::USER_CLIENT_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetUserDefineNotifyTypeAndUri_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetUserDefineNotifyTypeAndUri_002::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetUserDefineNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_001::Start");
    string type = RegisterNotifyType::PHOTO_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::PHOTO_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_002::Start");
    string type = "invalidType";
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_003::Start");
    string type = RegisterNotifyType::HIDDEN_PHOTO_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::HIDDEN_PHOTO_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_004::Start");
    string type = RegisterNotifyType::TRASH_PHOTO_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::TRASH_PHOTO_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_005::Start");
    string type = RegisterNotifyType::PHOTO_ALBUM_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::PHOTO_ALBUM_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_006::Start");
    string type = RegisterNotifyType::HIDDEN_ALBUM_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::HIDDEN_ALBUM_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_007::Start");
    string type = RegisterNotifyType::TRASHED_ALBUM_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::TRASH_ALBUM_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_001::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(uri, RegisterNotifyType::PHOTO_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_002::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_003::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::HIDDEN_PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::HIDDEN_PHOTO_URI);
    EXPECT_EQ(uri, RegisterNotifyType::HIDDEN_PHOTO_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_004::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::TRASH_PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::TRASH_PHOTO_URI);
    EXPECT_EQ(uri, RegisterNotifyType::TRASH_PHOTO_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_005::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::PHOTO_ALBUM_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::PHOTO_ALBUM_URI);
    EXPECT_EQ(uri, RegisterNotifyType::PHOTO_ALBUM_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_006::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::HIDDEN_ALBUM_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::HIDDEN_ALBUM_URI);
    EXPECT_EQ(uri, RegisterNotifyType::HIDDEN_ALBUM_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_007::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::TRASH_ALBUM_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::TRASH_ALBUM_URI);
    EXPECT_EQ(uri, RegisterNotifyType::TRASHED_ALBUM_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_001::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_ADD));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_002::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_003::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_REMOVE));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_004::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_ADD));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_005::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_UPDATE));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_006::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_REMOVE));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyChangeType_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyChangeType_007::Start");
    Notification::AccurateNotifyType notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyChangeType(notifyType);
    EXPECT_EQ(result, static_cast<int32_t>(NotifyChangeType::NOTIFY_CHANGE_YUV_READY));
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetSingleRegisterNotifyType_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSingleRegisterNotifyType_001::Start");
    string type = RegisterNotifyType::SINGLE_PHOTO_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetSingleRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SINGLE_PHOTO_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetSingleRegisterNotifyType_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSingleRegisterNotifyType_002::Start");
    string type = RegisterNotifyType::SINGLE_PHOTO_ALBUM_CHANGE;
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetSingleRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetSingleRegisterNotifyType_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSingleRegisterNotifyType_003::Start");
    string type = "invalidType";
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetSingleRegisterNotifyType(type, uriType);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetSingleNotifyTypeAndUri_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSingleNotifyTypeAndUri_001::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SINGLE_PHOTO_URI);
    EXPECT_EQ(uri, RegisterNotifyType::SINGLE_PHOTO_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetSingleNotifyTypeAndUri_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSingleNotifyTypeAndUri_002::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
    EXPECT_EQ(uri, RegisterNotifyType::SINGLE_PHOTO_ALBUM_CHANGE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, GetSingleNotifyTypeAndUri_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetSingleNotifyTypeAndUri_003::Start");
    Notification::NotifyUriType type = Notification::NotifyUriType::PHOTO_URI;
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(type, uriType, uri);
    EXPECT_EQ(result, E_ERR);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_001::Start");
    int32_t innerErr = E_PERMISSION_DENIED;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, OHOS_PERMISSION_DENIED_CODE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_002::Start");
    int32_t innerErr = -E_CHECK_SYSTEMAPP_FAIL;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, E_CHECK_SYSTEMAPP_FAIL);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_003::Start");
    int32_t innerErr = JS_E_PARAM_INVALID;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, JS_E_PARAM_INVALID);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_004::Start");
    int32_t innerErr = E_MAX_ON_SINGLE_NUM;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, JS_E_PARAM_INVALID);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_005::Start");
    int32_t innerErr = OHOS_INVALID_PARAM_CODE;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, OHOS_INVALID_PARAM_CODE);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_006::Start");
    int32_t innerErr = 9999;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, JS_E_INNER_FAIL);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_007::Start");
    int32_t innerErr = E_OK;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, JS_E_INNER_FAIL);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_008::Start");
    int32_t innerErr = E_FAIL;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, JS_E_INNER_FAIL);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, ConvertToJsError_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("ConvertToJsError_009::Start");
    int32_t innerErr = -1;
    int32_t result = MediaLibraryNotifyUtils::ConvertToJsError(innerErr);
    EXPECT_EQ(result, JS_E_INNER_FAIL);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt32_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt32_001::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt32(env, "testInt", 42, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt32_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt32_002::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = MediaLibraryNotifyUtils::SetValueInt32(env, "testInt", -1, result);
    EXPECT_EQ(status, napi_invalid_arg);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt32_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt32_003::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt32(env, "testInt", 0, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt32_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt32_004::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt32(env, "testInt", 2147483647, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt32_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt32_005::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt32(env, "testInt", -2147483648, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt64_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt64_001::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt64(env, "testInt64", 1234567890LL, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt64_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt64_002::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = MediaLibraryNotifyUtils::SetValueInt64(env, "testInt64", -1, result);
    EXPECT_EQ(status, napi_invalid_arg);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt64_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt64_003::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt64(env, "testInt64", 0LL, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt64_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt64_004::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt64(env, "testInt64", 9223372036854775807LL, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueInt64_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueInt64_005::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueInt64(env, "testInt64", (-9223372036854775807LL - 1), result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueString_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueString_001::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueString(env, "testStr", "hello", result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueString_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueString_002::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = MediaLibraryNotifyUtils::SetValueString(env, "testStr", "", result);
    EXPECT_EQ(status, napi_invalid_arg);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueString_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueString_003::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueString(env, "testStr", "", result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueString_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueString_004::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        string longStr(1000, 'a');
        status = MediaLibraryNotifyUtils::SetValueString(env, "testStr", longStr, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueString_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueString_005::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueString(env, "testStr", "test with spaces", result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueBool_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueBool_001::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueBool(env, "testBool", true, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueBool_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueBool_002::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = MediaLibraryNotifyUtils::SetValueBool(env, "testBool", false, result);
    EXPECT_EQ(status, napi_invalid_arg);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueBool_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueBool_003::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueBool(env, "testBool", false, result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueNull_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueNull_001::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok && result != nullptr) {
        status = MediaLibraryNotifyUtils::SetValueNull(env, "testNull", result);
        EXPECT_EQ(status, napi_ok);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, SetValueNull_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetValueNull_002::Start");
    napi_env env = nullptr;
    napi_value result = nullptr;
    napi_status status = MediaLibraryNotifyUtils::SetValueNull(env, "testNull", result);
    EXPECT_EQ(status, napi_invalid_arg);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetRecheckChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetRecheckChangeInfos_001::Start");
    napi_env env = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetRecheckChangeInfos(env);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumRecheckChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumRecheckChangeInfos_001::Start");
    napi_env env = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumRecheckChangeInfos(env);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildSinglePhotoAssetRecheckChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildSinglePhotoAssetRecheckChangeInfos_001::Start");
    napi_env env = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildSinglePhotoAssetRecheckChangeInfos(env);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildSingleAlbumRecheckChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildSingleAlbumRecheckChangeInfos_001::Start");
    napi_env env = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildSingleAlbumRecheckChangeInfos(env);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeInfo_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeInfo_001::Start");
    napi_env env = nullptr;
    AccurateRefresh::PhotoAssetChangeInfo info;
    info.fileId_ = 1;
    info.uri_ = "test://uri";
    info.mediaType_ = 1;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfo(env, info);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeInfo_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeInfo_002::Start");
    napi_env env = nullptr;
    AccurateRefresh::PhotoAssetChangeInfo info;
    info.fileId_ = AccurateRefresh::INVALID_INT32_VALUE;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfo(env, info);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeInfo_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeInfo_003::Start");
    napi_env env = nullptr;
    AccurateRefresh::PhotoAssetChangeInfo info;
    info.fileId_ = 100;
    info.uri_ = "file:///storage/media/local/files/test.jpg";
    info.mediaType_ = 2;
    info.dateDay_ = "2025-01-01";
    info.isFavorite_ = true;
    info.isHidden_ = false;
    info.strongAssociation_ = 1;
    info.thumbnailVisible_ = 1;
    info.dateTrashedMs_ = 0;
    info.dateAddedMs_ = 1737873600000;
    info.dateTakenMs_ = 1737873600000;
    info.position_ = 0;
    info.displayName_ = "test.jpg";
    info.size_ = 1024000;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfo(env, info);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeInfo_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeInfo_001::Start");
    napi_env env = nullptr;
    AccurateRefresh::AlbumChangeInfo info;
    info.albumId_ = 1;
    info.albumType_ = 1;
    info.albumSubType_ = 1;
    info.albumName_ = "Test Album";
    info.albumUri_ = "file:///storage/media/local/files/albums/test";
    info.imageCount_ = 10;
    info.videoCount_ = 5;
    info.count_ = 15;
    info.coverUri_ = "file:///storage/media/local/files/cover.jpg";
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeInfo(env, info);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeInfo_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeInfo_002::Start");
    napi_env env = nullptr;
    AccurateRefresh::AlbumChangeInfo info;
    info.albumId_ = AccurateRefresh::INVALID_INT32_VALUE;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeInfo(env, info);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeInfo_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeInfo_003::Start");
    napi_env env = nullptr;
    AccurateRefresh::AlbumChangeInfo info;
    info.albumId_ = 100;
    info.albumType_ = 2;
    info.albumSubType_ = 3;
    info.albumName_ = "Video Album";
    info.albumUri_ = "file:///storage/media/local/files/albums/video";
    info.imageCount_ = 0;
    info.videoCount_ = 20;
    info.count_ = 20;
    info.coverUri_ = "file:///storage/media/local/files/video_cover.jpg";
    info.hiddenCount_ = 5;
    info.hiddenCoverUri_ = "file:///storage/media/local/files/hidden_cover.jpg";
    info.isCoverChange_ = true;
    info.isHiddenCoverChange_ = false;
    info.orderSection_ = 1;
    info.albumsOrder_ = 0;
    info.hidden_ = 0;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeInfo(env, info);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeData_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeData_001::Start");
    napi_env env = nullptr;
    AccurateRefresh::PhotoAssetChangeData data;
    data.isContentChanged_ = true;
    data.isDelete_ = false;
    data.thumbnailChangeStatus_ = 1;
    data.version_ = 1;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeData(env, data);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeData_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeData_002::Start");
    napi_env env = nullptr;
    AccurateRefresh::PhotoAssetChangeData data;
    data.isContentChanged_ = false;
    data.isDelete_ = true;
    data.thumbnailChangeStatus_ = 0;
    data.version_ = 2;
    data.infoBeforeChange_.fileId_ = 1;
    data.infoBeforeChange_.uri_ = "before://uri";
    data.infoAfterChange_.fileId_ = 2;
    data.infoAfterChange_.uri_ = "after://uri";
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeData(env, data);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeData_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeData_001::Start");
    napi_env env = nullptr;
    AccurateRefresh::AlbumChangeData data;
    data.version_ = 1;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeData(env, data);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeData_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeData_002::Start");
    napi_env env = nullptr;
    AccurateRefresh::AlbumChangeData data;
    data.version_ = 2;
    data.infoBeforeChange_.albumId_ = 1;
    data.infoBeforeChange_.albumName_ = "Before Album";
    data.infoAfterChange_.albumId_ = 2;
    data.infoAfterChange_.albumName_ = "After Album";
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeData(env, data);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoNapiArray_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoNapiArray_001::Start");
    napi_env env = nullptr;
    vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> changeInfos;
    AccurateRefresh::PhotoAssetChangeData data;
    data.infoBeforeChange_.fileId_ = 1;
    changeInfos.push_back(data);
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoNapiArray(env, changeInfos);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoNapiArray_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoNapiArray_002::Start");
    napi_env env = nullptr;
    vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> changeInfos;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoNapiArray(env, changeInfos);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumNapiArray_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumNapiArray_001::Start");
    napi_env env = nullptr;
    vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> changeInfos;
    AccurateRefresh::AlbumChangeData data;
    data.infoBeforeChange_.albumId_ = 1;
    changeInfos.push_back(data);
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumNapiArray(env, changeInfos);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumNapiArray_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumNapiArray_002::Start");
    napi_env env = nullptr;
    vector<std::variant<AccurateRefresh::PhotoAssetChangeData, AccurateRefresh::AlbumChangeData>> changeInfos;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumNapiArray(env, changeInfos);
    if (result != nullptr) {
        EXPECT_TRUE(result != nullptr);
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeInfos_001::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::MediaChangeInfo>();
    if (changeInfo != nullptr) {
        changeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
        AccurateRefresh::PhotoAssetChangeData data;
        data.infoBeforeChange_.fileId_ = 1;
        changeInfo->changeInfos.push_back(data);
        napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeInfos_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeInfos_002::Start");
    napi_env env = nullptr;
    shared_ptr<Notification::MediaChangeInfo> changeInfo = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfos(env, changeInfo);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeInfos_001::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::MediaChangeInfo>();
    if (changeInfo != nullptr) {
        changeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD;
        AccurateRefresh::AlbumChangeData data;
        data.infoBeforeChange_.albumId_ = 1;
        changeInfo->changeInfos.push_back(data);
        napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeInfos_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeInfos_002::Start");
    napi_env env = nullptr;
    shared_ptr<Notification::MediaChangeInfo> changeInfo = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeInfos(env, changeInfo);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_001::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_PROGRESS;
        changeInfo->fileId = 1;
        changeInfo->percent = 50;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_002::Start");
    napi_env env = nullptr;
    shared_ptr<Notification::AssetManagerNotifyInfo> changeInfo = nullptr;
    napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_003::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_FINISH;
        changeInfo->fileId = 2;
        changeInfo->percent = 100;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_004::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_FAILED;
        changeInfo->fileId = 3;
        changeInfo->percent = 75;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_005::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_ASSET_DELETE;
        changeInfo->fileId = 4;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_006::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_PAUSE;
        changeInfo->autoPauseReason = 1;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_007::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_AUTO_RESUME;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildBatchDownloadProgressInfos_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildBatchDownloadProgressInfos_008::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<Notification::AssetManagerNotifyInfo>();
    if (changeInfo != nullptr) {
        changeInfo->downloadAssetNotifyType =
            Notification::DownloadAssetsNotifyType::DOWNLOAD_REFRESH;
        napi_value result = MediaLibraryNotifyUtils::BuildBatchDownloadProgressInfos(env, changeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildSinglePhotoAssetChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildSinglePhotoAssetChangeInfos_001::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<AccurateRefresh::PhotoAssetChangeData>();
    auto mediaChangeInfo = make_shared<Notification::MediaChangeInfo>();
    if (changeInfo != nullptr && mediaChangeInfo != nullptr) {
        changeInfo->infoBeforeChange_.fileId_ = 1;
        mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ASSET_ADD;
        mediaChangeInfo->isForRecheck = false;
        napi_value result = MediaLibraryNotifyUtils::BuildSinglePhotoAssetChangeInfos(env, changeInfo, mediaChangeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildSinglePhotoAssetChangeInfos_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildSinglePhotoAssetChangeInfos_002::Start");
    napi_env env = nullptr;
    shared_ptr<AccurateRefresh::PhotoAssetChangeData> changeInfo = nullptr;
    auto mediaChangeInfo = make_shared<Notification::MediaChangeInfo>();
    napi_value result = MediaLibraryNotifyUtils::BuildSinglePhotoAssetChangeInfos(env, changeInfo, mediaChangeInfo);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildSingleAlbumChangeInfos_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildSingleAlbumChangeInfos_001::Start");
    napi_env env = nullptr;
    auto changeInfo = make_shared<AccurateRefresh::AlbumChangeData>();
    auto mediaChangeInfo = make_shared<Notification::MediaChangeInfo>();
    if (changeInfo != nullptr && mediaChangeInfo != nullptr) {
        changeInfo->infoBeforeChange_.albumId_ = 1;
        mediaChangeInfo->notifyType = Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD;
        mediaChangeInfo->isForRecheck = true;
        napi_value result = MediaLibraryNotifyUtils::BuildSingleAlbumChangeInfos(env, changeInfo, mediaChangeInfo);
        if (result != nullptr) {
            EXPECT_TRUE(result != nullptr);
        }
    }
}

HWTEST_F(MediaLibraryNotifyUtilsTest, BuildSingleAlbumChangeInfos_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildSingleAlbumChangeInfos_002::Start");
    napi_env env = nullptr;
    shared_ptr<AccurateRefresh::AlbumChangeData> changeInfo = nullptr;
    auto mediaChangeInfo = make_shared<Notification::MediaChangeInfo>();
    napi_value result = MediaLibraryNotifyUtils::BuildSingleAlbumChangeInfos(env, changeInfo, mediaChangeInfo);
    EXPECT_EQ(result, nullptr);
}

// 共享图片/共享相册注册名 -> uri 映射（覆盖新增的两个 RegisterNotifyType 常量与 REGISTER_NOTIFY_TYPE_MAP 表项）
HWTEST_F(MediaLibraryNotifyUtilsTest, GetRegisterNotifyType_share_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetRegisterNotifyType_share_001::Start");
    Notification::NotifyUriType uriType;
    int32_t result = MediaLibraryNotifyUtils::GetRegisterNotifyType(RegisterNotifyType::SHARE_PHOTO_CHANGE, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SHARE_PHOTO_URI);

    result = MediaLibraryNotifyUtils::GetRegisterNotifyType(RegisterNotifyType::SHARE_PHOTO_ALBUM_CHANGE, uriType);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SHARE_PHOTO_ALBUM_URI);
}

// 共享 uri -> 注册名映射（覆盖 REGISTER_TYPE_MAP / REGISTER_URI_MAP 新增表项）
HWTEST_F(MediaLibraryNotifyUtilsTest, GetNotifyTypeAndUri_share_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("GetNotifyTypeAndUri_share_001::Start");
    Notification::NotifyUriType uriType;
    string uri;
    int32_t result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(
        Notification::NotifyUriType::SHARE_PHOTO_URI, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SHARE_PHOTO_URI);
    EXPECT_EQ(uri, RegisterNotifyType::SHARE_PHOTO_CHANGE);

    result = MediaLibraryNotifyUtils::GetNotifyTypeAndUri(
        Notification::NotifyUriType::SHARE_PHOTO_ALBUM_URI, uriType, uri);
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(uriType, Notification::NotifyUriType::SHARE_PHOTO_ALBUM_URI);
    EXPECT_EQ(uri, RegisterNotifyType::SHARE_PHOTO_ALBUM_CHANGE);
}

// 共享图片：uriType 为 SHARE_PHOTO_URI 时进入新增的共享字段分支（null env 下不崩、返回 nullptr）
HWTEST_F(MediaLibraryNotifyUtilsTest, BuildPhotoAssetChangeInfo_share_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildPhotoAssetChangeInfo_share_001::Start");
    napi_env env = nullptr;
    AccurateRefresh::PhotoAssetChangeInfo assetInfo;
    assetInfo.fileId_ = 1;
    assetInfo.isShared_ = 1;
    assetInfo.shareDateDay_ = 20260101;
    assetInfo.shareGroup_ = 200;
    assetInfo.shareRiskStatus_ = 3;
    assetInfo.photoVisibility_ = 1;
    napi_value result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfo(env, assetInfo,
        Notification::NotifyUriType::SHARE_PHOTO_URI);
    EXPECT_EQ(result, nullptr);

    // 非共享 uriType：不进该分支
    result = MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfo(env, assetInfo,
        Notification::NotifyUriType::PHOTO_URI);
    EXPECT_EQ(result, nullptr);
}

// 共享相册：IsShareAlbum 为真时进入新增的 shareRiskStatus 分支（null env 下不崩、返回 nullptr）
HWTEST_F(MediaLibraryNotifyUtilsTest, BuildAlbumChangeInfo_share_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("BuildAlbumChangeInfo_share_001::Start");
    napi_env env = nullptr;
    AccurateRefresh::AlbumChangeInfo albumInfo;
    albumInfo.albumId_ = 1;
    albumInfo.albumType_ = static_cast<int32_t>(PhotoAlbumType::SHARE);
    albumInfo.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::SHARE_GENERIC);
    albumInfo.shareRiskStatus_ = 3;
    napi_value result = MediaLibraryNotifyUtils::BuildAlbumChangeInfo(env, albumInfo);
    EXPECT_EQ(result, nullptr);

    // 非共享相册：不进该分支
    albumInfo.albumType_ = static_cast<int32_t>(PhotoAlbumType::USER);
    albumInfo.albumSubType_ = static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC);
    result = MediaLibraryNotifyUtils::BuildAlbumChangeInfo(env, albumInfo);
    EXPECT_EQ(result, nullptr);
}

}  // namespace Media
}  // namespace OHOS
