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

#ifndef FRAMEWORKS_SERVICES_MEDIA_OLD_PHOTOS_COLUMNS_H
#define FRAMEWORKS_SERVICES_MEDIA_OLD_PHOTOS_COLUMNS_H

#include <string>
#include <set>

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))

class TabOldPhotosColumn {
public:
    // table name
    static const std::string OLD_PHOTOS_TABLE;

    // Table columns: GalleryID and GalleryPath
    static const std::string MEDIA_ID;
    static const std::string MEDIA_FILE_PATH;
    static const std::string MEDIA_OLD_ID;
    static const std::string MEDIA_OLD_FILE_PATH;
    static const std::string MEDIA_CLONE_SEQUENCE;

    // columns only in tab_old_photos
    static const std::set<std::string> DEFAULT_TAB_OLD_PHOTOS_COLUMNS;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_FILE_ASSET_COLUMNS_H