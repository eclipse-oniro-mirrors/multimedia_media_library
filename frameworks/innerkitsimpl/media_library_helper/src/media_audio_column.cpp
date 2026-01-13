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

#include "media_audio_column.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

const std::string AudioColumn::AUDIO_ALBUM = "audio_album";
const std::string AudioColumn::AUDIO_ARTIST = "artist";
const std::string AudioColumn::AUDIO_FILE_SOURCE_TYPE = "file_source_type";
const std::string AudioColumn::AUDIO_IS_TEMP = "is_temp";

const std::string AudioColumn::AUDIOS_TABLE = "Audios";

const std::string AudioColumn::AUDIO_URI_PREFIX = "file://media/Audio/";
const std::string AudioColumn::DEFAULT_AUDIO_URI = "file://media/Audio";
const std::string AudioColumn::AUDIO_TYPE_URI = "/Audio";

const std::string AudioColumn::CREATE_AUDIO_TABLE = "CREATE TABLE IF NOT EXISTS " +
    AUDIOS_TABLE + " (" +
    MEDIA_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    MEDIA_FILE_PATH + " TEXT, " +
    MEDIA_SIZE + " BIGINT, " +
    MEDIA_TITLE + " TEXT, " +
    MEDIA_NAME + " TEXT, " +
    MEDIA_TYPE + " INT, " +
    MEDIA_MIME_TYPE + " TEXT, " +
    MEDIA_OWNER_PACKAGE + " TEXT, " +
    MEDIA_OWNER_APPID + " TEXT, " +
    MEDIA_PACKAGE_NAME + " TEXT, " +
    MEDIA_DEVICE_NAME + " TEXT, " +
    AUDIO_ARTIST + " TEXT, " +
    AUDIO_FILE_SOURCE_TYPE + " INT NOT NULL DEFAULT 0, " +
    AUDIO_IS_TEMP + " INT DEFAULT 0," +
    MEDIA_DATE_ADDED + " BIGINT, " +
    MEDIA_DATE_MODIFIED + " BIGINT, " +
    MEDIA_DATE_TAKEN + " BIGINT DEFAULT 0, " +
    MEDIA_DURATION + " INT, " +
    MEDIA_TIME_PENDING + " BIGINT DEFAULT 0, " +
    MEDIA_IS_FAV + " INT DEFAULT 0, " +
    MEDIA_DATE_TRASHED + " BIGINT DEFAULT 0, " +
    MEDIA_DATE_DELETED + " BIGINT DEFAULT 0, " +
    MEDIA_PARENT_ID + " INT DEFAULT 0, " +
    MEDIA_RELATIVE_PATH + " TEXT, " +
    MEDIA_VIRTURL_PATH + " TEXT UNIQUE, " +
    AUDIO_ALBUM + " TEXT)";

const std::string AudioColumn::QUERY_MEDIA_VOLUME = "SELECT sum(" + MediaColumn::MEDIA_SIZE + ") AS " +
    MediaColumn::MEDIA_SIZE + "," +
    MediaColumn::MEDIA_TYPE + " FROM " +
    AudioColumn::AUDIOS_TABLE + " WHERE " +
    MediaColumn::MEDIA_TYPE + " = " + std::to_string(MEDIA_TYPE_AUDIO) + " GROUP BY " +
    MediaColumn::MEDIA_TYPE;

const std::set<std::string> AudioColumn::AUDIO_COLUMNS = {
    AudioColumn::AUDIO_ALBUM, AudioColumn::AUDIO_ARTIST, AudioColumn::AUDIO_FILE_SOURCE_TYPE, AudioColumn::AUDIO_IS_TEMP
};

bool AudioColumn::IsAudioColumn(const std::string &columnName)
{
    return (AUDIO_COLUMNS.find(columnName) != AUDIO_COLUMNS.end()) ||
        (MEDIA_COLUMNS.find(columnName) != MEDIA_COLUMNS.end());
}

}  // namespace Media
}  // namespace OHOS
