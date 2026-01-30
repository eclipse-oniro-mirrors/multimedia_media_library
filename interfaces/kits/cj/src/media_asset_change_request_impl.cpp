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

#include "media_asset_change_request_impl.h"

#include <sys/sendfile.h>

#include "ability_runtime/cj_ability_context.h"
#include "access_token.h"
#include "accesstoken_kit.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "delete_callback.h"
#include "directory_ex.h"
#include "file_uri.h"
#include "image_packer.h"
#include "media_file_utils.h"
#include "permission_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
constexpr int64_t CREATE_ASSET_REQUEST_PENDING = -4;
constexpr int32_t MAX_DELETE_NUMBER = 300;
const std::string DEFAULT_TITLE_TIME_FORMAT = "%Y%m%d_%H%M%S";
const std::string DEFAULT_TITLE_IMG_PREFIX = "IMG_";
const std::string DEFAULT_TITLE_VIDEO_PREFIX = "VID_";
const std::string MOVING_PHOTO_VIDEO_EXTENSION = "mp4";
std::atomic<uint32_t> MediaAssetChangeRequestImpl::cacheFileId_ = 0;

static const std::array<int, 4> ORIENTATION_ARRAY = { 0, 90, 180, 270 };

int32_t MediaDataSource::ReadData(const std::shared_ptr<AVSharedMemory>& mem, uint32_t length)
{
    if (readPos_ >= size_) {
        LOGE("Failed to check read position");
        return SOURCE_ERROR_EOF;
    }

    if (memcpy_s(mem->GetBase(), mem->GetSize(), (char*)buffer_ + readPos_, length) != E_OK) {
        LOGE("Failed to copy buffer to mem");
        return SOURCE_ERROR_IO;
    }
    readPos_ += static_cast<int64_t>(length);
    return static_cast<int32_t>(length);
}

int32_t MediaDataSource::ReadAt(const std::shared_ptr<AVSharedMemory>& mem, uint32_t length, int64_t pos)
{
    readPos_ = pos;
    return ReadData(mem, length);
}

int32_t MediaDataSource::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory>& mem)
{
    readPos_ = pos;
    return ReadData(mem, length);
}

int32_t MediaDataSource::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory>& mem)
{
    return ReadData(mem, length);
}

int32_t MediaDataSource::GetSize(int64_t& size)
{
    size = size_;
    return E_OK;
}

std::shared_ptr<FileAsset> MediaAssetChangeRequestImpl::GetFileAssetInstance() const
{
    return fileAsset_;
}

MediaAssetChangeRequestImpl::MediaAssetChangeRequestImpl(std::shared_ptr<FileAsset> fileAssetPtr)
{
    fileAsset_ = fileAssetPtr;
}

static bool ParseFileUri(const std::string& fileUriStr, MediaType mediaType, std::string& realPath)
{
    AppFileService::ModuleFileUri::FileUri fileUri(fileUriStr);
    std::string path = fileUri.GetRealPath();
    if (!PathToRealPath(path, realPath)) {
        return false;
    }
    if (mediaType != MediaFileUtils::GetMediaType(realPath)) {
        return false;
    }
    return true;
}

static bool CheckMovingPhotoCreationArgs(DataShare::DataShareValuesBucket valuesBucket)
{
    bool isValid = false;
    int32_t mediaType = valuesBucket.Get(CONST_MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    if (!isValid) {
        return false;
    }
    if (mediaType != static_cast<int32_t>(MEDIA_TYPE_IMAGE)) {
        return false;
    }
    std::string extension = valuesBucket.Get(CONST_ASSET_EXTENTION, isValid);
    if (isValid) {
        return MediaFileUtils::CheckMovingPhotoExtension(extension);
    }

    std::string displayName = valuesBucket.Get(CONST_MEDIA_DATA_DB_NAME, isValid);
    return isValid && MediaFileUtils::CheckMovingPhotoExtension(MediaFileUtils::GetExtensionFromPath(displayName));
}

static bool CheckCreateOption(DataShare::DataShareValuesBucket valuesBucket, bool isSystemApi)
{
    bool isValid = false;
    int32_t subType = valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);
    if (isValid) {
        if (subType < static_cast<int32_t>(PhotoSubType::DEFAULT) ||
            subType >= static_cast<int32_t>(PhotoSubType::SUBTYPE_END)) {
            return false;
        }
        if (subType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) &&
            !CheckMovingPhotoCreationArgs(valuesBucket)) {
            return false;
        }
        if (!isSystemApi && subType != static_cast<int32_t>(PhotoSubType::DEFAULT) &&
            subType != static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            return false;
        }
    }
    std::string cameraShotKey = valuesBucket.Get(PhotoColumn::CAMERA_SHOT_KEY, isValid);
    if (isValid) {
        if (cameraShotKey.size() < CAMERA_SHOT_KEY_SIZE) {
            return false;
        }
        if (subType == static_cast<int32_t>(PhotoSubType::SCREENSHOT)) {
            return false;
        } else {
            valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CAMERA));
        }
    }
    return true;
}

MediaAssetChangeRequestImpl::MediaAssetChangeRequestImpl(
    int64_t contextId, int32_t photoType, std::string extension, std::string title, int32_t subType, int32_t* errCode)
{
    DataShare::DataShareValuesBucket valuesBucket;
    if (!MediaAssetChangeRequestImpl::InitUserFileClient(contextId)) {
        *errCode = JS_INNER_FAIL;
        return;
    }
    MediaType mediaType = static_cast<MediaType>(photoType);
    if (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO &&
        mediaType != MediaFileUtils::GetMediaType("." + extension)) {
        LOGE("Invalid photoType or failed to check extension");
        *errCode = OHOS_INVALID_PARAM_CODE;
        return;
    }
    valuesBucket.Put(CONST_ASSET_EXTENTION, extension);
    valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    if (!title.empty()) {
        valuesBucket.Put(PhotoColumn::MEDIA_TITLE, title);
    }
    if (subType != -1) {
        valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, subType);
    }
    if (!CheckCreateOption(valuesBucket, false)) {
        *errCode = OHOS_INVALID_PARAM_CODE;
        return;
    }
    bool isValid = false;
    std::string newTitle = valuesBucket.Get(PhotoColumn::MEDIA_TITLE, isValid);
    if (!isValid) {
        newTitle = mediaType == MEDIA_TYPE_IMAGE ? DEFAULT_TITLE_IMG_PREFIX : DEFAULT_TITLE_VIDEO_PREFIX;
        newTitle += MediaFileUtils::StrCreateTime(DEFAULT_TITLE_TIME_FORMAT, MediaFileUtils::UTCTimeSeconds());
        valuesBucket.Put(PhotoColumn::MEDIA_TITLE, newTitle);
    }
    std::string displayName = newTitle + "." + extension;
    if (MediaFileUtils::CheckDisplayName(displayName) != E_OK) {
        *errCode = OHOS_INVALID_PARAM_CODE;
        return;
    }
    valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, displayName);
    auto emptyFileAsset = std::make_unique<FileAsset>();
    emptyFileAsset->SetDisplayName(displayName);
    emptyFileAsset->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
    emptyFileAsset->SetMediaType(MediaFileUtils::GetMediaType(displayName));
    emptyFileAsset->SetPhotoSubType(subType);
    emptyFileAsset->SetTimePending(CREATE_ASSET_REQUEST_PENDING);
    emptyFileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset_ = std::move(emptyFileAsset);
    creationValuesBucket_ = std::move(valuesBucket);
    RecordChangeOperation(AssetChangeOperation::CREATE_FROM_SCRATCH);
}

static int32_t ParseArgsDeleteAssets(int64_t contextId, std::vector<std::string> uris)
{
    if (!MediaAssetChangeRequestImpl::InitUserFileClient(contextId)) {
        return JS_INNER_FAIL;
    }
    if (uris.empty()) {
        LOGE("Failed to check empty array");
        return OHOS_INVALID_PARAM_CODE;
    }
    for (const auto& uri : uris) {
        if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) == std::string::npos) {
            return JS_E_URI;
        }
    }
    return 0;
}

