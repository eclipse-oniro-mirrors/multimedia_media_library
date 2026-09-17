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

#ifndef OHOS_MEDIA_PHOTO_ALBUM_TABLE_CONFIG_H
#define OHOS_MEDIA_PHOTO_ALBUM_TABLE_CONFIG_H

#include <string>

#include "field_config/clone_field_meta.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {

inline CloneTableMeta BuildPhotoAlbumTableMeta()
{
    using FP = FieldPolicy;
    using FT = CloneFieldType;
    const std::string nameNotNull = PhotoAlbumColumns::ALBUM_NAME + " IS NOT NULL";
    const std::string typeNotSystem = PhotoAlbumColumns::ALBUM_TYPE + " != " + std::to_string(PhotoAlbumType::SYSTEM);

    CloneTableMeta t{ PhotoAlbumColumns::TABLE, PhotoMap::TABLE };
    t.fields = {
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_ID, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_TYPE, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, typeNotSystem, typeNotSystem },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_SUBTYPE, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_NAME, FT::STRING, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, nameNotNull, nameNotNull },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_BUNDLE_NAME, FT::STRING, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, true, "", "" },

        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_COVER_URI, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_COUNT, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::CONTAINS_HIDDEN, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::HIDDEN_COUNT, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::HIDDEN_COVER, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_IMAGE_COUNT, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_VIDEO_COUNT, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_DIRTY, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_CLOUD_ID, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::ALBUM_ORDER, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoColumn::PHOTO_METADATA_FLAGS, FT::INT32, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
        CloneFieldMeta{ PhotoAlbumColumns::UNIQUE_ID, FT::STRING, {}, FP::SKIP, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, false, false, "", "" },
    };
    return t;
}

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_PHOTO_ALBUM_TABLE_CONFIG_H
