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

#include "media_asset_change_request_impl.h"

#include <fcntl.h>
#include <sys/sendfile.h>
#include "securec.h"

#include "oh_media_asset.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_column.h"
#include "file_uri.h"
#include "directory_ex.h"
#include "medialibrary_db_const.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "image_packer.h"
#include "permission_utils.h"
#include "userfile_client.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Security::AccessToken;

atomic<uint32_t> MediaAssetChangeRequestImpl::cacheFileId_(0);
const string MOVING_PHOTO_VIDEO_EXTENSION = "mp4";
const string API_VERSION = "api_version";

std::shared_ptr<MediaAssetChangeRequest> MediaAssetChangeRequestFactory::CreateMediaAssetChangeRequest(
    std::shared_ptr<MediaAsset> mediaAsset)
{
    std::shared_ptr<MediaAssetChangeRequestImpl> impl = std::make_shared<MediaAssetChangeRequestImpl>(mediaAsset);
    CHECK_AND_PRINT_LOG(impl != nullptr, "Failed to create MediaAssetChangeRequestImpl instance.");

    return impl;
}

MediaAssetChangeRequestImpl::MediaAssetChangeRequestImpl(std::shared_ptr<MediaAsset> mediaAsset)
{
    mediaAsset_ = mediaAsset;
    movingPhotoVideoDataBuffer_ = nullptr;
    dataBuffer_ = nullptr;
    movingPhotoVideoResourceMode_ = AddResourceMode::DEFAULT;
    addResourceMode_ = AddResourceMode::DEFAULT;
    movingPhotoVideoBufferSize_ = 0;
    dataBufferSize_ = 0;
}

MediaAssetChangeRequestImpl::~MediaAssetChangeRequestImpl()
{
    mediaAsset_ = nullptr;
    if (movingPhotoVideoDataBuffer_ != nullptr) {
        delete[] movingPhotoVideoDataBuffer_;
        movingPhotoVideoDataBuffer_ = nullptr;
    }

    if (dataBuffer_ != nullptr) {
        delete[] dataBuffer_;
        dataBuffer_ = nullptr;
    }

    addResourceTypes_.clear();
    assetChangeOperations_.clear();
}

MediaLibrary_ErrorCode MediaAssetChangeRequestImpl::GetWriteCacheHandler(int32_t* fd)
{
    unique_lock<mutex> ulock(mutex_);
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset get failed!");
    CHECK_AND_RETURN_RET_LOG(!IsMovingPhoto(), MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "cann't be moving photo!");
    CHECK_AND_RETURN_RET_LOG(CheckWriteOperation(MediaLibrary_ResourceType::MEDIA_LIBRARY_VIDEO_RESOURCE),
        MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "Not supported!");

    int32_t ret = OpenWriteCacheHandler();
    CHECK_AND_RETURN_RET_LOG(ret >= 0,
        MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "Failed to open write cache handler,ret: %{public}d", ret);

    *fd = ret;
    RecordChangeOperation(AssetChangeOperation::GET_WRITE_CACHE_HANDLER);
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetChangeRequestImpl::SaveCameraPhoto(MediaLibrary_ImageFileType imageFileType)
{
    unique_lock<mutex> ulock(mutex_);
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "fileAsset get failed!");

    MediaType mediaType = fileAsset->GetMediaType();
    if ((mediaType == MEDIA_TYPE_IMAGE && imageFileType == MEDIA_LIBRARY_IMAGE_JPEG) ||
        (mediaType == MEDIA_TYPE_IMAGE && imageFileType == MEDIA_LIBRARY_IMAGE_HEIF) ||
        (mediaType == MEDIA_TYPE_VIDEO && imageFileType == MEDIA_LIBRARY_FILE_VIDEO)) {
        imageFileType_ = imageFileType;
        RecordChangeOperation(AssetChangeOperation::SAVE_CAMERA_PHOTO);
        return MEDIA_LIBRARY_OK;
    }
    MEDIA_ERR_LOG("type mismatch, mediaType: %{public}d, imageFileType: %{public}d", mediaType, imageFileType);
    return MEDIA_LIBRARY_PARAMETER_ERROR;
}

