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

#include "media_app_uri_permission_column.h"
#include "media_column.h"
#include "base_column.h"

namespace OHOS {
namespace Media {

const std::string AppUriPermissionColumn::ID = "id";
const std::string AppUriPermissionColumn::APP_ID = "appid";
const std::string AppUriPermissionColumn::FILE_ID = "file_id";
const std::string AppUriPermissionColumn::URI_TYPE = "uri_type";
const std::string AppUriPermissionColumn::PERMISSION_TYPE = "permission_type";
const std::string AppUriPermissionColumn::DATE_MODIFIED = "date_modified";

const int AppUriPermissionColumn::URI_PHOTO = 1;
const int AppUriPermissionColumn::URI_AUDIO = 2;
const std::set<int> AppUriPermissionColumn::URI_TYPES_ALL = {
    AppUriPermissionColumn::URI_PHOTO, AppUriPermissionColumn::URI_AUDIO};

const int AppUriPermissionColumn::PERMISSION_TEMPORARY_READ = 0;
const int AppUriPermissionColumn::PERMISSION_PERSIST_READ = 1;
const int AppUriPermissionColumn::PERMISSION_TEMPORARY_WHRITE = 2;
const int AppUriPermissionColumn::PERMISSION_PERSIST_WHRITE = 3;
const int AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE = 4;
const std::set<int> AppUriPermissionColumn::PERMISSION_TYPES_ALL = {
    AppUriPermissionColumn::PERMISSION_TEMPORARY_READ, AppUriPermissionColumn::PERMISSION_PERSIST_READ,
    AppUriPermissionColumn::PERMISSION_TEMPORARY_WHRITE, AppUriPermissionColumn::PERMISSION_PERSIST_WHRITE,
    AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE};
const std::set<int> AppUriPermissionColumn::PERMISSION_TYPES_PICKER = {
    AppUriPermissionColumn::PERMISSION_TEMPORARY_READ, AppUriPermissionColumn::PERMISSION_PERSIST_READ};


const std::string AppUriPermissionColumn::URI_URITYPE_APPID_INDEX = "uri_uritype_appid_index";

const std::string AppUriPermissionColumn::APP_URI_PERMISSION_TABLE = "UriPermission";

const std::set<std::string> AppUriPermissionColumn::DEFAULT_FETCH_COLUMNS = {AppUriPermissionColumn::ID};

const std::string AppUriPermissionColumn::CREATE_APP_URI_PERMISSION_TABLE =
    "CREATE TABLE IF NOT EXISTS " + AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "(" +
    AppUriPermissionColumn::ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " +
    AppUriPermissionColumn::APP_ID + " TEXT, " + AppUriPermissionColumn::FILE_ID + " INTEGER, " +
    AppUriPermissionColumn::URI_TYPE + " INTEGER, " + AppUriPermissionColumn::PERMISSION_TYPE + " INTEGER, " +
    AppUriPermissionColumn::DATE_MODIFIED + " BIGINT)";

const std::string AppUriPermissionColumn::CREATE_URI_URITYPE_APPID_INDEX = BaseColumn::CreateIndex() +
    AppUriPermissionColumn::URI_URITYPE_APPID_INDEX + " ON " +
    AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + " (" +
    AppUriPermissionColumn::FILE_ID + " DESC," +
    AppUriPermissionColumn::URI_TYPE + "," +
    AppUriPermissionColumn::APP_ID + " DESC)";

const std::set<std::string> AppUriPermissionColumn::ALL_COLUMNS = {
    AppUriPermissionColumn::ID, AppUriPermissionColumn::APP_ID, AppUriPermissionColumn::FILE_ID,
    AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::PERMISSION_TYPE,
    AppUriPermissionColumn::DATE_MODIFIED
};
} // namespace Media
} // namespace OHOS