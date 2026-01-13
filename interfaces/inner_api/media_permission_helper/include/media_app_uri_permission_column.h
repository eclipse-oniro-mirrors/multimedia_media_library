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
    static const std::string ID EXPORT;
    static const std::string APP_ID EXPORT;
    static const std::string FILE_ID EXPORT;
    static const std::string URI_TYPE EXPORT;
    static const std::string PERMISSION_TYPE EXPORT;
    static const std::string DATE_MODIFIED EXPORT;
    static const std::string SOURCE_TOKENID EXPORT;
    static const std::string TARGET_TOKENID EXPORT;

    // uriTypes
    static const int URI_PHOTO EXPORT;
    static const int URI_AUDIO EXPORT;
    static const std::set<int> URI_TYPES_ALL EXPORT;

    // permissionTypes
    static const int PERMISSION_TEMPORARY_READ EXPORT;
    static const int PERMISSION_PERSIST_READ EXPORT;
    static const int PERMISSION_TEMPORARY_WRITE EXPORT;
    static const int PERMISSION_TEMPORARY_READ_WRITE EXPORT;
    static const int PERMISSION_PERSIST_READ_WRITE EXPORT;
    static const int PERMISSION_PERSIST_WRITE EXPORT;

    static const std::set<int> PERMISSION_TYPE_WRITE EXPORT;
    static const std::set<int> PERMISSION_TYPE_READ EXPORT;
    static const std::set<int> PERMISSION_TYPES_ALL EXPORT;
    static const std::set<int> PERMISSION_TYPES_PICKER EXPORT;
    static const std::set<int> PERMISSION_TYPES_TEMPORARY EXPORT;
    static const std::set<int> PERMISSION_TYPES_PERSIST EXPORT;
    static const std::vector<std::string> PERMISSION_TYPES_READ_STR EXPORT;
    static const std::vector<std::string> PERMISSION_TYPES_WRITE_STR EXPORT;

    // index
    static const std::string URI_URITYPE_APPID_INDEX EXPORT;

    // table name
    static const std::string APP_URI_PERMISSION_TABLE EXPORT;

    // default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS EXPORT;

    // create sql
    static const std::string CREATE_APP_URI_PERMISSION_TABLE EXPORT;
    static const std::string CREATE_URI_URITYPE_APPID_INDEX EXPORT;
    static const std::string CREATE_URI_URITYPE_TOKENID_INDEX EXPORT;

    // all columns
    static const std::set<std::string> ALL_COLUMNS EXPORT;

    // delete sql
    static const std::string DROP_URI_URITYPE_APPID_INDEX EXPORT;
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