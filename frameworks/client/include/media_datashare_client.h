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

#ifndef FRAMEWORKS_CLIENT_MEDIA_DATASHARE_CLIENT_H_
#define FRAMEWORKS_CLIENT_MEDIA_DATASHARE_CLIENT_H_

#include "media_datashare_helper.h"

#include <vector>

#include "datashare_predicates.h"
#include "medialibrary_operation.h"

namespace OHOS::Media::IPC {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaDataShareClient : public MediaDataShareHelper {
public:
    EXPORT static MediaDataShareClient &GetInstance();
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> Query(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns,
        int &errCode, const int32_t userId = -1);
    EXPORT int Insert(Uri &uri, const DataShare::DataShareValuesBucket &value, const int32_t userId = -1);
    EXPORT int InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value, std::string &result,
        const int32_t userId = -1);
    EXPORT int BatchInsert(Uri &uri, const std::vector<DataShare::DataShareValuesBucket> &values);
    EXPORT int Delete(Uri &uri, const DataShare::DataSharePredicates &predicates);
    EXPORT void NotifyChange(const Uri &uri);
    EXPORT void RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);
    EXPORT void UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);
    EXPORT int OpenFile(Uri &uri, const std::string &mode, const int32_t userId = -1);
    EXPORT int Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
        const DataShare::DataShareValuesBucket &value, const int32_t userId = -1);
    EXPORT void RegisterObserverExt(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants);
    EXPORT void UnregisterObserverExt(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver);
    EXPORT void Clear();
    EXPORT std::string GetType(Uri &uri);
    EXPORT int32_t RegisterObserverExtProvider(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver, bool isDescendants);
    EXPORT int32_t UnregisterObserverExtProvider(const Uri &uri,
        std::shared_ptr<DataShare::DataShareObserver> dataObserver);

protected:
    EXPORT bool IsNoIpc(Uri &uri, OperationObject &object, const DataShare::DataSharePredicates &predicates,
        bool isIgnoreSELinux = false);
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> QueryWithoutIpc(
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, OperationObject &object,
        int &errCode);

    MediaDataShareClient();
    ~MediaDataShareClient();
};
} // namespace OHOS::Media::IPC

#endif // FRAMEWORKS_CLIENT_MEDIA_DATASHARE_CLIENT_H_
