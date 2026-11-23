/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "dummy_data_share_helper.h"

#include <memory>

namespace OHOS {
namespace Media {
std::shared_ptr<DummyDataShareHelper> DummyDataShareHelper::GetInstance()
{
    // Function-local static: created on first use, never destroyed before process exit.
    static std::shared_ptr<DummyDataShareHelper> instance(new DummyDataShareHelper());
    return instance;
}

bool DummyDataShareHelper::Release()
{
    // Nothing is held by the stub; releasing always succeeds.
    return true;
}

std::vector<std::string> DummyDataShareHelper::GetFileTypes(Uri &uri, const std::string &mimeTypeFilter)
{
    return {};
}

int DummyDataShareHelper::OpenFile(Uri &uri, const std::string &mode)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::OpenFileWithErrCode(Uri &uri, const std::string &mode, int32_t &errCode)
{
    errCode = DataShare::E_ERROR;
    return -1;
}

int DummyDataShareHelper::OpenRawFile(Uri &uri, const std::string &mode)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::Insert(Uri &uri, const DataShare::DataShareValuesBucket &value)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value, std::string &result)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
    const DataShare::DataShareValuesBucket &value)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::BatchUpdate(const DataShare::UpdateOperations &operations,
    std::vector<DataShare::BatchUpdateResult> &results)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::Delete(Uri &uri, const DataShare::DataSharePredicates &predicates)
{
    return DataShare::E_ERROR;
}

std::shared_ptr<DataShare::DataShareResultSet> DummyDataShareHelper::Query(Uri &uri,
    const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns,
    DataShare::DatashareBusinessError *businessError)
{
    // Returns nullptr on purpose: callers such as MediaFileUri::GetFilePath check the result
    // and fail cleanly, preserving the "no helper available" degraded behaviour.
    return nullptr;
}

std::string DummyDataShareHelper::GetType(Uri &uri)
{
    return "";
}

int DummyDataShareHelper::BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::ExecuteBatch(const std::vector<DataShare::OperationStatement> &statements,
    DataShare::ExecResultSet &result)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    return DataShare::E_ERROR;
}

void DummyDataShareHelper::NotifyChange(const Uri &uri) {}

int DummyDataShareHelper::RegisterObserverExtProvider(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::UnregisterObserverExtProvider(const Uri &uri,
    std::shared_ptr<DataShare::DataShareObserver> dataObserver)
{
    return DataShare::E_ERROR;
}

void DummyDataShareHelper::NotifyChangeExtProvider(const DataShare::DataShareObserver::ChangeInfo &changeInfo) {}

Uri DummyDataShareHelper::NormalizeUri(Uri &uri)
{
    return Uri(uri.ToString());
}

Uri DummyDataShareHelper::DenormalizeUri(Uri &uri)
{
    return Uri(uri.ToString());
}

int DummyDataShareHelper::AddQueryTemplate(const std::string &uri, int64_t subscriberId, DataShare::Template &tpl)
{
    return DataShare::E_ERROR;
}

int DummyDataShareHelper::DelQueryTemplate(const std::string &uri, int64_t subscriberId)
{
    return DataShare::E_ERROR;
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::Publish(const DataShare::Data &data,
    const std::string &bundleName)
{
    return {};
}

DataShare::Data DummyDataShareHelper::GetPublishedData(const std::string &bundleName, int &resultCode)
{
    resultCode = DataShare::E_ERROR;
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::SubscribeRdbData(
    const std::vector<std::string> &uris, const DataShare::TemplateId &templateId,
    const std::function<void(const DataShare::RdbChangeNode &changeNode)> &callback)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::UnsubscribeRdbData(
    const std::vector<std::string> &uris, const DataShare::TemplateId &templateId)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::EnableRdbSubs(
    const std::vector<std::string> &uris, const DataShare::TemplateId &templateId)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::DisableRdbSubs(
    const std::vector<std::string> &uris, const DataShare::TemplateId &templateId)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::SubscribePublishedData(
    const std::vector<std::string> &uris, int64_t subscriberId,
    const std::function<void(const DataShare::PublishedDataChangeNode &changeNode)> &callback)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::UnsubscribePublishedData(
    const std::vector<std::string> &uris, int64_t subscriberId)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::EnablePubSubs(
    const std::vector<std::string> &uris, int64_t subscriberId)
{
    return {};
}

std::vector<DataShare::OperationResult> DummyDataShareHelper::DisablePubSubs(
    const std::vector<std::string> &uris, int64_t subscriberId)
{
    return {};
}
} // namespace Media
} // namespace OHOS