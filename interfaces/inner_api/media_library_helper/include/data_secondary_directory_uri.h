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
#define CONST_MEDIALIBRARY_DIRECTORY_URI "datashare:///media/MediaTypeDirectory"
#define CONST_MEDIALIBRARY_BUNDLEPERM_URI "datashare:///media/bundle_permission_insert_operation"

#define CONST_MEDIALIBRARY_CHECK_URIPERM_URI "datashare:///media/phaccess_checkuripermission"
#define CONST_MEDIALIBRARY_GRANT_URIPERM_URI "datashare:///media/phaccess_granturipermission"
#define CONST_PAH_CREATE_APP_URI_PERMISSION "datashare:///media/app_uri_permission_operation/create"

#define CONST_MEDIALIBRARY_AUDIO_URI "datashare:///media/audio"
#define CONST_MEDIALIBRARY_VIDEO_URI "datashare:///media/video"
#define CONST_MEDIALIBRARY_IMAGE_URI "datashare:///media/image"
#define CONST_MEDIALIBRARY_FILE_URI "datashare:///media/file"
#define CONST_MEDIALIBRARY_ALBUM_URI "datashare:///media/album"
#define CONST_MEDIALIBRARY_SMARTALBUM_CHANGE_URI "datashare:///media/smartalbum"
#define CONST_MEDIALIBRARY_DEVICE_URI "datashare:///media/device"
#define CONST_MEDIALIBRARY_SMART_URI "datashare:///media/smart"
#define CONST_MEDIALIBRARY_REMOTEFILE_URI "datashare:///media/remotfile"
} // namespace Media
} // namespace OHOS

#endif // DATA_SECONDARY_DIRECTORY_URI_H