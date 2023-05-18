/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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
#include "media_column.h"
#define MLOG_TAG "FetchResult"

#include "fetch_result.h"
#include "album_asset.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_tracer.h"
#include "photo_album_column.h"

using namespace std;

namespace OHOS {
namespace Media {
using ResultTypeMap = unordered_map<string, ResultSetDataType>;

static const ResultTypeMap &GetResultTypeMap()
{
    static const ResultTypeMap RESULT_TYPE_MAP = {
        { MEDIA_DATA_DB_ID, TYPE_INT32 },
        { MEDIA_DATA_DB_NAME, TYPE_STRING },
        { MEDIA_DATA_DB_RELATIVE_PATH, TYPE_STRING },
        { MEDIA_DATA_DB_MEDIA_TYPE, TYPE_INT32 },
        { MEDIA_DATA_DB_PARENT_ID, TYPE_INT32 },
        { MEDIA_DATA_DB_SIZE, TYPE_INT64 },
        { MEDIA_DATA_DB_DATE_ADDED, TYPE_INT64 },
        { MEDIA_DATA_DB_DATE_MODIFIED, TYPE_INT64 },
        { MEDIA_DATA_DB_DATE_TAKEN, TYPE_INT64 },
        { MEDIA_DATA_DB_FILE_PATH, TYPE_STRING },
        { MEDIA_DATA_DB_MIME_TYPE, TYPE_STRING },
        { MEDIA_DATA_DB_TITLE, TYPE_STRING },
        { MEDIA_DATA_DB_ARTIST, TYPE_STRING },
        { MEDIA_DATA_DB_ALBUM, TYPE_STRING },
        { MEDIA_DATA_DB_WIDTH, TYPE_INT32 },
        { MEDIA_DATA_DB_HEIGHT, TYPE_INT32 },
        { MEDIA_DATA_DB_DURATION, TYPE_INT32 },
        { MEDIA_DATA_DB_ORIENTATION, TYPE_INT32 },
        { MEDIA_DATA_DB_BUCKET_ID, TYPE_INT32 },
        { MEDIA_DATA_DB_BUCKET_NAME, TYPE_STRING },
        { MEDIA_DATA_DB_TIME_PENDING, TYPE_INT64 },
        { MEDIA_DATA_DB_IS_FAV, TYPE_INT32 },
        { MEDIA_DATA_DB_DATE_TRASHED, TYPE_INT64 },
        { MEDIA_DATA_DB_SELF_ID, TYPE_STRING },
        { MEDIA_DATA_DB_RECYCLE_PATH, TYPE_STRING },
        { MEDIA_DATA_DB_IS_TRASH, TYPE_INT32 },
        { MEDIA_DATA_DB_AUDIO_ALBUM, TYPE_STRING },
        { MEDIA_DATA_DB_OWNER_PACKAGE, TYPE_STRING },
        { MEDIA_DATA_DB_POSITION, TYPE_INT32 },
        { MediaColumn::MEDIA_HIDDEN, TYPE_INT32 },
        { MediaColumn::MEDIA_VIRTURL_PATH, TYPE_STRING },
        { PhotoColumn::PHOTO_SUBTYPE, TYPE_INT32 },
    };
    return RESULT_TYPE_MAP;
}

template <class T>
FetchResult<T>::FetchResult(const shared_ptr<DataShare::DataShareResultSet> &resultset)
{
    resultset_ = resultset;
    networkId_ = "";
    resultNapiType_ = ResultNapiType::TYPE_NAPI_MAX;
    if (std::is_same<T, FileAsset>::value) {
        fetchResType_ = FetchResType::TYPE_FILE;
    } else if (std::is_same<T, AlbumAsset>::value) {
        fetchResType_ = FetchResType::TYPE_ALBUM;
    } else if (std::is_same<T, PhotoAlbum>::value) {
        fetchResType_ = FetchResType::TYPE_PHOTOALBUM;
    } else if (std::is_same<T, SmartAlbumAsset>::value) {
        fetchResType_ = FetchResType::TYPE_SMARTALBUM;
    } else {
        MEDIA_ERR_LOG("unsupported FetchResType");
        fetchResType_ = FetchResType::TYPE_FILE;
    }
    GetCount();
}

template <class T>
// empty constructor napi
FetchResult<T>::FetchResult() : resultNapiType_(ResultNapiType::TYPE_NAPI_MAX), resultset_(nullptr)
{
}

template <class T>
FetchResult<T>::~FetchResult()
{
    resultset_.reset();
}

template <class T>
void FetchResult<T>::Close()
{
    if (resultset_ != nullptr) {
        resultset_->Close();
        resultset_ = nullptr;
    }
}

template <class T>
int32_t FetchResult<T>::GetCount()
{
    int32_t count = 0;
    if (resultset_ == nullptr || resultset_->GetRowCount(count) != NativeRdb::E_OK) {
        return 0;
    }
    return count < 0 ? 0 : count;
}

template <class T>
void FetchResult<T>::SetInfo(unique_ptr<FetchResult<T>> &fetch)
{
    networkId_ = fetch->networkId_;
    resultNapiType_ = fetch->resultNapiType_;
    typeMask_ = fetch->typeMask_;
}

template <class T>
void FetchResult<T>::SetNetworkId(const string &networkId)
{
    networkId_ = networkId;
}

template<class T>
void FetchResult<T>::SetResultNapiType(const ResultNapiType napiType)
{
    resultNapiType_ = napiType;
}

template<class T>
void FetchResult<T>::SetFetchResType(const FetchResType resType)
{
    fetchResType_ = resType;
}

template<class T>
void FetchResult<T>::SetTypeMask(const string &mask)
{
    typeMask_ = mask;
}

template<class T>
string FetchResult<T>::GetNetworkId()
{
    return networkId_;
}

template<class T>
ResultNapiType FetchResult<T>::GetResultNapiType()
{
    return resultNapiType_;
}

template<class T>
shared_ptr<DataShare::DataShareResultSet> &FetchResult<T>::GetDataShareResultSet()
{
    return resultset_;
}

template<class T>
FetchResType FetchResult<T>::GetFetchResType()
{
    return fetchResType_;
}

template<class T>
string FetchResult<T>::GetTypeMask()
{
    return typeMask_;
}

template <class T>
unique_ptr<T> FetchResult<T>::GetObjectAtPosition(int32_t index)
{
    if (resultset_ == nullptr) {
        MEDIA_ERR_LOG("rs is null");
        return nullptr;
    }

    int32_t count = GetCount();
    if ((index < 0) || (index > (count - 1))) {
        MEDIA_ERR_LOG("index not proper");
        return nullptr;
    }

    if (resultset_->GoToRow(index) != 0) {
        MEDIA_ERR_LOG("failed to go to row at index pos");
        return nullptr;
    }

    return GetObject();
}

template <class T>
unique_ptr<T> FetchResult<T>::GetFirstObject()
{
    if ((resultset_ == nullptr) || (resultset_->GoToFirstRow() != 0)) {
        MEDIA_DEBUG_LOG("resultset is null|first row failed");
        return nullptr;
    }

    return GetObject();
}

template <class T>
unique_ptr<T> FetchResult<T>::GetNextObject()
{
    if ((resultset_ == nullptr) || (resultset_->GoToNextRow() != 0)) {
        MEDIA_DEBUG_LOG("resultset is null|go to next row failed");
        return nullptr;
    }

    return GetObject();
}

template <class T>
unique_ptr<T> FetchResult<T>::GetLastObject()
{
    if ((resultset_ == nullptr) || (resultset_->GoToLastRow() != 0)) {
        MEDIA_ERR_LOG("resultset is null|go to last row failed");
        return nullptr;
    }

    return GetObject();
}

template <class T>
bool FetchResult<T>::IsAtLastRow()
{
    if (resultset_ == nullptr) {
        MEDIA_ERR_LOG("resultset null");
        return false;
    }

    bool retVal = false;
    resultset_->IsAtLastRow(retVal);
    return retVal;
}

variant<int32_t, int64_t, string> ReturnDefaultOnError(string errMsg, ResultSetDataType dataType)
{
    MEDIA_ERR_LOG("%{public}s", errMsg.c_str());
    if (dataType == TYPE_STRING) {
        return "";
    } else if (dataType == TYPE_INT64) {
        return static_cast<int64_t>(0);
    } else {
        return 0;
    }
}

template <class T>
variant<int32_t, int64_t, string> FetchResult<T>::GetRowValFromColumn(string columnName, ResultSetDataType dataType,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if ((resultset_ == nullptr) && (resultSet == nullptr)) {
        return ReturnDefaultOnError("Resultset is null", dataType);
    }
    int index;
    int status;
    if (resultSet) {
        status = resultSet->GetColumnIndex(columnName, index);
    } else {
        status = resultset_->GetColumnIndex(columnName, index);
    }
    if (status != NativeRdb::E_OK) {
        ReturnDefaultOnError("failed to obtain the index", dataType);
    }
    return GetValByIndex(index, dataType, resultSet);
}

template <class T>
variant<int32_t, int64_t, string> FetchResult<T>::GetValByIndex(int32_t index, ResultSetDataType dataType,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if ((resultset_ == nullptr) && (resultSet == nullptr)) {
        return ReturnDefaultOnError("Resultset is null", dataType);
    }

    variant<int32_t, int64_t, string> cellValue;
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    int status;
    switch (dataType) {
        case TYPE_STRING:
            if (resultSet) {
                status = resultSet->GetString(index, stringVal);
            } else {
                status = resultset_->GetString(index, stringVal);
            }
            cellValue = move(stringVal);
            break;
        case TYPE_INT32:
            if (resultSet) {
                status = resultSet->GetInt(index, integerVal);
            } else {
                status = resultset_->GetInt(index, integerVal);
            }
            cellValue = integerVal;
            break;
        case TYPE_INT64:
            if (resultSet) {
                status = resultSet->GetLong(index, longVal);
            } else {
                status = resultset_->GetLong(index, longVal);
            }
            cellValue = longVal;
            break;
        default:
            MEDIA_ERR_LOG("not match  dataType %{public}d", dataType);
            break;
    }

    return cellValue;
}

static void MediaTypeToMask(MediaType mediaType, string &typeMask)
{
    typeMask.resize(TYPE_MASK_STRING_SIZE, TYPE_MASK_BIT_DEFAULT);
    if ((mediaType >= MEDIA_TYPE_FILE) && (mediaType <= MEDIA_TYPE_AUDIO)) {
        typeMask[std::get<POS_TYPE_MASK_STRING_INDEX>(MEDIA_TYPE_TUPLE_VEC[mediaType])] = TYPE_MASK_BIT_SET;
    }
}

template<class T>
void FetchResult<T>::SetFileAsset(FileAsset *fileAsset, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if ((resultset_ == nullptr) && (resultSet == nullptr)) {
        MEDIA_ERR_LOG("SetFileAsset fail, result is nullptr");
        return;
    }
    vector<string> columnNames;
    if (resultSet != nullptr) {
        resultSet->GetAllColumnNames(columnNames);
    } else {
        resultset_->GetAllColumnNames(columnNames);
    }
    int32_t index = -1;
    auto &map = fileAsset->GetMemberMap();
    for (const auto &name : columnNames) {
        index++;
        if (GetResultTypeMap().count(name) == 0) {
            continue;
        }
        auto memberType = GetResultTypeMap().at(name);
        map.emplace(move(name), move(GetValByIndex(index, memberType, resultSet)));
    }
    fileAsset->SetResultNapiType(resultNapiType_);
    if (!columnNames.empty() && columnNames[0].find("count(") != string::npos) {
        int count = 1;
        if (resultset_) {
            resultset_->GetInt(0, count);
        }
        fileAsset->SetCount(count);
    }
    string uri;
    if (resultNapiType_ == ResultNapiType::TYPE_USERFILE_MGR) {
        string typeMask;
        MediaTypeToMask(fileAsset->GetMediaType(), typeMask);
        fileAsset->SetTypeMask(typeMask);
        MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()),
             networkId_, MEDIA_API_VERSION_V10);
        uri = fileUri.ToString();
    } else {
        MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()),
            networkId_);
        uri = fileUri.ToString();
    }
    if (fileAsset->GetAlbumId() != DEFAULT_INT32) {
        fileAsset->SetAlbumUri(MediaFileUri(MEDIA_TYPE_ALBUM, to_string(fileAsset->GetAlbumId()),
            networkId_).ToString());
    }
    fileAsset->SetUri(move(uri));
}

