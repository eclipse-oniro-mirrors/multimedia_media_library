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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_ANI_H

#include <memory>
#include <string>
#include <vector>
#include "avmetadatahelper.h"
#include "file_asset_ani.h"
#include "media_asset_edit_data.h"
#include "media_change_request_ani.h"
#include "photo_proxy.h"

namespace OHOS {
namespace Media {
enum class AssetChangeOperation {
    CREATE_FROM_SCRATCH,
    CREATE_FROM_URI,
    GET_WRITE_CACHE_HANDLER,
    ADD_RESOURCE,
    SET_EDIT_DATA,
    SET_FAVORITE,
    SET_HIDDEN,
    SET_TITLE,
    SET_USER_COMMENT,
    SET_MOVING_PHOTO_EFFECT_MODE,
    SET_PHOTO_QUALITY_AND_PHOTOID,
    SET_LOCATION,
    SET_CAMERA_SHOT_KEY,
    SAVE_CAMERA_PHOTO,
    ADD_FILTERS,
    DISCARD_CAMERA_PHOTO,
    SET_ORIENTATION,
    SET_SUPPORTED_WATERMARK_TYPE,
    SET_VIDEO_ENHANCEMENT_ATTR,
    SET_DISPLAY_NAME,
};

enum class AddResourceMode {
    DATA_BUFFER,
    FILE_URI,
    PHOTO_PROXY,
};

class MediaDataSource : public IMediaDataSource {
public:
    MediaDataSource(void* buffer, int64_t size) : buffer_(buffer), size_(size), readPos_(0) {}
    ~MediaDataSource() = default;

    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1) override;
    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    int32_t GetSize(int64_t &size) override;

private:
    int32_t ReadData(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length);

    void* buffer_;
    int64_t size_;
    int64_t readPos_;
};

struct MediaAssetChangeRequestAniContext;

class MediaAssetChangeRequestAni : public MediaChangeRequestAni {
public:
    explicit MediaAssetChangeRequestAni(FileAssetAni *fileAssetAni);
    ~MediaAssetChangeRequestAni();
    static ani_status Init(ani_env *env);
    static ani_status Constructor(ani_env *env, ani_object aniObject, ani_object fileAssetAni);
    static ani_object Wrap(ani_env *env, std::unique_ptr<MediaAssetChangeRequestAni> &changeRequest);
    static MediaAssetChangeRequestAni* Unwrap(ani_env *env, ani_object aniObject);
    ani_status ApplyChanges(ani_env *env) override;

    static ani_object CreateAssetRequestByPhotoCreateOptions(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_string displayName, ani_object photoCreateOptions);
    static ani_object CreateAssetRequestByCreateOptions(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_enum_item photoType, ani_string extension, ani_object createOptions);

    static ani_object CreateImageAssetRequest(ani_env *env, [[maybe_unused]] ani_class clazz, ani_object context,
        ani_string fileUri);
    static ani_object CreateVideoAssetRequest(ani_env *env, [[maybe_unused]] ani_class clazz, ani_object context,
        ani_string fileUri);
    static ani_object CreateAssetRequestFromRealPath(ani_env *env, const std::string &realPath);

    static ani_object AddResourceByFileUri(ani_env *env, ani_object aniObject, ani_enum_item resourceTypeAni,
        ani_string fileUri);
    static ani_object AddResourceByArrayBuffer(ani_env *env, ani_object aniObject,
        ani_enum_item resourceTypeAni, ani_arraybuffer arrayBuffer);
    static ani_object AddResourceByPhotoProxy(ani_env *env, ani_object aniObject, ani_enum_item resourceTypeAni,
        ani_object proxy);
    static ani_object AddMovingPhotoVideoResourceByFileUri(ani_env *env, ani_object aniObject, ani_string fileUri);
    static ani_object AddMovingPhotoVideoResourceByArrayBuffer(ani_env *env, ani_object aniObject,
        ani_arraybuffer arrayBuffer);

    static ani_object GetAsset(ani_env *env, ani_object aniObject);
    static ani_object DeleteAssets(ani_env *env, [[maybe_unused]] ani_class clazz, ani_object context,
        ani_object assets);

