/*
 * Copyright (C) 2022-2024 Huawei Device Co., Ltd.
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

#include "userfile_client.h"
#include "medialibrary_errno.h"
#include "medialibrary_helper_container.h"
#include "media_log.h"
#include "userfilemgr_uri.h"
#include "medialibrary_operation.h"
#include "media_asset_rdbstore.h"
#include "iservice_registry.h"
#include "media_uri_utils.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::AppExecFwk;

static const int STORAGE_MANAGER_MANAGER_ID = 5003;

namespace OHOS {
namespace Media {
int32_t UserFileClient::userId_ = -1;
bool UserFileClient::IsValid(const int32_t userId)
{
    return sDataShareHelper_ != nullptr;
}

void UserFileClient::Init()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("Get system ability mgr failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    if (remoteObj == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbility Service Failed.");
        return;
    }

    Init(remoteObj);
    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("Create DataShareHelper failed.");
        return;
    }
}

void UserFileClient::Init(const sptr<IRemoteObject> &token, bool isSetHelper, const int32_t userId)
{
    if (sDataShareHelper_ == nullptr) {
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    }

    if (isSetHelper) {
        MediaLibraryHelperContainer::GetInstance()->SetDataShareHelper(sDataShareHelper_);
    }
}

shared_ptr<DataShareResultSet> UserFileClient::Query(Uri &uri, const DataSharePredicates &predicates,
    std::vector<std::string> &columns, int &errCode)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("Query fail, helper null");
        return nullptr;
    }

    shared_ptr<DataShareResultSet> resultSet = nullptr;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates)) {
        resultSet = MediaAssetRdbStore::GetInstance()->Query(predicates, columns, object, errCode);
    } else {
        DatashareBusinessError businessError;
        resultSet = sDataShareHelper_->Query(uri, predicates, columns, &businessError);
        errCode = businessError.GetCode();
    }
    return resultSet;
}

int UserFileClient::Insert(Uri &uri, const DataShareValuesBucket &value)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("insert fail, helper null");
        return E_FAIL;
    }
    int index = sDataShareHelper_->Insert(uri, value);
    return index;
}

int UserFileClient::InsertExt(Uri &uri, const DataShareValuesBucket &value, string &result)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("insert fail, helper null");
        return E_FAIL;
    }
    int index = sDataShareHelper_->InsertExt(uri, value, result);
    return index;
}


int UserFileClient::Delete(Uri &uri, const DataSharePredicates &predicates)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("delete fail, helper null");
        return E_FAIL;
    }
    return sDataShareHelper_->Delete(uri, predicates);
}

int UserFileClient::OpenFile(Uri &uri, const std::string &mode)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("Open file fail, helper null");
        return E_FAIL;
    }
    std::string uriString = uri.ToString();
    MediaUriUtils::AppendKeyValue(uriString, CONST_CALLER, CONST_CAPI);
    Uri newUri = Uri(uriString);
    MEDIA_ERR_LOG("open uri: %{public}s", uriString.c_str());
    return sDataShareHelper_->OpenFile(newUri, mode);
}

int UserFileClient::Update(Uri &uri, const DataSharePredicates &predicates,
    const DataShareValuesBucket &value)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("update fail, helper null");
        return E_FAIL;
    }
    return sDataShareHelper_->Update(uri, predicates, value);
}

int32_t UserFileClient::UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("UserDefineFunc fail, helper null");
        return E_FAIL;
    }
    return sDataShareHelper_->UserDefineFunc(data, reply, option);
}

int32_t UserFileClient::UserDefineFunc(const int32_t &userId, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    if (!IsValid()) {
        MEDIA_ERR_LOG("UserDefineFunc fail, helper null");
        return E_FAIL;
    }
    return sDataShareHelper_->UserDefineFunc(data, reply, option);
}

void UserFileClient::Clear()
{
    sDataShareHelper_ = nullptr;
}

void UserFileClient::SetUserId(const int32_t userId)
{
    userId_ = userId;
}

int32_t UserFileClient::GetUserId()
{
    return userId_;
}
}
}