std::vector<AssetChangeOperation> MediaAssetChangeRequestImpl::GetAssetChangeOperations() const
{
    return assetChangeOperations_;
}

std::vector<ResourceType> MediaAssetChangeRequestImpl::GetAddResourceTypes() const
{
    return addResourceTypes_;
}

static bool HasWritePermission()
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int result = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_WRITE_IMAGEVIDEO);
    return result == PermissionState::PERMISSION_GRANTED;
}

static bool InitDeleteRequest(std::string& appName, std::vector<std::string>& uris, OHOS::AAFwk::Want& request,
    std::shared_ptr<DeleteCallback>& callback)
{
    request.SetElementName(DELETE_UI_PACKAGE_NAME, DELETE_UI_EXT_ABILITY_NAME);
    request.SetParam(DELETE_UI_EXTENSION_TYPE, DELETE_UI_REQUEST_TYPE);

    if (appName.empty()) {
        return false;
    }
    request.SetParam(DELETE_UI_APPNAME, appName);

    request.SetParam(DELETE_UI_URIS, uris);
    callback->SetUris(uris);
    return true;
}

static int32_t DeleteAssetsExecute(OHOS::DataShare::DataSharePredicates& predicates,
    OHOS::DataShare::DataShareValuesBucket& valuesBucket)
{
    std::string trashUri = CONST_PAH_SYS_TRASH_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(trashUri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(trashUri);
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        LOGE("Failed to delete assets, err: %{public}d", changedRows);
        return changedRows;
    }
    return 0;
}

static int32_t SetSessionId(std::string &appName, std::vector<std::string> &uris, Ace::UIContent* uiContent,
    std::shared_ptr<DeleteCallback>& callback, Ace::ModalUIExtensionCallbacks &extensionCallback)
{
    OHOS::AAFwk::Want request;
    if (!InitDeleteRequest(appName, uris, request, callback)) {
        return JS_INNER_FAIL;
    }
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int32_t sessionId = uiContent->CreateModalUIExtension(request, extensionCallback, config);
    if (sessionId == 0) {
        return JS_INNER_FAIL;
    }
    callback->SetSessionId(sessionId);
    return 0;
}

int32_t MediaAssetChangeRequestImpl::CJDeleteAssets(int64_t contextId, std::vector<std::string> uris)
{
    if (ParseArgsDeleteAssets(contextId, uris) != 0) {
        return OHOS_INVALID_PARAM_CODE;
    }
    OHOS::DataShare::DataSharePredicates predicates;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    predicates.In(PhotoColumn::MEDIA_ID, uris);
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeSeconds());
    if (MediaLibraryNapiUtils::IsSystemApp()) {
        DeleteAssetsExecute(predicates, valuesBucket);
    }
#ifdef HAS_ACE_ENGINE_PART
    if (!HasWritePermission()) {
        return OHOS_PERMISSION_DENIED_CODE;
    }
    if (uris.size() > MAX_DELETE_NUMBER) {
        LOGE("No more than 300 assets can be deleted at one time");
        return OHOS_INVALID_PARAM_CODE;
    }
    auto cjAbilityContext = FFI::FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (cjAbilityContext == nullptr || cjAbilityContext->GetAbilityContext() == nullptr) {
        LOGE("Failed to get native stage context instance");
        return JS_INNER_FAIL;
    }
    auto abilityContext = cjAbilityContext->GetAbilityContext();
    auto abilityInfo = abilityContext->GetAbilityInfo();
    std::string appName;
    abilityContext->GetResourceManager()->GetStringById(abilityInfo->labelId, appName);
    auto uiContent = abilityContext->GetUIContent();
    if (uiContent == nullptr) {
        return JS_INNER_FAIL;
    }
    auto callback = std::make_shared<DeleteCallback>(uiContent);
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        [callback](int32_t releaseCode) { callback->OnRelease(releaseCode); },
        [callback](int32_t resultCode, const AAFwk::Want& result) { callback->OnResult(resultCode, result); },
        [callback](const OHOS::AAFwk::WantParams& request) { callback->OnReceive(request); },
        [callback](int32_t code, const std::string& name, const std::string& message) {
            callback->OnError(code, name, message);
        },
    };
    return SetSessionId(appName, uris, uiContent, callback, extensionCallback);
#else
    LOGE("ace_engine is not support");
    return JS_INNER_FAIL;
#endif
}

int64_t MediaAssetChangeRequestImpl::CJGetAsset(int32_t* errCode)
{
    if (fileAsset_ == nullptr) {
        *errCode = JS_INNER_FAIL;
        return 0;
    }
    if (fileAsset_->GetId() > 0) {
        auto photoAssetImpl = FFIData::Create<PhotoAssetImpl>(fileAsset_);
        if (!photoAssetImpl) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        return photoAssetImpl->GetID();
    }
    return 0;
}

MediaAssetChangeRequestImpl::MediaAssetChangeRequestImpl(
    int64_t contextId, const std::string& filePath, MediaType meidiaType, int32_t* errCode)
{
    if (!MediaAssetChangeRequestImpl::InitUserFileClient(contextId)) {
        *errCode = JS_INNER_FAIL;
        return;
    }
    std::string realPath;
    if ((meidiaType == MediaType::MEDIA_TYPE_IMAGE && ParseFileUri(filePath, MediaType::MEDIA_TYPE_IMAGE, realPath)) ||
        (meidiaType == MediaType::MEDIA_TYPE_VIDEO && ParseFileUri(filePath, MediaType::MEDIA_TYPE_VIDEO, realPath))) {
        std::string displayName = MediaFileUtils::GetFileName(realPath);
        if (MediaFileUtils::CheckDisplayName(displayName) != E_OK) {
            *errCode = OHOS_INVALID_PARAM_CODE;
            return;
        }
        std::string title = MediaFileUtils::GetTitleFromDisplayName(displayName);
        MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
        auto emptyFileAsset = std::make_unique<FileAsset>();
        emptyFileAsset->SetDisplayName(displayName);
        emptyFileAsset->SetTitle(title);
        emptyFileAsset->SetMediaType(mediaType);
        emptyFileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::DEFAULT));
        emptyFileAsset->SetTimePending(CREATE_ASSET_REQUEST_PENDING);
        emptyFileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
        fileAsset_ = std::move(emptyFileAsset);
        realPath_ = realPath;
        creationValuesBucket_.Put(CONST_MEDIA_DATA_DB_NAME, displayName);
        creationValuesBucket_.Put(CONST_ASSET_EXTENTION, MediaFileUtils::GetExtensionFromPath(displayName));
        creationValuesBucket_.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
        creationValuesBucket_.Put(PhotoColumn::MEDIA_TITLE, title);
        RecordChangeOperation(AssetChangeOperation::CREATE_FROM_URI);
        return;
    }
    *errCode = OHOS_INVALID_PARAM_CODE;
    return;
}

void MediaAssetChangeRequestImpl::RecordChangeOperation(AssetChangeOperation changeOperation)
{
    if ((changeOperation == AssetChangeOperation::GET_WRITE_CACHE_HANDLER ||
            changeOperation == AssetChangeOperation::ADD_RESOURCE ||
            changeOperation == AssetChangeOperation::ADD_FILTERS) &&
        Contains(AssetChangeOperation::CREATE_FROM_SCRATCH)) {
        assetChangeOperations_.insert(assetChangeOperations_.begin() + 1, changeOperation);
        return;
    }
    if (changeOperation == AssetChangeOperation::ADD_RESOURCE &&
        Contains(AssetChangeOperation::SET_MOVING_PHOTO_EFFECT_MODE)) {
        assetChangeOperations_.insert(assetChangeOperations_.begin(), changeOperation);
        return;
    }
    assetChangeOperations_.push_back(changeOperation);
}

bool MediaAssetChangeRequestImpl::Contains(AssetChangeOperation changeOperation) const
{
    return std::find(assetChangeOperations_.begin(), assetChangeOperations_.end(), changeOperation) !=
           assetChangeOperations_.end();
}

