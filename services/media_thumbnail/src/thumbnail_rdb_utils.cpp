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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_rdb_utils.h"

#include "asset_accurate_refresh.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "media_log.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

const std::string SQL_REPAIR_EXIF_ROTATE_WITH_REGENERATE_THUMBNAIL = "\
    UPDATE Photos \
    SET \
        exif_rotate = ?, \
        thumbnail_ready = 2, \
        lcd_visit_time = 0, \
        meta_date_modified = strftime('%s000', 'now'), \
        dirty = CASE \
            WHEN dirty IN (0, 2, 6, 8) THEN ? \
            ELSE dirty \
        END \
    WHERE file_id = ? \
    ;";

const std::string SQL_REPAIR_EXIF_ROTATE_WITHOUT_REGENERATE_THUMBNAIL = "\
    UPDATE Photos \
    SET \
        exif_rotate = ?, \
        meta_date_modified = strftime('%s000', 'now'), \
        dirty = CASE \
            WHEN dirty IN (0, 2, 6, 8) THEN ? \
            ELSE dirty \
        END \
    WHERE file_id = ? \
    ;";

using HandleFunc = void(*)(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data);
const std::unordered_map<std::string, HandleFunc> ThumbnailRdbUtils::RESULT_SET_HANDLER = {
    {MEDIA_DATA_DB_ID, HandleId},
    {MEDIA_DATA_DB_FILE_PATH, HandleFilePath},
    {MEDIA_DATA_DB_DATE_ADDED, HandleDateAdded},
    {MEDIA_DATA_DB_NAME, HandleDisplayName},
    {MEDIA_DATA_DB_MEDIA_TYPE, HandleMediaType},
    {MEDIA_DATA_DB_DATE_TAKEN, HandleDateTaken},
    {MEDIA_DATA_DB_DATE_MODIFIED, HandleDateModified},
    {MEDIA_DATA_DB_ORIENTATION, HandleOrientation},
    {PhotoColumn::PHOTO_EXIF_ROTATE, HandleExifRotate},
    {MEDIA_DATA_DB_POSITION, HandlePosition},
    {MEDIA_DATA_DB_HEIGHT, HandlePhotoHeight},
    {MEDIA_DATA_DB_WIDTH, HandlePhotoWidth},
    {MEDIA_DATA_DB_DIRTY, HandleDirty},
    {MEDIA_DATA_DB_THUMBNAIL_READY, HandleReady},
    {PhotoColumn::PHOTO_LCD_VISIT_TIME, HandleLcdVisitTime},
    {PhotoColumn::PHOTO_THUMB_STATUS, HandleThumbStatus},
};

void ThumbnailRdbUtils::HandleId(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx, ThumbnailData &data)
{
    ParseStringResult(resultSet, idx, data.id);
}

void ThumbnailRdbUtils::HandleFilePath(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseStringResult(resultSet, idx, data.path);
}

void ThumbnailRdbUtils::HandleDateAdded(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx,
    ThumbnailData &data)
{
    ParseStringResult(resultSet, idx, data.dateAdded);
}

void ThumbnailRdbUtils::HandleDisplayName(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseStringResult(resultSet, idx, data.displayName);
}

void ThumbnailRdbUtils::HandleDateTaken(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseStringResult(resultSet, idx, data.dateTaken);
}

void ThumbnailRdbUtils::HandleDateModified(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseStringResult(resultSet, idx, data.dateModified);
}

void ThumbnailRdbUtils::HandleMediaType(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    data.mediaType = MediaType::MEDIA_TYPE_ALL;
    ParseInt32Result(resultSet, idx, data.mediaType);
}

void ThumbnailRdbUtils::HandleOrientation(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt32Result(resultSet, idx, data.orientation);
}

void ThumbnailRdbUtils::HandleExifRotate(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt32Result(resultSet, idx, data.exifRotate);
}

