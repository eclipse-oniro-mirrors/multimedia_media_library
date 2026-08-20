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

#ifndef FRAMEWORKS_CLIENT_MEDIA_DATASHARE_HELPER_H_
#define FRAMEWORKS_CLIENT_MEDIA_DATASHARE_HELPER_H_

#include "datashare_helper.h"
#include "safe_map.h"

#include <iremote_object.h>
#include <mutex>
#include <string>
 
#include "message_parcel.h"
#include "message_option.h"

namespace OHOS::Media::IPC {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaDataShareHelper {
protected:
    int32_t userId_{-1};
public:
    EXPORT MediaDataShareHelper();
    EXPORT ~MediaDataShareHelper();
    EXPORT void Init(const sptr<IRemoteObject> &token, const int32_t userId = -1);
    EXPORT bool IsValid(const int32_t userId = -1);
    EXPORT int32_t GetUserId();
    EXPORT void SetUserId(const int32_t userId);

    // Resolve effective userId: if userId == -1, use member userId_; if also -1, get from GetCurrentAccountId
    EXPORT int32_t ResolveUserId(int32_t userId);
 
    // Init from SystemAbility (for CAPI/Tool layers without app context)
    EXPORT void InitFromSa(const int32_t userId = -1);
 
    // Init for active user (auto-get userId via OsAccountManager)
    EXPORT void InitForActiveUser();
 
protected:
    std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;

    SafeMap<int32_t, std::shared_ptr<DataShare::DataShareHelper>> dataShareHelperMap_;
    std::shared_ptr<DataShare::DataShareHelper> GetDataShareHelper(const sptr<IRemoteObject> &token,
        const int32_t userId = -1);
    EXPORT std::shared_ptr<DataShare::DataShareHelper> GetDataShareHelperByUser(const int32_t userId);
     
    // Force reconnect helper for a specific userId (clears and re-creates)
    EXPORT bool ForceReconnect(const int32_t userId = -1);
};
} // namespace OHOS::Media::IPC

#endif // FRAMEWORKS_CLIENT_MEDIA_DATASHARE_HELPER_H_