template<class T>
void FetchResult<T>::GetObjectFromResultSet(FileAsset *asset, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    SetFileAsset(asset, resultSet);
}

template<class T>
void FetchResult<T>::GetObjectFromResultSet(AlbumAsset *asset, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    SetAlbumAsset(asset, resultSet);
}

template<class T>
void FetchResult<T>::GetObjectFromResultSet(PhotoAlbum *asset, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    SetPhotoAlbum(asset, resultSet);
}

template<class T>
void FetchResult<T>::GetObjectFromResultSet(SmartAlbumAsset *asset, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    SetSmartAlbumAsset(asset, resultSet);
}

template<class T>
unique_ptr<T> FetchResult<T>::GetObject(shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    unique_ptr<T> asset = make_unique<T>();
    GetObjectFromResultSet(asset.get(), resultSet);
    return asset;
}

template <class T>
unique_ptr<T> FetchResult<T>::GetObject()
{
    shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    return GetObject(resultSet);
}

template <class T>
unique_ptr<T> FetchResult<T>::GetObjectFromRdb(shared_ptr<NativeRdb::ResultSet> &resultSet, int idx)
{
    if ((resultSet == nullptr) || (resultSet->GoToFirstRow() != 0) || (resultSet->GoTo(idx))) {
        MEDIA_ERR_LOG("resultset is null|first row failed");
        return nullptr;
    }

    return GetObject(resultSet);
}