    static ani_object SetEditData(ani_env *env, ani_object aniObject, ani_object editData);
    static ani_object SetFavorite(ani_env *env, ani_object object, ani_boolean favoriteState);
    static ani_object SetHidden(ani_env *env, ani_object object, ani_boolean hiddenState);
    static ani_object SetUserComment(ani_env *env, ani_object object, ani_string userComment);
    static ani_object SetLocation(ani_env *env, ani_object object, ani_double longitude, ani_double latitude);
    static ani_object SetTitle(ani_env *env, ani_object object, ani_string title);
    static ani_object SetEffectMode(ani_env *env, ani_object aniObject, ani_enum_item mode);
    static ani_object SetCameraShotKey(ani_env *env, ani_object aniObject, ani_string shotKey);
    static ani_object SaveCameraPhoto(ani_env *env, ani_object aniObject);
    static ani_object SaveCameraPhotoByImageFileType(ani_env *env, ani_object aniObject,
        ani_enum_item imageFileTypeAni);
    static ani_object DiscardCameraPhoto(ani_env *env, ani_object aniObject);
    static ani_int GetWriteCacheHandler(ani_env *env, ani_object aniObject);
    static ani_object DeleteLocalAssetsPermanently(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object assets);
    static ani_object DeleteLocalAssetsPermanentlyWithUri(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object assetUris);
    static ani_object SetOrientation(ani_env *env, ani_object aniObject, ani_int orientation);
    static ani_object SetSupportedWatermarkType(ani_env *env, ani_object aniObject,
        ani_enum_item watermarkTypeAni);
    static ani_object SetVideoEnhancementAttr(ani_env *env, ani_object aniObject,
        ani_enum_item videoEnhancementType, ani_string photoId);
    void RecordChangeOperation(AssetChangeOperation changeOperation);
    bool Contains(AssetChangeOperation changeOperation) const;
    bool ContainsResource(ResourceType resourceType) const;
    bool IsMovingPhoto() const;
    bool CheckMovingPhotoResource(ResourceType resourceType) const;
    bool CheckEffectModeWriteOperation();
    bool CheckMovingPhotoWriteOperation();
    bool CheckChangeOperations(ani_env *env);
    int32_t CreateAssetBySecurityComponent(std::string &assetUri);
    int32_t CopyToMediaLibrary(bool isCreation, AddResourceMode mode);
    int32_t PutMediaAssetEditData(DataShare::DataShareValuesBucket &valuesBucket);
    void SetNewFileAsset(int32_t id, const std::string &uri);
    int32_t CopyFileToMediaLibrary(const OHOS::UniqueFd &destFd, bool isMovingPhotoVideo = false);
    int32_t CopyDataBufferToMediaLibrary(const OHOS::UniqueFd &destFd, bool isMovingPhotoVideo = false);
    int32_t CopyMovingPhotoVideo(const std::string &assetUri);

    std::shared_ptr<FileAsset> GetFileAssetInstance() const;
    sptr<PhotoProxy> GetPhotoProxyObj();
    void ReleasePhotoProxyObj();
    uint32_t FetchAddCacheFileId();
    void SetCacheFileName(std::string &fileName);
    void SetCacheMovingPhotoVideoName(std::string &fileName);
    std::string GetFileRealPath() const;
    AddResourceMode GetAddResourceMode() const;
    void* GetDataBuffer() const;
    size_t GetDataBufferSize() const;
    std::string GetMovingPhotoVideoPath() const;
    AddResourceMode GetMovingPhotoVideoMode() const;
    void* GetMovingPhotoVideoBuffer() const;
    size_t GetMovingPhotoVideoSize() const;
    void SetIsEditDisplayName(bool val);
    bool GetIsEditDisplayName();
    void SetOldDisplayName(const std::string &oldDisplayName);
    std::string GetOldDisplayName();
    void SetImageFileType(int32_t imageFileType);
    int32_t GetImageFileType();

    void SetIsWriteGpsAdvanced(bool val);
    bool GetIsWriteGpsAdvanced();
    void PutStringToCreationValue(const std::string &columnName, const std::string &val);

    int32_t SubmitCache(bool isCreation, bool isSetEffectMode, bool isWriteGpsAdvanced, const int32_t userId = -1);
    int32_t SubmitCacheWithCreation(
        std::string &uri, std::string &assetUri, bool isSetEffectMode, const int32_t userId);
    int32_t SubmitCacheWithoutCreation(std::string &uri, bool isSetEffectMode, bool isWriteGpsAdvanced,
        const int32_t userId);

private:
    static ani_object CreateAssetRequestInner(ani_env *env,
        std::unique_ptr<MediaAssetChangeRequestAniContext> &context);

    static std::atomic<uint32_t> cacheFileId_;
    sptr<PhotoProxy> photoProxy_ = nullptr;
    std::shared_ptr<FileAsset> fileAsset_;
    std::shared_ptr<MediaAssetEditData> editData_ = nullptr;
    std::string cacheFileName_;
    DataShare::DataShareValuesBucket creationValuesBucket_;
    std::vector<AssetChangeOperation> assetChangeOperations_;
    std::string realPath_;
    void* dataBuffer_ = nullptr;
    size_t dataBufferSize_ = 0;
    AddResourceMode addResourceMode_ = AddResourceMode::DATA_BUFFER;
    std::string movingPhotoVideoRealPath_;
    std::string cacheMovingPhotoVideoName_;
    void* movingPhotoVideoDataBuffer_ = nullptr;
    size_t movingPhotoVideoBufferSize_ = 0;
    AddResourceMode movingPhotoVideoResourceMode_ = AddResourceMode::DATA_BUFFER;
    std::vector<ResourceType> addResourceTypes_; // support adding resource multiple times
    int32_t imageFileType_;
    bool isWriteGpsAdvanced_{false};
    bool isEditDisplayName_{false};
    std::string oldDisplayName_;
};

struct MediaAssetChangeRequestAniContext : public AniError {
    MediaAssetChangeRequestAni* objectInfo;
    std::vector<AssetChangeOperation> assetChangeOperations;
    std::vector<ResourceType> addResourceTypes;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<std::string> uris;
    std::string appName;
    std::string realPath;
    int32_t fd;
    int32_t userId_ = -1;
    std::vector<std::string> fileIds;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_ANI_H