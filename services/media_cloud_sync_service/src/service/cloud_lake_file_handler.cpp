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

#include "cloud_lake_file_handler.h"

#include "abs_rdb_predicates.h"
#include "lake_file_operations.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;

namespace OHOS {
namespace Media {

typedef struct {
    std::string path;
    std::string storagePath;
    std::string displayName;
    std::string lpath;
    int64_t dateTrashed;
    int32_t fileSourceType;
    int32_t hidden;
    int32_t fileId;
    int32_t ownerAlbumId;
} LakeData;

static int32_t QueryLakeData(int32_t fileId, LakeData &data)
{
    const vector<string> columns = {
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_NAME,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_OWNER_ALBUM_ID,
        PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_LPATH,
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> onClause = { PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = " +
            PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_ID };
    predicates.LeftOuterJoin(PhotoAlbumColumns::TABLE)->On(onClause);
    predicates.IsNotNull(PhotoColumn::PHOTO_STORAGE_PATH)->And()->EqualTo(PhotoColumn::MEDIA_ID, fileId);
    auto resultSet = MediaLibraryRdbStore::Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to query lake data");
        return E_HAS_DB_ERROR;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        data.path = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        data.storagePath = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_STORAGE_PATH, resultSet, TYPE_STRING));
        data.displayName = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_NAME, resultSet, TYPE_STRING));
        data.dateTrashed = get<int64_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_DATE_TRASHED, resultSet, TYPE_INT64));
        data.hidden = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_HIDDEN, resultSet, TYPE_INT32));
        data.fileSourceType = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet, TYPE_INT32));
        data.fileId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MEDIA_ID, resultSet, TYPE_INT32));
        data.lpath = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_LPATH, resultSet, TYPE_STRING));
        data.ownerAlbumId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet, TYPE_INT32));
        MEDIA_DEBUG_LOG("query lake data, file id: %{public}d", data.fileId);
    }
    resultSet->Close();
    return E_OK;
}

static void HandleTrashAndHiddenState(const LakeData &data,
    vector<string> &moveAssetsFromLakeList, vector<string> &moveAssetsToLakeList)
{
    // 查询为湖内文件，且是删除或者回收站中的    从湖内移动到湖外
    if (data.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE) &&
        (data.dateTrashed > 0 || data.hidden > 0)) {
        MEDIA_INFO_LOG("move file %{public}d from lake", data.fileId);
        moveAssetsFromLakeList.push_back({to_string(data.fileId)});
    }
    // 湖外文件并且不是回收站也不是隐藏     移动到湖内
    if (data.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA) && data.dateTrashed == 0 &&
        data.hidden == 0) {
        MEDIA_INFO_LOG("move file %{public}d to lake", data.fileId);
        moveAssetsToLakeList.push_back({to_string(data.fileId)});
    }
}

static int32_t GetLpath(const string &storagePath, string &lpath)
{
    size_t lastSlash = storagePath.rfind('/');
    if (lastSlash == string::npos) {
        MEDIA_ERR_LOG("slash not found in storage path: %{public}s", storagePath.c_str());
        return E_ERR;
    }

    string dir = storagePath.substr(0, lastSlash);
    string prefix = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
    if (!MediaFileUtils::StartsWith(dir, prefix)) {
        MEDIA_ERR_LOG("Failed to check storage path: %{public}s", storagePath.c_str());
        return E_ERR;
    }

    lpath = dir.substr(prefix.length());
    MEDIA_INFO_LOG("lpath of storage path: %{public}s is %{public}s", storagePath.c_str(), lpath.c_str());
    return E_OK;
}

static void HandleLakeFileMove(const LakeData &data, unordered_map<int32_t, vector<string>> &moveToNewAlbum)
{
    string lpath;
    int errCode = GetLpath(data.storagePath, lpath);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Failed to parse lpath from storagePath");
        return;
    }
    // 判断是不是发生了路径变更，是的话需要加入移动文件变更的数组中
    if (data.lpath == lpath) {
        return;
    }

    if (moveToNewAlbum.find(data.ownerAlbumId) == moveToNewAlbum.end()) {
        moveToNewAlbum[data.ownerAlbumId] = {to_string(data.fileId)};
    } else {
        moveToNewAlbum[data.ownerAlbumId].push_back(to_string(data.fileId));
    }
    MEDIA_INFO_LOG("move file %{public}d to album %{public}d", data.fileId, data.ownerAlbumId);
}

static void HandleLakeFileRename(AssetAccurateRefresh &refresh, const LakeData &data)
{
    if (MediaFileUtils::EndsWith(data.storagePath, data.displayName)) {
        return;
    }

    int32_t ret = LakeFileOperations::RenamePhoto(refresh, data.fileId, data.displayName, data.storagePath, data.path);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to handle lake file rename, ret: %{public}d", ret);
}

// 元数据变更下行的处理
void CloudLakeFileHandler::HandleMetaChanged(int32_t fileId)
{
    LakeData lakeData;
    // 根据file_id 查询数据库中的id\data\storage_path\display_name\date_trashed\hidden\source_type\album_id\lpath
    int32_t errCode = QueryLakeData(fileId, lakeData);
    if (errCode != E_OK) {
        return;
    }
    MEDIA_INFO_LOG("delete the %{public}d storage_path is %{public}s, trash is %{public}lld, hidden is %{public}d",
        lakeData.fileId, lakeData.storagePath.c_str(), lakeData.dateTrashed, lakeData.hidden);

    // MEDIA_INFO_LOG("%{public}d lake data changes meta", static_cast<int32_t>(lakeDataList.size()));
    vector<string> moveAssetsFromLakeList;
    vector<string> moveAssetsToLakeList;
    unordered_map<int32_t, vector<string>> moveToNewAlbum;
    AssetAccurateRefresh assetRefresh;
    HandleTrashAndHiddenState(lakeData, moveAssetsFromLakeList, moveAssetsToLakeList);
    HandleLakeFileMove(lakeData, moveToNewAlbum);
    HandleLakeFileRename(assetRefresh, lakeData);

    if (!moveAssetsFromLakeList.empty()) {
        LakeFileOperations::MoveAssetsFromLake(moveAssetsFromLakeList);
    }
    if (!moveAssetsToLakeList.empty()) {
        LakeFileOperations::MoveAssetsToLake(assetRefresh, moveAssetsToLakeList);
    }

    for (const auto& movePair: moveToNewAlbum) {
        LakeFileOperations::MoveInnerLakeAssetsToNewAlbum(assetRefresh, movePair.second, movePair.first);
    }
}
} //namespace Media
} //namespace OHOS