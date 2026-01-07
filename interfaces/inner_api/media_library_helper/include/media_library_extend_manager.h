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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_EXTEND_MANAGER_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_EXTEND_MANAGER_H_

#include "datashare_helper.h"
#include "media_app_uri_sensitive_column.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;
#define EXPORT __attribute__ ((visibility ("default")))

class MediaLibraryExtendManager {
public:
    EXPORT MediaLibraryExtendManager() = default;
    EXPORT virtual ~MediaLibraryExtendManager() = default;

    /**
     * @brief Returns the Media Library Manager Extend Instance
     *
     * @return Returns the Media Library Manager Extend Instance
     * @since 1.0
     * @version 1.0
     */
    EXPORT static MediaLibraryExtendManager *GetMediaLibraryExtendManager();

    /**
     * @brief Initializes the environment for Media Library Extend Manager
     *
     * @since 1.0
     * @version 1.0
     */
    EXPORT void InitMediaLibraryExtendManager();

    /**
     * @brief open photo or video
     *
     * @param uri uri of the asset
     * @param openMode openMode "rw", "w", "r"
     * @param type force sensitive type
     * @return fileDescriptor for success and <-1> for fail
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t OpenAsset(string &uri, const string openMode, HideSensitiveType type);

    /**
     * @brief Open private moving photo to read
     *
     * @param uri asset uri of the moving photo
     * @param type force type
     * @return read fd for success and <-1> for fail
     */
    EXPORT int32_t ReadPrivateMovingPhoto(string &uri, const HideSensitiveType type);

    /**
     * @brief query photo by condition of input
     *
     * @param value asset uri
     * @param columns result columns
     * @return resultset of query
     */
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> GetResultSetFromPhotos(const string &value,
        vector<string> &columns);

    /**
     * @brief query photo by condition of input
     *
     * @param columnName query columnName
     * @param value query condition
     * @param columns result columns
     * @return resultset of query
     */
    EXPORT std::shared_ptr<DataShare::DataShareResultSet> GetResultSetFromDb(string columnName,
        const string &value, vector<string> &columns);

    /**
     * @brief send broker change operation
     *
     * @param columns columns
     * @return send ok or not
     */
    EXPORT int32_t SendBrokerChangeOperation(string operation);

    /**
     * @brief Open photo or video compress with edit data
     *
     * @param uri uri of the asset
     * @param type force sensitive type
     * @param version compress version
     * @return read fd for success and <-1> for fail
     */
    EXPORT int32_t OpenAssetCompress(const string &uri, HideSensitiveType type, int32_t version);

    /**
     * @brief notify asset compress sended
     *
     * @param uri uri of the asset
     * @return notify ok or not
     */
    EXPORT int32_t NotifyAssetSended(const string &uri);

    /**
     * @brief Get total compressed size of assets
     *
     * @param uris list of asset URIs (max 500)
     * @return total size in bytes on success, error code otherwise
     */
    EXPORT int64_t GetCompressAssetSize(const std::vector<std::string> &uris);

    /**
     * @brief get asset compress version
     *
     * @return asset compress version
     */
    EXPORT int32_t GetAssetCompressVersion();
private:

    int32_t userId_;
    shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
    bool ForceReconnect();
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_EXTEND_MANAGER_H_