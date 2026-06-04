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

#ifndef UPGRADE_ALBUM_SQLS_H
#define UPGRADE_ALBUM_SQLS_H
// table name need to be added here
#define TABLE_ALBUM_PLUGIN "album_plugin"
#define TABLE_PHOTO_ALBUM "PhotoAlbum"
#define TABLE_TAB_COVER_RECORD "tab_cover_record"

// column name should be added here
#define COLUMN_COVER_ORDER_KEY "cover_order_key"
#define COLUMN_COVER_ORDER_SUBKEY "cover_order_subkey"
#define COLUMN_COVER_ORDER_TYPE "cover_order_type"
#define COLUMN_HIDDEN_COVER_ORDER_KEY "hidden_cover_order_key"
#define COLUMN_HIDDEN_COVER_ORDER_SUBKEY "hidden_cover_order_subkey"
#define COLUMN_HIDDEN_COVER_ORDER_TYPE "hidden_cover_order_type"

// sqls only execute in upgrade progress should be added here
#define SQL_UPGRADE_CREATE_TAB_COVER_RECORD \
    "CREATE TABLE IF NOT EXISTS tab_cover_record (" \
    "album_type INT NOT NULL DEFAULT 0, " \
    "album_subtype  INT NOT NULL DEFAULT 0, " \
    "lpath TEXT DEFAULT NULL COLLATE NOCASE, " \
    "cover_order_key TEXT DEFAULT NULL, " \
    "cover_order_subkey TEXT DEFAULT NULL, " \
    "cover_order_type INT NOT NULL DEFAULT 0, " \
    "hidden_cover_order_key TEXT DEFAULT NULL, " \
    "hidden_cover_order_subkey TEXT DEFAULT NULL, " \
    "hidden_cover_order_type INT NOT NULL DEFAULT 0)"
#endif // UPGRADE_ALBUM_SQLS_H