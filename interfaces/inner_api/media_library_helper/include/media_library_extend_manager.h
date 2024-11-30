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
#define USERID "100"

#include "datashare_helper.h"
#include "unique_fd.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;
#define EXPORT __attribute__ ((visibility ("default")))
/**
 * @brief Interface for accessing all the File operation and AlbumAsset operation APIs
 *
 * @since 1.0
 * @version 1.0
 */
enum class PhotoPermissionType : int32_t {
    TEMPORARY_READ_IMAGEVIDEO = 0,
    PERSIST_READ_IMAGEVIDEO,
    TEMPORARY_WRITE_IMAGEVIDEO,
    TEMPORARY_READWRITE_IMAGEVIDEO,
    PERSIST_READWRITE_IMAGEVIDEO,
};

enum class HideSensitiveType : int32_t {
    ALL_DESENSITIZE = 0,
    GEOGRAPHIC_LOCATION_DESENSITIZE,
    SHOOTING_PARAM_DESENSITIZE,
    NO_DESENSITIZE
};

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
     * @brief Check PhotoUri Permission
     *
     * @param tokenId a parameter for input, indicating the expected app's tokenId to check
     * @param urisSource a parameter for input, indicating the source of URIs expected to check
     * @param result a parameter for output, indicating the check result (permission granted or not)
     * @param flag a parameter for input, indicating the expected type of permission check
     * @return If the check is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t CheckPhotoUriPermission(uint32_t tokenId,
        const std::vector<string> &urisSource, std::vector<bool> &result, uint32_t flag);

    /**
     * @brief Grant PhotoUri Permission
     *
     * @param strTokenId a parameter for input, indicating the calling sourceTokenId
     * @param targetTokenId a parameter for input, indicating the calling targetTokenId
     * @param uris a parameter for input, indicating the uris expected to grant permission
     * @param photoPermissionType a parameter for input, indicating the expected grant permission type for photos
     * @param hideSensitiveType a parameter for input, indicating the expected grant hideSensitiveType
     * @return If the grant is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t GrantPhotoUriPermission(uint32_t srcTokenId, uint32_t targetTokenId, const std::vector<string> &uris,
        PhotoPermissionType photoPermissionType, HideSensitiveType hideSensitiveTpye);

    /**
     * @brief Cancel PhotoUri Permission
     *
     * @param strTokenId a parameter for input, indicating the calling sourceTokenId
     * @param targetTokenId a parameter for input, indicating the calling targetTokenId
     * @param uris a parameter for input, indicating the uris expected to grant permission
     * @return If the cancel is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t CancelPhotoUriPermission(uint32_t srcTokenId, uint32_t targetTokenId,
        const std::vector<string> &uris);
private:

    shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
    int32_t CheckPhotoUriPermissionQueryOperation(const DataShare::DataSharePredicates &predicates,
        std::map<string, int32_t> &resultMap);
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_EXTEND_MANAGER_H_