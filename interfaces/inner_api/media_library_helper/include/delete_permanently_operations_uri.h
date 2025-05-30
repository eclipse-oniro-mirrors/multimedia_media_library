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

#ifndef INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_ASSET_OPERATIONS_URI_H_
#define INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_ASSET_OPERATIONS_URI_H_

#include <string>

#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
const std::string OPRN_DELETE_LOCAL_ASSETS_PERMANENTLY = "delete_local_photos_permanently";
const std::string URI_DELETE_PHOTOS_COMPLETED = MEDIALIBRARY_DATA_URI + "/"
        + PHOTO_ALBUM_OPRN + "/" + OPRN_DELETE_LOCAL_ASSETS_PERMANENTLY;
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_INNERAPI_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_ASSET_OPERATIONS_URI_H_
