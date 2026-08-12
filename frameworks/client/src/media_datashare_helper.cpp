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

#include "media_datashare_helper.h"

#include "base_data_uri.h"
#include "media_uri_utils.h"
#include "medialibrary_napi_log.h"
#include "media_client_utils.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "medialibrary_errno.h"
#include "os_account_manager.h"
#include <unistd.h>

namespace OHOS::Media::IPC {
MediaDataShareHelper::MediaDataShareHelper() {}
MediaDataShareHelper::~MediaDataShareHelper() {}

int32_t MediaDataShareHelper::ResolveUserId(int32_t userId)
{
    if (userId != -1) {
        return userId;
    }
    return MediaClientUtils::GetCurrentAccountId();
}

// LCOV_EXCL_START
std::shared_ptr<DataShare::DataShareHelper> MediaDataShareHelper::GetDataShareHelper(const sptr<IRemoteObject> &token,
    const int32_t userId)
{
    Uri uri = Uri(MEDIALIBRARY_DATA_URI);
    std::string multiUri = MediaUriUtils::GetMultiUri(uri, userId).ToString();
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token, multiUri);
    if (dataShareHelper == nullptr) {
        NAPI_ERR_LOG("dataShareHelper Creator failed");
        dataShareHelper = DataShare::DataShareHelper::Creator(token, multiUri);
    }
    return dataShareHelper;
}

bool MediaDataShareHelper::IsValid(const int32_t userId)
{
    int32_t uid = ResolveUserId(userId);
    std::shared_ptr<DataShare::DataShareHelper> helper;
    if (dataShareHelperMap_.Find(uid, helper)) {
        return helper != nullptr;
    }
    return false;
}

void MediaDataShareHelper::SetUserId(const int32_t userId)
{
    userId_ = userId;
}

int32_t MediaDataShareHelper::GetUserId()
{
    return userId_;
}

std::shared_ptr<DataShare::DataShareHelper> MediaDataShareHelper::GetDataShareHelperByUser(const int32_t userId)
{
    return dataShareHelperMap_.ReadVal(userId);
}

void MediaDataShareHelper::Init(const sptr<IRemoteObject> &token, const int32_t userId)
{
    int32_t uid = ResolveUserId(userId);
    if (GetDataShareHelperByUser(uid) == nullptr) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = GetDataShareHelper(token, userId);
        if (dataShareHelper != nullptr) {
            if (!IsValid(uid)) {
                dataShareHelperMap_.EnsureInsert(uid, dataShareHelper);
            } else {
                NAPI_ERR_LOG("dataShareHelperMap has userId and value");
            }
        } else {
            NAPI_ERR_LOG("Failed to getDataShareHelper, dataShareHelper is null");
        }
    }
}

void MediaDataShareHelper::InitFromSa(const int32_t userId)
{
    sptr<IRemoteObject> token = MediaClientUtils::GetSaToken();
    if (token == nullptr) {
        NAPI_ERR_LOG("InitFromSa: failed to get SA token");
        return;
    }
    Init(token, userId);
}

void MediaDataShareHelper::InitForActiveUser()
{
    int32_t activeUserId = MediaClientUtils::GetCurrentAccountId();
    NAPI_INFO_LOG("InitForActiveUser: activeUserId is %{public}d", activeUserId);
    if (activeUserId == GetUserId() && IsValid(activeUserId)) {
        NAPI_INFO_LOG("InitForActiveUser: already initialized for userId %{public}d", activeUserId);
        return;
    }
    SetUserId(activeUserId);
    sptr<IRemoteObject> token = MediaClientUtils::GetSaToken();
    if (token == nullptr) {
        NAPI_ERR_LOG("InitForActiveUser: failed to get SA token, userId: %{public}d", activeUserId);
        return;
    }
    Init(token, activeUserId);
}

bool MediaDataShareHelper::ForceReconnect(const int32_t userId)
{
    int32_t uid = ResolveUserId(userId);
    NAPI_INFO_LOG("ForceReconnect: userId %{public}d", uid);
    // Remove old helper if exists using a local variable (not instance member)
    std::shared_ptr<DataShare::DataShareHelper> oldHelper;
    if (dataShareHelperMap_.Find(uid, oldHelper)) {
        dataShareHelperMap_.Erase(uid);
    }
    // Re-initialize from SA token
    sptr<IRemoteObject> token = MediaClientUtils::GetSaToken();
    if (token == nullptr) {
        NAPI_ERR_LOG("ForceReconnect: failed to get SA token");
        return false;
    }
    Init(token, uid);
    return IsValid(uid);
}
// LCOV_EXCL_STOP
}