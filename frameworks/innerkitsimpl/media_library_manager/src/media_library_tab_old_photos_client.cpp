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

#include <charconv>
#include <limits>
#include <string>
#include <vector>
#include <unordered_map>

#include "userfilemgr_uri.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_old_photos_column.h"
#include "medialibrary_errno.h"
#include "result_set_utils.h"
#include "datashare_helper.h"
#include "user_inner_ipc_client.h"
#include "medialibrary_business_code.h"
#include "get_uris_by_old_uris_inner_vo.h"

using namespace std;

namespace OHOS::Media {
    
static constexpr int32_t FIRST = 0;
static constexpr int32_t SECOND = 1;
static constexpr int32_t THIRD = 2;

std::unordered_map<std::string, std::string> TabOldPhotosClient::UrisByOldUrisTest(std::vector<std::string>& uris,
    std::vector<std::vector<int32_t>>& file_and_outFile_Ids,
    std::vector<std::vector<std::string>>& stringParams)
{
    std::vector<int32_t>& fileIds = file_and_outFile_Ids[FIRST];
    std::vector<int32_t> oldFileIds = file_and_outFile_Ids[SECOND];
    std::vector<std::string> datas = stringParams[FIRST];
    std::vector<std::string> displayNames = stringParams[SECOND];
    std::vector<std::string> oldDatas = stringParams[THIRD];
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> dataMapping;
    for (size_t i = 0; i < fileIds.size(); i++) {
        TabOldPhotosClient::TabOldPhotosClientObj obj;
        obj.fileId = fileIds[i];
        obj.data = datas[i];
        obj.displayName = displayNames[i];
        obj.oldFileId = oldFileIds[i];
        obj.oldData = oldDatas[i];
        dataMapping.emplace_back(obj);
    }
    std::vector<TabOldPhotosClient::RequestUriObj> uriList = this->Parse(uris);
    return this->Parse(dataMapping, uriList);
}

std::unordered_map<std::string, std::string> TabOldPhotosClient::GetUrisByOldUris(std::vector<std::string>& uris)
{
    std::unordered_map<std::string, std::string> resultMap;
    bool cond = (uris.empty() || static_cast<std::int32_t>(uris.size()) > this->URI_MAX_SIZE);
    CHECK_AND_RETURN_RET_LOG(!cond, resultMap, "the size is invalid, size = %{public}d",
        static_cast<std::int32_t>(uris.size()));
    std::vector<std::string> column;
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "file_id");
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "data");
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_file_id");
    column.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_data");
    column.push_back(PhotoColumn::PHOTOS_TABLE + "." + "display_name");
    return this->GetResultSetFromTabOldPhotos(uris, column);
}

int32_t TabOldPhotosClient::GetClonedAssetUrisInner(const std::vector<std::string> &uris,
    std::map<std::string, std::string> &clonedUris)
{
    clonedUris.clear();
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> dataMapping;
    int32_t ret = this->QueryClonedDataMapping(uris, dataMapping);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "QueryClonedDataMapping failed");
    clonedUris = this->BuildClonedResultMap(dataMapping, uris);
    return E_SUCCESS;
}

int32_t TabOldPhotosClient::QueryClonedDataMapping(const std::vector<std::string> &uris,
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> &dataMapping)
{
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_CLONED_ASSET_URIS);

    sptr<IRemoteObject> token = this->mediaLibraryManager_.InitToken();
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_HAS_DB_ERROR, "dataShareHelper is nullptr");
    GetUrisByOldUrisInnerReqBody reqBody;
    GetUrisByOldUrisInnerRespBody respBody;
    reqBody.uris = uris;
    int32_t result =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, reqBody, respBody);
    CHECK_AND_RETURN_RET_LOG(result == E_OK, result, "QueryClonedDataMapping IPC Call Failed");

    auto fileIds_size = respBody.fileIds.size();
    auto datas_size = respBody.datas.size();
    auto displayNames_size = respBody.displayNames.size();
    auto oldFileIds_size = respBody.oldFileIds.size();
    auto oldDatas_size = respBody.oldDatas.size();
    bool isValid = true;
    isValid &= fileIds_size == datas_size;
    isValid &= datas_size == displayNames_size;
    isValid &= displayNames_size == oldFileIds_size;
    isValid &= oldFileIds_size == oldDatas_size;
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVALID_VALUES, "QueryClonedDataMapping Failed");

    for (size_t i = 0; i < respBody.fileIds.size(); i++) {
        TabOldPhotosClient::TabOldPhotosClientObj obj;
        obj.fileId = respBody.fileIds[i];
        obj.data = respBody.datas[i];
        obj.displayName = respBody.displayNames[i];
        obj.oldFileId = respBody.oldFileIds[i];
        obj.oldData = respBody.oldDatas[i];
        dataMapping.emplace_back(obj);
    }
    return E_OK;
}

