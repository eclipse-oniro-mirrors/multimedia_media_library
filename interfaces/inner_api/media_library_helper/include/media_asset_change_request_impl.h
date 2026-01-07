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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_IMPL_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_IMPL_H

#include <fstream>
#include <vector>
#include <mutex>

#include "media_asset_base_capi.h"
#include "nocopyable.h"
#include "media_asset_types.h"
#include "unique_fd.h"
#include "media_asset_edit_data.h"
#include "datashare_helper.h"
#include "media_asset.h"
#include "media_asset_change_request.h"

constexpr int32_t YES = 1;
constexpr int32_t NO = 0;

namespace OHOS {
namespace Media {

class MediaAssetChangeRequestImpl : public MediaAssetChangeRequest, public NoCopyable {
public:
    MediaAssetChangeRequestImpl(std::shared_ptr<MediaAsset> mediaAsset);
    ~MediaAssetChangeRequestImpl();

    MediaLibrary_ErrorCode GetWriteCacheHandler(int32_t* fd) override;
    MediaLibrary_ErrorCode AddResourceWithUri(MediaLibrary_ResourceType resourceType, char* fileUri) override;
    MediaLibrary_ErrorCode AddResourceWithBuffer(MediaLibrary_ResourceType resourceType, uint8_t* buffer,
        uint32_t length) override;
    MediaLibrary_ErrorCode SaveCameraPhoto(MediaLibrary_ImageFileType imageFileType) override;
    MediaLibrary_ErrorCode DiscardCameraPhoto() override;
    MediaLibrary_ErrorCode ApplyChanges() override;

    void RecordChangeOperation(AssetChangeOperation changeOperation) override;

private:
    bool IsMovingPhoto();
    bool CheckWriteOperation(MediaLibrary_ResourceType resourceType);
    bool ContainsResource(MediaLibrary_ResourceType resourceType);
    bool Contains(AssetChangeOperation changeOperation);
    int32_t OpenWriteCacheHandler(bool isMovingPhotoVideo = false);
    uint32_t FetchAddCacheFileId();
    bool ChangeOperationExecute(AssetChangeOperation option);
    bool CheckChangeOperations();
    bool SubmitCacheExecute();
    bool AddResourceExecute();
    bool SaveCameraPhotoExecute();
    bool DiscardCameraPhotoExecute();
    bool HasWritePermission();
    bool WriteBySecurityComponent();
    int32_t CopyToMediaLibrary(AddResourceMode mode);
    int32_t CopyFileToMediaLibrary(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo = false);
    int32_t CopyDataBufferToMediaLibrary(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo = false);
    bool SendToCacheFile(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo = false);
    int32_t SubmitCache();
    int32_t SendFile(const OHOS::UniqueFd& srcFd, const OHOS::UniqueFd& destFd);
    bool AddResourceByMode(const OHOS::UniqueFd& uniqueFd, AddResourceMode mode,
        bool isMovingPhotoVideo = false);
    bool WriteCacheByArrayBuffer(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo = false);
    void DiscardHighQualityPhoto();

private:
    std::shared_ptr<MediaAsset> mediaAsset_ = nullptr;
    std::vector<MediaLibrary_ResourceType> addResourceTypes_;
    std::vector<AssetChangeOperation> assetChangeOperations_;
    std::string cacheMovingPhotoVideoName_;
    std::string cacheFileName_;
    static std::atomic<uint32_t> cacheFileId_;
    std::mutex mutex_;
    uint8_t* dataBuffer_ = nullptr;
    uint8_t* movingPhotoVideoDataBuffer_ = nullptr;
    OHOS::DataShare::DataShareValuesBucket creationValuesBucket_;
    AddResourceMode movingPhotoVideoResourceMode_;
    std::string realPath_;
    std::string movingPhotoVideoRealPath_;
    uint32_t movingPhotoVideoBufferSize_;
    uint32_t dataBufferSize_;
    AddResourceMode addResourceMode_;
    MediaLibrary_ImageFileType imageFileType_ = MediaLibrary_ImageFileType::MEDIA_LIBRARY_IMAGE_JPEG;
};

}
}

#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_ASSET_CHANGE_REQUEST_IMPL_H
