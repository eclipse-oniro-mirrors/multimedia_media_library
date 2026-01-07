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

#ifndef FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_DATABASE_OPERATIONS_H
#define FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_DATABASE_OPERATIONS_H

#include "enhancement_service_callback.h"

#include "file_asset.h"
#include "result_set.h"
#include "medialibrary_command.h"
#include "rdb_predicates.h"
#include "medialibrary_rdb_transaction.h"
#include "asset_accurate_refresh.h"
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EnhancementDatabaseOperations {
public:
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> Query(MediaLibraryCommand &cmd,
        NativeRdb::RdbPredicates &servicePredicates, const std::vector<std::string> &columns);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> BatchQuery(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns, std::unordered_map<int32_t, std::string> &fileId2Uri);
    EXPORT static int32_t Update(NativeRdb::ValuesBucket &rdbValues, NativeRdb::AbsRdbPredicates &predicates,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh = nullptr);
    EXPORT static int32_t InsertCloudEnhancementImageInDb(MediaLibraryCommand &cmd, const FileAsset &fileAsset,
        int32_t sourceFileId, std::shared_ptr<CloudEnhancementFileInfo> info,
        std::shared_ptr<NativeRdb::ResultSet> resultSet, std::shared_ptr<TransactionOperations> trans = nullptr);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> GetPair(MediaLibraryCommand &cmd);
    EXPORT static int64_t InsertCloudEnhancementPerm(int32_t sourceFileId, int32_t targetFileId);
    EXPORT static int32_t QueryAndUpdatePhotos(const std::vector<std::string> &photoIds);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_CLOUD_ENHANCEMENT_INCLUDE_ENHANCEMENT_DATABASE_OPERATIONS_H