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
 
#define MLOG_TAG "TabFaCardPhotosEventHandler"
 
#include "tab_facard_photos_table_event_handler.h"
 
#include <string>
 
#include "rdb_store.h"
#include "rdb_errno.h"
#include "media_facard_photos_column.h"
#include "media_log.h"
 
namespace OHOS::Media {
/**
 * @brief execute sql while database is created
 * @param store: rdb store
 */
int32_t TabFaCardPhotosTableEventHandler::OnCreate(NativeRdb::RdbStore &store)
{
    MEDIA_INFO_LOG("OnCreate begin create %{public}s table", TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE.c_str());
    int32_t ret = this->CreateTable(store);
    if (ret != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }
    this->CreateIndex(store);
    MEDIA_INFO_LOG("OnCreate end create %{public}s table", TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE.c_str());
    return NativeRdb::E_OK;
}
 
/**
 * @brief execute sql while database is upgraded
 * @param store: rdb store
 */
int32_t TabFaCardPhotosTableEventHandler::OnUpgrade(NativeRdb::RdbStore &store, int oldVersion, int newVersion)
{
    return 0;
}
 
int32_t TabFaCardPhotosTableEventHandler::CreateTable(NativeRdb::RdbStore &store)
{
    int32_t ret = store.ExecuteSql(std::string(this->CREATE_TABLE_SQL));
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Media_Library: CreateTable failed, ret=%{public}d, sql=%{public}s",
            ret,
            std::string(this->CREATE_TABLE_SQL).c_str());
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}
 
int32_t TabFaCardPhotosTableEventHandler::CreateIndex(NativeRdb::RdbStore &store)
{
    for (const std::string_view &sql : this->CREATE_INDEX_SQLS) {
        int32_t ret = store.ExecuteSql(std::string(sql));
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG(
                "Media_Library: CreateIndex failed, ret=%{public}d, sql=%{public}s", ret, std::string(sql).c_str());
            return NativeRdb::E_ERROR;
        }
    }
    return NativeRdb::E_OK;
}
} // namespace OHOS::Media