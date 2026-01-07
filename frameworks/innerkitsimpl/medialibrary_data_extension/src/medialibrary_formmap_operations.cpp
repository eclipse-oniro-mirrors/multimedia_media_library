/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "medialibrary_formmap_operations.h"

#include <sys/stat.h>
#include <fcntl.h>

#include "map_operation_flag.h"
#include "media_file_utils.h"
#include "medialibrary_object_utils.h"
#include "result_set_utils.h"
#include "medialibrary_data_manager.h"
#include "ithumbnail_helper.h"
#include "form_map.h"
#include "medialibrary_data_manager_utils.h"

using namespace OHOS::DataShare;
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS {
namespace Media {
std::mutex MediaLibraryFormMapOperations::mutex_;
const string MEDIA_LIBRARY_PROXY_URI = "datashareproxy://com.ohos.medialibrary.medialibrarydata";
const string MEDIA_LIBRARY_PROXY_DATA_URI = "datashareproxy://com.ohos.medialibrary.medialibrarydata/image_data";
const string MEDIA_LIBRARY_PROXY_IMAGE_URI = "datashareproxy://com.ohos.medialibrary.medialibrarydata/image_uri";
const string NO_PICTURES = "";

static void ReadThumbnailFile(const string &path, vector<uint8_t> &buffer)
{
    string thumbnailFileName = GetThumbnailPath(path, THUMBNAIL_LCD_SUFFIX);
    auto fd = open(thumbnailFileName.c_str(), O_RDONLY);
    UniqueFd uniqueFd(fd);
    struct stat statInfo;
    if (fstat(uniqueFd.Get(), &statInfo) == E_ERR) {
        return ;
    }
    buffer.reserve(statInfo.st_size);
    uint8_t *tempBuffer = (uint8_t *)malloc(statInfo.st_size);
    if (tempBuffer == nullptr) {
        MEDIA_ERR_LOG("The point tempBuffer is null!");
        return ;
    }
    ssize_t bytes = read(uniqueFd.Get(), tempBuffer, statInfo.st_size);
    if (bytes < 0) {
        MEDIA_ERR_LOG("read file failed!");
        free(tempBuffer);
        return ;
    }
    buffer.assign(tempBuffer, tempBuffer + statInfo.st_size);
    free(tempBuffer);
}

string MediaLibraryFormMapOperations::GetUriByFileId(const int32_t &fileId, const string &path)
{
    MediaLibraryCommand queryCmd(OperationObject::UFM_PHOTO, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, to_string(fileId));
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("UniStore is nullptr");
        return "";
    }
    vector<string> columns = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_NAME
    };
    auto queryResult = uniStore->Query(queryCmd, columns);
    if (queryResult == nullptr || queryResult->GoToFirstRow() != NativeRdb::E_OK) {
        return "";
    }
    string displayName = GetStringVal(MEDIA_DATA_DB_NAME, queryResult);
    int32_t mediaType = GetInt32Val(MEDIA_DATA_DB_MEDIA_TYPE, queryResult);
    if (mediaType != MEDIA_TYPE_IMAGE) {
        return "";
    }
    string extraUri = MediaFileUtils::GetExtraUri(displayName, path, false);
    return MediaFileUri(MediaType(mediaType), ToString(fileId), "", MEDIA_API_VERSION_V10,  extraUri).ToString();
}

void MediaLibraryFormMapOperations::GetFormMapFormId(const string &uri, vector<int64_t> &formIds)
{
    lock_guard<mutex> lock(mutex_);
    MediaLibraryCommand queryFormMapCmd(OperationObject::PAH_FORM_MAP, OperationType::QUERY);
    queryFormMapCmd.GetAbsRdbPredicates()->EqualTo(FormMap::FORMMAP_URI, uri);

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("UniStore is nullptr");
        return;
    }
    vector<string> columns = {FormMap::FORMMAP_FORM_ID, FormMap::FORMMAP_URI};
    auto queryResult = uniStore->Query(queryFormMapCmd, columns);
    if (queryResult == nullptr) {
        MEDIA_ERR_LOG("Failed to query form id!");
        return;
    }
    while (queryResult->GoToNextRow() == NativeRdb::E_OK) {
        string formId = GetStringVal(FormMap::FORMMAP_FORM_ID, queryResult);
        if (!MediaLibraryDataManagerUtils::IsNumber(formId)) {
            MEDIA_WARN_LOG("invalid formId: %{public}s", formId.c_str());
            continue;
        }
        if (GetStringVal(FormMap::FORMMAP_URI, queryResult) == uri) {
            formIds.push_back(std::stoll(formId));
        }
    }
}

