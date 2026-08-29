/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Dao"

#include "media_share_assets_dao.h"

#include "album_accurate_refresh.h"
#include "media_album_order_back.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"
#include "result_set_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
constexpr int32_t SHARED_ASSET_FLAG = 1;
constexpr int32_t BATCH_UPDATE_LIMIT_COUNT = 500;
constexpr int32_t BATCH_DELETE_LIMIT_COUNT = 300;
constexpr int64_t REAL_LCD_VISIT_TIME_INVALID = -2;

const std::string DELETE_DISPLAY_NAME = "cloud_media_asset_deleted";

bool MediaShareAssetsDao::HasShareAssetToMarkDeleted(std::vector<std::string> &updateFileIds,
    const std::string &lastFileId)
{
    updateFileIds.clear();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "HasShareAssetToMarkDeleted failed. rdbStore is null.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.GreaterThan(MediaColumn::MEDIA_ID, lastFileId);
    predicates.EqualTo(PhotoColumn::PHOTO_IS_SHARED, SHARED_ASSET_FLAG);
    predicates.NotEqualTo(MediaColumn::MEDIA_NAME, DELETE_DISPLAY_NAME);
    predicates.OrderByAsc(MediaColumn::MEDIA_ID);
    predicates.Limit(BATCH_UPDATE_LIMIT_COUNT);
    std::vector<std::string> columns = { MediaColumn::MEDIA_ID };
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "HasShareAssetToMarkDeleted failed. resultSet is null.");

    updateFileIds.reserve(BATCH_UPDATE_LIMIT_COUNT);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        updateFileIds.emplace_back(GetStringVal(MediaColumn::MEDIA_ID, resultSet));
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(updateFileIds.size() > 0, false, "the size of updateFileIds 0.");
    return true;
}

int32_t MediaShareAssetsDao::MarkDeletedAndClearCloudInfo(const std::vector<std::string> &updateFileIds)
{
    CHECK_AND_RETURN_RET_LOG(!updateFileIds.empty(), E_ERR, "updateFileIds is null.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "MarkDeletedAndClearCloudInfo failed. rdbStore is null.");
    AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, updateFileIds);
    predicates.EqualTo(PhotoColumn::PHOTO_IS_SHARED, SHARED_ASSET_FLAG);

    ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, DELETE_DISPLAY_NAME);
    values.PutInt(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, 0);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, -1);
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    values.PutNull(PhotoColumn::PHOTO_CLOUD_ID);

    int32_t changedRows = -1;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && changedRows > 0), E_ERR,
        "Failed to MarkDeletedAndClearCloudInfo, ret: %{public}d, updateRows: %{public}d", ret, changedRows);
    MEDIA_INFO_LOG("MarkDeletedAndClearCloudInfo successfully. ret: %{public}d, updateRows: %{public}d",
        ret, changedRows);
    return E_OK;
}

int32_t MediaShareAssetsDao::DeleteShareAssets(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), E_ERR, "DeleteShareAssets failed. fileIds is null.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "DeleteShareAssets failed. rdbStore is null.");
    AbsRdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
    deletePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    int32_t deletedRows = E_HAS_DB_ERROR;
    int32_t ret = rdbStore->Delete(deletedRows, deletePredicates);
    CHECK_AND_RETURN_RET_LOG((ret == NativeRdb::E_OK && deletedRows > 0), E_ERR,
        "Delete db operation failed. ret %{public}d. Deleted %{public}d", ret, deletedRows);
    MEDIA_INFO_LOG("Delete db operation successful. ret %{public}d. Deleted %{public}d", ret, deletedRows);
    return E_OK;
}

int32_t MediaShareAssetsDao::DeleteShareAlbums()
{
    MediaLibraryTracer tracer;
    tracer.Start("DeleteShareAlbums");

    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_ERR, "DeleteShareAlbums failed. albumRefresh is null");
    int32_t ret = albumRefresh->Init(this->SQL_SHARE_ALBUM_QUERY_ALL, std::vector<NativeRdb::ValueObject>());
    CHECK_AND_PRINT_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, "Failed to init albumRefresh");

    ret = albumRefresh->ExecuteSql(this->SQL_SHARE_ALBUM_DELETE_ALL,
        AccurateRefresh::RdbOperation::RDB_OPERATION_REMOVE);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, E_ERR,
        "Failed to delete. ret %{public}d.", ret);
    albumRefresh->Notify();
    MEDIA_INFO_LOG("DeleteShareAlbums. ret %{public}d.", ret);
    return E_OK;
}

int32_t MediaShareAssetsDao::GetShareAssetToRemove(std::vector<PhotosPo> &queryResult)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetShareAssetToRemove");
    AbsRdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    // don't care about smart data processing mode
    queryPredicates.EqualTo(PhotoColumn::PHOTO_IS_SHARED, SHARED_ASSET_FLAG);
    queryPredicates.EqualTo(MediaColumn::MEDIA_NAME, DELETE_DISPLAY_NAME);
    queryPredicates.Or();
    queryPredicates.BeginWrap();
        queryPredicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NEED_CLEAN));
        queryPredicates.EqualTo(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, REAL_LCD_VISIT_TIME_INVALID);
    queryPredicates.EndWrap();

    queryPredicates.Limit(BATCH_DELETE_LIMIT_COUNT);
    vector<string> columns = {};

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetShareAssetToRemove failed. rdbStorePtr is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetShareAssetToRemove failed. resultSet is null");

    ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(queryResult);
    return E_OK;
}
} // namespace OHOS::Media
