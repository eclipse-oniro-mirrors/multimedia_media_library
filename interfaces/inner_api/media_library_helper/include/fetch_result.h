/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_FETCH_RESULT_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_FETCH_RESULT_H_

#include <variant>
#include "abs_shared_result_set.h"
#include "album_asset.h"
#include "album_order.h"
#include "datashare_result_set.h"
#include "file_asset.h"
#include "medialibrary_type_const.h"
#include "medialibrary_db_const.h"
#include "rdb_errno.h"
#include "photo_album.h"
#include "smart_album_asset.h"
#include "userfile_manager_types.h"
#include "photo_asset_custom_record.h"
#include "custom_records_column.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
/**
 * @brief Class for returning the data cursor to application.
 *
 * @since 1.0
 * @version 1.0
 */
template <class T>
class FetchResult {
public:
    EXPORT explicit FetchResult(const std::shared_ptr<DataShare::DataShareResultSet> &resultset);
    EXPORT FetchResult();
    EXPORT virtual ~FetchResult();

    EXPORT void Close();
    EXPORT int32_t GetCount();
    EXPORT bool IsAtLastRow();
    EXPORT void SetInfo(const unique_ptr<FetchResult<T>> &fetch);
    EXPORT void SetNetworkId(const string &networkId);
    EXPORT void SetResultNapiType(const ResultNapiType napiType);
    EXPORT void SetFetchResType(const FetchResType resType);
    EXPORT void SetHiddenOnly(const bool hiddenOnly);
    EXPORT void SetLocationOnly(const bool locationOnly);

    EXPORT const std::string& GetNetworkId() const;
    EXPORT ResultNapiType GetResultNapiType();
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> &GetDataShareResultSet();
    EXPORT FetchResType GetFetchResType();
    EXPORT bool GetHiddenOnly() const;
    EXPORT bool GetLocationOnly() const;

    EXPORT std::unique_ptr<T> GetObjectAtPosition(int32_t index);
    EXPORT std::unique_ptr<T> GetFirstObject();
    EXPORT std::unique_ptr<T> GetObjectFromRdb(std::shared_ptr<NativeRdb::ResultSet> &resultSet, int idx);
    EXPORT std::unique_ptr<T> GetNextObject();
    EXPORT std::unique_ptr<T> GetLastObject();
    EXPORT std::unique_ptr<T> GetObject();
    EXPORT int32_t GetObjectIndexById(int32_t assetId);
    EXPORT int32_t GetAlbumIndex(int32_t assetId, int32_t photoAlbumType, int32_t photoAlbumSubType);
    EXPORT void SetUserId(int32_t userId);
    EXPORT int32_t GetUserId();

private:
    EXPORT std::unique_ptr<T> GetObject(std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    EXPORT std::variant<int32_t, int64_t, std::string, double> GetRowValFromColumn(std::string columnName,
        ResultSetDataType dataType, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    std::variant<int32_t, int64_t, std::string, double> GetValByIndex(int32_t index, ResultSetDataType dataType,
        std::shared_ptr<NativeRdb::ResultSet> &resultSet);

    void SetFileAsset(FileAsset *fileAsset, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void SetAlbumAsset(AlbumAsset* albumData, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void SetPhotoAlbum(PhotoAlbum* photoAlbumData, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void SetSmartAlbumAsset(SmartAlbumAsset* smartAlbumData, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void SetPhotoAssetCustomRecordAsset(PhotoAssetCustomRecord* smartAlbumData,
        std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    void SetAssetUri(FileAsset *fileAsset);
    void SetAlbumOrder(AlbumOrder *albumOrderData, std::shared_ptr<NativeRdb::ResultSet> &resultSet);

    void GetObjectFromResultSet(FileAsset *asset, shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetObjectFromResultSet(AlbumAsset *asset, shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetObjectFromResultSet(PhotoAlbum *asset, shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetObjectFromResultSet(SmartAlbumAsset *asset, shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetObjectFromResultSet(PhotoAssetCustomRecord *asset, shared_ptr<NativeRdb::ResultSet> &resultSet);
    void GetObjectFromResultSet(AlbumOrder *asset, shared_ptr<NativeRdb::ResultSet> &resultSet);

    std::string networkId_;
    ResultNapiType resultNapiType_;
    std::shared_ptr<DataShare::DataShareResultSet> resultset_ = nullptr;
    FetchResType fetchResType_ = FetchResType::TYPE_FILE;
    int32_t userId_ = -1;
    bool hiddenOnly_ = false;
    bool locationOnly_ = false;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_FETCH_RESULT_H_