void MediaLibraryFormMapOperations::GetFormIdsByUris(const vector<string> &notifyUris, vector<int64_t> &formIds)
{
    lock_guard<mutex> lock(mutex_);
    MediaLibraryCommand queryFormMapCmd(OperationObject::PAH_FORM_MAP, OperationType::QUERY);
    queryFormMapCmd.GetAbsRdbPredicates()->And()->In(FormMap::FORMMAP_URI, notifyUris);

    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("UniStore is nullptr");
        return;
    }
    vector<string> columns = {FormMap::FORMMAP_FORM_ID, FormMap::FORMMAP_URI};
    auto queryResult = uniStore->Query(queryFormMapCmd, columns);
    if (queryResult == nullptr) {
        MEDIA_ERR_LOG("Failed to query form id!");
        return;
    }
    while (queryResult->GoToNextRow() == NativeRdb::E_OK) {
        string formId = GetStringVal(FormMap::FORMMAP_FORM_ID, queryResult);
        if (!MediaLibraryDataManagerUtils::IsNumber(formId)) {
            MEDIA_WARN_LOG("invalid formId: %{public}s", formId.c_str());
            continue;
        }
        string uri = GetStringVal(FormMap::FORMMAP_URI, queryResult);
        if (std::count(notifyUris.begin(), notifyUris.end(), uri) > 0) {
            formIds.push_back(std::stoll(formId));
        }
    }
    queryResult->Close();
}

string MediaLibraryFormMapOperations::GetFilePathById(const string &fileId)
{
    MediaLibraryCommand queryCmd(OperationObject::UFM_PHOTO, OperationType::QUERY);
    queryCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, fileId);
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("UniStore is nullptr");
        return "";
    }
    vector<string> columns = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH
    };
    auto queryResult = uniStore->Query(queryCmd, columns);
    if (queryResult == nullptr || queryResult->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Get path by Id failed , id is %{private}s", fileId.c_str());
        return "";
    }
    return GetStringVal(MEDIA_DATA_DB_FILE_PATH, queryResult);
}

void MediaLibraryFormMapOperations::ModifyFormMapMessage(const string &uri, const int64_t &formId, const bool &isSave)
{
    if (isSave) {
        MEDIA_INFO_LOG("Modify FormMap message return!, the case is saveFormInfo");
        return;
    }
    lock_guard<mutex> lock(mutex_);
    ValuesBucket value;
    value.PutString(FormMap::FORMMAP_URI, uri);
    RdbPredicates predicates(FormMap::FORM_MAP_TABLE);
    predicates.And()->EqualTo(FormMap::FORMMAP_FORM_ID, std::to_string(formId));
    int32_t updateRow = MediaLibraryRdbStore::UpdateWithDateTime(value, predicates);
    if (updateRow < 0) {
        MEDIA_ERR_LOG("Modify FormMap message err!, uri is %{private}s, formId is %{private}s",
            uri.c_str(), to_string(formId).c_str());
    }
    return;
}

void MediaLibraryFormMapOperations::PublishedChange(const string newUri, const vector<int64_t> &formIds,
    const bool &isSave)
{
    CreateOptions options;
    options.enabled_ = true;
    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(MEDIA_LIBRARY_PROXY_URI, options);
    if (dataShareHelper == nullptr) {
        MEDIA_ERR_LOG("dataShareHelper is nullptr");
        return;
    }
    Data data;
    PublishedDataItem::DataType tempData;
    if (newUri.empty()) {
        nlohmann::json noPicData = NO_PICTURES;
        tempData = noPicData.dump();
        for (auto &formId : formIds) {
            MEDIA_INFO_LOG("Published formId is %{private}s, size of value is %{private}zu!",
                to_string(formId).c_str(), NO_PICTURES.size());
            data.datas_.emplace_back(PublishedDataItem(MEDIA_LIBRARY_PROXY_DATA_URI, formId, tempData));
            data.datas_.emplace_back(PublishedDataItem(MEDIA_LIBRARY_PROXY_IMAGE_URI, formId, tempData));
            dataShareHelper->Publish(data, BUNDLE_NAME);
            MediaLibraryFormMapOperations::ModifyFormMapMessage(NO_PICTURES, formId, isSave);
        }
    } else {
        MediaFileUri fileUri = MediaFileUri(newUri);
        ThumbnailWait thumbnailWait(false);
        thumbnailWait.CheckAndWait(fileUri.GetFileId(), true);
        string filePath = MediaLibraryFormMapOperations::GetFilePathById(fileUri.GetFileId());
        if (MediaType(MediaFileUtils::GetMediaType(filePath)) == MEDIA_TYPE_IMAGE) {
            vector<uint8_t> buffer;
            ReadThumbnailFile(filePath, buffer);
            tempData = buffer;
            nlohmann::json uriJson = newUri;
            PublishedDataItem::DataType uriData = uriJson.dump();
            for (auto &formId : formIds) {
                MEDIA_INFO_LOG("Published formId: %{private}s!, value size: %{private}zu, image uri: %{private}s",
                    to_string(formId).c_str(), buffer.size(), newUri.c_str());
                data.datas_.emplace_back(PublishedDataItem(MEDIA_LIBRARY_PROXY_DATA_URI, formId, tempData));
                data.datas_.emplace_back(PublishedDataItem(MEDIA_LIBRARY_PROXY_IMAGE_URI, formId, uriData));
                dataShareHelper->Publish(data, BUNDLE_NAME);
                MediaLibraryFormMapOperations::ModifyFormMapMessage(newUri, formId, isSave);
            }
        }
    }
}

