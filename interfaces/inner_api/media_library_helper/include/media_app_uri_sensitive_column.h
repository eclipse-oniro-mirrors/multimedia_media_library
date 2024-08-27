/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance whit the License.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_APP_URI_SENSITIVE_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_APP_URI_SENSITIVE_H_

#include <string>
#include <set>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class AppUriSensitiveColumn {
public:
    // columns
    static const std::string ID EXPORT;
    static const std::string APP_ID EXPORT;
    static const std::string FILE_ID EXPORT;
    static const std::string URI_TYPE EXPORT;
    static const std::string HIDE_SENSITIVE_TYPE EXPORT;
    static const std::string DATE_MODIFIED EXPORT;

    // uriTypes
    static const int URI_PHOTO EXPORT;
    static const int URI_AUDIO EXPORT;
    static const std::set<int> URI_TYPES_ALL EXPORT;

    // permissionTypes
    static const int SENSITIVE_ALL_DESENSITIZE EXPORT;
    static const int SENSITIVE_GEOGRAPHIC_LOCATION_DESENSITIZE EXPORT;
    static const int SENSITIVE_SHOOTING_PARAM_DESENSITIZE EXPORT;
    static const int SENSITIVE_NO_DESENSITIZE EXPORT;
    static const std::set<int> SENSITIVE_TYPES_ALL EXPORT;
    // static const std::set<int> PERMISSION_TYPES_PICKER EXPORT;

    // index
    static const std::string URI_URITYPE_APPID_INDEX EXPORT;

    // table name
    static const std::string APP_URI_SENSITIVE_TABLE EXPORT;

    // default fetch columns
    static const std::set<std::string> DEFAULT_FETCH_COLUMNS EXPORT;

    // create table sql
    static const std::string CREATE_APP_URI_SENSITIVE_TABLE EXPORT;
    static const std::string CREATE_URI_URITYPE_APPID_INDEX EXPORT;

    // drop sensitive column sql
    static const std::string DROP_APP_URI_SENSITIVE_TABLE EXPORT;

    // all columns
    static const std::set<std::string> ALL_COLUMNS EXPORT;
};

} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_APP_URI_PERMISSION_H_