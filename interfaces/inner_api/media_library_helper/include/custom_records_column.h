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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_CUSTOM_RECORDS_COLUMN_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_CUSTOM_RECORDS_COLUMN_H

#include "base_column.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class CustomRecordsColumns : BaseColumn {
public:
    static const std::string FILE_ID EXPORT;
    static const std::string BUNDLE_NAME EXPORT;
    static const std::string SHARE_COUNT EXPORT;
    static const std::string LCD_JUMP_COUNT EXPORT;

    static const std::string TABLE EXPORT;

    static const std::string CREATE_TABLE EXPORT;

    static const std::string CUSTOM_RECORDS_URI_PREFIX EXPORT;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_CUSTOM_RECORDS_COLUMN_H