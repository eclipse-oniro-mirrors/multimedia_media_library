/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_MANAGER_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_MANAGER_H_
#define USERID "100"

#include "datashare_helper.h"
#include "media_volume.h"
#include "pixel_map.h"
#include "unique_fd.h"
#include "media_library_extend_manager.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;
#define EXPORT __attribute__ ((visibility ("default")))
struct UriParams;
class PhotoAlbum;
class FileAsset;
template <class T>
class FetchResult;

enum class CloudMediaRetainType : int32_t {
    RETAIN_FORCE = 0,
    HDC_RETAIN_FORCE,
};

class MediaLibraryManager {
public:
    EXPORT MediaLibraryManager() = default;
    EXPORT virtual ~MediaLibraryManager() = default;

    /**
     * @brief Returns the Media Library Manager Instance
     *
     * @return Returns the Media Library Manager Instance
     * @since 1.0
     * @version 1.0
     */
    EXPORT static MediaLibraryManager *GetMediaLibraryManager();

    /**
     * @brief Initializes the environment for Media Library Manager
     *
     * @param context The Ability context required for calling Data Ability Helper APIs
     * @since 1.0
     * @version 1.0
     */
    EXPORT void InitMediaLibraryManager(const sptr<IRemoteObject> &token);

    /**
     * @brief Initializes the environment for Media Library Manager
     *
     * @since 1.0
     * @version 1.0
     */
    EXPORT void InitMediaLibraryManager();

    /**
     * @brief Close an opened file
     *
     * @param uri source uri of a file which is to be closed
     * @param fd file descriptor for the file which is to be closed
     * @return close status. <0> for success and <-1> for fail
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t CloseAsset(const string &uri, const int32_t fd);

    /**
     * @brief create an photo or video asset
     *
     * @param displayName file displayName
     * @return asset uri for success and <""> for fail
     * @since 1.0
     * @version 1.0
     */
    EXPORT string CreateAsset(const string &displayName);

    /**
     * @brief open photo or video
     *
     * @param uri uri of the asset
     * @param openMode openMode "rw", "w", "r"
     * @return fileDescriptor for success and <-1> for fail
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t OpenAsset(string &uri, const string openMode);

    /**
     * @brief Obtain a mediaVolume object from MediaAssets can be obtained
     *
     * @param MediaVolume MediaVolume for outValue
     * @return errorcode
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t QueryTotalSize(MediaVolume &outMediaVolume);

    /**
     * @brief Query new uri by old uri
     *
     * @param uris old uris
     * @return map of old uris to new uris
     * @since 1.0
     * @version 1.0
     */
    EXPORT std::unordered_map<std::string, std::string> GetUrisByOldUris(std::vector<std::string> uris);

    /**
     * @brief Make a query from database
     *
     * @param columnName a column name in datebase
     * @param value a parameter for input which is a uri or path
     * @param columns query conditions
     * @return query result
     * @since 1.0
     * @version 1.0
     */
    EXPORT static std::shared_ptr<DataShareResultSet> GetResultSetFromDb(string columnName, const string &value,
        vector<string> &columns);

    /**
     * @brief get file path from uri
     *
     * @param fileUri a parameter for input  which is uri
     * @param filePath a parameter for output  which is path
     * @param userId  a parameter for user id
     * @return errorcode
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t GetFilePathFromUri(const Uri &fileUri, std::string &filePath, string userId = USERID);

    /**
     * @brief Obtain a mediaVolume object from MediaAssets can be obtained
     *
     * @param fileUri a parameter for output  which is uri
     * @param filePath a parameter for input  which is path
     * @param userId  a parameter for user id
     * @return errorcode
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t GetUriFromFilePath(const std::string &filePath, Uri &fileUri, string &userId);

    EXPORT std::unique_ptr<PixelMap> GetThumbnail(const Uri &uri);

    /**
     * @brief Obtain a batch of astc data
     *
     * @param uriBatch parameter for input, indicates the range of astc data that needs to be obtained
     * @param astcBatch parameter for output
     * @return if obtain success, return 0; Otherwise return error code.
     */
    EXPORT int32_t GetBatchAstcs(
        const std::vector<std::string> &uriBatch, std::vector<std::vector<uint8_t>> &astcBatch);

    /**
     * @brief Obtain pixelmap of astc
     *
     * @param uri a parameter for input which is uri
     * @return if obtain success, return PixelMap; Otherwise return nullptr
     */
    EXPORT std::unique_ptr<PixelMap> GetAstc(const Uri &uri);

    /**
     * @brief Open video of moving photo to read
     *
     * @param uri asset uri of the moving photo
     * @return read fd for success and <-1> for fail
     */
    EXPORT int32_t ReadMovingPhotoVideo(const string &uri);

    /**
     * @brief Open video of moving photo to read, support cloud file
     *
     * @param uri asset uri of the moving photo
     * @param offset offset of the video in fd
     * @return read fd for success and <-1> for fail
     */
    EXPORT int32_t ReadMovingPhotoVideo(const string &uri, off_t &offset);

