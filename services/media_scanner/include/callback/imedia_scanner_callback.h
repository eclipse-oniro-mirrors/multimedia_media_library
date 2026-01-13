/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef IMEDIA_SCANNER_CALLBACK_H
#define IMEDIA_SCANNER_CALLBACK_H

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class IMediaScannerCallback {
public:
    EXPORT virtual ~IMediaScannerCallback() = default;

    /**
     * @brief OnScanFinished will be executed when client receives callback from service after scan is finished/error
     *
     * @param status scan result
     * @param uri file uri generated after database updation. For scanDir(), uri will be empty
     * @param path The path which was requested for scanning
     */
    EXPORT virtual int32_t OnScanFinished(const int32_t status, const std::string &uri, const std::string &path) = 0;
};
} // namespace Media
} // namespace OHOS
#endif