std::unordered_map<std::string, std::string> TabOldPhotosClient::GetResultSetFromTabOldPhotos(
    std::vector<std::string>& uris, std::vector<std::string> &columns)
{
    std::unordered_map<std::string, std::string> resultMap;
    sptr<IRemoteObject> token = this->mediaLibraryManager_.InitToken();
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, resultMap, "dataShareHelper is nullptr");
    GetUrisByOldUrisInnerReqBody reqBody;
    GetUrisByOldUrisInnerRespBody respBody;
    reqBody.uris = uris;
    reqBody.columns = columns;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URIS_BY_OLD_URIS);
    MEDIA_INFO_LOG("before IPC::UserDefineIPCClient().Call, INNER_GET_URIS_BY_OLD_URIS");
    int32_t result =
        IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, reqBody, respBody);
    CHECK_AND_RETURN_RET_LOG(result == E_OK, resultMap, "GetResultSetFromTabOldPhotos IPC Call Failed");
    auto fileIds_size = respBody.fileIds.size();
    auto datas_size = respBody.datas.size();
    auto displayNames_size = respBody.displayNames.size();
    auto oldFileIds_size = respBody.oldFileIds.size();
    auto oldDatas_size = respBody.oldDatas.size();
    bool isValid = true;
    isValid &= fileIds_size == datas_size;
    isValid &= datas_size == displayNames_size;
    isValid &= displayNames_size == oldFileIds_size;
    isValid &= oldFileIds_size == oldDatas_size;
    CHECK_AND_RETURN_RET_LOG(isValid, resultMap, "GetResultSetFromTabOldPhotos Failed");
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> dataMapping;
    for (size_t i = 0; i < respBody.fileIds.size(); i++) {
        TabOldPhotosClient::TabOldPhotosClientObj obj;
        obj.fileId = respBody.fileIds[i];
        obj.data = respBody.datas[i];
        obj.displayName = respBody.displayNames[i];
        obj.oldFileId = respBody.oldFileIds[i];
        obj.oldData = respBody.oldDatas[i];
        dataMapping.emplace_back(obj);
    }
    std::vector<TabOldPhotosClient::RequestUriObj> uriList = this->Parse(uris);
    return this->Parse(dataMapping, uriList);
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
    CHECK_AND_RETURN_RET_LOG(conditionCount != 0, E_FAIL, "Zero uri condition");
    return E_OK;
}

std::vector<TabOldPhotosClient::TabOldPhotosClientObj> TabOldPhotosClient::Parse(
    std::shared_ptr<DataShare::DataShareResultSet> &resultSet)
{
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> result;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "resultSet is null");
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

static bool StoiBoundCheck(std::string toCheck)
{
    const std::string intMax = std::to_string(std::numeric_limits<int>::max());

    return toCheck.length() < intMax.length();
}

std::map<std::string, std::string> TabOldPhotosClient::BuildClonedResultMap(
    const std::vector<TabOldPhotosClient::TabOldPhotosClientObj> &dataMapping,
    const std::vector<std::string> &uris)
{
    std::map<std::string, std::string> resultMap;
    for (const auto &uri : uris) {
        resultMap[uri] = "";
        std::string oldId = MediaFileUtils::GetIdFromUri(uri);
        int32_t oldFileId = 0;
        const char *end = oldId.data() + oldId.size();
        auto [parseEnd, parseErr] = std::from_chars(oldId.data(), end, oldFileId);
        if (oldId.empty() || parseErr != std::errc() || parseEnd != end) {
            MEDIA_WARN_LOG("BuildClonedResultMap: Failed to extract fileId from URI: %{private}s", uri.c_str());
            continue;
        }
        auto it = std::find_if(dataMapping.begin(), dataMapping.end(),
            [oldFileId](const TabOldPhotosClient::TabOldPhotosClientObj &obj) {
                return obj.oldFileId == oldFileId;
            });
        if (it == dataMapping.end()) {
            MEDIA_WARN_LOG("BuildClonedResultMap: No matching record found for oldFileId: %{public}d", oldFileId);
            continue;
        }
        resultMap[uri] = MediaFileUtils::GetUriByExtrConditions(
            PhotoColumn::PHOTO_URI_PREFIX,
            std::to_string(it->fileId),
            MediaFileUtils::GetExtraUri(it->displayName, it->data)
        );
    }
    return resultMap;
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
            if (lastSlashPos == std::string::npos || lastSlashPos + 1 >= uri.length()) {
                MEDIA_ERR_LOG("Error locating media id in media uri: %{public}s", uri.c_str());
                continue;
            }
            std::string idStr = uri.substr(lastSlashPos + 1);
            if (!(!idStr.empty() && std::all_of(idStr.begin(), idStr.end(), ::isdigit)) || !StoiBoundCheck(idStr)) {
                MEDIA_ERR_LOG("Media id is invalid in uri: %{public}s", uri.c_str());
                continue;
            }
            obj.type = URI_TYPE_ID_LINK;
            obj.oldFileId = std::stoi(idStr);
        } else if (uri.find(GALLERY_PATH) != std::string::npos) {
            obj.type = URI_TYPE_PATH;
            obj.oldData = uri;
        } else if (!uri.empty() && std::all_of(uri.begin(), uri.end(), ::isdigit) && StoiBoundCheck(uri)) {
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
        CHECK_AND_RETURN_RET(it == dataMapping.end(),
            std::make_pair(requestUriObj.requestUri, this->BuildRequestUri(*it)));
    }
    if (requestUriObj.type == URI_TYPE_PATH) {
        std::string oldData = requestUriObj.oldData;
        auto it = std::find_if(dataMapping.begin(),
            dataMapping.end(),
            [oldData](const TabOldPhotosClient::TabOldPhotosClientObj &obj) {return obj.oldData == oldData;});
        CHECK_AND_RETURN_RET(it == dataMapping.end(),
            std::make_pair(requestUriObj.requestUri, this->BuildRequestUri(*it)));
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
    }
    return resultMap;
}

std::unordered_map<std::string, std::string> TabOldPhotosClient::GetResultMap(
    std::shared_ptr<DataShareResultSet> &resultSet, std::vector<std::string> &queryTabOldPhotosUris)
{
    std::unordered_map<std::string, std::string> resultMap;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, resultMap, "resultSet is null");
    std::vector<TabOldPhotosClient::TabOldPhotosClientObj> dataMapping = this->Parse(resultSet);
    std::vector<TabOldPhotosClient::RequestUriObj> uriList = this->Parse(queryTabOldPhotosUris);
    return this->Parse(dataMapping, uriList);
}
} // namespace OHOS::Media
