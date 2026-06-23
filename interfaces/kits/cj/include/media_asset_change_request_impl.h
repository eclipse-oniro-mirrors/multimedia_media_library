/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MEDIA_ASSET_CHANGE_REQUES_IMPL_H
#define MEDIA_ASSET_CHANGE_REQUES_IMPL_H

#include <vector>

#include "avmetadatahelper.h"
#include "datashare_helper.h"
#include "ffi_remote_data.h"
#include "media_asset_edit_data.h"
#include "media_change_request_impl.h"
#include "photo_asset_impl.h"
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
    SET_HAS_APPLINK,
    SET_APPLINK,
    SET_VIDEO_ENHANCEMENT_ATTR,
    SET_COMPOSITE_DISPLAY_MODE,
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

class MediaAssetChangeRequestImpl : public OHOS::FFI::FFIData, public MediaChangeRequestImpl {
    DECL_TYPE(MediaAssetChangeRequestImpl, OHOS::FFI::FFIData)
public:
    MediaAssetChangeRequestImpl() = default;
    ~MediaAssetChangeRequestImpl() override = default;

    std::shared_ptr<FileAsset> GetFileAssetInstance() const;
    bool Contains(AssetChangeOperation changeOperation) const;
    bool ContainsResource(ResourceType resourceType) const;
    bool IsMovingPhoto() const;
    bool CheckMovingPhotoResource(ResourceType resourceType) const;
    std::string GetMovingPhotoVideoPath() const;
    std::string GetFileRealPath() const;
    AddResourceMode GetAddResourceMode() const;
    const uint8_t* GetDataBuffer() const;
    size_t GetDataBufferSize() const;
    AddResourceMode GetMovingPhotoVideoMode() const;
    const uint8_t* GetMovingPhotoVideoBuffer() const;
    size_t GetMovingPhotoVideoSize() const;
    void RecordChangeOperation(AssetChangeOperation changeOperation);
    uint32_t FetchAddCacheFileId();
    void SetCacheFileName(std::string& fileName);
    void SetCacheMovingPhotoVideoName(std::string& fileName);
    int32_t SubmitCache(bool isCreation, bool isSetEffectMode);
    int32_t CopyToMediaLibrary(bool isCreation, AddResourceMode mode);
    int32_t CreateAssetBySecurityComponent(std::string& assetUri);
    int32_t PutMediaAssetEditData(DataShare::DataShareValuesBucket& valuesBucket);
    int32_t GetImageFileType();

    MediaAssetChangeRequestImpl(std::shared_ptr<FileAsset> fileAssetPtr);
    MediaAssetChangeRequestImpl(int64_t contextId, const std::string& filePath, MediaType meidiaType, int32_t* errCode);
    MediaAssetChangeRequestImpl(int64_t contextId, int32_t photoType, std::string extension, std::string title,
        int32_t subType, int32_t* errCode);

    int32_t CJDeleteAssets(int64_t contextId, std::vector<std::string> uris);
    int64_t CJGetAsset(int32_t* errCode);
    int32_t CJSetTitle(std::string title);
    int32_t CJGetWriteCacheHandler(int32_t* errCode);
    int32_t CJAddResource(int32_t resourceType, std::string fileUri);
    int32_t CJAddResource(int32_t resourceType, uint8_t* dataBuffer, size_t dataBufferSize);
    int32_t AddMovingPhotoVideoResource(std::string fileUri);
    int32_t AddMovingPhotoVideoResource(uint8_t* dataBuffer, size_t dataBufferSize);
    int32_t CJSaveCameraPhoto();
    int32_t CJDiscardCameraPhoto();
    int32_t CJSetOrientation(int32_t orientation);
    int32_t ApplyChanges() override;

    sptr<PhotoProxy> GetPhotoProxyObj();
    void ReleasePhotoProxyObj();

    std::vector<AssetChangeOperation> GetAssetChangeOperations() const;
    std::vector<ResourceType> GetAddResourceTypes() const;

private:
    bool CheckChangeOperations();
    bool CheckMovingPhotoWriteOperation();
    bool CheckEffectModeWriteOperation();
    int32_t CopyFileToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo = false);
    int32_t CopyDataBufferToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo = false);
    int32_t CopyMovingPhotoVideo(const std::string& assetUri);
    void SetNewFileAsset(int32_t id, const std::string& uri);

    sptr<PhotoProxy> photoProxy_ = nullptr;
    static std::atomic<uint32_t> cacheFileId_;
    std::shared_ptr<FileAsset> fileAsset_ = nullptr;
    std::shared_ptr<MediaAssetEditData> editData_ = nullptr;
    OHOS::DataShare::DataShareValuesBucket creationValuesBucket_;
    std::string realPath_;
    std::string cacheFileName_;
    std::vector<uint8_t> dataBufferOwned_;
    AddResourceMode addResourceMode_ = AddResourceMode::DATA_BUFFER;
    std::string movingPhotoVideoRealPath_;
    std::string cacheMovingPhotoVideoName_;
    std::vector<uint8_t> movingPhotoVideoDataBufferOwned_;
    AddResourceMode movingPhotoVideoResourceMode_ = AddResourceMode::DATA_BUFFER;
    std::vector<ResourceType> addResourceTypes_; // support adding resource multiple times
    std::vector<AssetChangeOperation> assetChangeOperations_;
    int32_t imageFileType_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif