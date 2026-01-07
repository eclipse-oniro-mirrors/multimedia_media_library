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

#ifndef OHOS_MEDIA_NOTIFY_REGISTER_PERMISSION_H
#define OHOS_MEDIA_NOTIFY_REGISTER_PERMISSION_H

#include "observer_info.h"
#include "permission_utils.h"
#include "media_change_info.h"

namespace OHOS::Media {
namespace Notification {
#define EXPORT __attribute__ ((visibility ("default")))
/**
 * 数据库表鉴权处理器
 */
class NotifyRegisterPermission {
public:
    NotifyRegisterPermission(){};
    ~NotifyRegisterPermission(){};

    int32_t ExecuteCheckPermission(const NotifyUriType &registerUriType);
    bool isSystemApp();
    EXPORT int32_t SinglePermissionCheck(const NotifyUriType &registerUriType, const std::string& singleId);

private:
    int32_t BasicPermissionCheck();
    int32_t TranshPermissionCheck();
    int32_t HiddenPermissionCheck();
};

}
} // namespace OHOS::Media
#endif //OHOS_MEDIA_NOTIFY_REGISTER_PERMISSION_H