MediaLibrary_ErrorCode MediaAssetChangeRequestImpl::DiscardCameraPhoto()
{
    unique_lock<mutex> ulock(mutex_);
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "fileAsset get failed!");

    RecordChangeOperation(AssetChangeOperation::DISCARD_CAMERA_PHOTO);
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetChangeRequestImpl::AddResourceWithUri(MediaLibrary_ResourceType resourceType,
    char* fileUri)
{
    unique_lock<mutex> ulock(mutex_);
    CHECK_AND_RETURN_RET_LOG(CheckWriteOperation(resourceType), MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "operation not support");

    string realPath;
    OHOS::AppFileService::ModuleFileUri::FileUri fileUriStr(fileUri);
    string path = fileUriStr.GetRealPath();
    bool result = OHOS::PathToRealPath(path, realPath);
    CHECK_AND_RETURN_RET_LOG(result, MEDIA_LIBRARY_NO_SUCH_FILE, "File real path isn't existed");

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "fileAsset get failed!");
    CHECK_AND_RETURN_RET_LOG(!IsMovingPhoto(), MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "not support edit moving photo with uri");

    CHECK_AND_RETURN_RET_LOG(fileAsset->GetMediaType() == MediaFileUtils::GetMediaType(realPath),
        MEDIA_LIBRARY_PARAMETER_ERROR, "Invalid file type");

    realPath_ = realPath;
    addResourceMode_ = AddResourceMode::FILE_URI;
    RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    addResourceTypes_.push_back(resourceType);
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetChangeRequestImpl::AddResourceWithBuffer(MediaLibrary_ResourceType resourceType,
    uint8_t* buffer, uint32_t length)
{
    unique_lock<mutex> ulock(mutex_);
    CHECK_AND_RETURN_RET_LOG(CheckWriteOperation(resourceType), MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "operation not support");

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED, "fileAsset get failed!");
    CHECK_AND_RETURN_RET_LOG(!IsMovingPhoto(), MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
        "not support edit moving photo with buffer");

    if (dataBuffer_ != nullptr) {
        delete[] dataBuffer_;
    }
    dataBuffer_ = new uint8_t[length + 1];
    CHECK_AND_RETURN_RET_LOG(dataBuffer_ != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
        "create dataBuffer_ failed!");

    dataBufferSize_ = length;
    if (length > 0) {
        int ret = memcpy_s(dataBuffer_, length + 1, buffer, length);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "memcpy buffer failed!");
    }
    addResourceMode_ = AddResourceMode::DATA_BUFFER;
    RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    addResourceTypes_.push_back(resourceType);
    return MEDIA_LIBRARY_OK;
}

MediaLibrary_ErrorCode MediaAssetChangeRequestImpl::ApplyChanges()
{
    unique_lock<mutex> ulock(mutex_);
    bool result = CheckChangeOperations();
    CHECK_AND_RETURN_RET_LOG(result, MEDIA_LIBRARY_PARAMETER_ERROR, "Failed to check asset change request operations");

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "fileAsset is nullptr");

    unordered_set<AssetChangeOperation> appliedOperations;
    for (const auto& changeOperation : assetChangeOperations_) {
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = ChangeOperationExecute(changeOperation);
        CHECK_AND_RETURN_RET_LOG(valid, MEDIA_LIBRARY_OPERATION_NOT_SUPPORTED,
            "Failed to apply asset change request, operation: %{public}d", changeOperation);

        appliedOperations.insert(changeOperation);
    }
    assetChangeOperations_.clear();
    addResourceTypes_.clear();
    movingPhotoVideoResourceMode_ = AddResourceMode::DEFAULT;
    addResourceMode_ = AddResourceMode::DEFAULT;
    return MEDIA_LIBRARY_OK;
}

