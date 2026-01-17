/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloudAlbum"

#include "cloud_album_handler.h"

#include "medialibrary_unistore_manager.h"
#include "medialibrary_album_operations.h"
#include "photo_album_column.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"
#include "medialibrary_notify.h"
#include "rdb_predicates.h"
#include "media_file_utils.h"
#include "photo_query_filter.h"
#include "accurate_common_data.h"
#include "album_accurate_refresh.h"

using namespace std;

namespace OHOS {
namespace Media {

using ChangeType = DataShare::DataShareObserver::ChangeType;

static vector<string> GetIds(const CloudSyncHandleData &handleData)
{
    vector<string> fileIds;
    for (auto &uri : handleData.orgInfo.uris) {
        string uriString = uri.ToString();
        auto index = uriString.rfind('/');
        if (index == string::npos) {
            continue;
        }
        auto fileIdStr = uriString.substr(index + 1);
        fileIds.push_back(fileIdStr);
    }
    return fileIds;
}

static void UpdateCloudAlbum(const string &id, int32_t count)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Can not get rdbstore");
    int changeRows = 0;
    NativeRdb::ValuesBucket valuesNew;
    valuesNew.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_NEW));
    valuesNew.PutInt(PhotoAlbumColumns::ALBUM_COUNT, count);
    NativeRdb::RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, id);
    rdbStore->Update(changeRows, valuesNew, rdbPredicates);
    CHECK_AND_PRINT_LOG(changeRows >= 0, "Failed to update cloudAlbum , ret = %{public}d", changeRows);
}
static int32_t GetCloudAlbumCount(const string &id)
{
    const std::vector<std::string> columnInfo = {"count(*) AS count"};

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, id);
    PhotoQueryFilter::ModifyPredicate(PhotoQueryFilter::Option::FILTER_VISIBLE, predicates);

    auto resultSet = MediaLibraryRdbStore::Query(predicates, columnInfo);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("GetCloudAlbumCount error: %{public}d", errno);
        return E_HAS_DB_ERROR;
    }
    int32_t count = GetInt32Val("count", resultSet);
    resultSet->Close();
    return count;
}

static void UpdateSourcePath(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    NativeRdb::RdbPredicates &predicates)
{
    for (auto albumId: predicates.GetWhereArgs()) {
        const std::string QUERY_FILE_ASSET_INFO = "SELECT file_id FROM Photos WHERE owner_album_id = " + albumId +
            " AND clean_flag =0 AND hidden =0";
        shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(QUERY_FILE_ASSET_INFO);
        vector<string> fileAssetsIds, fileAssetsUri;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            fileAssetsIds.push_back(to_string(fileId));
        }
        resultSet->Close();
        MediaLibraryPhotoOperations::UpdateSourcePath(fileAssetsIds);
    }
}

static int32_t DeletePhotoAlbum(NativeRdb::RdbPredicates &predicates)
{
    constexpr int32_t AFTER_AGR_SIZE = 2;
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "DeletePhotoAlbum Failed to get albumRefresh");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("DeletePhotoAlbum failed. rdbStore is null");
        return E_HAS_DB_ERROR;
    }
    UpdateSourcePath(rdbStore, predicates);
    predicates.And()->BeginWrap();
    predicates.BeginWrap()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    predicates.EndWrap();
    predicates.Or()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SOURCE));
    predicates.EndWrap();
    int deleteRow = -1;
    auto ret = albumRefresh->Delete(deleteRow, predicates);
    if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK || deleteRow <= 0) {
        MEDIA_ERR_LOG("DeletePhotoAlbum failed, errCode = %{public}d, deleteRow = %{public}d", ret, deleteRow);
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    const vector<string> &notifyUris = predicates.GetWhereArgs();
    size_t count = notifyUris.size() - AFTER_AGR_SIZE;
    for (size_t i = 0; i < count; i++) {
        if (deleteRow > 0) {
            watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
                notifyUris[i]), NotifyType::NOTIFY_REMOVE);
        }
    }
    albumRefresh->Notify();
    return deleteRow;
}

void CloudAlbumHandler::DeleteOrUpdateCloudAlbums(const vector<string> &ids)
{
    for (const auto &id : ids) {
        auto count = GetCloudAlbumCount(id);
        if (count == 0) {
            NativeRdb::RdbPredicates rdbPredicate(PhotoAlbumColumns::TABLE);
            rdbPredicate.EqualTo(PhotoAlbumColumns::ALBUM_ID, id);
            if (DeletePhotoAlbum(rdbPredicate) > 0) {
                MEDIA_INFO_LOG("delete Album {%{public}s} succ", id.c_str());
            } else {
                MEDIA_INFO_LOG("delete Album {%{public}s} fail", id.c_str());
            }
        } else {
            MEDIA_INFO_LOG("Album {%{public}s} not empty, setcount %{public}d", id.c_str(), count);
            UpdateCloudAlbum(id, count);
        }
    }
}

void CloudAlbumHandler::Handle(const CloudSyncHandleData &handleData)
{
    if (handleData.orgInfo.type == ChangeType::DELETE) {
        vector<string> fileIds;
        fileIds = GetIds(handleData);
        DeleteOrUpdateCloudAlbums(fileIds);
    }
    if (nextHandler_ != nullptr) {
        nextHandler_->Handle(handleData);
    }
}

} //namespace Media
} //namespace OHOS
