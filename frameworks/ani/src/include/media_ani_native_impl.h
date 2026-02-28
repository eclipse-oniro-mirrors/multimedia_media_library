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

#ifndef __MEDIA_ANI_NATIVE_IMPL_H__
#define __MEDIA_ANI_NATIVE_IMPL_H__

#include <memory>
#include <string>
#include <vector>
#include "datashare_predicates.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "media_library_ani.h"

namespace OHOS {
namespace Media {
class MediaAniNativeImpl {
private:
    static std::shared_ptr<MediaLibraryAsyncContext> GetAssetsContext(ani_env *env,
        const std::vector<std::string> &fetchColumns, const DataShare::DataSharePredicates *predicate);
    static bool HandleSpecialPredicate(std::shared_ptr<MediaLibraryAsyncContext> context,
        const DataShare::DataSharePredicates *predicate, const FetchOptionType &fetchOptType);
    static bool IsPredicateValid(const DataShare::DataSharePredicates *predicate,
        const FetchOptionType &fetchOptType);
    static bool GetLocationPredicate(std::shared_ptr<MediaLibraryAsyncContext> context,
        const DataShare::DataSharePredicates *predicate);
    static bool AddDefaultAssetColumns(ani_env *env, std::vector<std::string> &fetchColumn,
        std::function<bool(const std::string &columnName)> isValidColumn,
        const PhotoAlbumSubType subType = PhotoAlbumSubType::USER_GENERIC);
    static bool PhotoAccessGetAssetsExecuteSync(std::shared_ptr<MediaLibraryAsyncContext> context,
        std::vector<std::unique_ptr<FileAsset>>& fileAssetArray);
    static bool PhotoAccessGetAssetsExecute(std::shared_ptr<MediaLibraryAsyncContext> context);
    static bool PhotoAccessGetFileAssetsInfoExecute(std::shared_ptr<MediaLibraryAsyncContext> context,
        std::vector<std::unique_ptr<FileAsset>> &fileAssetArray);
    static std::unique_ptr<FileAsset> GetNextRowFileAsset(shared_ptr<NativeRdb::ResultSet> resultSet);
    static void GetFileAssetField(int32_t index, string name, const shared_ptr<NativeRdb::ResultSet> resultSet,
        std::unique_ptr<FileAsset> &fileAsset);
    static bool ExtractSpecialFields(std::shared_ptr<MediaLibraryAsyncContext> context,
        const DataShare::OperationItem &item, const FetchOptionType &fetchOptType,
        std::vector<DataShare::OperationItem> &operations);

public:
    static std::vector<std::unique_ptr<FileAsset>> GetAssetsSync(ani_env *env,
        const std::vector<std::string> &fetchColumns, const DataShare::DataSharePredicates *predicate);
    static std::unique_ptr<FetchResult<FileAsset>> GetAssets(ani_env *env,
        const std::vector<std::string> &fetchColumns, const DataShare::DataSharePredicates *predicate);
    static std::vector<std::unique_ptr<FileAsset>> GetFileAssetsInfo(ani_env *env,
        const std::vector<std::string> &fetchColumns, const DataShare::DataSharePredicates *predicate);
};
} // namespace Media
} // namespace OHOS

#endif // __MEDIA_ANI_NATIVE_IMPL_H__