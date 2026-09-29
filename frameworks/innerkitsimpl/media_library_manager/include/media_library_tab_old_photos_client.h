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

#ifndef FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_TAB_OLD_PHOTOS_CLIENT_H
#define FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_TAB_OLD_PHOTOS_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include "datashare_result_set.h"
#include "file_uri.h"
#include "datashare_predicates.h"
#include "media_library_manager.h"

namespace OHOS::Media {
class TabOldPhotosClient {
private:
    struct TabOldPhotosClientObj {
        int32_t fileId;
        std::string data;
        std::string displayName;
        int32_t oldFileId;
        std::string oldData;
    };

    struct RequestUriObj {
        int32_t type;
        int32_t oldFileId;
        std::string oldData;
        std::string requestUri;
    };

public:
    TabOldPhotosClient(MediaLibraryManager &mediaLibraryManager) : mediaLibraryManager_(mediaLibraryManager) {};
    std::unordered_map<std::string, std::string> GetUrisByOldUris(std::vector<std::string>& uris);
    int32_t GetClonedAssetUrisInner(const std::vector<std::string> &uris,
        std::map<std::string, std::string> &clonedUris);
    std::unordered_map<std::string, std::string> UrisByOldUrisTest(std::vector<std::string>& uris,
        std::vector<std::vector<int32_t>>& file_and_outFile_Ids,
        std::vector<std::vector<std::string>>& stringParams);

private:
    int BuildPredicates(const std::vector<std::string> &queryTabOldPhotosUris,
        DataShare::DataSharePredicates &predicates);
    std::vector<TabOldPhotosClientObj> Parse(std::shared_ptr<DataShare::DataShareResultSet> &resultSet);
    std::vector<RequestUriObj> Parse(std::vector<std::string> &queryTabOldPhotosUris);
    std::string BuildRequestUri(const TabOldPhotosClientObj &dataObj);
    std::pair<std::string, std::string> Build(
        const RequestUriObj &requestUriObj, const std::vector<TabOldPhotosClientObj> &dataMapping);
    std::unordered_map<std::string, std::string> Parse(
        const std::vector<TabOldPhotosClientObj> &dataMapping, std::vector<RequestUriObj> &uriList);
    std::unordered_map<std::string, std::string> GetResultMap(
        std::shared_ptr<DataShare::DataShareResultSet> &resultSet, std::vector<std::string> &queryTabOldPhotosUris);
    std::unordered_map<std::string, std::string> GetResultSetFromTabOldPhotos(
        std::vector<std::string>& uris, std::vector<std::string> &columns);
    int32_t QueryClonedDataMapping(const std::vector<std::string> &uris,
        std::vector<TabOldPhotosClientObj> &dataMapping);
    std::map<std::string, std::string> BuildClonedResultMap(
        const std::vector<TabOldPhotosClientObj> &dataMapping, const std::vector<std::string> &uris);

private:
    const int32_t URI_MAX_SIZE = 100;
    const std::string COLUMN_FILE_ID = "file_id";
    const std::string COLUMN_DATA = "data";
    const std::string COLUMN_OLD_FILE_ID = "old_file_id";
    const std::string COLUMN_OLD_DATA = "old_data";
    const std::string COLUMN_DISPLAY_NAME = "display_name";
    enum { URI_TYPE_DEFAULT, URI_TYPE_ID_LINK, URI_TYPE_PATH, URI_TYPE_ID };
    MediaLibraryManager &mediaLibraryManager_;
};
} // namespace OHOS::Media

#endif // FRAMEWORKS_INNERKITSIMPL_MEDIA_LIBRARY_TAB_OLD_PHOTOS_CLIENT_H