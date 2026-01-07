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

#ifndef FRAMEWORKS_CLIENT_MEDIA_COMMON_CLIENT_H_
#define FRAMEWORKS_CLIENT_MEDIA_COMMON_CLIENT_H_

#include "media_datashare_client.h"

#include "rdb_store.h"

#include "datashare_predicates.h"
#include "medialibrary_operation.h"

namespace OHOS::Media::IPC {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaCommonClient : public MediaDataShareClient {
public:
    EXPORT static MediaCommonClient &GetInstance();
    EXPORT std::pair<bool, std::shared_ptr<DataShare::DataShareResultSet>> QueryAccessibleViaSandBox(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode,
        const int32_t userId = -1);
    EXPORT bool IsNoIpc(Uri &uri, OperationObject &object, const DataShare::DataSharePredicates &predicates,
        bool isIgnoreSELinux = false);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryRdb(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns);
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const std::string &sql);
    EXPORT int OpenFileWithErrCode(
        Uri &uri, const std::string &mode, int32_t &realErr, const int32_t userId = -1);
protected:
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> QueryWithoutIpc(
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, OperationObject &object,
        int &errCode);

    MediaCommonClient();
    ~MediaCommonClient();
};
} // namespace OHOS::Media::IPC

#endif // FRAMEWORKS_CLIENT_MEDIA_COMMON_CLIENT_H_
