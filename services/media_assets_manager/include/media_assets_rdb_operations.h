/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ASSETS_RDB_OPERATIONS_H
#define OHOS_MEDIA_ASSETS_RDB_OPERATIONS_H

#include <stdint.h>
#include <string>
#include <mutex>
#include <map>

#include "rdb_predicates.h"
#include "medialibrary_rdb_transaction.h"
#include "file_asset.h"
#include "medialibrary_operation.h"
#include "datashare_values_bucket.h"
#include "datashare_predicates.h"
#include "query_cloud_enhancement_task_state_dto.h"
#include "datashare_result_set.h"

namespace OHOS::Media {

class MediaAssetsRdbOperations {
public:
    MediaAssetsRdbOperations();
    ~MediaAssetsRdbOperations() = default;
    int32_t RemoveFormInfo(const std::string& formId);
    int32_t RemoveGalleryFormInfo(const std::string& formId);
    int32_t SaveFormInfo(const std::string& formId, const std::string& uri);
    int32_t SaveGalleryFormInfo(const std::vector<std::string>& formIds,
        const std::vector<std::string>& fileUris);
    int32_t RevertToOrigin(const int32_t &fileId);

    void QueryAssetsUri(const std::vector<std::string> &fileIds, std::vector<std::string> &uris);

    bool QueryAlbumIdIfExists(const std::string& albumId);
    bool QueryFileIdIfExists(const std::string& fileId);
    bool QueryFormIdIfExists(const std::string& formId);
    int32_t CommitEditInsert(const std::string& editData, int32_t fileId);
    int32_t QueryEnhancementTaskState(const std::string& photoUri, QueryCloudEnhancementTaskStateDto& dto);
    static void DeleteFromVisionTables(const std::string& fileId);

    int32_t GrantPhotoUriPermission(MediaLibraryCommand &cmd);
    int32_t GrantPhotoUrisPermission(
        MediaLibraryCommand &cmd, const std::vector<DataShare::DataShareValuesBucket> &values);
    int32_t CancelPhotoUriPermission(NativeRdb::RdbPredicates &rdbPredicate);
    int32_t StartThumbnailCreationTask(NativeRdb::RdbPredicates &rdbPredicate, int32_t requestId, pid_t pid);
    int32_t StopThumbnailCreationTask(int32_t requestId, pid_t pid);
    int32_t CancelPhotoUrisPermissionInner(MediaLibraryCommand &cmd,
        const DataShare::DataSharePredicates &values);
    int32_t CheckPhotoUriPermissionInner(MediaLibraryCommand &cmd, const DataShare::DataSharePredicates &predicates,
        const std::vector<std::string> &columns, std::vector<std::string> &outFileIds,
        std::vector<int32_t> &permissionTypes);
    int32_t GrantPhotoUrisPermissionInner(
        MediaLibraryCommand &cmd, const std::vector<DataShare::DataShareValuesBucket> &values);
    int32_t UpdateTmpCompatibleDup(int32_t fileId, size_t size, bool validate = true);
    std::shared_ptr<DataShare::DataShareResultSet> GetUrisByOldUrisInner(MediaLibraryCommand &cmd,
        const DataShare::DataSharePredicates &predicates, const std::vector<std::string> &columns);

private:
    static std::shared_ptr<FileAsset> GetFileAssetFromDb(const std::string &column, const std::string &value,
        OperationObject oprnObject, const std::vector<std::string> &columns = {}, const std::string &networkId = "");
    static std::mutex facardMutex_;
};

} // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_RDB_OPERATIONS_H