bool MediaAssetChangeRequestImpl::IsMovingPhoto()
{
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, false, "fileAsset is nullptr");

    return fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
}

bool MediaAssetChangeRequestImpl::CheckWriteOperation(MediaLibrary_ResourceType resourceType)
{
    if (Contains(AssetChangeOperation::GET_WRITE_CACHE_HANDLER) ||
        Contains(AssetChangeOperation::ADD_RESOURCE)) {
        MEDIA_ERR_LOG("The previous asset creation/modification request has not been applied");
        return false;
    }
    return true;
}

bool MediaAssetChangeRequestImpl::ContainsResource(MediaLibrary_ResourceType resourceType)
{
    return find(addResourceTypes_.begin(), addResourceTypes_.end(), resourceType) != addResourceTypes_.end();
}

bool MediaAssetChangeRequestImpl::Contains(AssetChangeOperation changeOperation)
{
    return find(assetChangeOperations_.begin(), assetChangeOperations_.end(), changeOperation) !=
           assetChangeOperations_.end();
}

int32_t MediaAssetChangeRequestImpl::OpenWriteCacheHandler(bool isMovingPhotoVideo)
{
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_FAIL, "fileAsset is null");

    // specify mp4 extension for cache file of moving photo video
    string extension = isMovingPhotoVideo ? MOVING_PHOTO_VIDEO_EXTENSION
        : MediaFileUtils::GetExtensionFromPath(fileAsset->GetDisplayName());
    int64_t currentTimestamp = MediaFileUtils::UTCTimeNanoSeconds();
    uint32_t cacheFileId = FetchAddCacheFileId();
    string cacheFileName = to_string(currentTimestamp) + "_" + to_string(cacheFileId) + "." + extension;
    string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + cacheFileName;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);
    int32_t ret = UserFileClient::OpenFile(openCacheUri, MEDIA_FILEMODE_WRITEONLY);
    CHECK_AND_RETURN_RET_LOG(ret != E_PERMISSION_DENIED, ret, "Open cache file failed, permission denied");
    CHECK_AND_PRINT_LOG(ret >= 0, "Open cache file failed, ret: %{public}d", ret);

    if (isMovingPhotoVideo) {
        cacheMovingPhotoVideoName_ = cacheFileName;
    } else {
        cacheFileName_ = cacheFileName;
    }
    return ret;
}

uint32_t MediaAssetChangeRequestImpl::FetchAddCacheFileId()
{
    return cacheFileId_.fetch_add(1);
}

void MediaAssetChangeRequestImpl::RecordChangeOperation(AssetChangeOperation changeOperation)
{
    assetChangeOperations_.push_back(changeOperation);
}

bool MediaAssetChangeRequestImpl::CheckChangeOperations()
{
    CHECK_AND_RETURN_RET_LOG(assetChangeOperations_.size() != 0, false, "None request to apply");

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, false, "fileAsset is null");
    CHECK_AND_RETURN_RET_LOG(fileAsset->GetId() > 0, false, "Invalid asset change request");

    return true;
}

bool MediaAssetChangeRequestImpl::ChangeOperationExecute(AssetChangeOperation option)
{
    bool ret = false;
    switch (option) {
        case AssetChangeOperation::GET_WRITE_CACHE_HANDLER:
            ret = SubmitCacheExecute();
            break;
        case AssetChangeOperation::ADD_RESOURCE:
            ret = AddResourceExecute();
            break;
        case AssetChangeOperation::SAVE_CAMERA_PHOTO:
            ret = SaveCameraPhotoExecute();
            break;
        case AssetChangeOperation::DISCARD_CAMERA_PHOTO:
            ret = DiscardCameraPhotoExecute();
            break;
        default:
            break;
    }
    return ret;
}