int32_t MediaLibraryFormMapOperations::RemoveFormIdOperations(RdbPredicates &predicates)
{
    if (MAP_OPERATION_FLAG) {
        MEDIA_INFO_LOG("GetParameter map operation flag end, flag is true");
        lock_guard<mutex> lock(mutex_);
        return MediaLibraryRdbStore::Delete(predicates);
    }
    return E_OK;
}

static string GetStringObject(MediaLibraryCommand &cmd, const string &columnName)
{
    ValueObject valueObject;
    ValuesBucket values = cmd.GetValueBucket();
    string value;
    if (values.GetObject(columnName, valueObject)) {
        valueObject.GetString(value);
        return value;
    }
    return "";
}

bool MediaLibraryFormMapOperations::CheckQueryIsInDb(const OperationObject &operationObject, const string &queryId)
{
    lock_guard<mutex> lock(mutex_);
    MediaLibraryCommand queryFormMapCmd(operationObject, OperationType::QUERY);
    vector<string> columns;
    if (operationObject == OperationObject::PAH_FORM_MAP) {
        queryFormMapCmd.GetAbsRdbPredicates()->EqualTo(FormMap::FORMMAP_FORM_ID, queryId);
        columns = { FormMap::FORMMAP_FORM_ID };
    } else if (operationObject == OperationObject::UFM_PHOTO) {
        queryFormMapCmd.GetAbsRdbPredicates()->EqualTo(MEDIA_DATA_DB_ID, queryId);
        columns = { MEDIA_DATA_DB_ID };
    }
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (uniStore == nullptr) {
        MEDIA_ERR_LOG("UniStore is nullptr");
        return false;
    }
    auto queryResultSet = uniStore->Query(queryFormMapCmd, columns);
    if (queryResultSet != nullptr) {
        if (queryResultSet->GoToFirstRow() == NativeRdb::E_OK) {
            MEDIA_INFO_LOG("The id queried already exists!");
            return true;
        }
    }
    return false;
}

int32_t MediaLibraryFormMapOperations::HandleStoreFormIdOperation(MediaLibraryCommand &cmd)
{
    string formId = GetStringObject(cmd, FormMap::FORMMAP_FORM_ID);
    if (formId.empty()) {
        MEDIA_ERR_LOG("GetObject failed");
        return E_GET_PRAMS_FAIL;
    }
    string uri = GetStringObject(cmd, FormMap::FORMMAP_URI);
    ValuesBucket value;
    value.PutString(FormMap::FORMMAP_URI, uri);

    if (!uri.empty()) {
        MediaFileUri mediaUri(uri);
        CHECK_AND_RETURN_RET_LOG(MediaLibraryFormMapOperations::CheckQueryIsInDb(OperationObject::UFM_PHOTO,
            mediaUri.GetFileId()), E_GET_PRAMS_FAIL, "the fileId is not exist");
        CHECK_AND_RETURN_RET_LOG(MediaLibraryDataManagerUtils::IsNumber(formId),
            E_GET_PRAMS_FAIL, "invalid formId: %{public}s", formId.c_str());
        vector<int64_t> formIds = { std::stoll(formId) };
        MediaLibraryFormMapOperations::PublishedChange(uri, formIds, true);
    }
    if (MediaLibraryFormMapOperations::CheckQueryIsInDb(OperationObject::PAH_FORM_MAP, formId)) {
        lock_guard<mutex> lock(mutex_);
        RdbPredicates predicates(FormMap::FORM_MAP_TABLE);
        predicates.EqualTo(FormMap::FORMMAP_FORM_ID, formId);
        return MediaLibraryRdbStore::UpdateWithDateTime(value, predicates);
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return E_HAS_DB_ERROR;
    }
    int64_t outRowId = -1;
    lock_guard<mutex> lock(mutex_);
    int32_t errCode = rdbStore->Insert(cmd, outRowId);
    if (errCode != NativeRdb::E_OK || outRowId < 0) {
        MEDIA_ERR_LOG("Insert into db failed, errCode = %{public}d", errCode);
        return E_HAS_DB_ERROR;
    }
    return static_cast<int32_t>(outRowId);
}
} // namespace Media
} // namespace OHOS