void ThumbnailRdbUtils::HandlePosition(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    int position = 0;
    ParseInt32Result(resultSet, idx, position);
    data.isLocalFile = (position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
        position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    data.position = position;
}

void ThumbnailRdbUtils::HandlePhotoHeight(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt32Result(resultSet, idx, data.photoHeight);
}

void ThumbnailRdbUtils::HandlePhotoWidth(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt32Result(resultSet, idx, data.photoWidth);
}

void ThumbnailRdbUtils::HandleDirty(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt32Result(resultSet, idx, data.dirty);
}

void ThumbnailRdbUtils::HandleReady(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt64Result(resultSet, idx, data.thumbnailReady);
}

void ThumbnailRdbUtils::HandleLcdVisitTime(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt64Result(resultSet, idx, data.lcdVisitTime);
}

void ThumbnailRdbUtils::HandleThumbStatus(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int idx, ThumbnailData &data)
{
    ParseInt32Result(resultSet, idx, data.thumbnailStatus);
}

bool ThumbnailRdbUtils::QueryThumbnailDataInfos(shared_ptr<MediaLibraryRdbStore> store,
    NativeRdb::RdbPredicates &rdbPredicates, const vector<string> &column, vector<ThumbnailData> &datas)
{
    int err;
    return QueryThumbnailDataInfos(store, rdbPredicates, column, datas, err);
}

bool ThumbnailRdbUtils::QueryThumbnailDataInfos(shared_ptr<MediaLibraryRdbStore> store,
    NativeRdb::RdbPredicates &rdbPredicates, const vector<string> &column, vector<ThumbnailData> &datas, int &err)
{
    CHECK_AND_RETURN_RET_LOG(store != nullptr, false, "RdbStore is nullptr");
    shared_ptr<ResultSet> resultSet = store->QueryByStep(rdbPredicates, column);
    return QueryThumbnailDataInfos(resultSet, column, datas, err);
}

bool ThumbnailRdbUtils::QueryThumbnailDataInfos(const shared_ptr<ResultSet> &resultSet,
    const vector<string> &column, vector<ThumbnailData> &datas, int &err)
{
    if (!CheckResultSetCount(resultSet, err)) {
        MEDIA_ERR_LOG("CheckResultSetCount failed %{public}d", err);
        if (err == E_EMPTY_VALUES_BUCKET) {
            return true;
        }
        return false;
    }

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed GoToFirstRow %{public}d", err);

    ThumbnailData data;
    do {
        ParseQueryResult(resultSet, data, err, column);
        if (!data.path.empty()) {
            datas.push_back(data);
        }
    } while (resultSet->GoToNextRow() == E_OK);
    return true;
}

bool ThumbnailRdbUtils::QueryThumbnailDataInfo(shared_ptr<MediaLibraryRdbStore> store,
    NativeRdb::RdbPredicates &rdbPredicates, const vector<string> &column, ThumbnailData &data)
{
    int err;
    return QueryThumbnailDataInfo(store, rdbPredicates, column, data, err);
}

bool ThumbnailRdbUtils::QueryThumbnailDataInfo(shared_ptr<MediaLibraryRdbStore> store,
    NativeRdb::RdbPredicates &rdbPredicates, const vector<string> &column, ThumbnailData &data, int &err)
{
    CHECK_AND_RETURN_RET_LOG(store != nullptr, false, "RdbStore is nullptr");
    auto resultSet = store->QueryByStep(rdbPredicates, column);
    return QueryThumbnailDataInfo(resultSet, column, data, err);
}

bool ThumbnailRdbUtils::QueryThumbnailDataInfo(const shared_ptr<ResultSet> &resultSet,
    const vector<string> &column, ThumbnailData &data, int &err)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "ResultSet is nullptr");

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false, "Fail to GoToFirstRow");

    ParseQueryResult(resultSet, data, err, column);
    return true;
}

bool ThumbnailRdbUtils::CheckResultSetCount(const shared_ptr<ResultSet> &resultSet, int &err)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("ResultSet is nullptr!");
        return false;
    }
    int rowCount = 0;
    err = resultSet->GetRowCount(rowCount);
    if (err != E_OK || rowCount < 0) {
        MEDIA_ERR_LOG("Failed to get row count %{public}d", err);
        return false;
    } else if (rowCount == 0) {
        MEDIA_INFO_LOG("CheckCount No match!");
        err = E_EMPTY_VALUES_BUCKET;
        return false;
    }
    return true;
}

void ThumbnailRdbUtils::ParseQueryResult(const shared_ptr<ResultSet> &resultSet, ThumbnailData &data,
    int &err, const std::vector<std::string> &column)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "ResultSet is nullptr!");
    int index;
    for (auto &columnValue : column) {
        err = resultSet->GetColumnIndex(columnValue, index);
        if (err != NativeRdb::E_OK) {
            continue;
        }
        auto iter = RESULT_SET_HANDLER.find(columnValue);
        if (iter != RESULT_SET_HANDLER.end()) {
            iter->second(resultSet, index, data);
        }
    }
}

