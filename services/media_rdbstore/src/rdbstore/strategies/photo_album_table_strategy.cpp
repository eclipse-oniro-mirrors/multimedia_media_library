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

#define MLOG_TAG "PhotoAlbumTableStrategy"

#include "photo_album_table_strategy.h"

#include "media_column.h"
#include "media_log.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "result_set_utils.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
const std::string INVALID_STR = "Invalid";

int32_t PhotoAlbumTableStrategy::AddCoverOrderValuesFromRecord(ValuesBucket& values, RdbStore &store)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryRdbHelper::AddCoverOrderValuesFromRecord");

    int32_t albumType = -1;
    int32_t albumSubtype = 0;
    string lpath = INVALID_STR;
    ValueObject value;
    if (values.GetObject(PhotoAlbumColumns::ALBUM_TYPE, value)) {
        value.GetInt(albumType);
    }
    if (values.GetObject(PhotoAlbumColumns::ALBUM_SUBTYPE, value)) {
        value.GetInt(albumSubtype);
    }
    if (values.GetObject(PhotoAlbumColumns::ALBUM_LPATH, value)) {
        value.GetString(lpath);
    }
    string sql = "SELECT cover_order_key, cover_order_subkey, cover_order_type, hidden_cover_order_key, "
        "hidden_cover_order_subkey, hidden_cover_order_type FROM tab_cover_record ";
    sql += albumType == PhotoAlbumType::SYSTEM ? "WHERE album_type = ? AND album_subtype = ?" :
        "WHERE album_type = ? AND album_subtype = ? AND lpath = ?";
    vector<ValueObject> args;
    args = albumType == PhotoAlbumType::SYSTEM ? vector<ValueObject>{ albumType, albumSubtype } :
        vector<ValueObject>{ albumType, albumSubtype, lpath };
    auto resultSet = store.QuerySql(sql, args);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr.");
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        string coverOrderKey = GetStringVal("cover_order_key", resultSet);
        string coverOrderSubKey = GetStringVal("cover_order_subkey", resultSet);
        int32_t coverOrderType = GetInt32Val("cover_order_type", resultSet);
        string hiddenCoverOrderKey = GetStringVal("hidden_cover_order_key", resultSet);
        string hiddenCoverOrderSubKey = GetStringVal("hidden_cover_order_subkey", resultSet);
        int32_t hiddenCoverOrderType = GetInt32Val("hidden_cover_order_type", resultSet);
        CHECK_AND_EXECUTE(coverOrderKey.empty(), values.PutString("cover_order_key", coverOrderKey));
        CHECK_AND_EXECUTE(coverOrderSubKey.empty(), values.PutString("cover_order_subkey", coverOrderSubKey));
        CHECK_AND_EXECUTE(coverOrderType != 0 && coverOrderType != 1,
            values.PutInt("cover_order_type", coverOrderType));
        CHECK_AND_EXECUTE(hiddenCoverOrderKey.empty(), values.PutString("hidden_cover_order_key", hiddenCoverOrderKey));
        CHECK_AND_EXECUTE(hiddenCoverOrderSubKey.empty(),
            values.PutString("hidden_cover_order_subkey", hiddenCoverOrderSubKey));
        CHECK_AND_EXECUTE(hiddenCoverOrderType != 0 && hiddenCoverOrderType != 1,
            values.PutInt("hidden_cover_order_type", hiddenCoverOrderType));
    }
    resultSet->Close();
    return E_OK;
}

int32_t PhotoAlbumTableStrategy::ExtendInsertValues(NativeRdb::ValuesBucket& values, RdbStore &store,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        CHECK_AND_RETURN_RET_LOG(AddCoverOrderValuesFromRecord(values, store) == E_OK, E_HAS_DB_ERROR,
            "AddCoverOrderValuesFromRecord failed.");
    }
    return E_OK;
}

int32_t PhotoAlbumTableStrategy::ExtendBatchInsertValues(std::vector<NativeRdb::ValuesBucket>& values, RdbStore &store,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        for (auto& value : values) {
            CHECK_AND_RETURN_RET_LOG(AddCoverOrderValuesFromRecord(value, store) == E_OK, E_HAS_DB_ERROR,
                "AddCoverOrderValuesFromRecord failed.");
        }
    }
    return E_OK;
}

TableStrategyErrno PhotoAlbumTableStrategy::ExtendDeleteValues(NativeRdb::ValuesBucket& values,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        values.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
    }
    return TableStrategyErrno::STRATEGY_OK;
}

std::string PhotoAlbumTableStrategy::GetQueryFilter(const TableStrategyConfig &config) const
{
    return PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_DIRTY + " != " +
        to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
}
} // namespace OHOS::Media