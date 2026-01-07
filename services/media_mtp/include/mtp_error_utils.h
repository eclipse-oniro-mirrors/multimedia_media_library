/*
* Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_ERROR_UTILS_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_ERROR_UTILS_H_
#include <memory>
#include <vector>
#include <string>

namespace OHOS {
namespace Media {
class MtpErrorUtils {
public:
    static int32_t SolveGetHandlesError(const int32_t mediaError);
    static int32_t SolveGetObjectInfoError(const int32_t mediaError);
    static int32_t SolveGetFdError(const int32_t mediaError);
    static int32_t SolveSendObjectInfoError(const int32_t mediaError);
    static int32_t SolveMoveObjectError(const int32_t mediaError);
    static int32_t SolveCopyObjectError(const int32_t mediaError);
    static int32_t SolveDeleteObjectError(const int32_t mediaError);
    static int32_t SolveObjectPropValueError(const int32_t mediaError);
    static int32_t SolveCloseFdError(const int32_t mediaError);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_ERROR_UTILS_H_
