/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_SMART_ALBUM_COLUMN_H
#define FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_SMART_ALBUM_COLUMN_H

#include <string>

namespace OHOS {
namespace Media {
const std::string SMARTALBUM_TABLE_NAME = "smartalbum";
const std::string SMARTALBUM_DB_ID = "album_id";
const std::string SMARTALBUM_DB_ALBUM_TYPE = "album_type";
const std::string SMARTALBUM_DB_NAME = "album_name";
const std::string SMARTALBUM_DB_DESCRIPTION = "description";
const std::string SMARTALBUM_DB_CAPACITY = "capacity";
const std::string SMARTALBUM_DB_LATITUDE = "latitude";
const std::string SMARTALBUM_DB_LONGITUDE = "longitude";
const std::string SMARTALBUM_DB_COVER_URI = "cover_uri";
const std::string SMARTALBUM_DB_EXPIRED_TIME = "expired_id";
const std::string SMARTALBUM_DB_SELF_ID = "self_id";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_SMART_ALBUM_COLUMN_H