template<class T>
void FetchResult<T>::SetAlbumAsset(AlbumAsset *albumData, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    // Get album id index and value
    albumData->SetAlbumId(get<int32_t>(GetRowValFromColumn(MEDIA_DATA_DB_BUCKET_ID, TYPE_INT32, resultSet)));

    // Get album title index and value
    albumData->SetAlbumName(get<string>(GetRowValFromColumn(MEDIA_DATA_DB_TITLE, TYPE_STRING, resultSet)));

    // Get album asset count index and value
    albumData->SetCount(get<int32_t>(GetRowValFromColumn(MEDIA_DATA_DB_COUNT, TYPE_INT32, resultSet)));
    string albumUri;
    if (resultNapiType_ == ResultNapiType::TYPE_USERFILE_MGR) {
        albumUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(albumData->GetAlbumId());
    } else {
        albumUri = ML_FILE_URI_PREFIX + MEDIALIBRARY_TYPE_ALBUM_URI + "/" + to_string(albumData->GetAlbumId());
    }
    albumData->SetAlbumUri(albumUri);
    // Get album relativePath index and value
    albumData->SetAlbumRelativePath(get<string>(GetRowValFromColumn(MEDIA_DATA_DB_RELATIVE_PATH,
        TYPE_STRING, resultSet)));
    albumData->SetAlbumDateModified(get<int64_t>(GetRowValFromColumn(MEDIA_DATA_DB_DATE_MODIFIED,
        TYPE_INT64, resultSet)));

    albumData->SetResultNapiType(resultNapiType_);
    albumData->SetAlbumTypeMask(typeMask_);
}

