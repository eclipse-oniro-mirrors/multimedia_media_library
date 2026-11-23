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

#include "datashare_helper.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * A resource-free stub implementation of DataShare::DataShareHelper, used only inside the
 * media library provider process.
 *
 * MediaLibraryHelperContainer is never populated in this process: creating a real
 * DataShareHelper here would connect back to our own DataShareExtension
 * (self-connect). Callers that still need a non-null DataShareHelper object (e.g.
 * InitMediaLibraryMgr calls RegisterObserverExt on it) can fall back to this stub to keep
 * a valid vtable (CFI safe) without introducing any connection, thread or member state.
 *
 * All pure virtual interfaces are no-ops returning failure, so accidental data access
 * through the stub fails cleanly instead of blocking or crashing. Note that the
 * non-virtual members RegisterObserverExt/UnregisterObserverExt/NotifyChangeExt keep the
 * base class behaviour: they register observers through DataObsMgrClient and do not
 * depend on 'this' state.
 */
class DummyDataShareHelper : public DataShare::DataShareHelper {
public:
    EXPORT static std::shared_ptr<DummyDataShareHelper> GetInstance();

    ~DummyDataShareHelper() override = default;

    bool Release() override;
    std::vector<std::string> GetFileTypes(Uri &uri, const std::string &mimeTypeFilter) override;
    int OpenFile(Uri &uri, const std::string &mode) override;
    int OpenFileWithErrCode(Uri &uri, const std::string &mode, int32_t &errCode) override;
    int OpenRawFile(Uri &uri, const std::string &mode) override;
    int Insert(Uri &uri, const DataShare::DataShareValuesBucket &value) override;
    int InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value, std::string &result) override;
    int Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
        const DataShare::DataShareValuesBucket &value) override;
    int BatchUpdate(const DataShare::UpdateOperations &operations,
        std::vector<DataShare::BatchUpdateResult> &results) override;
    int Delete(Uri &uri, const DataShare::DataSharePredicates &predicates) override;
    std::shared_ptr<DataShare::DataShareResultSet> Query(Uri &uri, const DataShare::DataSharePredicates &predicates,
        std::vector<std::string> &columns, DataShare::DatashareBusinessError *businessError) override;
    std::string GetType(Uri &uri) override;
    int BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values) override;
    int ExecuteBatch(const std::vector<DataShare::OperationStatement> &statements,
        DataShare::ExecResultSet &result) override;
    int RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver) override;
    int UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver) override;
    void NotifyChange(const Uri &uri) override;
    int RegisterObserverExtProvider(const Uri &uri, std::shared_ptr<DataShare::DataShareObserver> dataObserver,
        bool isDescendants) override;
    int UnregisterObserverExtProvider(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver) override;
    void NotifyChangeExtProvider(const DataShare::DataShareObserver::ChangeInfo &changeInfo) override;
    Uri NormalizeUri(Uri &uri) override;
    Uri DenormalizeUri(Uri &uri) override;
    int AddQueryTemplate(const std::string &uri, int64_t subscriberId, DataShare::Template &tpl) override;
    int DelQueryTemplate(const std::string &uri, int64_t subscriberId) override;
    std::vector<DataShare::OperationResult> Publish(const DataShare::Data &data,
        const std::string &bundleName) override;
    DataShare::Data GetPublishedData(const std::string &bundleName, int &resultCode) override;
    std::vector<DataShare::OperationResult> SubscribeRdbData(const std::vector<std::string> &uris,
        const DataShare::TemplateId &templateId,
        const std::function<void(const DataShare::RdbChangeNode &changeNode)> &callback) override;
    std::vector<DataShare::OperationResult> UnsubscribeRdbData(const std::vector<std::string> &uris,
        const DataShare::TemplateId &templateId) override;
    std::vector<DataShare::OperationResult> EnableRdbSubs(const std::vector<std::string> &uris,
        const DataShare::TemplateId &templateId) override;
    std::vector<DataShare::OperationResult> DisableRdbSubs(const std::vector<std::string> &uris,
        const DataShare::TemplateId &templateId) override;
    std::vector<DataShare::OperationResult> SubscribePublishedData(const std::vector<std::string> &uris,
        int64_t subscriberId,
        const std::function<void(const DataShare::PublishedDataChangeNode &changeNode)> &callback) override;
    std::vector<DataShare::OperationResult> UnsubscribePublishedData(const std::vector<std::string> &uris,
        int64_t subscriberId) override;
    std::vector<DataShare::OperationResult> EnablePubSubs(const std::vector<std::string> &uris,
        int64_t subscriberId) override;
    std::vector<DataShare::OperationResult> DisablePubSubs(const std::vector<std::string> &uris,
        int64_t subscriberId) override;

private:
    DummyDataShareHelper() = default;
    static std::shared_ptr<DummyDataShareHelper> instance_;
};
} // namespace Media
} // namespace OHOS