/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SCANNER_CONST_H
#define MEDIA_SCANNER_CONST_H

#include <string>

namespace OHOS {
namespace Media {
enum ScannerApiTypes : int32_t {
    MEDIA_SCAN_DIR_ABILITY = 0,
    MEDIA_SCAN_FILE_ABILITY,
    MEDIA_SCAN_ON_CALLBACK,
    MEDIA_GET_SCAN_STATUS
};

enum ScanType : uint32_t {
    SCAN_FILE = 0,
    SCAN_DIR
};

enum ScannerIpcErrorTypes : int32_t {
    SCAN_IPC_ERR = -1,
    SCAN_IPC_SUCCESS,
    SCAN_PROXY_WR_ERR,
    SCAN_PROXY_RD_ERR,
    SCAN_PROXY_IF_TOKEN_WR_ERR,
    SCAN_STUB_WR_ERR,
    SCAN_STUB_RD_ERR,
    SCAN_STUB_IF_TOKEN_INVALID
};

enum ConnectionState : int32_t {
    CONN_NONE,
    CONN_IN_PROGRESS,
    CONN_SUCCESS,
    CONN_ERROR
};

const int32_t NO_PARENT = 0;
} // namespace Media
} // namespace OHOS

#endif // MEDIA_SCANNER_CONST_H
