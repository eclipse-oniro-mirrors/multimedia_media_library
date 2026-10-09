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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_SHARE_MEMBER_COLUMN_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_SHARE_MEMBER_COLUMN_H

#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

#define SQL_CREATE_TAB_SHARE_ALBUM_MEMBER \
    "CREATE TABLE IF NOT EXISTS tab_share_album_member (" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT, " \
    "album_id INTEGER DEFAULT NULL, " \
    "share_member TEXT DEFAULT NULL, " \
    "share_member_status INT DEFAULT 0)"

#define SQL_CREATE_TAB_SHARE_ALBUM_MEMBER_INDEX \
    "CREATE UNIQUE INDEX IF NOT EXISTS idx_share_album_member_album_id_member ON " \
    "tab_share_album_member (album_id, share_member)"

class ShareMemberColumn {
public:
    static const std::string TABLE_NAME EXPORT;
    static const std::string COLUMN_ID EXPORT;
    static const std::string COLUMN_ALBUM_ID EXPORT;
    static const std::string COLUMN_SHARE_MEMBER EXPORT;
    static const std::string COLUMN_SHARE_MEMBER_STATUS EXPORT;
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_SHARE_MEMBER_COLUMN_H