bool MediaAssetChangeRequestImpl::SubmitCacheExecute()
{
    int32_t ret = SubmitCache();
    CHECK_AND_RETURN_RET_LOG(ret >= 0, false, "Failed to write cache, ret: %{public}d", ret);

    return true;
}

bool MediaAssetChangeRequestImpl::AddResourceExecute()
{
    MEDIA_INFO_LOG("AddResourceExecute begin.");
    CHECK_AND_RETURN_RET_LOG(!IsMovingPhoto(), false, "not support edit moving photo with buffer or uri");

    if (!HasWritePermission()) {
        return WriteBySecurityComponent();
    }

    int32_t cacheFd = OpenWriteCacheHandler();
    CHECK_AND_RETURN_RET_LOG(cacheFd >= 0, false, "Failed to open write cache handler, err: %{public}d", cacheFd);

    OHOS::UniqueFd uniqueFd(cacheFd);
    AddResourceMode mode = addResourceMode_;
    CHECK_AND_RETURN_RET_LOG(AddResourceByMode(uniqueFd, mode), false, "Faild to write cache file");

    return SubmitCacheExecute();
}

bool MediaAssetChangeRequestImpl::SaveCameraPhotoExecute()
{
    MEDIA_INFO_LOG("SaveCameraPhotoExecute begin.");
    bool containsAddResource = find(assetChangeOperations_.begin(), assetChangeOperations_.end(),
        AssetChangeOperation::ADD_RESOURCE) != assetChangeOperations_.end();
    std::string uriStr = PAH_SAVE_CAMERA_PHOTO;
    if (containsAddResource && !PermissionUtils::IsSystemApp()) {
        // remove high quality photo
        MEDIA_INFO_LOG("discard high quality photo because add resource by third app");
        DiscardHighQualityPhoto();

        // set dirty flag when third-party hap calling addResource to save camera photo
        MediaFileUtils::UriAppendKeyValue(uriStr, PhotoColumn::PHOTO_DIRTY,
            to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)));
    }

    // The watermark will trigger the scan. If the watermark is turned on, there is no need to trigger the scan again.
    bool needScan = std::find(assetChangeOperations_.begin(), assetChangeOperations_.end(),
        AssetChangeOperation::ADD_FILTERS) == assetChangeOperations_.end();

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, false, "fileAsset is nullptr");

    MediaFileUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaFileUtils::UriAppendKeyValue(uriStr, MEDIA_OPERN_KEYWORD, to_string(needScan));
    MediaFileUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_FILE_PATH, fileAsset->GetUri());
    MediaFileUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    MediaFileUtils::UriAppendKeyValue(uriStr, PhotoColumn::PHOTO_SUBTYPE, to_string(fileAsset->GetPhotoSubType()));
    MediaFileUtils::UriAppendKeyValue(uriStr, IMAGE_FILE_TYPE, to_string(static_cast<int32_t>(imageFileType_)));
    MediaFileUtils::UriAppendKeyValue(uriStr, CONTAIN_ADD_RESOURCE, to_string(containsAddResource));
    Uri uri(uriStr);
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    OHOS::DataShare::DataSharePredicates predicates;
    int32_t changedRows = UserFileClient::Update(uri, predicates, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(changedRows >= 0, false, "save camera photo fail, err: %{public}d", changedRows);

    return true;
}

bool MediaAssetChangeRequestImpl::DiscardCameraPhotoExecute()
{
    OHOS::DataShare::DataSharePredicates predicates;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, true);

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));

    string uri = PAH_DISCARD_CAMERA_PHOTO;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaFileUtils::UriAppendKeyValue(uri, PhotoColumn::MEDIA_ID, to_string(fileAsset->GetId()));
    Uri updateAssetUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(changedRows >= 0, false,
        "Failed to update property of asset, err: %{public}d", changedRows);
    return true;
}

