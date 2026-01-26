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

#ifndef CUSTOM_RESTORE_CONST_H
#define CUSTOM_RESTORE_CONST_H

#include <string>

namespace OHOS {
namespace Media {
const int32_t SHARE_RESTORE_DB_VERSION = 1;
constexpr ssize_t SHARE_RESTORE_DB_WAL_LIMIT_SIZE = 1024 * 1024 * 1024;
const std::string SHARE_RESTORE_TABLE_NAME = "media_info";
const std::string SHARE_RESTORE_MEDIA_INFO_FILE_NAME = "file_name";
const std::string SHARE_RESTORE_MEDIA_INFO_DATE_ADDED = "date_added";
const std::string SHARE_RESTORE_MEDIA_INFO_DATE_TAKEN = "date_taken";
const std::string SHARE_RESTORE_MEDIA_INFO_DETAIL_TIME = "detail_time";
}  // namespace Media
}  // namespace OHOS

#endif  // CUSTOM_RESTORE_CONST_H
