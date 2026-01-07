/*
* Copyright (C) 2022-2025 Huawei Device Co., Ltd.
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

#include "thumbnail_generation_post_process.h"

#include "asset_accurate_refresh.h"
#include "dfx_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "thumbnail_utils.h"
#include "refresh_business_name.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

int32_t ThumbnailGenerationPostProcess::PostProcess(ThumbnailData& data, const ThumbRdbOpt& opts)
{
    ValueObject valueObject;
    bool hasLcdVisitTime = data.rdbUpdateCache.GetObject(PhotoColumn::PHOTO_LCD_VISIT_TIME, valueObject);
    CHECK_AND_RETURN_RET_INFO_LOG(hasLcdVisitTime, false, "Do not cache lcd_visit_time value");
    int32_t lcdVisitTime;
    valueObject.GetInt(lcdVisitTime);
    MEDIA_INFO_LOG("lcdVisitTime value is: %{public}d", lcdVisitTime);
    
    CHECK_AND_RETURN_RET(!data.rdbUpdateCache.IsEmpty(), E_OK);
    bool hasGeneratedThumb = HasGeneratedThumb(data);
    MEDIA_INFO_LOG("HasGeneratedThumb: %{public}d id: %{public}s, path: %{public}s",
        hasGeneratedThumb, data.id.c_str(), DfxUtils::GetSafePath(data.path).c_str());
    int32_t err = hasGeneratedThumb ? UpdateCachedRdbValueAndNotify(data, opts) : UpdateCachedRdbValue(data, opts);
    if (err == E_RDB_UPDATE_NO_ROWS_CHANGED || err == E_RDB_QUERY_NO_RES) {
        auto deleteRes = ThumbnailUtils::DeleteThumbnailDirAndAstc(opts, data);
        MEDIA_ERR_LOG("There is no such id: %{public}s path: %{public}s in the db,"
            " the corresponding thumb needs to be deleted. err: %{public}d, deleteRes: %{public}d",
            data.id.c_str(), DfxUtils::GetSafePath(data.path).c_str(), err, deleteRes);
        return err;
    }

    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Update failed. err: %{public}d id: %{public}s path: %{public}s",
        err, data.id.c_str(), DfxUtils::GetSafePath(data.path).c_str());
    data.rdbUpdateCache.Clear();
    return E_OK;
}

int32_t ThumbnailGenerationPostProcess::UpdateCachedRdbValue(ThumbnailData& data, const ThumbRdbOpt& opts)
{
    const string& photosTable = PhotoColumn::PHOTOS_TABLE;
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, E_ERR, "RdbStore is nullptr");
    CHECK_AND_RETURN_RET_LOG(opts.table == photosTable, false,
        "Not %{public}s table, table: %{public}s", photosTable.c_str(), opts.table.c_str());
    MediaLibraryTracer tracer;
    tracer.Start("UpdateCachedRdbValue opts.store->Update");
    int32_t changedRows;
    int32_t err = opts.store->Update(changedRows, photosTable,
        data.rdbUpdateCache, MEDIA_DATA_DB_ID + " = ?", { data.id });
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "UpdateCachedRdbValue failed. table: %{public}s, err: %{public}d",
        photosTable.c_str(), err);
    CHECK_AND_RETURN_RET_LOG(changedRows != 0, E_RDB_UPDATE_NO_ROWS_CHANGED,
        "No such id: %{public}s in db. changedRows: %{public}d", data.id.c_str(), changedRows);
    return E_OK;
}

int32_t ThumbnailGenerationPostProcess::UpdateCachedRdbValueAndNotify(ThumbnailData& data, const ThumbRdbOpt& opts)
{
    // 必须在更新数据库前获取通知类型
    NotifyType notifyType;
    int32_t err = GetNotifyType(data, opts, notifyType);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "GetNotifyType failed. err: %{public}d", err);

    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::THUMBNAIL_GENERATION_BUSSINESS_NAME);
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, data.id);
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailGenerationPostProcess assetRefresh.Update id: " + data.id);
    int32_t changedRows;
    err = assetRefresh.Update(changedRows, data.rdbUpdateCache, predicates);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "AssetRefresh.Update failed. err: %{public}d", err);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0, E_RDB_UPDATE_NO_ROWS_CHANGED,
        "AssetRefresh.Update falied changedRows: %{public}d", changedRows);
    err = assetRefresh.Notify();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "AssetRefresh.Notify failed. err: %{public}d", err);

    err = Notify(data, notifyType);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Notify failed. err: %{public}d", err);

    return E_OK;
}

int32_t ThumbnailGenerationPostProcess::Notify(const ThumbnailData& data, const NotifyType notifyType)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "SendThumbNotify watch is nullptr");
    watch->Notify(data.fileUri, notifyType);
    MEDIA_INFO_LOG("ThumbnailGenerationPostProcess::Notify() "
        "id: %{public}s, path: %{public}s, notifyType: %{public}d",
        data.id.c_str(), DfxUtils::GetSafePath(data.path).c_str(), notifyType);
    return E_OK;
}


int32_t ThumbnailGenerationPostProcess::GetNotifyType(const ThumbnailData& data,
    const ThumbRdbOpt& opts, NotifyType& notifyType)
{
    int32_t err = E_OK;
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, E_ERR, "RdbStore is nullptr");
    CHECK_AND_RETURN_RET_LOG(!data.id.empty(), E_ERR, "Data.id is empty");

    vector<string> columns = { PhotoColumn::PHOTO_THUMBNAIL_VISIBLE };
    string strQueryCondition = MEDIA_DATA_DB_ID + " = " + data.id;
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.SetWhereClause(strQueryCondition);

    auto resultSet = opts.store->QueryByStep(rdbPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "QueryByStep() result is null");
    int32_t rowsCount = 0;
    auto ret = resultSet->GetRowCount(rowsCount);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "ResultSet->GetRowCount err: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(rowsCount > 0, E_RDB_QUERY_NO_RES,
        "There is no such id: %{public}s path: %{public}s in the db",
        data.id.c_str(), DfxUtils::GetSafePath(data.path).c_str());
    ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "ResultSet->GoToFirstRow() err: %{public}d", ret);
    int32_t thumbnailVisible = GetInt32Val(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, resultSet);

    notifyType = thumbnailVisible > 0 ? NotifyType::NOTIFY_THUMB_UPDATE : NotifyType::NOTIFY_THUMB_ADD;
    return E_OK;
}

bool ThumbnailGenerationPostProcess::HasGeneratedThumb(const ThumbnailData& data)
{
    ValueObject valueObject;
    bool hasThumbReadyColumn = data.rdbUpdateCache.GetObject(PhotoColumn::PHOTO_THUMBNAIL_READY, valueObject);
    CHECK_AND_RETURN_RET_INFO_LOG(hasThumbReadyColumn, false, "Do not cache thumbnail_ready value in photos table");

    int64_t thumbReady;
    valueObject.GetLong(thumbReady);
    return thumbReady != static_cast<int64_t>(ThumbnailReady::GENERATE_THUMB_RETRY);
}

} // namespace Media
} // namespace OHOS