uint32_t MediaAssetChangeRequestImpl::FetchAddCacheFileId()
{
    uint32_t id = cacheFileId_.fetch_add(1);
    return id;
}

void MediaAssetChangeRequestImpl::SetCacheFileName(std::string& fileName)
{
    cacheFileName_ = fileName;
}

void MediaAssetChangeRequestImpl::SetCacheMovingPhotoVideoName(std::string& fileName)
{
    cacheMovingPhotoVideoName_ = fileName;
}

std::string MediaAssetChangeRequestImpl::GetMovingPhotoVideoPath() const
{
    return movingPhotoVideoRealPath_;
}

std::string MediaAssetChangeRequestImpl::GetFileRealPath() const
{
    return realPath_;
}

AddResourceMode MediaAssetChangeRequestImpl::GetAddResourceMode() const
{
    return addResourceMode_;
}

void* MediaAssetChangeRequestImpl::GetDataBuffer() const
{
    return dataBuffer_;
}

size_t MediaAssetChangeRequestImpl::GetDataBufferSize() const
{
    return dataBufferSize_;
}

AddResourceMode MediaAssetChangeRequestImpl::GetMovingPhotoVideoMode() const
{
    return movingPhotoVideoResourceMode_;
}

void* MediaAssetChangeRequestImpl::GetMovingPhotoVideoBuffer() const
{
    return movingPhotoVideoDataBuffer_;
}

size_t MediaAssetChangeRequestImpl::GetMovingPhotoVideoSize() const
{
    return movingPhotoVideoBufferSize_;
}

sptr<PhotoProxy> MediaAssetChangeRequestImpl::GetPhotoProxyObj()
{
    return photoProxy_;
}

void MediaAssetChangeRequestImpl::ReleasePhotoProxyObj()
{
    photoProxy_->Release();
    photoProxy_ = nullptr;
}

int32_t MediaAssetChangeRequestImpl::GetImageFileType()
{
    return imageFileType_;
}

int32_t MediaAssetChangeRequestImpl::CreateAssetBySecurityComponent(std::string& assetUri)
{
    bool isValid = false;
    std::string title = creationValuesBucket_.Get(PhotoColumn::MEDIA_TITLE, isValid);
    if (!isValid) {
        LOGE("Failed to get title");
        return E_FAIL;
    }
    std::string extension = creationValuesBucket_.Get(CONST_ASSET_EXTENTION, isValid);
    if (!isValid || MediaFileUtils::CheckDisplayName(title + "." + extension) != E_OK) {
        LOGE("Failed to check displayName");
        return E_FAIL;
    }
    creationValuesBucket_.valuesMap.erase(CONST_MEDIA_DATA_DB_NAME);

    std::string uri = CONST_PAH_CREATE_PHOTO_COMPONENT; // create asset by security component
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri createAssetUri(uri);
    return UserFileClient::InsertExt(createAssetUri, creationValuesBucket_, assetUri);
}

int32_t MediaAssetChangeRequestImpl::PutMediaAssetEditData(DataShare::DataShareValuesBucket& valuesBucket)
{
    if (editData_ == nullptr) {
        return E_OK;
    }

    std::string compatibleFormat = editData_->GetCompatibleFormat();
    if (compatibleFormat.empty()) {
        LOGE("Failed to check compatibleFormat");
        return E_FAIL;
    }
    std::string formatVersion = editData_->GetFormatVersion();
    if (formatVersion.empty()) {
        LOGE("Failed to check formatVersion");
        return E_FAIL;
    }
    std::string data = editData_->GetData();
    if (data.empty()) {
        LOGE("Failed to check data");
        return E_FAIL;
    }

    valuesBucket.Put(CONST_COMPATIBLE_FORMAT, compatibleFormat);
    valuesBucket.Put(CONST_FORMAT_VERSION, formatVersion);
    valuesBucket.Put(CONST_EDIT_DATA, data);
    return E_OK;
}

int32_t MediaAssetChangeRequestImpl::CopyToMediaLibrary(bool isCreation, AddResourceMode mode)
{
    if (fileAsset_ == nullptr) {
        LOGE("Failed to check fileAsset_");
        return E_FAIL;
    }
    int32_t ret = E_ERR;
    int32_t id = 0;
    std::string assetUri;
    if (isCreation) {
        ret = CreateAssetBySecurityComponent(assetUri);
        if (ret <= 0) {
            LOGE("Failed to create asset by security component");
            return ret == 0 ? E_ERR : ret;
        }
        id = ret;
    } else {
        assetUri = fileAsset_->GetUri();
    }
    if (assetUri.empty()) {
        LOGE("Failed to check empty asset uri");
        return E_ERR;
    }

    if (IsMovingPhoto()) {
        ret = CopyMovingPhotoVideo(assetUri);
        if (ret != E_OK) {
            LOGE("Failed to copy data to moving photo video with error: %{public}d", ret);
            return ret;
        }
    }

    Uri uri(assetUri);
    UniqueFd destFd(UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY));
    if (destFd.Get() < 0) {
        LOGE("Failed to open %{private}s with error: %{public}d", assetUri.c_str(), destFd.Get());
        return destFd.Get();
    }

    if (mode == AddResourceMode::FILE_URI) {
        ret = CopyFileToMediaLibrary(destFd);
    } else if (mode == AddResourceMode::DATA_BUFFER) {
        ret = CopyDataBufferToMediaLibrary(destFd);
    } else {
        LOGE("Invalid mode: %{public}d", mode);
        return E_INVALID_VALUES;
    }

    if (ret == E_OK && isCreation) {
        SetNewFileAsset(id, assetUri);
    }
    return ret;
}

bool MediaAssetChangeRequestImpl::ContainsResource(ResourceType resourceType) const
{
    return std::find(addResourceTypes_.begin(), addResourceTypes_.end(), resourceType) != addResourceTypes_.end();
}

bool MediaAssetChangeRequestImpl::IsMovingPhoto() const
{
    return fileAsset_ != nullptr &&
           (fileAsset_->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
               (fileAsset_->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::DEFAULT) &&
                   fileAsset_->GetMovingPhotoEffectMode() == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)));
}

bool MediaAssetChangeRequestImpl::CheckMovingPhotoResource(ResourceType resourceType) const
{
    if (resourceType == ResourceType::INVALID_RESOURCE) {
        LOGE("Invalid resource type");
        return false;
    }

    bool isResourceTypeVaild = !ContainsResource(resourceType);
    int addResourceTimes =
        std::count(assetChangeOperations_.begin(), assetChangeOperations_.end(), AssetChangeOperation::ADD_RESOURCE);
    return isResourceTypeVaild && addResourceTimes <= 1; // currently, add resource no more than once
}

static const std::unordered_map<MovingPhotoEffectMode, std::unordered_map<ResourceType, bool>>
    EFFECT_MODE_RESOURCE_CHECK = {
        { MovingPhotoEffectMode::DEFAULT,
            { { ResourceType::IMAGE_RESOURCE, false }, { ResourceType::VIDEO_RESOURCE, false } } },
        { MovingPhotoEffectMode::BOUNCE_PLAY,
            { { ResourceType::IMAGE_RESOURCE, false }, { ResourceType::VIDEO_RESOURCE, true } } },
        { MovingPhotoEffectMode::LOOP_PLAY,
            { { ResourceType::IMAGE_RESOURCE, false }, { ResourceType::VIDEO_RESOURCE, true } } },
        { MovingPhotoEffectMode::CINEMA_GRAPH,
            { { ResourceType::IMAGE_RESOURCE, false }, { ResourceType::VIDEO_RESOURCE, true } } },
        { MovingPhotoEffectMode::LONG_EXPOSURE,
            { { ResourceType::IMAGE_RESOURCE, true }, { ResourceType::VIDEO_RESOURCE, false } } },
        { MovingPhotoEffectMode::MULTI_EXPOSURE,
            { { ResourceType::IMAGE_RESOURCE, true }, { ResourceType::VIDEO_RESOURCE, false } } },
        { MovingPhotoEffectMode::IMAGE_ONLY,
            { { ResourceType::IMAGE_RESOURCE, false }, { ResourceType::VIDEO_RESOURCE, false } } },
    };

