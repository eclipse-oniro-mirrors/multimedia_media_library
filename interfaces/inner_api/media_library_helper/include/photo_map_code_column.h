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

#ifndef PHOTO_MAP_CODE_COLUMN_H
#define PHOTO_MAP_CODE_COLUMN_H

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class PhotoMapCodeColumn {
public:
    static const std::string MAPCODE_LEVEL_SCALE_NUMBER EXPORT;
    static const std::string PHOTOS_MAP_CODE_TABLE EXPORT;

    static const std::string MAPCODE_FILE_ID EXPORT;
    static const std::string MAPCODE_LEVEL_20 EXPORT;

    static const std::string MAPCODE_LEVEL_5_INDEX EXPORT;
    static const std::string MAPCODE_LEVEL_20_INDEX EXPORT;

    static const std::string CREATE_MAP_CODE_TABLE EXPORT;

    static const std::string CREATE_MAPCODE_LEVEL_5_INDEX EXPORT;
    static const std::string CREATE_MAPCODE_LEVEL_20_INDEX EXPORT;

    static const std::string DROP_MAPCODE_LEVEL_5_INDEX EXPORT;
    static const std::string DROP_MAPCODE_LEVEL_20_INDEX EXPORT;

    static const std::string INSERT_MAP_CODE_TRIGGER EXPORT;
    static const std::string CREATE_MAP_CODE_INSERT_TRIGGER EXPORT;

    static const std::string UPDATE_MAP_CODE_TRIGGER EXPORT;
    static const std::string CREATE_MAP_CODE_UPDATE_TRIGGER EXPORT;

    static const std::string CLEAR_MAP_CODE_TRIGGER EXPORT;
    static const std::string CREATE_MAP_CODE_CLEAR_TRIGGER EXPORT;

    static const std::string DELETE_MAP_CODE_TRIGGER EXPORT;
    static const std::string CREATE_MAP_CODE_DELETE_TRIGGER EXPORT;
};
} // namespace Media
} // namespace OHOS

#endif // PHOTO_MAP_CODE_COLUMN_H
