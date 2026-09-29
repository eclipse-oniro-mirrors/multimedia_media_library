/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MediaLibraryHelperContainer"

#include "medialibrary_helper_container.h"

#include "base_data_uri.h"
#include "media_uri_utils.h"
#include "media_account_utils.h"
#include "media_log.h"

namespace OHOS {
namespace Media {


std::shared_ptr<MediaLibraryHelperContainer> MediaLibraryHelperContainer::instance_ = nullptr;
std::mutex MediaLibraryHelperContainer::mutex_;
std::shared_ptr<DataShare::DataShareHelper> MediaLibraryHelperContainer::dataShareHelper_ = nullptr;

std::shared_ptr<MediaLibraryHelperContainer> MediaLibraryHelperContainer::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<MediaLibraryHelperContainer>();
        }
    }
    return instance_;
}

void MediaLibraryHelperContainer::CreateDataShareHelper(const sptr<IRemoteObject> &token,
    const std::string &uri)
{
    if (dataShareHelper_ == nullptr) {
        dataShareHelper_ = DataShare::DataShareHelper::Creator(token, uri);
    }
}

void MediaLibraryHelperContainer::SetDataShareHelper(const std::shared_ptr<DataShare::DataShareHelper> &helper)
{
    dataShareHelper_ = helper;
}

std::shared_ptr<DataShare::DataShareHelper> MediaLibraryHelperContainer::CreateHelperFromSa()
{
    auto remoteObj = MediaAccountUtils::GetSaToken();
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("CreateHelperFromSa: GetSaToken failed");
        return nullptr;
    }
    int32_t activeUserId = MediaAccountUtils::GetCurrentAccountId();
    Uri uri = Uri(MEDIALIBRARY_DATA_URI);
    std::string multiUri = MediaUriUtils::GetMultiUri(uri, activeUserId).ToString();
    auto helper = DataShare::DataShareHelper::Creator(remoteObj, multiUri);
    if (helper == nullptr) {
        MEDIA_ERR_LOG("CreateHelperFromSa: DataShareHelper Creator failed, retrying");
        helper = DataShare::DataShareHelper::Creator(remoteObj, multiUri);
    }
    return helper;
}

std::shared_ptr<DataShare::DataShareHelper> MediaLibraryHelperContainer::GetDataShareHelper()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (dataShareHelper_ != nullptr) {
        return dataShareHelper_;
    }
    // Not initialized by client — create with SA token + current user
    dataShareHelper_ = CreateHelperFromSa();
    if (dataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("GetDataShareHelper: failed to create helper from SA token");
    }
    return dataShareHelper_;
}

void MediaLibraryHelperContainer::SetDataShareHelperForUser(int32_t userId,
    const std::shared_ptr<DataShare::DataShareHelper> &helper)
{
    userDataShareHelperMap_.EnsureInsert(userId, helper);
}

std::shared_ptr<DataShare::DataShareHelper> MediaLibraryHelperContainer::GetDataShareHelperForUser(
    int32_t userId)
{
    return userDataShareHelperMap_.ReadVal(userId);
}
} // namespace Media
} // namespace OHOS
