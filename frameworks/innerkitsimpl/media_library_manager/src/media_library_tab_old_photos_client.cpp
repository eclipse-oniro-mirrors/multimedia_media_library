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
#include "media_library_tab_old_photos_client.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "userfilemgr_uri.h"
#include "media_column.h"
#include "media_log.h"
#include "media_old_photos_column.h"
#include "medialibrary_errno.h"
#include "result_set_utils.h"
#include "datashare_helper.h"

using namespace std;

namespace OHOS::Media {
std::unordered_map<std::string, std::string> TabOldPhotosClient::GetUrisByOldUris(std::vector<std::string> uris)
{
    std::unordered_map<std::string, std::string> resultMap;
    if (uris.empty() || static_cast<std::int32_t>(uris.size()) > this->URI_MAX_SIZE) {
        MEDIA_ERR_LOG("the size is invalid, size = %{public}d", static_cast<std::int32_t>(uris.size()));
        return resultMap;
    }
    std::string queryUri = QUERY_TAB_OLD_PHOTO;
    Uri uri(queryUri);
    DataShare::DataSharePredicates predicates;
    int ret = BuildPredicates(uris, predicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("build predicates failed");
        return resultMap;
    }
    std::vector<std::string> column;
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "file_id");
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "data");
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_file_id");
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_data");
    column.push_back(PhotoColumn::PHOTOS_TABLE + "." + "display_name");
    int errCode = 0;
    std::shared_ptr<DataShare::DataShareResultSet> dataShareResultSet =
        this->GetResultSetFromTabOldPhotos(uri, predicates, column, errCode);
    if (dataShareResultSet == nullptr) {
        MEDIA_ERR_LOG("query failed");
        return resultMap;
    }
    return this->GetResultMap(dataShareResultSet, uris);
}

std::shared_ptr<DataShare::DataShareResultSet> TabOldPhotosClient::GetResultSetFromTabOldPhotos(
    Uri &uri, const DataShare::DataSharePredicates &predicates, std::vector<std::string> &columns, int &errCode)
{
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
    DataShare::DatashareBusinessError businessError;
    sptr<IRemoteObject> token = this->mediaLibraryManager_.InitToken();
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    if (dataShareHelper == nullptr) {
        MEDIA_ERR_LOG("dataShareHelper is nullptr");
        return nullptr;
    }
    resultSet = dataShareHelper->Query(uri, predicates, columns, &businessError);
    int count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Resultset check failed, ret: %{public}d", ret);
        return nullptr;
    }
    return resultSet;
}

int TabOldPhotosClient::BuildPredicates(const std::vector<std::string> &queryTabOldPhotosUris,
    DataShare::DataSharePredicates &predicates)
{
    const std::string GALLERY_URI_PREFIX = "//media";
    const std::string GALLERY_PATH = "/storage/emulated";

    vector<string> clauses;
        clauses.push_back(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " +
        TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + TabOldPhotosColumn::MEDIA_ID);
    predicates.InnerJoin(PhotoColumn::PHOTOS_TABLE)->On(clauses);

    int conditionCount = 0;
    for (const auto &uri : queryTabOldPhotosUris) {
        if (uri.find(GALLERY_URI_PREFIX) != std::string::npos) {
            size_t lastSlashPos = uri.rfind('/');
            if (lastSlashPos != std::string::npos && lastSlashPos + 1 < uri.length()) {
                std::string idStr = uri.substr(lastSlashPos + 1);
                predicates.Or()->EqualTo(TabOldPhotosColumn::MEDIA_OLD_ID, idStr);
                conditionCount += 1;
            }
        } else if (uri.find(GALLERY_PATH) != std::string::npos) {
            predicates.Or()->EqualTo(TabOldPhotosColumn::MEDIA_OLD_FILE_PATH, uri);
            conditionCount += 1;
        } else if (!uri.empty() && std::all_of(uri.begin(), uri.end(), ::isdigit)) {
            predicates.Or()->EqualTo(TabOldPhotosColumn::MEDIA_OLD_ID, uri);
            conditionCount += 1;
        }
    }
    if (conditionCount == 0) {
        MEDIA_ERR_LOG("Zero uri condition");
        return E_FAIL;
    }

    return E_OK;
}

std::vector<TabOldPhotosClient::TabOldPhotosClientObj> TabOldPhotosClient::Parse(
    std::shared_ptr<DataShare::DataShareResultSet> &resultSet)
{
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> result;
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        TabOldPhotosClient::TabOldPhotosClientObj obj;
        obj.fileId = GetInt32Val(this->COLUMN_FILE_ID, resultSet);
        obj.data = GetStringVal(this->COLUMN_DATA, resultSet);
        obj.displayName = GetStringVal(this->COLUMN_DISPLAY_NAME, resultSet);
        obj.oldFileId = GetInt32Val(this->COLUMN_OLD_FILE_ID, resultSet);
        obj.oldData = GetStringVal(this->COLUMN_OLD_DATA, resultSet);
        result.emplace_back(obj);
    }
    return result;
}

