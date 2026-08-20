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

#ifndef FRAMEWORKS_NATIVE_CAPI_COMMON_MEDIA_USERFILE_CLIENT_H
#define FRAMEWORKS_NATIVE_CAPI_COMMON_MEDIA_USERFILE_CLIENT_H

#include "datashare_helper.h"
#include "uri.h"

#include "media_common_client.h"
#include "media_client_utils.h"

namespace OHOS {
namespace Media {

// UserFileClient is now a thin static wrapper around MediaCommonClient.
// All DataShare operations are delegated to the unified MediaCommonClient singleton.
class UserFileClient {
public:
    UserFileClient() {}
    virtual ~UserFileClient() {}

    static bool IsValid(const int32_t userId = -1)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().IsValid(userId);
    }

    static void Init()
    {
        OHOS::Media::IPC::MediaCommonClient::GetInstance().InitFromSa();
    }

    static void Init(const sptr<IRemoteObject> &token, bool isSetHelper = false, const int32_t userId = -1)
    {
        auto &client = OHOS::Media::IPC::MediaCommonClient::GetInstance();
        client.Init(token, userId);
    }

    static std::shared_ptr<DataShare::DataShareResultSet> Query(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Query(
            uri, predicates, columns, errCode);
    }

    static int Insert(Uri &uri, const DataShare::DataShareValuesBucket &value)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Insert(uri, value);
    }

    static int InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value, std::string &result)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().InsertExt(uri, value, result);
    }

    static int Delete(Uri &uri, const DataShare::DataSharePredicates &predicates)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Delete(uri, predicates);
    }

    static int OpenFile(Uri &uri, const std::string &mode)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().OpenFile(uri, mode);
    }

    static int Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
        const DataShare::DataShareValuesBucket &value)
    {
        return OHOS::Media::IPC::MediaCommonClient::GetInstance().Update(uri, predicates, value);
    }
};
}
}

#endif // FRAMEWORKS_NATIVE_CAPI_COMMON_MEDIA_USERFILE_CLIENT_H
