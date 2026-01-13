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
#ifndef MTP_PTP_CONST_H
#define MTP_PTP_CONST_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace OHOS {
namespace Media {

using PathMap = std::unordered_map<std::string, std::string>;
const std::string CHINESE_ABBREVIATION  = "zh-Hans";
const std::string ENGLISH_ABBREVIATION  = "en-Latn-US";
const std::string PTP_DOC_NAME_EN       = "Gallery";
const std::string PTP_DOC_NAME_CN       = "图库";
const std::string DEFAULT_PREDICATE     = "0";
const std::string FILEMODE_READONLY     = "r";
const std::string DELIMETER_NAME        = ".";
const std::string DEFAULT_FORMAT_MP4    = ".mp4";
const std::string PATH_LOCAL_STR        = "/local";
constexpr uint32_t ALL_HANDLE_ID        = 0xFFFFFFFF;
constexpr uint32_t BASE_USER_RANGE      = 200000;

enum HANDLE_DEFAULT_ID : uint32_t {
    DEFAULT_PARENT_ROOT = 0,
    PTP_IN_MTP_ID = 500000000,
    START_ID
};

enum STORAGE_ID : uint32_t {
    INNER_STORAGE_ID = 1,
    SD_START_ID = INNER_STORAGE_ID + 1,
    SD_END_ID = SD_START_ID + 127
};

} // namespace Media
} // namespace OHOS
#endif // MTP_PTP_CONST_H
