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

#ifndef OHOS_ANALYSIS_METADATA_QUERY_TEST_HELPER_H
#define OHOS_ANALYSIS_METADATA_QUERY_TEST_HELPER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "datashare_helper.h"
#include "gmock/gmock.h"
#include "media_library_manager.h"
#include "os_account_manager.h"

namespace OHOS::Media {
class AnalysisMetadataMockDataShareHelper final : public DataShare::DataShareHelper {
public:
    AnalysisMetadataMockDataShareHelper() = default;
    ~AnalysisMetadataMockDataShareHelper() override = default;
    AnalysisMetadataMockDataShareHelper(const AnalysisMetadataMockDataShareHelper &) = delete;
    AnalysisMetadataMockDataShareHelper &operator=(const AnalysisMetadataMockDataShareHelper &) = delete;
    AnalysisMetadataMockDataShareHelper(AnalysisMetadataMockDataShareHelper &&) = delete;
    AnalysisMetadataMockDataShareHelper &operator=(AnalysisMetadataMockDataShareHelper &&) = delete;

    MOCK_METHOD(std::shared_ptr<DataShare::DataShareResultSet>, Query,
        (Uri &, const DataShare::DataSharePredicates &, std::vector<std::string> &,
        DataShare::DatashareBusinessError *), (override));
    MOCK_METHOD(int, Insert, (Uri &, const DataShare::DataShareValuesBucket &), (override));
    MOCK_METHOD(int, Update, (Uri &, const DataShare::DataSharePredicates &,
        const DataShare::DataShareValuesBucket &), (override));
    MOCK_METHOD(int, Delete, (Uri &, const DataShare::DataSharePredicates &), (override));
    MOCK_METHOD(std::shared_ptr<DataShare::DataShareResultSet>, Query, (Uri &, const DataShare::DataSharePredicates &,
        std::vector<std::string> &, DataShare::DataShareOption &, DataShare::DatashareBusinessError *), (override));
    MOCK_METHOD(bool, Release, (), (override));
    MOCK_METHOD(std::vector<std::string>, GetFileTypes, (Uri &, const std::string &), (override));
    MOCK_METHOD(int, OpenFile, (Uri &, const std::string &), (override));
    MOCK_METHOD(int, OpenFileWithErrCode, (Uri &, const std::string &, int32_t &), (override));
    MOCK_METHOD(int, OpenRawFile, (Uri &, const std::string &), (override));
    MOCK_METHOD(int, InsertExt, (Uri &, const DataShare::DataShareValuesBucket &, std::string &), (override));
    MOCK_METHOD(int, BatchUpdate,
        (const DataShare::UpdateOperations &, std::vector<DataShare::BatchUpdateResult> &), (override));
    MOCK_METHOD(std::string, GetType, (Uri &), (override));
    MOCK_METHOD(int, BatchInsert, (Uri &, const std::vector<DataShare::DataShareValuesBucket> &), (override));
    MOCK_METHOD(int, ExecuteBatch,
        (const std::vector<DataShare::OperationStatement> &, DataShare::ExecResultSet &), (override));
    MOCK_METHOD(int, RegisterObserver, (const Uri &, const sptr<AAFwk::IDataAbilityObserver> &), (override));
    MOCK_METHOD(int, UnregisterObserver, (const Uri &, const sptr<AAFwk::IDataAbilityObserver> &), (override));
    MOCK_METHOD(void, NotifyChange, (const Uri &), (override));
    MOCK_METHOD(int, RegisterObserverExtProvider,
        (const Uri &, std::shared_ptr<DataShare::DataShareObserver>, bool), (override));
    MOCK_METHOD(int, UnregisterObserverExtProvider,
        (const Uri &, std::shared_ptr<DataShare::DataShareObserver>), (override));
    MOCK_METHOD(void, NotifyChangeExtProvider, (const DataShare::DataShareObserver::ChangeInfo &), (override));
    MOCK_METHOD(Uri, NormalizeUri, (Uri &), (override));
    MOCK_METHOD(Uri, DenormalizeUri, (Uri &), (override));
    MOCK_METHOD(int, AddQueryTemplate, (const std::string &, int64_t, DataShare::Template &), (override));
    MOCK_METHOD(int, DelQueryTemplate, (const std::string &, int64_t), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, Publish,
        (const DataShare::Data &, const std::string &), (override));
    MOCK_METHOD(DataShare::Data, GetPublishedData, (const std::string &, int &), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, SubscribeRdbData, (const std::vector<std::string> &,
        const DataShare::TemplateId &, const std::function<void(const DataShare::RdbChangeNode &)> &), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, UnsubscribeRdbData, (const std::vector<std::string> &,
        const DataShare::TemplateId &), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, EnableRdbSubs, (const std::vector<std::string> &,
        const DataShare::TemplateId &), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, DisableRdbSubs, (const std::vector<std::string> &,
        const DataShare::TemplateId &), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, SubscribePublishedData, (const std::vector<std::string> &,
        int64_t, const std::function<void(const DataShare::PublishedDataChangeNode &)> &), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, UnsubscribePublishedData, (const std::vector<std::string> &,
        int64_t), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, EnablePubSubs,
        (const std::vector<std::string> &, int64_t), (override));
    MOCK_METHOD(std::vector<DataShare::OperationResult>, DisablePubSubs,
        (const std::vector<std::string> &, int64_t), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), InsertEx, (Uri &, const DataShare::DataShareValuesBucket &), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), UpdateEx, (Uri &, const DataShare::DataSharePredicates &,
        const DataShare::DataShareValuesBucket &), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), DeleteEx, (Uri &, const DataShare::DataSharePredicates &), (override));
    MOCK_METHOD(int32_t, UserDefineFunc, (MessageParcel &, MessageParcel &, MessageOption &), (override));
};

class ScopedAnalysisMetadataHelper final {
public:
    explicit ScopedAnalysisMetadataHelper(const std::shared_ptr<DataShare::DataShareHelper> &helper)
        : savedHelper_(MediaLibraryManager::currentDataShareHelper_), savedUserId_(MediaLibraryManager::userId_)
    {
        constexpr int32_t DEFAULT_ACCOUNT_ID = 100;
        int32_t activeUser = DEFAULT_ACCOUNT_ID;
        // Match Manager's account selection so it keeps the injected helper.
        (void)AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(activeUser);
        MediaLibraryManager::userId_ = activeUser;
        MediaLibraryManager::currentDataShareHelper_ = helper;
    }

    ~ScopedAnalysisMetadataHelper()
    {
        MediaLibraryManager::currentDataShareHelper_ = savedHelper_;
        MediaLibraryManager::userId_ = savedUserId_;
    }

    ScopedAnalysisMetadataHelper(const ScopedAnalysisMetadataHelper &) = delete;
    ScopedAnalysisMetadataHelper &operator=(const ScopedAnalysisMetadataHelper &) = delete;
    ScopedAnalysisMetadataHelper(ScopedAnalysisMetadataHelper &&) = delete;
    ScopedAnalysisMetadataHelper &operator=(ScopedAnalysisMetadataHelper &&) = delete;

private:
    std::shared_ptr<DataShare::DataShareHelper> savedHelper_;
    int32_t savedUserId_;
};
} // namespace OHOS::Media
#endif // OHOS_ANALYSIS_METADATA_QUERY_TEST_HELPER_H