void ThumbnailRdbUtils::ParseStringResult(const shared_ptr<ResultSet> &resultSet, int index, string &data)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "ResultSet is nullptr!");
    bool isNull = true;
    int err = resultSet->IsColumnNull(index, isNull);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to check column %{public}d null %{public}d", index, err);
    }

    if (!isNull) {
        err = resultSet->GetString(index, data);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Failed to get column %{public}d string %{public}d", index, err);
        }
    }
}

void ThumbnailRdbUtils::ParseInt32Result(const shared_ptr<ResultSet> &resultSet, int index, int32_t &data)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "ResultSet is nullptr!");
    bool isNull = true;
    int err = resultSet->IsColumnNull(index, isNull);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to check column %{public}d null %{public}d", index, err);
    }

    if (!isNull) {
        err = resultSet->GetInt(index, data);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Failed to get column %{public}d int32 %{public}d", index, err);
        }
    }
}

void ThumbnailRdbUtils::ParseInt64Result(const shared_ptr<ResultSet> &resultSet, int index, int64_t &data)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "ResultSet is nullptr!");
    bool isNull = true;
    int err = resultSet->IsColumnNull(index, isNull);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to check column %{public}d null %{public}d", index, err);
    }

    if (!isNull) {
        err = resultSet->GetLong(index, data);
        if (err != E_OK) {
            MEDIA_ERR_LOG("Failed to get column %{public}d int64 %{public}d", index, err);
        }
    }
}

bool ThumbnailRdbUtils::QueryLocalNoExifRotateInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_NAME,
    };
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);
    rdbPredicates.BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_EXIF_ROTATE, "0")
        ->And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0")
        ->And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, "0")
        ->EndWrap();

    rdbPredicates.BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, "1")->Or()->EqualTo(PhotoColumn::PHOTO_POSITION, "3")
        ->EndWrap();
    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos),
        false, "QueryThumbnailDataInfos failed");
    return true;
}

int32_t ThumbnailRdbUtils::UpdateRdbStoreById(ThumbRdbOpt &opts, const string &id,
    const NativeRdb::ValuesBucket &values)
{
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, E_RDB_STORE_NULL, "Store is nullptr!");
    CHECK_AND_RETURN_RET_LOG(!values.IsEmpty(), E_OK, "ValuesBucket is empty");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(opts.table);
    predicates.EqualTo(MediaColumn::MEDIA_ID, id);
    int32_t changedRows = -1;
    MediaLibraryTracer tracer;
    tracer.Start("UpdateRdbStoreById");
    int32_t ret = opts.store->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Store execute update err: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0, E_RDB_UPDATE_NO_ROWS_CHANGED,
        "Update changedRows: %{public}d", changedRows);
    return E_OK;
}

int32_t ThumbnailRdbUtils::UpdateExifRotateAndDirty(const ThumbnailData &data, DirtyType dirtyType,
    bool needRegenerateThumbnail)
{
    CHECK_AND_RETURN_RET_LOG(dirtyType == DirtyType::TYPE_FDIRTY || dirtyType == DirtyType::TYPE_MDIRTY,
        E_ERR, "Not support update this type dirty, type:%{public}d", dirtyType);

    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(assetRefresh != nullptr, E_ERR, "Create assetRefresh failed");

    std::vector<NativeRdb::ValueObject> bindArgs = {
        data.exifRotate, static_cast<int32_t>(dirtyType), data.id,
    };
    int32_t ret = 0;
    if (needRegenerateThumbnail) {
        ret = assetRefresh->ExecuteSql(SQL_REPAIR_EXIF_ROTATE_WITH_REGENERATE_THUMBNAIL,
            bindArgs, AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE);
    } else {
        ret = assetRefresh->ExecuteSql(SQL_REPAIR_EXIF_ROTATE_WITHOUT_REGENERATE_THUMBNAIL,
            bindArgs, AccurateRefresh::RdbOperation::RDB_OPERATION_UPDATE);
    }

    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK,
        E_ERR, "Failed to Update, ret: %{public}d", ret);
    assetRefresh->Notify();
    return E_OK;
}
} // namespace Media
} // namespace OHOS