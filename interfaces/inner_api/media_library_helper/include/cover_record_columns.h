/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_COVER_RECORD_COLUMNS_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_COVER_RECORD_COLUMNS_H

#include "base_column.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class CoverRecordColumns : BaseColumn {
public:
    static const std::string ALBUM_TYPE EXPORT;
    static const std::string ALBUM_SUBTYPE EXPORT;
    static const std::string ALBUM_LPATH EXPORT;
    static const std::string COVER_ORDER_KEY EXPORT;
    static const std::string COVER_ORDER_SUBKEY EXPORT;
    static const std::string COVER_ORDER_TYPE EXPORT;
    static const std::string HIDDEN_COVER_ORDER_KEY EXPORT;
    static const std::string HIDDEN_COVER_ORDER_SUBKEY EXPORT;
    static const std::string HIDDEN_COVER_ORDER_TYPE EXPORT;

    static const std::string COVER_RECORD_TABLE EXPORT;
    static const std::string ALBUM_LPATH_INDEX EXPORT;

    static const std::string CREATE_COVER_RECORD_TABLE EXPORT;
    static const std::string CREATE_ALBUM_LPATH_INDEX EXPORT;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_COVER_RECORD_COLUMNS_H