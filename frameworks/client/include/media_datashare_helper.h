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

namespace OHOS::Media::IPC {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaDataShareHelper {
public:
    EXPORT MediaDataShareHelper();
    EXPORT ~MediaDataShareHelper();
    EXPORT void Init(const sptr<IRemoteObject> &token, const int32_t userId = -1);
    EXPORT bool IsValid(const int32_t userId = -1);
    EXPORT int32_t GetUserId();
    EXPORT void SetUserId(const int32_t userId);

protected:
    std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_ = nullptr;
    int32_t userId_;
    SafeMap<int32_t, std::shared_ptr<DataShare::DataShareHelper>> dataShareHelperMap_;
    std::shared_ptr<DataShare::DataShareHelper> GetDataShareHelper(const sptr<IRemoteObject> &token,
        const int32_t userId = -1);
    EXPORT std::shared_ptr<DataShare::DataShareHelper> GetDataShareHelperByUser(const int32_t userId);
};
} // namespace OHOS::Media::IPC

#endif // FRAMEWORKS_CLIENT_MEDIA_DATASHARE_HELPER_H_
