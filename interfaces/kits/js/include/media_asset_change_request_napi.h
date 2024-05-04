/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_NAPI_H

#include <vector>
#include <buffer_handle_parcel.h>

#include "avmetadatahelper.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "file_asset_napi.h"
#include "media_asset_edit_data.h"
#include "media_change_request_napi.h"
#ifdef ABILITY_CAMERA_SUPPORT
#include "output/deferred_photo_proxy.h"
#endif
#include "unique_fd.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
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
    SET_PHOTO_QUALITY_AND_PHOTOID,
    SET_LOCATION,
    SET_CAMERA_SHOT_KEY,
    SAVE_CAMERA_PHOTO,
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

    int32_t ReadAt(const std::shared_ptr<AVSharedMemory>& mem, uint32_t length, int64_t pos = -1) override;
    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory>& mem) override;
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory>& mem) override;
    int32_t GetSize(int64_t& size) override;

private:
    int32_t ReadData(const std::shared_ptr<AVSharedMemory>& mem, uint32_t length);

    void* buffer_;
    int64_t size_;
    int64_t readPos_;
};

class MediaAssetChangeRequestNapi : public MediaChangeRequestNapi {
public:
    EXPORT MediaAssetChangeRequestNapi() = default;
    EXPORT ~MediaAssetChangeRequestNapi() override = default;

    EXPORT static napi_value Init(napi_env env, napi_value exports);

    std::shared_ptr<FileAsset> GetFileAssetInstance() const;
    bool Contains(AssetChangeOperation changeOperation) const;
    bool IsMovingPhoto() const;
    bool CheckMovingPhotoResource(ResourceType resourceType) const;
    std::string GetFileRealPath() const;
    AddResourceMode GetAddResourceMode() const;
    void* GetDataBuffer() const;
    size_t GetDataBufferSize() const;
    std::string GetMovingPhotoVideoPath() const;
    AddResourceMode GetMovingPhotoVideoMode() const;
    void* GetMovingPhotoVideoBuffer() const;
    size_t GetMovingPhotoVideoSize() const;
    std::string GetCacheMovingPhotoVideoName() const;
    void RecordChangeOperation(AssetChangeOperation changeOperation);
    void SetCacheFileName(std::string& fileName);
    void SetCacheMovingPhotoVideoName(std::string& fileName);
    int32_t SubmitCache(bool isCreation);
    int32_t CopyToMediaLibrary(bool isCreation, AddResourceMode mode);
    int32_t CreateAssetBySecurityComponent(std::string& assetUri);
    napi_value ApplyChanges(napi_env env, napi_callback_info info) override;
    #ifdef ABILITY_CAMERA_SUPPORT
    sptr<CameraStandard::DeferredPhotoProxy> GetPhotoProxyObj();
    void ReleasePhotoProxyObj();
    #endif

private:
    EXPORT static napi_value Constructor(napi_env env, napi_callback_info info);
    EXPORT static void Destructor(napi_env env, void* nativeObject, void* finalizeHint);

    EXPORT static napi_value JSGetAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCreateAssetRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCreateImageAssetRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCreateVideoAssetRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateAssetRequestFromRealPath(napi_env env, const std::string& realPath);
    EXPORT static napi_value JSDeleteAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetEditData(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetFavorite(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetHidden(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetTitle(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetUserComment(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetWriteCacheHandler(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSAddResource(napi_env env, napi_callback_info info);
    EXPORT static napi_value AddMovingPhotoVideoResource(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetLocation(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetCameraShotKey(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSaveCameraPhoto(napi_env env, napi_callback_info info);

    bool CheckChangeOperations(napi_env env);
    bool CheckMovingPhotoWriteOperation();
    int32_t PutMediaAssetEditData(DataShare::DataShareValuesBucket& valuesBucket);
    int32_t CopyFileToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo = false);
    int32_t CopyDataBufferToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo = false);
    int32_t CopyMovingPhotoVideo(const std::string& assetUri);
    void SetNewFileAsset(int32_t id, const std::string& uri);

    static thread_local napi_ref constructor_;
    #ifdef ABILITY_CAMERA_SUPPORT
    sptr<CameraStandard::DeferredPhotoProxy> photoProxy_ = nullptr;
    #endif
    std::shared_ptr<FileAsset> fileAsset_ = nullptr;
    std::shared_ptr<MediaAssetEditData> editData_ = nullptr;
    DataShare::DataShareValuesBucket creationValuesBucket_;
    std::string realPath_;
    std::string cacheFileName_;
    void* dataBuffer_;
    size_t dataBufferSize_;
    AddResourceMode addResourceMode_;
    std::string movingPhotoVideoRealPath_;
    std::string cacheMovingPhotoVideoName_;
    void* movingPhotoVideoDataBuffer_;
    size_t movingPhotoVideoBufferSize_;
    AddResourceMode movingPhotoVideoResourceMode_;
    std::vector<ResourceType> addResourceTypes_; // support adding resource multiple times
    std::vector<AssetChangeOperation> assetChangeOperations_;
};

struct MediaAssetChangeRequestAsyncContext : public NapiError {
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;

    MediaAssetChangeRequestNapi* objectInfo;
    std::vector<AssetChangeOperation> assetChangeOperations;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<std::string> uris;
    std::string appName;
    std::string realPath;
    int32_t fd;
};
} // namespace Media
} // namespace OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_NAPI_H