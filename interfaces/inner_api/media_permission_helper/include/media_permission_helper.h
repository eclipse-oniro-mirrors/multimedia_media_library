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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_PERMISSION_HELPER_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_PERMISSION_HELPER_H_

#include "datashare_helper.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
//LCOV_EXCL_START
namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;
#define EXPORT __attribute__ ((visibility ("default")))
enum class OperationMode : uint32_t {
    READ_MODE = 0b01,
    WRITE_MODE = 0b10,
    READ_WRITE_MODE = 0b11,
};
// LCOV_EXCL_START
class MediaPermissionHelper {
public:
    EXPORT MediaPermissionHelper() = default;
    EXPORT virtual ~MediaPermissionHelper() = default;

    /**
     * @brief Returns the Media Permission Helper Instance
     *
     * @return Returns the Media Permission Helper Instance
     * @since 1.0
     * @version 1.0
     */
    EXPORT static MediaPermissionHelper *GetMediaPermissionHelper();

    /**
     * @brief Initializes the environment for Media Permission Helper
     *
     * @since 1.0
     * @version 1.0
     */
    EXPORT void InitMediaPermissionHelper();

    /**
     * @brief Check PhotoUri Permission
     *
     * @param tokenId a parameter for input, indicating the expected app's tokenId to check
     * @param urisSource a parameter for input, indicating the source of URIs expected to check
     * @param result a parameter for output, indicating the check result (permission granted or not)
     * @param flags a parameter for input, indicating the expected type of permission check
     * @return If the check is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t CheckPhotoUriPermission(uint32_t tokenId,
        const std::vector<string> &urisSource, std::vector<bool> &result, const std::vector<uint32_t> &flags,
        bool readWriteIsolation = false);

    /**
     * @brief Check cloud download Permission
     *
     * @param tokenId a parameter for input, indicating the expected app's tokenId to check
     * @param uris a parameter for input, indicating the source of URIs expected to check
     * @param result a parameter for output, indicating the check result (permission granted or not)
     * @param flags a parameter for input, indicating the expected type of permission check
     * @return If the check is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t CheckCloudDownloadPermission(uint32_t tokenId,
        const std::vector<string> &uris, std::vector<bool> &result, const std::vector<uint32_t> &flags);

    /**
     * @brief Grant PhotoUri Permission
     *
     * @param strTokenId a parameter for input, indicating the calling sourceTokenId
     * @param targetTokenId a parameter for input, indicating the calling targetTokenId
     * @param uris a parameter for input, indicating the uris expected to grant permission
     * @param photoPermissionTypes a parameter for input, indicating the expected grant permission type for photos
     * @param hideSensitiveType a parameter for input, indicating the expected grant hideSensitiveType
     * @return If the grant is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t GrantPhotoUriPermission(uint32_t srcTokenId, uint32_t targetTokenId, const std::vector<string> &uris,
        const std::vector<PhotoPermissionType> &photoPermissionTypes, HideSensitiveType hideSensitiveTpye);

    /**
     * @brief Cancel PhotoUri Permission
     *
     * @param strTokenId a parameter for input, indicating the calling sourceTokenId
     * @param targetTokenId a parameter for input, indicating the calling targetTokenId
     * @param uris a parameter for input, indicating the uris expected to grant permission
     * @param persistFlag a parameter for cancel persist_permission or temporary_permission
     * @param OperationMode a parameter for cancel read_permission or write_permission
     * @return If the cancel is successful, return 0; otherwise, return -1 for failure.
     */
    EXPORT int32_t CancelPhotoUriPermission(uint32_t srcTokenId, uint32_t targetTokenId,
        const std::vector<string> &uris, const bool persistFlag = false,
        const std::vector<OperationMode> &mode = {OperationMode::READ_WRITE_MODE});

    /**
     * @brief check if the application has the corresponding permissions for uris
     *
     * @param targetTokenld token of the target application
     * @param uris query the list of uris
     * @param photoPermissionType permission type to be queried
     * @param result boolean result with permission or not
     * @return container for Boolean Results
     */
    EXPORT int32_t GetPhotoUrisPermission(uint32_t targetTokenld, const std::vector<string> &uris,
        const std::vector<PhotoPermissionType> &photoPermissionTypes, std::vector<bool> &result);

    /**
     * @brief convert path to URI
     *
     * @param paths path
     * @param uris uri
     * @return container converted to URI
     */
    EXPORT int32_t GetUrisFromFusePaths(const std::vector<std::string> paths, std::vector<std::string> &uris);

private:
    int32_t userId_;
    shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
    int32_t GetPhotoUrisPermission(uint32_t targetTokenId, const std::vector<string> &uris,
        PhotoPermissionType photoPermissionType, std::vector<bool> &result);
    bool ForceReconnect();
    int32_t QueryGrantedIndex(uint32_t targetTokenId, const std::string &uriType, const std::vector<string> &fileIds,
        std::map<std::string, pair<bool, bool>> &permissionMap, uint32_t businessCode);
};
} // namespace Media
} // namespace OHOS
// LCOV_EXCL_STOP
#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_PERMISSION_HELPER_H_
//LCOV_EXCL_STOP