bool MediaAssetChangeRequestImpl::CheckEffectModeWriteOperation()
{
    if (fileAsset_ == nullptr) {
        LOGE("fileAsset is nullptr");
        return false;
    }

    if (fileAsset_->GetTimePending() != 0) {
        LOGE("Failed to check pending of fileAsset: %{public}" PRId64, fileAsset_->GetTimePending());
        return false;
    }

    MovingPhotoEffectMode effectMode = static_cast<MovingPhotoEffectMode>(fileAsset_->GetMovingPhotoEffectMode());
    auto iter = EFFECT_MODE_RESOURCE_CHECK.find(effectMode);
    if (iter == EFFECT_MODE_RESOURCE_CHECK.end()) {
        LOGE("Failed to check effect mode: %{public}d", static_cast<int32_t>(effectMode));
        return false;
    }

    bool isImageExist = ContainsResource(ResourceType::IMAGE_RESOURCE);
    bool isVideoExist = ContainsResource(ResourceType::VIDEO_RESOURCE);
    if (iter->second.at(ResourceType::IMAGE_RESOURCE) && !isImageExist) {
        LOGE("Failed to check image resource for effect mode: %{public}d", static_cast<int32_t>(effectMode));
        return false;
    }
    if (iter->second.at(ResourceType::VIDEO_RESOURCE) && !isVideoExist) {
        LOGE("Failed to check video resource for effect mode: %{public}d", static_cast<int32_t>(effectMode));
        return false;
    }
    return true;
}

bool MediaAssetChangeRequestImpl::CheckMovingPhotoWriteOperation()
{
    if (Contains(AssetChangeOperation::SET_MOVING_PHOTO_EFFECT_MODE)) {
        return CheckEffectModeWriteOperation();
    }

    bool containsAddResource = Contains(AssetChangeOperation::ADD_RESOURCE);
    if (!containsAddResource) {
        return true;
    }

    bool isCreation = Contains(AssetChangeOperation::CREATE_FROM_SCRATCH);
    if (!isCreation) {
        return true;
    }

    int addResourceTimes =
        std::count(assetChangeOperations_.begin(), assetChangeOperations_.end(), AssetChangeOperation::ADD_RESOURCE);
    bool isImageExist = ContainsResource(ResourceType::IMAGE_RESOURCE);
    bool isVideoExist = ContainsResource(ResourceType::VIDEO_RESOURCE);
    return addResourceTimes == 2 && isImageExist && isVideoExist; // must add resource 2 times with image and video
}

bool MediaAssetChangeRequestImpl::CheckChangeOperations()
{
    if (assetChangeOperations_.empty()) {
        LOGE("None request to apply");
        return false;
    }

    bool isCreateFromScratch = Contains(AssetChangeOperation::CREATE_FROM_SCRATCH);
    bool isCreateFromUri = Contains(AssetChangeOperation::CREATE_FROM_URI);
    bool containsEdit = Contains(AssetChangeOperation::SET_EDIT_DATA);
    bool containsGetHandler = Contains(AssetChangeOperation::GET_WRITE_CACHE_HANDLER);
    bool containsAddResource = Contains(AssetChangeOperation::ADD_RESOURCE);
    bool isSaveCameraPhoto = Contains(AssetChangeOperation::SAVE_CAMERA_PHOTO);
    if ((isCreateFromScratch || containsEdit) && !containsGetHandler && !containsAddResource && !isSaveCameraPhoto) {
        LOGE("Cannot create or edit asset without data to write");
        return false;
    }

    if (containsEdit && (isCreateFromScratch || isCreateFromUri)) {
        LOGE("Cannot create together with edit");
        return false;
    }

    auto fileAsset = GetFileAssetInstance();
    if (fileAsset == nullptr) {
        LOGE("fileAsset is null");
        return false;
    }

    AssetChangeOperation firstOperation = assetChangeOperations_.front();
    if (fileAsset->GetId() <= 0 && firstOperation != AssetChangeOperation::CREATE_FROM_SCRATCH &&
        firstOperation != AssetChangeOperation::CREATE_FROM_URI) {
        LOGE("Invalid asset change request");
        return false;
    }

    bool isMovingPhoto = IsMovingPhoto();
    if (isMovingPhoto && !CheckMovingPhotoWriteOperation()) {
        LOGE("Invalid write operation for moving photo");
        return false;
    }

    return true;
}

static int32_t SendFile(const UniqueFd& srcFd, const UniqueFd& destFd)
{
    if (srcFd.Get() < 0 || destFd.Get() < 0) {
        LOGE("Failed to check srcFd: %{public}d and destFd: %{public}d", srcFd.Get(), destFd.Get());
        return E_ERR;
    }

    struct stat statSrc {};
    int32_t status = fstat(srcFd.Get(), &statSrc);
    if (status != 0) {
        LOGE("Failed to get file stat, errno=%{public}d", errno);
        return status;
    }

    off_t offset = 0;
    off_t fileSize = statSrc.st_size;
    while (offset < fileSize) {
        ssize_t sent = sendfile(destFd.Get(), srcFd.Get(), &offset, fileSize - offset);
        if (sent < 0) {
            LOGE("Failed to sendfile with errno=%{public}d, srcFd=%{private}d, destFd=%{private}d", errno, srcFd.Get(),
                destFd.Get());
            return sent;
        }
    }

    return E_OK;
}

int32_t MediaAssetChangeRequestImpl::CopyFileToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo)
{
    std::string srcRealPath = isMovingPhotoVideo ? movingPhotoVideoRealPath_ : realPath_;
    if (srcRealPath.empty()) {
        LOGE("Failed to check real path of source");
        return E_FAIL;
    }

    std::string absFilePath;
    if (!PathToRealPath(srcRealPath, absFilePath)) {
        LOGE("Not real path %{private}s", srcRealPath.c_str());
        return E_FAIL;
    }
    UniqueFd srcFd(open(absFilePath.c_str(), O_RDONLY));
    if (srcFd.Get() < 0) {
        LOGE("Failed to open %{private}s, errno=%{public}d", absFilePath.c_str(), errno);
        return srcFd.Get();
    }

    int32_t err = SendFile(srcFd, destFd);
    if (err != E_OK) {
        LOGE("Failed to send file from %{public}d to %{public}d", srcFd.Get(), destFd.Get());
    }
    return err;
}

int32_t MediaAssetChangeRequestImpl::CopyDataBufferToMediaLibrary(const UniqueFd& destFd, bool isMovingPhotoVideo)
{
    size_t offset = 0;
    size_t length = isMovingPhotoVideo ? movingPhotoVideoBufferSize_ : dataBufferSize_;
    void* dataBuffer = isMovingPhotoVideo ? movingPhotoVideoDataBuffer_ : dataBuffer_;
    while (offset < length) {
        ssize_t written = write(destFd.Get(), (char*)dataBuffer + offset, length - offset);
        if (written < 0) {
            LOGE("Failed to copy data buffer, return %{public}d", static_cast<int>(written));
            return written;
        }
        offset += static_cast<size_t>(written);
    }
    return E_OK;
}

int32_t MediaAssetChangeRequestImpl::CopyMovingPhotoVideo(const std::string& assetUri)
{
    if (assetUri.empty()) {
        LOGE("Failed to check empty asset uri");
        return E_INVALID_URI;
    }

    std::string videoUri = assetUri;
    MediaFileUtils::UriAppendKeyValue(videoUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD, CONST_OPEN_MOVING_PHOTO_VIDEO);
    Uri uri(videoUri);
    int videoFd = UserFileClient::OpenFile(uri, MEDIA_FILEMODE_WRITEONLY);
    if (videoFd < 0) {
        LOGE("Failed to open video of moving photo with write-only mode");
        return videoFd;
    }

    int32_t ret = E_ERR;
    UniqueFd uniqueFd(videoFd);
    if (movingPhotoVideoResourceMode_ == AddResourceMode::FILE_URI) {
        ret = CopyFileToMediaLibrary(uniqueFd, true);
    } else if (movingPhotoVideoResourceMode_ == AddResourceMode::DATA_BUFFER) {
        ret = CopyDataBufferToMediaLibrary(uniqueFd, true);
    } else {
        LOGE("Invalid mode: %{public}d", movingPhotoVideoResourceMode_);
        return E_INVALID_VALUES;
    }
    return ret;
}

