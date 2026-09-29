/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_APP_URI_PERMISSION_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_APP_URI_PERMISSION_H_

#include <string>
#include <set>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class AppUriPermissionColumn {
public:
    // columns
    static const std::string ID;
    static const std::string APP_ID;
    static const std::string FILE_ID;
    static const std::string URI_TYPE;
    static const std::string PERMISSION_TYPE;
    static const std::string DATE_MODIFIED;
    static const std::string SOURCE_TOKENID;
    static const std::string TARGET_TOKENID;

    // uriTypes
    static const int URI_PHOTO;
    static const int URI_AUDIO;
    static const std::set<int> URI_TYPES_ALL;

    // permissionTypes
    static const int PERMISSION_TEMPORARY_READ;
    static const int PERMISSION_PERSIST_READ;
    static const int PERMISSION_TEMPORARY_WRITE;
    static const int PERMISSION_TEMPORARY_READ_WRITE;
    static const int PERMISSION_PERSIST_READ_WRITE;
    static const int PERMISSION_PERSIST_WRITE;

    static const std::set<int> PERMISSION_TYPE_WRITE;
    static const std::set<int> PERMISSION_TYPE_READ;
    static const std::set<int> PERMISSION_TYPES_ALL;
    static const std::set<int> PERMISSION_TYPES_PICKER;
    static const std::set<int> PERMISSION_TYPES_TEMPORARY;
    static const std::set<int> PERMISSION_TYPES_PERSIST;
    static const std::vector<std::string> PERMISSION_TYPES_READ_STR;
    static const std::vector<std::string> PERMISSION_TYPES_WRITE_STR;

    // index
    static const std::string URI_URITYPE_APPID_INDEX;
    static const std::string URI_PERMISSION_FILE_TOKEN_INDEX;

    // table name
    static const std::string APP_URI_PERMISSION_TABLE;

    // default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS;

    // create sql
    static const std::string CREATE_APP_URI_PERMISSION_TABLE;
    static const std::string CREATE_URI_URITYPE_APPID_INDEX;
    static const std::string CREATE_URI_URITYPE_TOKENID_INDEX;
    static const std::string CREATE_URI_PERMISSION_FILE_TOKEN_INDEX;

    // all columns
    static const std::set<std::string> ALL_COLUMNS;

    // delete sql
    static const std::string DROP_URI_URITYPE_APPID_INDEX;
};

enum class PhotoPermissionType : int32_t {
    TEMPORARY_READ_IMAGEVIDEO = 0,
    PERSIST_READ_IMAGEVIDEO,
    TEMPORARY_WRITE_IMAGEVIDEO,
    TEMPORARY_READWRITE_IMAGEVIDEO,
    PERSIST_READWRITE_IMAGEVIDEO, // Internal reserved value, not open to the public
    PERSIST_WRITE_IMAGEVIDEO,
    GRANT_PERSIST_READWRITE_IMAGEVIDEO,
};

} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_APP_URI_PERMISSION_H_