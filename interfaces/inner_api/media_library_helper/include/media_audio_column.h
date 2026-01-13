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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_AUDIO_COLUMN_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_AUDIO_COLUMN_H_

#include "media_column.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class AudioColumn : public MediaColumn {
public:
    // column only in AudioTable
    static const std::string AUDIO_ALBUM EXPORT;
    static const std::string AUDIO_ARTIST EXPORT;
    static const std::string AUDIO_FILE_SOURCE_TYPE EXPORT;
    static const std::string AUDIO_IS_TEMP EXPORT;

    // table name
    static const std::string AUDIOS_TABLE EXPORT;

    // create AudioTable sql
    static const std::string CREATE_AUDIO_TABLE EXPORT;

    // audio uri
    static const std::string AUDIO_URI_PREFIX EXPORT;
    static const std::string AUDIO_TYPE_URI EXPORT;
    static const std::string DEFAULT_AUDIO_URI EXPORT;

    // all columns
    static const std::set<std::string> AUDIO_COLUMNS EXPORT;

    static const std::string QUERY_MEDIA_VOLUME EXPORT;

    static bool IsAudioColumn(const std::string &columnName) EXPORT;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_AUDIO_COLUMN_H_