template<class T>
void FetchResult<T>::SetPhotoAlbum(PhotoAlbum* photoAlbumData, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t albumId = get<int32_t>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_ID, TYPE_INT32, resultSet));
    photoAlbumData->SetAlbumId(albumId);
    photoAlbumData->SetPhotoAlbumType(static_cast<PhotoAlbumType>(
        get<int32_t>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_TYPE, TYPE_INT32, resultSet))));
    photoAlbumData->SetPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(
        get<int32_t>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_SUBTYPE, TYPE_INT32, resultSet))));
    photoAlbumData->SetAlbumUri(PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(albumId));
    photoAlbumData->SetAlbumName(get<string>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_NAME, TYPE_STRING,
        resultSet)));
    photoAlbumData->SetRelativePath(get<string>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_RELATIVE_PATH, TYPE_STRING,
        resultSet)));
    photoAlbumData->SetCount(get<int32_t>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_COUNT, TYPE_INT32, resultSet)));
    photoAlbumData->SetCoverUri(get<string>(GetRowValFromColumn(PhotoAlbumColumns::ALBUM_COVER_URI, TYPE_STRING,
        resultSet)));
    photoAlbumData->SetResultNapiType(resultNapiType_);
    photoAlbumData->SetTypeMask(typeMask_);
}

template<class T>
void FetchResult<T>::SetSmartAlbumAsset(SmartAlbumAsset* smartAlbumData, shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    smartAlbumData->SetAlbumId(get<int32_t>(GetRowValFromColumn(SMARTALBUM_DB_ID, TYPE_INT32, resultSet)));
    smartAlbumData->SetAlbumName(get<string>(GetRowValFromColumn(SMARTALBUM_DB_NAME, TYPE_STRING, resultSet)));
    smartAlbumData->SetAlbumCapacity(get<int32_t>(GetRowValFromColumn(SMARTALBUM_DB_CAPACITY, TYPE_INT32, resultSet)));
    smartAlbumData->SetResultNapiType(resultNapiType_);
    smartAlbumData->SetTypeMask(typeMask_);
}

template class FetchResult<FileAsset>;
template class FetchResult<AlbumAsset>;
template class FetchResult<PhotoAlbum>;
template class FetchResult<SmartAlbumAsset>;
}  // namespace Media
}  // namespace OHOS