bool MediaAssetChangeRequestImpl::HasWritePermission()
{
    AccessTokenID tokenCaller = OHOS::IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_WRITE_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

bool MediaAssetChangeRequestImpl::WriteBySecurityComponent()
{
    int32_t ret = CopyToMediaLibrary(addResourceMode_);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, false,
        "Failed to write by security component, ret: %{public}d", ret);

    return true;
}

int32_t MediaAssetChangeRequestImpl::CopyToMediaLibrary(AddResourceMode mode)
{
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_FAIL, "fileAsset is null");

    int32_t ret = E_ERR;
    string assetUri = fileAsset->GetUri();
    CHECK_AND_RETURN_RET_LOG(!assetUri.empty(), E_ERR, "Failed to check empty asset uri");
    CHECK_AND_RETURN_RET_LOG(!IsMovingPhoto(), E_ERR, "not support edit moving photo with buffer or uri");

    Uri uri(assetUri);
    OHOS::UniqueFd destFd(UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY));
    CHECK_AND_RETURN_RET_LOG(destFd.Get() >= 0, destFd.Get(),
        "Failed to open %{public}s with error: %{public}d", assetUri.c_str(), destFd.Get());

    if (mode == AddResourceMode::FILE_URI) {
        ret = CopyFileToMediaLibrary(destFd);
    } else if (mode == AddResourceMode::DATA_BUFFER) {
        ret = CopyDataBufferToMediaLibrary(destFd);
    } else {
        MEDIA_ERR_LOG("Invalid mode: %{public}d", mode);
        return E_INVALID_VALUES;
    }

    return ret;
}

