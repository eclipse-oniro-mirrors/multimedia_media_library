/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_MAP_COLUMNS_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_MAP_COLUMNS_H

#include <string>

#include "base_column.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class PhotoMap : BaseColumn {
public:
    // Sql to create the table
    static const std::string CREATE_TABLE EXPORT;

    static const std::string TABLE EXPORT;
    static const std::string ALBUM_ID EXPORT;
    static const std::string ASSET_ID EXPORT;
    static const std::string DIRTY EXPORT;

    // create triggers
    static const std::string CREATE_NEW_TRIGGER EXPORT;
    static const std::string CREATE_DELETE_TRIGGER EXPORT;

    // create index
    static const std::string CREATE_IDX_FILEID_FOR_PHOTO_MAP EXPORT;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_PHOTO_MAP_COLUMNS_H
