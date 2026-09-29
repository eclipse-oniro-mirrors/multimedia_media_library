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

#include "field_config/clone_field_registry.h"

#include "field_config/tables/analysis_album_table_config.h"
#include "field_config/tables/analysis_photo_map_table_config.h"
#include "field_config/tables/audios_table_config.h"
#include "field_config/tables/photo_album_table_config.h"
#include "field_config/tables/photo_map_table_config.h"
#include "field_config/tables/photos_table_config.h"

namespace OHOS {
namespace Media {

CloneFieldRegistry &CloneFieldRegistry::Instance()
{
    static CloneFieldRegistry instance;
    return instance;
}

void CloneFieldRegistry::RegisterTable(const CloneTableMeta &meta)
{
    if (tables_.find(meta.table) == tables_.end()) {
        registeredOrder_.push_back(meta.table);
    }
    tables_[meta.table] = meta;
}

void CloneFieldRegistry::Init()
{
    if (inited_) {
        return;
    }
    inited_ = true;
    RegisterTable(BuildPhotosTableMeta());
    RegisterTable(BuildPhotoAlbumTableMeta());
    RegisterTable(BuildPhotoMapTableMeta());
    RegisterTable(BuildAnalysisAlbumTableMeta());
    RegisterTable(BuildAnalysisPhotoMapTableMeta());
    RegisterTable(BuildAudiosTableMeta());
}

const CloneTableMeta *CloneFieldRegistry::GetTable(const std::string &table) const
{
    auto it = tables_.find(table);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

const CloneFieldMeta *CloneFieldRegistry::GetField(const std::string &table, const std::string &col) const
{
    const CloneTableMeta *t = GetTable(table);
    if (t == nullptr) {
        return nullptr;
    }
    for (const auto &f : t->fields) {
        if (f.column == col) {
            return &f;
        }
    }
    return nullptr;
}

const std::vector<std::string> &CloneFieldRegistry::GetRegisteredTables() const
{
    return registeredOrder_;
}

} // namespace Media
} // namespace OHOS
