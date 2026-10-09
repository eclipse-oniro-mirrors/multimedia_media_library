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

#ifndef OHOS_MEDIA_ANALYSIS_PHOTO_MAP_TABLE_CONFIG_H
#define OHOS_MEDIA_ANALYSIS_PHOTO_MAP_TABLE_CONFIG_H

#include <string>

#include "field_config/clone_field_meta.h"
#include "field_config/tables/analysis_table_names.h"
#include "photo_map_column.h"

namespace OHOS {
namespace Media {

inline CloneTableMeta BuildAnalysisPhotoMapTableMeta()
{
    using FP = FieldPolicy;
    using FT = CloneFieldType;
    CloneTableMeta t{ ANALYSIS_PHOTO_MAP_TABLE_NAME, "" };
    t.fields = {
        CloneFieldMeta{ PhotoMap::ALBUM_ID, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
        CloneFieldMeta{ PhotoMap::ASSET_ID, FT::INT32, {}, FP::INHERIT_SOURCE, FP::INHERIT_SOURCE,
            FP::KEEP_TARGET, FP::KEEP_TARGET, true, false, "", "" },
    };
    return t;
}

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_ANALYSIS_PHOTO_MAP_TABLE_CONFIG_H
