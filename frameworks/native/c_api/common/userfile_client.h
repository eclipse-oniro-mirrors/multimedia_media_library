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

namespace OHOS {
namespace Media {

class UserFileClient {
public:
    UserFileClient() {}
    virtual ~UserFileClient() {}
    static bool IsValid(const int32_t userId = -1);

    static void Init();
    static void Init(const sptr<IRemoteObject> &token, bool isSetHelper = false, const int32_t userId = -1);
    static std::shared_ptr<DataShare::DataShareResultSet> Query(Uri &uri,
        const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode);
    static int Insert(Uri &uri, const DataShare::DataShareValuesBucket &value);
    static int InsertExt(Uri &uri, const DataShare::DataShareValuesBucket &value, std::string &result);
    static int Delete(Uri &uri, const DataShare::DataSharePredicates &predicates);
    static int OpenFile(Uri &uri, const std::string &mode);
    static int Update(Uri &uri, const DataShare::DataSharePredicates &predicates,
        const DataShare::DataShareValuesBucket &value);
    static void Clear();
    static int32_t UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option);
    static int32_t UserDefineFunc(const int32_t &userId, MessageParcel &data, MessageParcel &reply,
        MessageOption &option);
    static void SetUserId(const int32_t userId);
    static int32_t GetUserId();

private:
    static inline std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
    static int32_t userId_;
};
}
}

#endif // FRAMEWORKS_NATIVE_CAPI_COMMON_MEDIA_USERFILE_CLIENT_H