int32_t MediaAssetChangeRequestImpl::CopyFileToMediaLibrary(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo)
{
    string srcRealPath = isMovingPhotoVideo ? movingPhotoVideoRealPath_ : realPath_;
    CHECK_AND_RETURN_RET_LOG(!srcRealPath.empty(), E_FAIL, "Failed to check real path of source");

    string absFilePath;
    CHECK_AND_RETURN_RET_LOG(OHOS::PathToRealPath(srcRealPath, absFilePath), E_FAIL, "Not real path %{public}s",
        srcRealPath.c_str());

    OHOS::UniqueFd srcFd(open(absFilePath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(srcFd.Get() >= 0, srcFd.Get(),
        "Failed to open %{public}s, errno=%{public}d", absFilePath.c_str(), errno);

    int32_t err = SendFile(srcFd, destFd);
    CHECK_AND_PRINT_LOG(err == E_OK, "Failed to send file from %{public}d to %{public}d", srcFd.Get(), destFd.Get());

    return err;
}

int32_t MediaAssetChangeRequestImpl::CopyDataBufferToMediaLibrary(const OHOS::UniqueFd& destFd,
    bool isMovingPhotoVideo)
{
    size_t offset = 0;
    size_t length = isMovingPhotoVideo ? movingPhotoVideoBufferSize_ : dataBufferSize_;
    void* dataBuffer = isMovingPhotoVideo ? movingPhotoVideoDataBuffer_ : dataBuffer_;
    while (offset < length) {
        ssize_t written = write(destFd.Get(), (char*)dataBuffer + offset, length - offset);
        CHECK_AND_RETURN_RET_LOG(written >= 0, written,
            "Failed to copy data buffer, return %{public}d", static_cast<int>(written));

        offset += static_cast<size_t>(written);
    }
    return E_OK;
}

bool MediaAssetChangeRequestImpl::SendToCacheFile(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo)
{
    string realPath = isMovingPhotoVideo ? movingPhotoVideoRealPath_ : realPath_;
    string absFilePath;
    CHECK_AND_RETURN_RET_LOG(OHOS::PathToRealPath(realPath, absFilePath), false,
        "Not real path %{public}s, errno=%{public}d", realPath.c_str(), errno);

    OHOS::UniqueFd srcFd(open(absFilePath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(srcFd.Get() >= 0, false, "Failed to open file, errno=%{public}d", errno);

    int32_t err = SendFile(srcFd, destFd);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false,
        "Failed to send file from %{public}d to %{public}d", srcFd.Get(), destFd.Get());

    return true;
}

int32_t MediaAssetChangeRequestImpl::SubmitCache()
{
    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_FAIL, "fileAsset is null");
    CHECK_AND_RETURN_RET_LOG(!cacheFileName_.empty() || !cacheMovingPhotoVideoName_.empty(), E_FAIL,
        "Failed to check cache file");

    string uri = PAH_SUBMIT_CACHE;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri submitCacheUri(uri);
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileAsset->GetId());
    valuesBucket.Put(CACHE_FILE_NAME, cacheFileName_);
    int32_t ret = UserFileClient::Insert(submitCacheUri, valuesBucket);

    cacheFileName_.clear();
    cacheMovingPhotoVideoName_.clear();
    return ret;
}

int32_t MediaAssetChangeRequestImpl::SendFile(const OHOS::UniqueFd& srcFd, const OHOS::UniqueFd& destFd)
{
    CHECK_AND_RETURN_RET_LOG((srcFd.Get() >= 0 && destFd.Get() >= 0), E_ERR,
        "Failed to check srcFd: %{public}d and destFd: %{public}d", srcFd.Get(), destFd.Get());

    struct stat statSrc {};
    int32_t status = fstat(srcFd.Get(), &statSrc);
    CHECK_AND_RETURN_RET_LOG(status == 0, status, "Failed to get file stat, errno=%{public}d", errno);

    off_t offset = 0;
    off_t fileSize = statSrc.st_size;
    while (offset < fileSize) {
        ssize_t sent = sendfile(destFd.Get(), srcFd.Get(), &offset, fileSize - offset);
        CHECK_AND_RETURN_RET_LOG(sent >= 0, sent,
            "Failed to sendfile with errno=%{public}d, srcFd=%{public}d, destFd=%{public}d",
            errno, srcFd.Get(), destFd.Get());
    }

    return E_OK;
}

bool MediaAssetChangeRequestImpl::AddResourceByMode(const OHOS::UniqueFd& uniqueFd,
    AddResourceMode mode, bool isMovingPhotoVideo)
{
    bool isWriteSuccess = false;
    if (mode == AddResourceMode::DATA_BUFFER) {
        isWriteSuccess = WriteCacheByArrayBuffer(uniqueFd, isMovingPhotoVideo);
    } else if (mode == AddResourceMode::FILE_URI) {
        isWriteSuccess = SendToCacheFile(uniqueFd, isMovingPhotoVideo);
    } else {
        MEDIA_ERR_LOG("Unsupported addResource mode");
    }
    return isWriteSuccess;
}

bool MediaAssetChangeRequestImpl::WriteCacheByArrayBuffer(const OHOS::UniqueFd& destFd, bool isMovingPhotoVideo)
{
    size_t offset = 0;
    size_t length = isMovingPhotoVideo ? movingPhotoVideoBufferSize_ : dataBufferSize_;
    void* dataBuffer = isMovingPhotoVideo ? movingPhotoVideoDataBuffer_ : dataBuffer_;
    while (offset < length) {
        ssize_t written = write(destFd.Get(), (char*)dataBuffer + offset, length - offset);
        CHECK_AND_RETURN_RET_LOG(written >= 0, false,
            "Failed to write data buffer to cache file, return %{public}d", static_cast<int>(written));

        offset += static_cast<size_t>(written);
    }
    return true;
}

void MediaAssetChangeRequestImpl::DiscardHighQualityPhoto()
{
    string uriStr = PAH_REMOVE_MSC_TASK;
    MediaFileUtils::UriAppendKeyValue(uriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    OHOS::DataShare::DataSharePredicates predicates;
    int errCode = 0;

    auto fileAsset = mediaAsset_->GetFileAssetInstance();
    vector<string> columns { to_string(fileAsset->GetId()) };
    UserFileClient::Query(uri, predicates, columns, errCode);
}
