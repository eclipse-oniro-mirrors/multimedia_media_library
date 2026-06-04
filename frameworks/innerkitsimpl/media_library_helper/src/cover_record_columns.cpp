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

#include "cover_record_columns.h"

namespace OHOS::Media {
const std::string CoverRecordColumns::ALBUM_TYPE = "album_type";
const std::string CoverRecordColumns::ALBUM_SUBTYPE = "album_subtype";
const std::string CoverRecordColumns::ALBUM_LPATH = "lpath";
const std::string CoverRecordColumns::COVER_ORDER_KEY = "cover_order_key";
const std::string CoverRecordColumns::COVER_ORDER_SUBKEY = "cover_order_subkey";
const std::string CoverRecordColumns::COVER_ORDER_TYPE = "cover_order_type";
const std::string CoverRecordColumns::HIDDEN_COVER_ORDER_KEY = "hidden_cover_order_key";
const std::string CoverRecordColumns::HIDDEN_COVER_ORDER_SUBKEY = "hidden_cover_order_subkey";
const std::string CoverRecordColumns::HIDDEN_COVER_ORDER_TYPE = "hidden_cover_order_type";

const std::string CoverRecordColumns::COVER_RECORD_TABLE = "tab_cover_record";
const std::string CoverRecordColumns::ALBUM_LPATH_INDEX = "lpath_index";

const std::string CoverRecordColumns::CREATE_COVER_RECORD_TABLE = CreateTable() +
    COVER_RECORD_TABLE + " (" +
    ALBUM_TYPE + " INT NOT NULL DEFAULT 0, " +
    ALBUM_SUBTYPE + " INT NOT NULL DEFAULT 0, " +
    ALBUM_LPATH + " TEXT DEFAULT NULL COLLATE NOCASE, " +
    COVER_ORDER_KEY + " TEXT DEFAULT NULL, " +
    COVER_ORDER_SUBKEY + " TEXT DEFAULT NULL, " +
    COVER_ORDER_TYPE + " INT NOT NULL DEFAULT 0, " +
    HIDDEN_COVER_ORDER_KEY + " TEXT DEFAULT NULL, " +
    HIDDEN_COVER_ORDER_SUBKEY + " TEXT DEFAULT NULL, " +
    HIDDEN_COVER_ORDER_TYPE +" INT NOT NULL DEFAULT 0" +
    ")";

const std::string CoverRecordColumns::CREATE_ALBUM_LPATH_INDEX = CreateIndex() +
    ALBUM_LPATH_INDEX + " ON " + COVER_RECORD_TABLE + " (" + ALBUM_TYPE +","+ ALBUM_SUBTYPE + "," + ALBUM_LPATH +
    " COLLATE NOCASE)";
} // namespace OHOS::MEDIA