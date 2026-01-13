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

#ifndef FRAMEWORKS_CLIENT_MEDIA_CLIENT_UTILS_H_
#define FRAMEWORKS_CLIENT_MEDIA_CLIENT_UTILS_H_

#include <iremote_object.h>

namespace OHOS::Media::IPC {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaClientUtils {
public:
    EXPORT MediaClientUtils();
    EXPORT ~MediaClientUtils();
    EXPORT static int32_t GetCurrentAccountId();
    EXPORT static sptr<IRemoteObject> GetSaToken();
};
} // namespace OHOS::Media::IPC

#endif // FRAMEWORKS_CLIENT_MEDIA_CLIENT_UTILS_H_
