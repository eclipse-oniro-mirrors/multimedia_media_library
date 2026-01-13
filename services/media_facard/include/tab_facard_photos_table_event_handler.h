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
 
#ifndef OHOS_MEDIA_TAB_FACARD_PHOTOS_EVENT_HANDLER_H
#define OHOS_MEDIA_TAB_FACARD_PHOTOS_EVENT_HANDLER_H
 
#include <string>
#include <vector>
 
#include "rdb_store.h"
#include "rdb_open_event.h"
 
namespace OHOS::Media {
class TabFaCardPhotosTableEventHandler : public IRdbOpenEvent {
public:
    int32_t OnCreate(NativeRdb::RdbStore &store) override;
    int32_t OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion) override;
 
private:
    int32_t CreateTable(NativeRdb::RdbStore &store);
    int32_t CreateIndex(NativeRdb::RdbStore &store);
 
private:
    const std::string_view CREATE_TABLE_SQL = "CREATE TABLE "
                                              "IF"
                                              "  NOT EXISTS tab_facard_photos ( form_id TEXT, asset_uri TEXT );";

    const std::string_view INDEX_FORM_ID =
        "CREATE INDEX "
        "IF"
        "  NOT EXISTS idx_form_id_on_tab_facard_photos ON tab_facard_photos ( form_id );";

    const std::string_view INDEX_ASSET_URI =
        "CREATE INDEX "
        "IF"
        "  NOT EXISTS idx_asset_uri_on_tab_facard_photos ON tab_facard_photos ( asset_uri );";

    const std::vector<std::string_view> CREATE_INDEX_SQLS = {INDEX_FORM_ID, INDEX_ASSET_URI};
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_TAB_FACARD_PHOTOS_EVENT_HANDLER_H