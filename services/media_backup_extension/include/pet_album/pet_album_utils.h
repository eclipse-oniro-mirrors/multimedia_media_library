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

#ifndef PET_ALBUM_UTILS_H
#define PET_ALBUM_UTILS_H

#include <memory>
#include <string>
#include <vector>
#include "backup_const.h"
#include "backup_const_column.h"
#include "rdb_store.h"

namespace OHOS {
namespace Media {


/**
 * @brief Pet album utility class for delete operations
 */
class PetAlbumUtils {
public:
    /**
     * @brief Delete existing Pet album data
     * @param rdbStore Database store
     * @return int32_t Error code, E_OK for success
     */
    static int32_t DeleteExistingAlbumData(std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    /**
     * @brief Delete Pet album data only
     * @param rdbStore Database store
     * @return int32_t Error code, E_OK for success
     */
    static int32_t DeletePetAlbumData(std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    /**
     * @brief Delete analysis albums by type and subtype
     * @param rdbStore Database store
     * @param albumType Album type
     * @param albumSubtype Album subtype
     * @return int32_t Error code, E_OK for success
     */
    static int32_t DeleteAnalysisAlbums(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        int32_t albumType, int32_t albumSubtype);

    /**
     * @brief Delete pet tag table data
     * @param rdbStore Database store
     * @return int32_t Error code, E_OK for success
     */
    static int32_t DeletePetTagData(std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    /**
     * @brief Delete pet face data with optional file ID filter
     * @param rdbStore Database store
     * @return int32_t Error code, E_OK for success
     */
    static int32_t DeletePetFaceData(std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    /**
     * @brief Delete analysis photo map data for specific album type
     * @param rdbStore Database store
     * @param albumType Album type
     * @param albumSubtype Album subtype
     * @return int32_t Error code, E_OK for success
     */
    static int32_t DeleteAnalysisPhotoMapData(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        int32_t albumType, int32_t albumSubtype);

    /**
     * @brief Update analysis total table pet status
     * @param rdbStore Database store
     * @return int32_t Error code, E_OK for success
     */
    static int32_t UpdateAnalysisTotalPetStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    /**
     * @brief Update analysis search index cv status
     * @param rdbStore Database store
     * @return int32_t Error code, E_OK for success
     */
    static int32_t UpdateAnalysisSearchIndexCvStatus(std::shared_ptr<NativeRdb::RdbStore> rdbStore);

    /**
     * @brief Get album IDs by type and subtype
     * @param rdbStore Database store
     * @param albumType Album type
     * @param albumSubtype Album subtype
     * @param albumIds Output vector for album IDs
     * @return int32_t Error code, E_OK for success
     */
    static int32_t GetAlbumIdsByType(std::shared_ptr<NativeRdb::RdbStore> rdbStore,
        int32_t albumType, int32_t albumSubtype, std::vector<std::string>& albumIds);

private:
    /**
     * @brief Execute SQL with error handling
     * @param rdbStore Database store
     * @param sql SQL statement
     * @return int32_t Error code, E_OK for success
     */
    static int32_t ExecuteSQLSafe(std::shared_ptr<NativeRdb::RdbStore> rdbStore, const std::string& sql);
};

} // namespace Media
} // namespace OHOS

#endif // Pet_ALBUM_UTILS_H