    /**
     * @brief Open private moving photo to read
     *
     * @param uri asset uri of the moving photo
     * @return read fd for success and <-1> for fail
     */
    EXPORT int32_t ReadPrivateMovingPhoto(const string &uri);

    /**
     * @brief Get image uri of moving photo
     *
     * @param uri asset uri of the moving photo
     * @return image uri
     */
    EXPORT std::string GetMovingPhotoImageUri(const string &uri);

    /**
     * @brief Get date modified of moving photo
     *
     * @param uri asset uri of the moving photo
     * @return if obtain success, return date_modified; Otherwise return -1
     */
    EXPORT int64_t GetMovingPhotoDateModified(const string &uri);

    EXPORT static std::string GetSandboxPath(const std::string &path, const Size &size, bool isAstc);
    EXPORT static void GetUriIdPrefix(std::string &fileUri);
    EXPORT static bool IfSizeEqualsRatio(const Size &imageSize, const Size &targetSize);
    EXPORT static int32_t OpenReadOnlyAppSandboxVideo(const string& uri);
    EXPORT static int64_t GetSandboxMovingPhotoTime(const string& uri);
    EXPORT int32_t GetAstcYearAndMonth(const std::vector<string> &uris);

    sptr<IRemoteObject> InitToken();
    int32_t CheckResultSet(std::shared_ptr<DataShareResultSet> &resultSet);

    /**
     * @brief Get lpath by albumId, only supports user album and source album
     *
     * @param ownerAlbumId a parameter for input
     * @param lpath a parameter for output
     * @return errorCode
     */
    EXPORT int32_t GetAlbumLpath(uint32_t ownerAlbumId, std::string &lpath);

    /**
     * @brief Get lpaths by albumType, only supports user album and source album
     *
     * @param albumType a parameter for input
     * @param resultSet a parameter for output, including albumId and lpath
     * @return errorCode
     */
    EXPORT int32_t GetAlbumLpaths(uint32_t albumType, std::shared_ptr<DataShare::ResultSet> &resultSet);

    /**
     * @brief Retain CloudMediaAsset
     *
     * @param retainType a parameter for input, supports RETAIN_FORCE and HDC_RETAIN_FORCE
     * @return errorCode
     */
    EXPORT int32_t RetainCloudMediaAsset(CloudMediaRetainType retainType);

    /**
     * @brief Get Albums
     *
     * @param fetchColumns Indicates the members to query
     * @param predicate Rule options for getting albums
     * @return The albums obtained
     */
    EXPORT FetchResult<PhotoAlbum> GetAlbums(const std::vector<std::string> &fetchColumns,
        const DataShare::DataSharePredicates *predicate);

    /**
     * @brief Delete Albums
     *
     * @param albums Indicates the albums to delete
     * @return Number of albums successfully deleted or errorCode
     */
    EXPORT int32_t DeleteAlbums(const std::vector<std::unique_ptr<PhotoAlbum>> &albums);

    /**
     * @brief Create Album
     *
     * @param albumName Indicates the album name to create
     * @return Album id or errorCode
     */
    EXPORT int32_t CreateAlbum(const std::string &albumName);

    /**
     * @brief Delete Assets
     *
     * @param assets Indicates the assets to delete
     * @return Number of assets successfully deleted or errorCode
     */
    EXPORT int32_t DeleteAssets(const std::vector<std::unique_ptr<FileAsset>> &assets);

    /**
     * @brief Move Assets
     *
     * @param assets Indicates the assets to move
     * @param srcAlbum Indicates the album to move assets from
     * @param targetAlbum Indicates the album to move assets to
     * @return Number of assets successfully moved or errorCode
     */
    EXPORT int32_t MoveAssets(const std::vector<std::unique_ptr<FileAsset>> &assets, PhotoAlbum &srcAlbum,
        PhotoAlbum &targetAlbum);

    /**
     * @brief Get Assets
     * @param album Indicates the album to get assets from
     * @param fetchColumns Indicates the members to query
     * @param predicate Rule options for getting assets
     * @return The assets obtained
     */
    EXPORT FetchResult<FileAsset> GetAssets(const PhotoAlbum &album, const std::vector<std::string> &fetchColumns,
       const DataShare::DataSharePredicates *predicate);

private:
    int32_t ReadMovingPhotoVideo(const string &uri, const string &option);
    static int OpenThumbnail(std::string &uriStr, const std::string &path, const Size &size, bool isAstc);
    static unique_ptr<PixelMap> QueryThumbnail(UriParams& params);
    static unique_ptr<PixelMap> DecodeThumbnail(UniqueFd& uniqueFd, const Size& size, DecodeDynamicRange dynamicRange);
    static unique_ptr<PixelMap> GetPixelMapWithoutDecode(UniqueFd &uniqueFd, const Size& size);
    static unique_ptr<PixelMap> DecodeAstc(UniqueFd &uniqueFd);

    static shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    static sptr<IRemoteObject> token_;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_MANAGER_H_
