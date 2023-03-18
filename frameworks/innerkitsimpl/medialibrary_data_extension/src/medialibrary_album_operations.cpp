/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "AlbumOperation"

#include "medialibrary_album_operations.h"

#include "directory_ex.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"

#include "result_set_utils.h"
#include "values_bucket.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS::Media {
int32_t MediaLibraryAlbumOperations::CreateAlbumOperation(MediaLibraryCommand &cmd)
{
    int64_t outRow = -1;
    int32_t errCode = MediaLibraryObjectUtils::CreateDirObj(cmd, outRow);
    if (errCode == E_SUCCESS) {
        return outRow;
    }
    return errCode;
}

// only support modify in the same parent folder, like: a/b/c --> a/b/d
int32_t MediaLibraryAlbumOperations::ModifyAlbumOperation(MediaLibraryCommand &cmd)
{
    string strId = cmd.GetOprnFileId();
    string srcDirPath = MediaLibraryObjectUtils::GetPathByIdFromDb(strId);
    if (srcDirPath.empty()) {
        MEDIA_ERR_LOG("Get path of id %{private}s from database file!", strId.c_str());
        return E_INVALID_PATH;
    }

    auto values = cmd.GetValueBucket();
    string dstDirName;
    ValueObject valueObject;
    if (values.GetObject(MEDIA_DATA_DB_ALBUM_NAME, valueObject)) {
        valueObject.GetString(dstDirName);
    }
    string dstDirPath = MediaLibraryDataManagerUtils::GetParentPath(srcDirPath) + "/" + dstDirName;

    return MediaLibraryObjectUtils::RenameDirObj(cmd, srcDirPath, dstDirPath);
}

string MediaLibraryAlbumOperations::GetDistributedAlbumSql(const string &strQueryCondition, const string &tableName)
{
    string distributedAlbumSql = "SELECT * FROM ( " + DISTRIBUTED_ALBUM_COLUMNS + " FROM " + tableName + " " +
        FILE_TABLE + ", " + tableName + " " + ALBUM_TABLE +
        DISTRIBUTED_ALBUM_WHERE_AND_GROUPBY + " )";
    if (!strQueryCondition.empty()) {
        distributedAlbumSql += " WHERE " + strQueryCondition;
    }
    MEDIA_DEBUG_LOG("GetDistributedAlbumSql distributedAlbumSql = %{private}s", distributedAlbumSql.c_str());
    return distributedAlbumSql;
}

shared_ptr<NativeRdb::ResultSet> MediaLibraryAlbumOperations::QueryAlbumOperation(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("uniStore is nullptr!");
        return nullptr;
    }

    if (cmd.GetOprnObject() == OperationObject::MEDIA_VOLUME) {
        MEDIA_DEBUG_LOG("QUERY_MEDIA_VOLUME = %{public}s", QUERY_MEDIA_VOLUME.c_str());
        return uniStore->QuerySql(QUERY_MEDIA_VOLUME);
    }

    string strQueryCondition = cmd.GetAbsRdbPredicates()->GetWhereClause();
    strQueryCondition += " GROUP BY " + MEDIA_DATA_DB_BUCKET_ID;
    cmd.GetAbsRdbPredicates()->SetWhereClause(strQueryCondition);
    string networkId = cmd.GetOprnDevice();
    if (!networkId.empty()) {
        string tableName = cmd.GetTableName();
        MEDIA_INFO_LOG("tableName is %{private}s", tableName.c_str());
        if (!strQueryCondition.empty()) {
            strQueryCondition = MediaLibraryDataManagerUtils::ObtionCondition(strQueryCondition,
                cmd.GetAbsRdbPredicates()->GetWhereArgs());
        }
        string distributedAlbumSql = GetDistributedAlbumSql(strQueryCondition, tableName);
        return uniStore->QuerySql(distributedAlbumSql);
    }

    if (!strQueryCondition.empty()) {
        return uniStore->Query(cmd, columns);
    }
    string querySql = "SELECT * FROM " + cmd.GetTableName();
    return uniStore->QuerySql(querySql);
}

inline int32_t GetStringObject(const ValuesBucket &values, const string &key, string &value)
{
    value = "";
    ValueObject valueObject;
    if (values.GetObject(PhotoAlbum::ALBUM_NAME, valueObject)) {
        valueObject.GetString(value);
    } else {
        return -EINVAL;
    }
    return E_OK;
}

inline void PrepareUserAlbum(const string &albumName, const string &relativePath, ValuesBucket &values)
{
    values.PutString(PhotoAlbum::ALBUM_NAME, albumName);
    values.PutInt(PhotoAlbum::ALBUM_TYPE, PhotoAlbumType::USER);
    values.PutInt(PhotoAlbum::ALBUM_SUBTYPE, PhotoAlbumSubType::USER_GENERIC);

    if (!relativePath.empty()) {
        values.PutString(PhotoAlbum::ALBUM_RELATIVE_PATH, relativePath);
    }
}

inline void PrepareWhere(const string &albumName, const string &relativePath, RdbPredicates &predicates)
{
    predicates.EqualTo(PhotoAlbum::ALBUM_NAME, albumName);
    if (relativePath.empty()) {
        predicates.IsNull(PhotoAlbum::ALBUM_RELATIVE_PATH);
    } else {
        predicates.EqualTo(PhotoAlbum::ALBUM_RELATIVE_PATH, relativePath);
    }
}

// Caller is responsible for checking @albumName AND @relativePath
int DoCreatePhotoAlbum(const string &albumName, const string &relativePath)
{
    ValuesBucket albumValues;
    PrepareUserAlbum(albumName, relativePath, albumValues);

    RdbPredicates wherePredicates(PhotoAlbum::TABLE);
    PrepareWhere(albumName, relativePath, wherePredicates);

    return MediaLibraryRdbStore::InsertWithWhereExists(PhotoAlbum::TABLE,
        albumValues, false, wherePredicates);
}

inline int CreatePhotoAlbum(const string &albumName)
{
    int32_t err = MediaFileUtils::CheckAlbumName(albumName);
    if (err < 0) {
        return err;
    }

    return DoCreatePhotoAlbum(albumName, "");
}

int CreatePhotoAlbum(MediaLibraryCommand &cmd)
{
    string albumName;
    int err = GetStringObject(cmd.GetValueBucket(), PhotoAlbum::ALBUM_NAME, albumName);
    if (err < 0) {
        return err;
    }
    return CreatePhotoAlbum(albumName);
}

int32_t MediaLibraryAlbumOperations::DeletePhotoAlbum(const DataShare::DataSharePredicates &predicates)
{
    RdbPredicates rdbPredicate = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, PhotoAlbum::TABLE);

    // Only user generic albums can be deleted
    rdbPredicate.And()->BeginWrap()->EqualTo(PhotoAlbum::ALBUM_TYPE, to_string(PhotoAlbumType::USER));
    rdbPredicate.EqualTo(PhotoAlbum::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::USER_GENERIC));
    rdbPredicate.EndWrap();

    return MediaLibraryRdbStore::Delete(rdbPredicate);
}

shared_ptr<ResultSet> MediaLibraryAlbumOperations::QueryPhotoAlbum(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
    return MediaLibraryRdbStore::Query(*(cmd.GetAbsRdbPredicates()), columns);
}

int MediaLibraryAlbumOperations::HandlePhotoAlbumOperations(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnType()) {
        case OperationType::CREATE:
            return CreatePhotoAlbum(cmd);
        default:
            MEDIA_ERR_LOG("Unknown operation type: %{public}d", cmd.GetOprnType());
            return E_ERR;
    }
}
} // namespace OHOS::Media