void MediaAssetChangeRequestImpl::SetNewFileAsset(int32_t id, const std::string& uri)
{
    if (fileAsset_ == nullptr) {
        LOGE("fileAsset_ is nullptr");
        return;
    }

    if (id <= 0 || uri.empty()) {
        LOGE("Failed to check file_id: %{public}d and uri: %{public}s", id, uri.c_str());
        return;
    }
    fileAsset_->SetId(id);
    fileAsset_->SetUri(uri);
    fileAsset_->SetTimePending(0);
}

int32_t MediaAssetChangeRequestImpl::CJSetTitle(std::string title)
{
    if (fileAsset_ == nullptr) {
        return JS_INNER_FAIL;
    }
    std::string extension = MediaFileUtils::SplitByChar(fileAsset_->GetDisplayName(), '.');
    std::string displayName = title + "." + extension;
    if (MediaFileUtils::CheckDisplayName(displayName) != E_OK) {
        return OHOS_INVALID_PARAM_CODE;
    }
    fileAsset_->SetTitle(title);
    fileAsset_->SetDisplayName(displayName);
    RecordChangeOperation(AssetChangeOperation::SET_TITLE);
    if (Contains(AssetChangeOperation::CREATE_FROM_SCRATCH) || Contains(AssetChangeOperation::CREATE_FROM_URI)) {
        creationValuesBucket_.valuesMap[CONST_MEDIA_DATA_DB_NAME] = displayName;
        creationValuesBucket_.valuesMap[PhotoColumn::MEDIA_TITLE] = title;
    }
    return 0;
}

static int32_t OpenWriteCacheHandler(MediaAssetChangeRequestImpl* changeRequest, bool isMovingPhotoVideo = false)
{
    auto fileAsset = changeRequest->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        return E_FAIL;
    }
    std::string extension = isMovingPhotoVideo ? MOVING_PHOTO_VIDEO_EXTENSION
                                               : MediaFileUtils::GetExtensionFromPath(fileAsset->GetDisplayName());
    int64_t currentTimestamp = MediaFileUtils::UTCTimeNanoSeconds();
    uint32_t cacheFileId = changeRequest->FetchAddCacheFileId();
    std::string cacheFileName = std::to_string(currentTimestamp) + "_" + std::to_string(cacheFileId) + "." + extension;
    std::string uri = PhotoColumn::PHOTO_CACHE_URI_PREFIX + cacheFileName;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri openCacheUri(uri);
    int32_t ret = UserFileClient::OpenFile(openCacheUri, MEDIA_FILEMODE_WRITEONLY);
    if (ret == E_PERMISSION_DENIED) {
        LOGE("Open cache file failed, permission denied");
        return OHOS_PERMISSION_DENIED_CODE;
    }
    if (ret < 0) {
        LOGE("Open cache file failed, ret: %{public}d", ret);
    }

    if (isMovingPhotoVideo) {
        changeRequest->SetCacheMovingPhotoVideoName(cacheFileName);
    } else {
        changeRequest->SetCacheFileName(cacheFileName);
    }
    return ret;
}

int32_t MediaAssetChangeRequestImpl::CJGetWriteCacheHandler(int32_t* errCode)
{
    if (fileAsset_ == nullptr) {
        *errCode = JS_INNER_FAIL;
        return 0;
    }
    if (IsMovingPhoto()) {
        *errCode = JS_E_OPERATION_NOT_SUPPORT;
        return 0;
    }
    if (Contains(AssetChangeOperation::CREATE_FROM_URI) || Contains(AssetChangeOperation::GET_WRITE_CACHE_HANDLER) ||
        Contains(AssetChangeOperation::ADD_RESOURCE)) {
        *errCode = JS_E_OPERATION_NOT_SUPPORT;
        return 0;
    }
    int32_t ret = OpenWriteCacheHandler(this);
    if (ret < 0) {
        LOGE("Failed to open write cache handler, ret: %{public}d", ret);
        *errCode = ret;
        return 0;
    }
    RecordChangeOperation(AssetChangeOperation::GET_WRITE_CACHE_HANDLER);
    return ret;
}

static int32_t CheckWriteOperation(
    MediaAssetChangeRequestImpl* changeRequest, ResourceType resourceType = ResourceType::INVALID_RESOURCE)
{
    if (changeRequest == nullptr) {
        LOGE("changeRequest is null");
        return OHOS_INVALID_PARAM_CODE;
    }

    if (changeRequest->IsMovingPhoto()) {
        if (!changeRequest->CheckMovingPhotoResource(resourceType)) {
            LOGE("Failed to check resource to add for moving photo");
            return OHOS_INVALID_PARAM_CODE;
        }
        return 0;
    }

    if (changeRequest->Contains(AssetChangeOperation::CREATE_FROM_URI) ||
        changeRequest->Contains(AssetChangeOperation::GET_WRITE_CACHE_HANDLER) ||
        changeRequest->Contains(AssetChangeOperation::ADD_RESOURCE)) {
        LOGE("The previous asset creation/modification request has not been applied");
        return JS_E_OPERATION_NOT_SUPPORT;
    }
    return 0;
}

static ResourceType GetResourceType(int32_t value)
{
    ResourceType result = ResourceType::INVALID_RESOURCE;
    switch (value) {
        case static_cast<int32_t>(ResourceType::IMAGE_RESOURCE):
        case static_cast<int32_t>(ResourceType::VIDEO_RESOURCE):
        case static_cast<int32_t>(ResourceType::PHOTO_PROXY):
            result = static_cast<ResourceType>(value);
            break;
        default:
            break;
    }
    return result;
}

static bool CheckMovingPhotoVideo(void* dataBuffer, size_t size)
{
    auto dataSource = std::make_shared<MediaDataSource>(dataBuffer, static_cast<int64_t>(size));
    auto avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper == nullptr) {
        LOGE("Failed to create AVMetadataHelper, ignore checking duration of moving photo video");
        return true;
    }

    int32_t err = avMetadataHelper->SetSource(dataSource);
    if (err != E_OK) {
        LOGE("SetSource failed for dataSource, err = %{public}d", err);
        return false;
    }

    std::unordered_map<int32_t, std::string> resultMap = avMetadataHelper->ResolveMetadata();
    if (resultMap.find(AV_KEY_DURATION) == resultMap.end()) {
        LOGE("AV_KEY_DURATION does not exist");
        return false;
    }

    std::string durationStr = resultMap.at(AV_KEY_DURATION);
    int32_t duration = std::atoi(durationStr.c_str());
    if (!MediaFileUtils::CheckMovingPhotoVideoDuration(duration)) {
        LOGE("Failed to check duration of moving photo video: %{public}d ms", duration);
        return false;
    }
    return true;
}

int32_t MediaAssetChangeRequestImpl::AddMovingPhotoVideoResource(std::string fileUri)
{
    std::string realPath;
    if (!ParseFileUri(fileUri, MediaType::MEDIA_TYPE_VIDEO, realPath)) {
        return OHOS_INVALID_PARAM_CODE;
    }
    if (!MediaFileUtils::CheckMovingPhotoVideo(realPath)) {
        LOGE("Failed to check video resource of moving photo");
        return OHOS_INVALID_PARAM_CODE;
    }
    movingPhotoVideoRealPath_ = realPath;
    movingPhotoVideoResourceMode_ = AddResourceMode::FILE_URI;
    RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    addResourceTypes_.push_back(ResourceType::VIDEO_RESOURCE);
    return 0;
}

