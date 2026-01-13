/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef DATA_SECONDARY_DIRECTORY_URI_H
#define DATA_SECONDARY_DIRECTORY_URI_H
#include "base_data_uri.h"

namespace OHOS {
namespace Media {
const std::string MEDIALIBRARY_DIRECTORY_URI = MEDIALIBRARY_DATA_URI + "/MediaTypeDirectory";
const std::string MEDIALIBRARY_BUNDLEPERM_URI = MEDIALIBRARY_DATA_URI + "/bundle_permission_insert_operation";

const std::string MEDIALIBRARY_CHECK_URIPERM_URI = MEDIALIBRARY_DATA_URI + "/phaccess_checkuripermission";
const std::string MEDIALIBRARY_GRANT_URIPERM_URI = MEDIALIBRARY_DATA_URI + "/phaccess_granturipermission";
const std::string PAH_CREATE_APP_URI_PERMISSION =
    MEDIALIBRARY_DATA_URI + "/app_uri_permission_operation/" + OPRN_CREATE;

const std::string MEDIALIBRARY_AUDIO_URI = MEDIALIBRARY_DATA_URI + '/' + "audio";
const std::string MEDIALIBRARY_VIDEO_URI = MEDIALIBRARY_DATA_URI + '/' + "video";
const std::string MEDIALIBRARY_IMAGE_URI = MEDIALIBRARY_DATA_URI + '/' + "image";
const std::string MEDIALIBRARY_FILE_URI  =  MEDIALIBRARY_DATA_URI + '/' + "file";
const std::string MEDIALIBRARY_ALBUM_URI  =  MEDIALIBRARY_DATA_URI + '/' + "album";
const std::string MEDIALIBRARY_SMARTALBUM_CHANGE_URI  =  MEDIALIBRARY_DATA_URI + '/' + "smartalbum";
const std::string MEDIALIBRARY_DEVICE_URI  =  MEDIALIBRARY_DATA_URI + '/' + "device";
const std::string MEDIALIBRARY_SMART_URI = MEDIALIBRARY_DATA_URI + '/' + "smart";
const std::string MEDIALIBRARY_REMOTEFILE_URI = MEDIALIBRARY_DATA_URI + '/' + "remotfile";
} // namespace Media
} // namespace OHOS

#endif // DATA_SECONDARY_DIRECTORY_URI_H