std::vector<TabOldPhotosClient::RequestUriObj> TabOldPhotosClient::Parse(
    std::vector<std::string> &queryTabOldPhotosUris)
{
    const std::string GALLERY_URI_PREFIX = "//media";
    const std::string GALLERY_PATH = "/storage/emulated";
    std::vector<TabOldPhotosClient::RequestUriObj> result;
    for (const auto &uri : queryTabOldPhotosUris) {
        TabOldPhotosClient::RequestUriObj obj;
        obj.type = URI_TYPE_DEFAULT;
        obj.requestUri = uri;

        if (uri.find(GALLERY_URI_PREFIX) != std::string::npos) {
            size_t lastSlashPos = uri.rfind('/');
            if (lastSlashPos != std::string::npos && lastSlashPos + 1 < uri.length()) {
                std::string idStr = uri.substr(lastSlashPos + 1);
                obj.type = URI_TYPE_ID_LINK;
                obj.oldFileId = std::stoi(idStr);
            }
        } else if (uri.find(GALLERY_PATH) != std::string::npos) {
            obj.type = URI_TYPE_PATH;
            obj.oldData = uri;
        } else if (!uri.empty() && std::all_of(uri.begin(), uri.end(), ::isdigit)) {
            int oldFileId = std::stoi(uri);
            obj.type = URI_TYPE_ID;
            obj.oldFileId = oldFileId;
        }
        if (obj.type == URI_TYPE_DEFAULT) {
            continue;
        }
        result.emplace_back(obj);
    }
    return result;
}

std::string TabOldPhotosClient::BuildRequestUri(const TabOldPhotosClient::TabOldPhotosClientObj &dataObj)
{
    std::string filePath = dataObj.data;
    std::string displayName = dataObj.displayName;
    int32_t fileId = dataObj.fileId;
    std::string baseUri = "file://media";
    size_t lastSlashInData = filePath.rfind('/');
    std::string fileNameInData =
        (lastSlashInData != std::string::npos) ? filePath.substr(lastSlashInData + 1) : filePath;
    size_t dotPos = fileNameInData.rfind('.');
    if (dotPos != std::string::npos) {
        fileNameInData = fileNameInData.substr(0, dotPos);
    }
    return baseUri + "/Photo/" + std::to_string(fileId) + "/" + fileNameInData + "/" + displayName;
}

std::pair<std::string, std::string> TabOldPhotosClient::Build(const TabOldPhotosClient::RequestUriObj &requestUriObj,
    const std::vector<TabOldPhotosClient::TabOldPhotosClientObj> &dataMapping)
{
    if (requestUriObj.type == URI_TYPE_ID_LINK || requestUriObj.type == URI_TYPE_ID) {
        int32_t oldFileId = requestUriObj.oldFileId;
        auto it = std::find_if(dataMapping.begin(),
            dataMapping.end(),
            [oldFileId](const TabOldPhotosClient::TabOldPhotosClientObj &obj) {return obj.oldFileId == oldFileId;});
        if (it != dataMapping.end()) {
            return std::make_pair(requestUriObj.requestUri, this->BuildRequestUri(*it));
        }
    }
    if (requestUriObj.type == URI_TYPE_PATH) {
        std::string oldData = requestUriObj.oldData;
        auto it = std::find_if(dataMapping.begin(),
            dataMapping.end(),
            [oldData](const TabOldPhotosClient::TabOldPhotosClientObj &obj) {return obj.oldData == oldData;});
        if (it != dataMapping.end()) {
            return std::make_pair(requestUriObj.requestUri, this->BuildRequestUri(*it));
        }
    }
    return std::make_pair(requestUriObj.requestUri, "");
}

std::unordered_map<std::string, std::string> TabOldPhotosClient::Parse(
    const std::vector<TabOldPhotosClient::TabOldPhotosClientObj> &dataMapping, std::vector<RequestUriObj> &uriList)
{
    std::unordered_map<std::string, std::string> resultMap;
    for (const auto &requestUriObj : uriList) {
        std::pair<std::string, std::string> pair = this->Build(requestUriObj, dataMapping);
        resultMap[pair.first] = pair.second;
        MEDIA_INFO_LOG("Request URI = %{public}s , Resulting URI = %{public}s",
            pair.first.c_str(), pair.second.c_str());
    }
    return resultMap;
}

std::unordered_map<std::string, std::string> TabOldPhotosClient::GetResultMap(
    std::shared_ptr<DataShareResultSet> &resultSet, std::vector<std::string> &queryTabOldPhotosUris)
{
    std::unordered_map<std::string, std::string> resultMap;
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return resultMap;
    }
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> dataMapping = this->Parse(resultSet);
    std::vector<TabOldPhotosClient::RequestUriObj> uriList = this->Parse(queryTabOldPhotosUris);
    return this->Parse(dataMapping, uriList);
}
} // namespace OHOS::Media