int32_t MediaAssetChangeRequestImpl::AddMovingPhotoVideoResource(uint8_t* dataBuffer, size_t dataBufferSize)
{
    movingPhotoVideoDataBuffer_ = dataBuffer;
    movingPhotoVideoBufferSize_ = dataBufferSize;
    if (!CheckMovingPhotoVideo(movingPhotoVideoDataBuffer_, movingPhotoVideoBufferSize_)) {
        LOGE("Failed to check video resource of moving photo");
        return OHOS_INVALID_PARAM_CODE;
    }
    movingPhotoVideoResourceMode_ = AddResourceMode::DATA_BUFFER;
    RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    addResourceTypes_.push_back(ResourceType::VIDEO_RESOURCE);
    return 0;
}

int32_t MediaAssetChangeRequestImpl::CJAddResource(int32_t resourceType, std::string fileUri)
{
    if (fileAsset_ == nullptr) {
        return JS_INNER_FAIL;
    }
    if (CheckWriteOperation(this, GetResourceType(resourceType)) != E_OK) {
        return JS_E_OPERATION_NOT_SUPPORT;
    }
    if (IsMovingPhoto() && resourceType == static_cast<int32_t>(ResourceType::VIDEO_RESOURCE)) {
        return AddMovingPhotoVideoResource(fileUri);
    }
    if (!(resourceType == static_cast<int32_t>(fileAsset_->GetMediaType()) ||
            resourceType == static_cast<int32_t>(ResourceType::PHOTO_PROXY))) {
        LOGE("Failed to check resourceType");
        return OHOS_INVALID_PARAM_CODE;
    }
    std::string realPath;
    if (!ParseFileUri(fileUri, fileAsset_->GetMediaType(), realPath)) {
        return OHOS_INVALID_PARAM_CODE;
    }
    realPath_ = realPath;
    addResourceMode_ = AddResourceMode::FILE_URI;
    RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    addResourceTypes_.push_back(GetResourceType(resourceType));
    return 0;
}

int32_t MediaAssetChangeRequestImpl::CJAddResource(int32_t resourceType, uint8_t* dataBuffer, size_t dataBufferSize)
{
    if (dataBufferSize <= 0) {
        LOGE("Failed to check size of data buffer");
        return OHOS_INVALID_PARAM_CODE;
    }
    if (fileAsset_ == nullptr) {
        return JS_INNER_FAIL;
    }
    if (CheckWriteOperation(this, GetResourceType(resourceType)) != E_OK) {
        return JS_E_OPERATION_NOT_SUPPORT;
    }
    if (IsMovingPhoto() && resourceType == static_cast<int32_t>(ResourceType::VIDEO_RESOURCE)) {
        return AddMovingPhotoVideoResource(dataBuffer, dataBufferSize);
    }
    if (!(resourceType == static_cast<int32_t>(fileAsset_->GetMediaType()) ||
            resourceType == static_cast<int32_t>(ResourceType::PHOTO_PROXY))) {
        LOGE("Failed to check resourceType");
        return OHOS_INVALID_PARAM_CODE;
    }
    dataBuffer_ = dataBuffer;
    dataBufferSize_ = dataBufferSize;
    addResourceMode_ = AddResourceMode::DATA_BUFFER;
    RecordChangeOperation(AssetChangeOperation::ADD_RESOURCE);
    addResourceTypes_.push_back(GetResourceType(resourceType));
    return 0;
}

int32_t MediaAssetChangeRequestImpl::CJSaveCameraPhoto()
{
    if (fileAsset_ == nullptr) {
        return JS_INNER_FAIL;
    }
    if (Contains(AssetChangeOperation::SET_EDIT_DATA) && !Contains(AssetChangeOperation::ADD_FILTERS)) {
        RecordChangeOperation(AssetChangeOperation::ADD_FILTERS);
    }
    RecordChangeOperation(AssetChangeOperation::SAVE_CAMERA_PHOTO);
    return 0;
}

int32_t MediaAssetChangeRequestImpl::CJDiscardCameraPhoto()
{
    if (fileAsset_ == nullptr) {
        return JS_INNER_FAIL;
    }
    RecordChangeOperation(AssetChangeOperation::DISCARD_CAMERA_PHOTO);
    return 0;
}

int32_t MediaAssetChangeRequestImpl::CJSetOrientation(int32_t orientation)
{
    if (fileAsset_ == nullptr) {
        return OHOS_INVALID_PARAM_CODE;
    }
    if (std::find(ORIENTATION_ARRAY.begin(), ORIENTATION_ARRAY.end(), orientation) == ORIENTATION_ARRAY.end()) {
        LOGE("orientationValue value is invalid.");
        return OHOS_INVALID_PARAM_CODE;
    }
    fileAsset_->SetOrientation(orientation);
    RecordChangeOperation(AssetChangeOperation::SET_ORIENTATION);
    return 0;
}

static bool IsCreation(MediaAssetChangeRequestImpl* changeRequest)
{
    auto assetChangeOperations = changeRequest->GetAssetChangeOperations();
    bool isCreateFromScratch = std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
        AssetChangeOperation::CREATE_FROM_SCRATCH) != assetChangeOperations.end();
    bool isCreateFromUri = std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
        AssetChangeOperation::CREATE_FROM_URI) != assetChangeOperations.end();
    return isCreateFromScratch || isCreateFromUri;
}

static bool IsSetEffectMode(MediaAssetChangeRequestImpl* changeRequest)
{
    auto assetChangeOperations = changeRequest->GetAssetChangeOperations();
    return std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
        AssetChangeOperation::SET_MOVING_PHOTO_EFFECT_MODE) != assetChangeOperations.end();
}

static bool WriteBySecurityComponent(MediaAssetChangeRequestImpl* changeRequest)
{
    bool isCreation = IsCreation(changeRequest);
    int32_t ret = E_FAIL;
    auto assetChangeOperations = changeRequest->GetAssetChangeOperations();
    bool isCreateFromUri = std::find(assetChangeOperations.begin(), assetChangeOperations.end(),
        AssetChangeOperation::CREATE_FROM_URI) != assetChangeOperations.end();
    if (isCreateFromUri) {
        ret = changeRequest->CopyToMediaLibrary(isCreation, AddResourceMode::FILE_URI);
    } else {
        ret = changeRequest->CopyToMediaLibrary(isCreation, changeRequest->GetAddResourceMode());
    }

    if (ret < 0) {
        LOGE("Failed to write by security component, ret: %{public}d", ret);
        return false;
    }
    return true;
}

static bool SendToCacheFile(
    MediaAssetChangeRequestImpl* changeRequest, const UniqueFd& destFd, bool isMovingPhotoVideo = false)
{
    std::string realPath =
        isMovingPhotoVideo ? changeRequest->GetMovingPhotoVideoPath() : changeRequest->GetFileRealPath();

    std::string absFilePath;
    if (!PathToRealPath(realPath, absFilePath)) {
        LOGE("Not real path %{private}s, errno=%{public}d", realPath.c_str(), errno);
        return false;
    }

    UniqueFd srcFd(open(absFilePath.c_str(), O_RDONLY));
    if (srcFd.Get() < 0) {
        LOGE("Failed to open file, errno=%{public}d", errno);
        return false;
    }

    int32_t err = SendFile(srcFd, destFd);
    if (err != E_OK) {
        LOGE("Failed to send file from %{public}d to %{public}d", srcFd.Get(), destFd.Get());
        return false;
    }
    return true;
}

