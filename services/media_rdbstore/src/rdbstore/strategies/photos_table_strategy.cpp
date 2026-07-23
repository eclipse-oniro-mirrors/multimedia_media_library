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

#define MLOG_TAG "PhotosTableStrategy"

#include "photos_table_strategy.h"

#include "media_log.h"
#include "media_time_utils.h"
#include "media_values_bucket_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_type_const.h"
#include "photo_file_utils.h"
#include "scanner_utils.h"

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "parameters.h"
#include "permission_utils.h"

using namespace OHOS::Security::AccessToken;
#endif

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
static const std::string CONST_MEDIA_SECURE_ALBUM = "const.media.secure_album";

static void PutDefaultDateAddedYearMonthDay(ValuesBucket& values)
{
    std::string dateAddedStr = "0";
    MediaValuesBucketUtils::GetString(values, MediaColumn::MEDIA_DATE_ADDED, dateAddedStr);
    int64_t dateAdded {atoll(dateAddedStr.c_str())};
    if (dateAdded <= 0) {
        MEDIA_ERR_LOG("dateAdded is invalid, use current time");
        dateAdded = MediaTimeUtils::UTCTimeMilliSeconds();
    }

    const auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ConstructDateAddedDateParts(dateAdded);
    if (!values.HasColumn(PhotoColumn::PHOTO_DATE_ADDED_YEAR)) {
        values.Put(PhotoColumn::PHOTO_DATE_ADDED_YEAR, dateYear);
    }
    if (!values.HasColumn(PhotoColumn::PHOTO_DATE_ADDED_MONTH)) {
        values.Put(PhotoColumn::PHOTO_DATE_ADDED_MONTH, dateMonth);
    }
    if (!values.HasColumn(PhotoColumn::PHOTO_DATE_ADDED_DAY)) {
        values.Put(PhotoColumn::PHOTO_DATE_ADDED_DAY, dateDay);
    }
}

void PhotosTableStrategy::AddDefaultInsertPhotoValues(ValuesBucket& values)
{
    ValueObject tmpValue;
    std::string tmpStr {};
    if (values.GetObject(MediaColumn::MEDIA_NAME, tmpValue)) {
        tmpValue.GetString(tmpStr);
        values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(tmpStr));
    }
    PutDefaultDateAddedYearMonthDay(values);
}

int32_t PhotosTableStrategy::ExtendInsertValues(NativeRdb::ValuesBucket& values, RdbStore &store,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        AddDefaultInsertPhotoValues(values);
    }
    return E_OK;
}

int32_t PhotosTableStrategy::ExtendBatchInsertValues(std::vector<NativeRdb::ValuesBucket>& values, RdbStore &store,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        for (auto& value : values) {
            AddDefaultInsertPhotoValues(value);
        }
    }
    return E_OK;
}

TableStrategyErrno PhotosTableStrategy::ExtendDeleteValues(NativeRdb::ValuesBucket& values,
    const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        values.PutInt(CONST_MEDIA_DATA_DB_DIRTY, static_cast<int32_t>(DirtyType::TYPE_DELETED));
        values.PutInt(CONST_MEDIA_DATA_DB_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD));
        values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaTimeUtils::UTCTimeMilliSeconds());
    }
    return TableStrategyErrno::STRATEGY_OK;
}

int32_t PhotosTableStrategy::ExtendUpdateValues(NativeRdb::ValuesBucket& values, const TableStrategyConfig &config)
{
    if (config.enableDefault) {
        values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaTimeUtils::UTCTimeMilliSeconds());
        values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaTimeUtils::UTCTimeMilliSeconds());
    }
    return E_OK;
}

std::string PhotosTableStrategy::GetQueryFilter(const TableStrategyConfig &config) const
{
    std::string filter = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_SYNC_STATUS + " = " +
        std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)) + " AND " +
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_CLEAN_FLAG + " = " +
        std::to_string(static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE
    if (OHOS::system::GetParameter(CONST_MEDIA_SECURE_ALBUM, "") == "true" && !config.isAlbumRefresh) {
        // Check if the caller has MANAGE_RISK_PHOTOS permission
        AccessTokenID tokenCaller = IPCSkeleton::GetCallingTokenID();
        int res = AccessTokenKit::VerifyAccessToken(tokenCaller, MANAGE_RISK_PHOTOS);
        if (res != PermissionState::PERMISSION_GRANTED) {
            filter += " AND " + PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_IS_CRITICAL + " = 0";
            MEDIA_DEBUG_LOG("MANAGE_RISK_PHOTOS permission denied, filter: %{public}s", filter.c_str());
        } else {
            MEDIA_DEBUG_LOG("MANAGE_RISK_PHOTOS permission granted, filter: %{public}s", filter.c_str());
        }
    }
#endif

    return filter;
}
} // namespace OHOS::Media