int32_t MediaAssetChangeRequestImpl::SubmitCache(bool isCreation, bool isSetEffectMode)
{
    if (fileAsset_ == nullptr) {
        return E_FAIL;
    }
    if (cacheFileName_.empty() && cacheMovingPhotoVideoName_.empty()) {
        LOGE("Failed to check cache file");
        return E_FAIL;
    }
    std::string uri = CONST_PAH_SUBMIT_CACHE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri submitCacheUri(uri);
    std::string assetUri;
    int32_t ret;
    if (isCreation) {
        bool isValid = false;
        std::string displayName = creationValuesBucket_.Get(CONST_MEDIA_DATA_DB_NAME, isValid);
        if (!isValid || MediaFileUtils::CheckDisplayName(displayName) != E_OK) {
            LOGE("Failed to check displayName");
            return E_FAIL;
        }
        creationValuesBucket_.Put(CONST_CACHE_FILE_NAME, cacheFileName_);
        if (IsMovingPhoto()) {
            creationValuesBucket_.Put(CONST_CACHE_MOVING_PHOTO_VIDEO_NAME, cacheMovingPhotoVideoName_);
        }
        ret = UserFileClient::InsertExt(submitCacheUri, creationValuesBucket_, assetUri);
    } else {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(PhotoColumn::MEDIA_ID, fileAsset_->GetId());
        valuesBucket.Put(CONST_CACHE_FILE_NAME, cacheFileName_);
        ret = PutMediaAssetEditData(valuesBucket);
        if (ret != E_OK) {
            LOGE("Failed to put editData");
            return E_FAIL;
        }
        if (IsMovingPhoto()) {
            valuesBucket.Put(CONST_CACHE_MOVING_PHOTO_VIDEO_NAME, cacheMovingPhotoVideoName_);
        }
        if (isSetEffectMode) {
            valuesBucket.Put(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, fileAsset_->GetMovingPhotoEffectMode());
            valuesBucket.Put(CONST_CACHE_MOVING_PHOTO_VIDEO_NAME, cacheMovingPhotoVideoName_);
        }
        ret = UserFileClient::Insert(submitCacheUri, valuesBucket);
    }
    if (ret > 0 && isCreation) {
        SetNewFileAsset(ret, assetUri);
    }
    cacheFileName_.clear();
    cacheMovingPhotoVideoName_.clear();
    return ret;
}

static bool SubmitCacheExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    bool isCreation = IsCreation(changeRequest);
    bool isSetEffectMode = IsSetEffectMode(changeRequest);
    int32_t ret = changeRequest->SubmitCache(isCreation, isSetEffectMode);
    if (ret < 0) {
        LOGE("Failed to write cache, ret: %{public}d", ret);
        return false;
    }
    return true;
}

static bool CreateFromFileUriExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    if (!HasWritePermission()) {
        return WriteBySecurityComponent(changeRequest);
    }

    int32_t cacheFd = OpenWriteCacheHandler(changeRequest);
    if (cacheFd < 0) {
        LOGE("Failed to open write cache handler, err: %{public}d", cacheFd);
        return false;
    }

    UniqueFd uniqueFd(cacheFd);
    if (!SendToCacheFile(changeRequest, uniqueFd)) {
        LOGE("Faild to write cache file");
        return false;
    }
    return SubmitCacheExecute(changeRequest);
}

static bool HasAddResource(std::vector<ResourceType> addResourceTypes, ResourceType resourceType)
{
    return std::find(addResourceTypes.begin(), addResourceTypes.end(), resourceType) != addResourceTypes.end();
}

static bool WriteCacheByArrayBuffer(
    MediaAssetChangeRequestImpl* changeRequest, const UniqueFd& destFd, bool isMovingPhotoVideo = false)
{
    size_t offset = 0;
    size_t length = isMovingPhotoVideo ? changeRequest->GetMovingPhotoVideoSize() : changeRequest->GetDataBufferSize();
    void* dataBuffer = isMovingPhotoVideo ? changeRequest->GetMovingPhotoVideoBuffer() : changeRequest->GetDataBuffer();
    while (offset < length) {
        ssize_t written = write(destFd.Get(), (char*)dataBuffer + offset, length - offset);
        if (written < 0) {
            LOGE("Failed to write data buffer to cache file, return %{public}d", static_cast<int>(written));
            return false;
        }
        offset += static_cast<size_t>(written);
    }
    return true;
}

static int SavePhotoProxyImage(const UniqueFd& destFd, sptr<PhotoProxy> photoProxyPtr)
{
    void* imageAddr = photoProxyPtr->GetFileDataAddr();
    size_t imageSize = photoProxyPtr->GetFileSize();
    if (imageAddr == nullptr || imageSize == 0) {
        LOGE("imageAddr is nullptr or imageSize(%{public}zu)==0", imageSize);
        return E_ERR;
    }

    NAPI_INFO_LOG("start pack PixelMap");
    Media::InitializationOptions opts;
    opts.pixelFormat = Media::PixelFormat::RGBA_8888;
    opts.size = { .width = photoProxyPtr->GetWidth(), .height = photoProxyPtr->GetHeight() };
    auto pixelMap = Media::PixelMap::Create(opts);
    if (pixelMap == nullptr) {
        LOGE("Create pixelMap failed.");
        return E_ERR;
    }
    pixelMap->SetPixelsAddr(imageAddr, nullptr, imageSize, Media::AllocatorType::SHARE_MEM_ALLOC, nullptr);
    auto pixelSize = static_cast<uint32_t>(pixelMap->GetByteCount());

    // encode rgba to jpeg
    auto buffer = new (std::nothrow) uint8_t[pixelSize];
    int64_t packedSize = 0L;
    Media::ImagePacker imagePacker;
    Media::PackOption packOption;
    packOption.format = "image/jpeg";
    imagePacker.StartPacking(buffer, pixelSize, packOption);
    imagePacker.AddImage(*pixelMap);
    imagePacker.FinalizePacking(packedSize);
    if (buffer == nullptr) {
        LOGE("packet pixelMap failed");
        return E_ERR;
    }
    NAPI_INFO_LOG("pack pixelMap success, packedSize: %{public}" PRId64, packedSize);

    int ret = write(destFd, buffer, packedSize);
    if (ret < 0) {
        LOGE("Failed to write photo proxy to cache file, return %{public}d", ret);
        delete[] buffer;
        return ret;
    }
    delete[] buffer;
    return ret;
}

static bool AddPhotoProxyResourceExecute(MediaAssetChangeRequestImpl* changeRequest, const UniqueFd& destFd)
{
    std::string uri = CONST_PAH_ADD_IMAGE;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);

    auto fileAsset = changeRequest->GetFileAssetInstance();
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(PhotoColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ std::to_string(fileAsset->GetId()) });

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_ID, changeRequest->GetPhotoProxyObj()->GetPhotoId());
    NAPI_INFO_LOG("photoId: %{public}s", changeRequest->GetPhotoProxyObj()->GetPhotoId().c_str());
    valuesBucket.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE,
        static_cast<int32_t>(changeRequest->GetPhotoProxyObj()->GetDeferredProcType()));
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileAsset->GetId());
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        LOGE("Failed to set, err: %{public}d", changedRows);
        return false;
    }

    int err = SavePhotoProxyImage(destFd, changeRequest->GetPhotoProxyObj());
    changeRequest->ReleasePhotoProxyObj();
    if (err < 0) {
        LOGE("Failed to saveImage , err: %{public}d", err);
        return false;
    }
    return true;
}

static bool AddResourceByMode(MediaAssetChangeRequestImpl* changeRequest, const UniqueFd& uniqueFd,
    AddResourceMode mode, bool isMovingPhotoVideo = false)
{
    bool isWriteSuccess = false;
    if (mode == AddResourceMode::DATA_BUFFER) {
        isWriteSuccess = WriteCacheByArrayBuffer(changeRequest, uniqueFd, isMovingPhotoVideo);
    } else if (mode == AddResourceMode::FILE_URI) {
        isWriteSuccess = SendToCacheFile(changeRequest, uniqueFd, isMovingPhotoVideo);
    } else if (mode == AddResourceMode::PHOTO_PROXY) {
        isWriteSuccess = AddPhotoProxyResourceExecute(changeRequest, uniqueFd);
    } else {
        LOGE("Unsupported addResource mode");
    }
    return isWriteSuccess;
}

static bool AddMovingPhotoVideoExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    int32_t cacheVideoFd = OpenWriteCacheHandler(changeRequest, true);
    if (cacheVideoFd < 0) {
        LOGE("Failed to open cache moving photo video, err: %{public}d", cacheVideoFd);
        return false;
    }

    UniqueFd uniqueFd(cacheVideoFd);
    AddResourceMode mode = changeRequest->GetMovingPhotoVideoMode();
    if (!AddResourceByMode(changeRequest, uniqueFd, mode, true)) {
        LOGE("Faild to write cache file");
        return false;
    }
    return true;
}

static bool AddResourceExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    if (!HasWritePermission()) {
        return WriteBySecurityComponent(changeRequest);
    }

    if (changeRequest->IsMovingPhoto() &&
        HasAddResource(changeRequest->GetAddResourceTypes(), ResourceType::VIDEO_RESOURCE) &&
        !AddMovingPhotoVideoExecute(changeRequest)) {
        LOGE("Faild to write cache file for video of moving photo");
        return false;
    }

    // image resource is not mandatory when setting effect mode of moving photo
    if (changeRequest->IsMovingPhoto() &&
        !HasAddResource(changeRequest->GetAddResourceTypes(), ResourceType::IMAGE_RESOURCE)) {
        return SubmitCacheExecute(changeRequest);
    }

    int32_t cacheFd = OpenWriteCacheHandler(changeRequest);
    if (cacheFd < 0) {
        LOGE("Failed to open write cache handler, err: %{public}d", cacheFd);
        return false;
    }

    UniqueFd uniqueFd(cacheFd);
    AddResourceMode mode = changeRequest->GetAddResourceMode();
    if (!AddResourceByMode(changeRequest, uniqueFd, mode)) {
        LOGE("Faild to write cache file");
        return false;
    }
    return SubmitCacheExecute(changeRequest);
}

static bool UpdateAssetProperty(MediaAssetChangeRequestImpl* changeRequest, std::string uri,
    DataShare::DataSharePredicates& predicates, DataShare::DataShareValuesBucket& valuesBucket)
{
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        LOGE("Failed to update property of asset, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetTitleExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    // In the scenario of creation, the new title will be applied when the asset is created.
    AssetChangeOperation firstOperation = changeRequest->GetAssetChangeOperations().front();
    if (firstOperation == AssetChangeOperation::CREATE_FROM_SCRATCH ||
        firstOperation == AssetChangeOperation::CREATE_FROM_URI) {
        return true;
    }

    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::MEDIA_TITLE, fileAsset->GetTitle());
    return UpdateAssetProperty(changeRequest, CONST_PAH_UPDATE_PHOTO, predicates, valuesBucket);
}

static void DiscardHighQualityPhoto(MediaAssetChangeRequestImpl* changeRequest)
{
    std::string uriStr = CONST_PAH_REMOVE_MSC_TASK;
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri uri(uriStr);
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    std::vector<std::string> columns { std::to_string(fileAsset->GetId()) };
    UserFileClient::Query(uri, predicates, columns, errCode);
}

static bool SaveCameraPhotoExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    auto changeOpreations = changeRequest->GetAssetChangeOperations();
    bool containsAddResource = std::find(changeOpreations.begin(), changeOpreations.end(),
        AssetChangeOperation::ADD_RESOURCE) != changeOpreations.end();
    if (containsAddResource && !MediaLibraryNapiUtils::IsSystemApp()) {
        // remove high quality photo
        LOGI("discard high quality photo because add resource by third app");
        DiscardHighQualityPhoto(changeRequest);
    }

    // The watermark will trigger the scan. If the watermark is turned on, there is no need to trigger the scan again.
    bool needScan = std::find(changeOpreations.begin(), changeOpreations.end(),
        AssetChangeOperation::ADD_FILTERS) == changeOpreations.end();

    auto fileAsset = changeRequest->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        LOGE("fileAsset is nullptr");
        return false;
    }
    std::string uriStr = CONST_PAH_SAVE_CAMERA_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, CONST_MEDIA_OPERN_KEYWORD, std::to_string(needScan));
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_FILE_PATH, fileAsset->GetUri());
    MediaLibraryNapiUtils::UriAppendKeyValue(uriStr, PhotoColumn::MEDIA_ID, std::to_string(fileAsset->GetId()));
    MediaLibraryNapiUtils::UriAppendKeyValue(
        uriStr, PhotoColumn::PHOTO_SUBTYPE, std::to_string(fileAsset->GetPhotoSubType()));
    MediaLibraryNapiUtils::UriAppendKeyValue(
        uriStr, CONST_IMAGE_FILE_TYPE, std::to_string(changeRequest->GetImageFileType()));
    Uri uri(uriStr);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    DataShare::DataSharePredicates predicates;
    auto ret = UserFileClient::Update(uri, predicates, valuesBucket);
    if (ret < 0) {
        LOGE("save camera photo fail");
    }
    return true;
}

static bool AddFiltersExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    auto fileAsset = changeRequest->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        LOGE("Failed to check fileAsset");
        return false;
    }
    std::string uri = CONST_PAH_ADD_FILTERS;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri addFiltersUri(uri);

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileAsset->GetId());
    int ret = changeRequest->PutMediaAssetEditData(valuesBucket);
    if (ret != E_OK) {
        LOGE("Failed to put editData");
        return false;
    }
    ret = UserFileClient::Insert(addFiltersUri, valuesBucket);
    if (ret < 0) {
        LOGE("Failed to add filters, ret: %{public}d", ret);
        return false;
    }
    return true;
}

static bool DiscardCameraPhotoExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_IS_TEMP, true);
    auto fileAsset = changeRequest->GetFileAssetInstance();
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileAsset->GetId()));
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, "1"); // only temp camera photo can be discarded

    std::string uri = CONST_PAH_DISCARD_CAMERA_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        LOGE("Failed to update property of asset, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetOrientationExecute(MediaAssetChangeRequestImpl* changeRequest)
{
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        LOGE("fileAsset is null");
        return false;
    }
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileAsset->GetId()));
    valuesBucket.Put(PhotoColumn::PHOTO_ORIENTATION, fileAsset->GetOrientation());
    return UpdateAssetProperty(changeRequest, CONST_PAH_UPDATE_PHOTO, predicates, valuesBucket);
}

static const std::unordered_map<AssetChangeOperation, bool (*)(MediaAssetChangeRequestImpl*)> EXECUTE_MAP = {
    { AssetChangeOperation::CREATE_FROM_URI, CreateFromFileUriExecute },
    { AssetChangeOperation::GET_WRITE_CACHE_HANDLER, SubmitCacheExecute },
    { AssetChangeOperation::ADD_RESOURCE, AddResourceExecute },
    { AssetChangeOperation::SET_TITLE, SetTitleExecute },
    { AssetChangeOperation::SET_ORIENTATION, SetOrientationExecute },
    { AssetChangeOperation::SAVE_CAMERA_PHOTO, SaveCameraPhotoExecute },
    { AssetChangeOperation::ADD_FILTERS, AddFiltersExecute },
    { AssetChangeOperation::DISCARD_CAMERA_PHOTO, DiscardCameraPhotoExecute },
};

int32_t MediaAssetChangeRequestImpl::ApplyChanges()
{
    if (!CheckChangeOperations()) {
        LOGE("Failed to check asset change request operations");
        return OHOS_INVALID_PARAM_CODE;
    }
    std::unordered_set<AssetChangeOperation> appliedOperations;
    for (const auto& changeOperation : assetChangeOperations_) {
        // Keep the final result of each operation, and commit it only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = false;
        auto iter = EXECUTE_MAP.find(changeOperation);
        if (iter != EXECUTE_MAP.end()) {
            valid = iter->second(this);
        } else if (changeOperation == AssetChangeOperation::CREATE_FROM_SCRATCH ||
                   changeOperation == AssetChangeOperation::SET_EDIT_DATA) {
            // Perform CREATE_FROM_SCRATCH and SET_EDIT_DATA during GET_WRITE_CACHE_HANDLER or ADD_RESOURCE.
            valid = true;
        } else {
            LOGE("Invalid asset change operation: %{public}d", changeOperation);
            assetChangeOperations_.clear();
            addResourceTypes_.clear();
            return OHOS_INVALID_PARAM_CODE;
        }

        if (!valid) {
            LOGE("Failed to apply asset change request, operation: %{public}d", changeOperation);
            assetChangeOperations_.clear();
            addResourceTypes_.clear();
            return 0;
        }
        appliedOperations.insert(changeOperation);
    }
    assetChangeOperations_.clear();
    addResourceTypes_.clear();
    return 0;
}
} // namespace Media
} // namespace OHOS