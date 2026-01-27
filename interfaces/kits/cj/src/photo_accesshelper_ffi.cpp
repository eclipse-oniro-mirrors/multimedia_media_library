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

#include "photo_accesshelper_ffi.h"
#include "photo_asset_helper.h"

using namespace OHOS::FFI;

namespace OHOS {
namespace Media {
extern "C" {

    char* FfiPhotoAssetGetFileDisplayName(int64_t id)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            return nullptr;
        }
        auto fileDisplayName = photoAssetImpl->GetFileDisplayName();
        return MallocCString(fileDisplayName);
    }

    char* FfiPhotoAssetGetFileUri(int64_t id)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            return nullptr;
        }
        auto fileUri = photoAssetImpl->GetFileUri();
        return MallocCString(fileUri);
    }

    int32_t FfiPhotoAssetGetMediaType(int64_t id)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            return 0;
        }
        return static_cast<int32_t>(photoAssetImpl->GetMediaType());
    }

    PhotoAssetMember FfiPhotoAssetUserFileMgrGet(int64_t id, char* member, int32_t *errCode)
    {
        PhotoAssetMember assetMember = {
            .memberType = -1,
            .stringValue = nullptr,
            .boolValue = false
        };
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return assetMember;
        }
        std::string inputKey(member);
        return photoAssetImpl->UserFileMgrGet(inputKey, *errCode);
    }

    void FfiPhotoAssetUserFileMgrSet(int64_t id, char* member, char* data, int32_t* errCode)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
        std::string inputKey(member);
        std::string value(data);
        photoAssetImpl->UserFileMgrSet(inputKey, value, *errCode);
    }

    void FfiPhotoAssetCommitModify(int64_t id, int32_t* errCode)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
        photoAssetImpl->CommitModify(*errCode);
    }

    int64_t FfiPhotoAssetGetThumbnail(int64_t id, CSize cSize, int32_t* errCode)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (photoAssetImpl == nullptr) {
            LOGE("Invalid object PhotoAssetImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return 0;
        }
        return photoAssetImpl->GetThumbnail(cSize, *errCode);
    }

    int32_t FfiFetchResultGetCount(int64_t id, int32_t* errCode)
    {
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return 0;
        }
        return fetchResultImpl->GetCount(*errCode);
    }

    bool FfiFetchResultIsAfterLast(int64_t id, int32_t* errCode)
    {
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return false;
        }
        return fetchResultImpl->IsAfterLast(*errCode);
    }

    void FfiFetchResultClose(int64_t id)
    {
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            return;
        }
        fetchResultImpl->Close();
    }

    FetchResultObject FfiFetchResultGetFirstObject(int64_t id, int32_t* errCode)
    {
        FetchResultObject fetchResultObject = {
            .id = -1,
            .fetchResType = 0
        };
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return fetchResultObject;
        }
        return fetchResultImpl->GetFirstObject(*errCode);
    }

    FetchResultObject FfiFetchResultGetNextObject(int64_t id, int32_t* errCode)
    {
        FetchResultObject fetchResultObject = {
            .id = -1,
            .fetchResType = 0
        };
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return fetchResultObject;
        }
        return fetchResultImpl->GetNextObject(*errCode);
    }

    FetchResultObject FfiFetchResultGetLastObject(int64_t id, int32_t* errCode)
    {
        FetchResultObject fetchResultObject = {
            .id = -1,
            .fetchResType = 0
        };
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return fetchResultObject;
        }
        return fetchResultImpl->GetLastObject(*errCode);
    }

    FetchResultObject FfiFetchResultGetObjectAtPosition(int64_t id, int32_t position, int32_t* errCode)
    {
        FetchResultObject fetchResultObject = {
            .id = -1,
            .fetchResType = 0
        };
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return fetchResultObject;
        }
        return fetchResultImpl->GetObjectAtPosition(position, *errCode);
    }

    CArrayFetchResultObject FfiFetchResultGetAllObjects(int64_t id, int32_t* errCode)
    {
        CArrayFetchResultObject cArrayFetchResultObject = {
            .head = nullptr,
            .size = 0
        };
        auto fetchResultImpl = FFIData::GetData<FetchResultImpl>(id);
        if (fetchResultImpl == nullptr) {
            LOGE("Invalid object FetchResultImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return cArrayFetchResultObject;
        }
        return fetchResultImpl->GetAllObjects(*errCode);
    }

    int32_t FfiPhotoAlbumGetPhotoAlbumType(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return 0;
        }
        return photoAlbumImpl->GetPhotoAlbumType();
    }

    int32_t FfiPhotoAlbumGetPhotoAlbumSubType(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return 0;
        }
        return photoAlbumImpl->GetPhotoAlbumSubType();
    }

    char* FfiPhotoAlbumGetAlbumName(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return nullptr;
        }
        auto albumName =  photoAlbumImpl->GetAlbumName();
        char* cAlbumName = MallocCString(albumName);
        return cAlbumName;
    }

    void FfiPhotoAlbumSetAlbumName(int64_t id, char* cAlbumName)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return;
        }
        photoAlbumImpl->SetAlbumName(cAlbumName);
    }

    char* FfiPhotoAlbumGetAlbumUri(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return nullptr;
        }
        auto albumUri = photoAlbumImpl->GetAlbumUri();
        char* cAlbumUri = MallocCString(albumUri);
        return cAlbumUri;
    }

    int32_t FfiPhotoAlbumGetCount(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return 0;
        }
        return photoAlbumImpl->GetCount();
    }

    char* FfiPhotoAlbumGetCoverUri(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return nullptr;
        }
        auto coverUri = photoAlbumImpl->GetCoverUri();
        char* cCoverUri = MallocCString(coverUri);
        return cCoverUri;
    }

    int32_t FfiPhotoAlbumGetImageCount(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return 0;
        }
        return photoAlbumImpl->GetImageCount();
    }

    int32_t FfiPhotoAlbumGetVideoCount(int64_t id)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            return 0;
        }
        return photoAlbumImpl->GetVideoCount();
    }

    int64_t FfiPhotoAlbumGetAssets(int64_t id, COptions options, int32_t* errCode)
    {
        int64_t result = -1;
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            *errCode = OHOS_INVALID_PARAM_CODE;
            return result;
        }
        auto fetchFileResult = photoAlbumImpl->GetAssets(options, *errCode);
        if (fetchFileResult == nullptr) {
            LOGE("GetAssets failed.");
            return result;
        }
        std::unique_ptr<FfiFetchResultProperty> propertyPtr = std::make_unique<FfiFetchResultProperty>();
        propertyPtr->fetchFileResult_ = fetchFileResult;
        propertyPtr->fetchResType_ = FetchResType::TYPE_FILE;
        auto native = FFIData::Create<FetchResultImpl>(std::move(propertyPtr));
        if (native != nullptr) {
            result = native->GetID();
        } else {
            LOGE("Create FetchResultImpl instance failed.");
            *errCode = JS_INNER_FAIL;
        }
        return result;
    }

    void FfiPhotoAlbumCommitModify(int64_t id, int32_t* errCode)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (photoAlbumImpl == nullptr) {
            LOGE("Invalid object photoAlbumImpl");
            *errCode = OHOS_INVALID_PARAM_CODE;
            return;
        }
        photoAlbumImpl->CommitModify(*errCode);
    }

    int64_t FfiPhotoAccessHelperGetPhotoAccessHelper(int64_t id)
    {
        int64_t result = -1;
        auto ret = FFIData::Create<PhotoAccessHelperImpl>();
        if (!ret) {
            LOGE("creat PhotoAccessHelperImpl instance failed.");
            return result;
        }
        if (!ret->GetPhotoAccessHelper(id)) {
            LOGE("GetPhotoAccessHelper failed.");
            return result;
        }
        result = ret->GetID();
        return result;
    }

    int64_t FfiPhotoAccessHelperGetAssets(int64_t id, COptions options, int32_t* errCode)
    {
        int64_t result = -1;
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return result;
        }
        auto fetchFileResult = photoAccessHelperImpl->GetAssets(options, *errCode);
        if (fetchFileResult == nullptr) {
            LOGE("GetAssets failed.");
            return result;
        }
        std::unique_ptr<FfiFetchResultProperty> propertyPtr = std::make_unique<FfiFetchResultProperty>();
        propertyPtr->fetchFileResult_ = fetchFileResult;
        propertyPtr->fetchResType_ = FetchResType::TYPE_FILE;
        auto native = FFIData::Create<FetchResultImpl>(std::move(propertyPtr));
        if (native != nullptr) {
            result = native->GetID();
        } else {
            LOGE("Create FetchResultImpl instance failed.");
            *errCode = JS_INNER_FAIL;
        }
        return result;
    }

    int64_t FfiPhotoAccessHelperGetBurstAssets(int64_t id, char* cBurstKey, COptions options, int32_t* errCode)
    {
        int64_t result = -1;
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return result;
        }
        auto fetchFileResult = photoAccessHelperImpl->GetBurstAssets(cBurstKey, options, *errCode);
        if (fetchFileResult == nullptr) {
            LOGE("GetAssets failed.");
            return result;
        }
        std::unique_ptr<FfiFetchResultProperty> propertyPtr = std::make_unique<FfiFetchResultProperty>();
        propertyPtr->fetchFileResult_ = fetchFileResult;
        propertyPtr->fetchResType_ = FetchResType::TYPE_FILE;
        auto native = FFIData::Create<FetchResultImpl>(std::move(propertyPtr));
        if (native != nullptr) {
            result = native->GetID();
        } else {
            LOGE("Create FetchResultImpl instance failed.");
            *errCode = JS_INNER_FAIL;
        }
        return result;
    }

    int64_t FfiPhotoAccessHelperGetAlbums(int64_t id, int32_t type, int32_t subtype,
        COptions options, int32_t* errCode)
    {
        int64_t result = -1;
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return result;
        }
        auto fetchPhotoAlbumResult = photoAccessHelperImpl->GetAlbums(type, subtype, options, *errCode);
        if (fetchPhotoAlbumResult == nullptr) {
            LOGE("GetAlbums failed.");
            return result;
        }
        std::unique_ptr<FfiFetchResultProperty> propertyPtr = std::make_unique<FfiFetchResultProperty>();
        propertyPtr->fetchPhotoAlbumResult_ = fetchPhotoAlbumResult;
        propertyPtr->fetchResType_ = FetchResType::TYPE_PHOTOALBUM;
        auto native = FFIData::Create<FetchResultImpl>(std::move(propertyPtr));
        if (native != nullptr) {
            result = native->GetID();
        } else {
            LOGE("Create FetchResultImpl instance failed.");
            *errCode = JS_INNER_FAIL;
        }
        return result;
    }

    void FfiPhotoAccessHelperRegisterChange(int64_t id, char* uri,
        bool forChildUris, int64_t funcId, int32_t *errCode)
    {
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
        photoAccessHelperImpl->RegisterChange(uri, forChildUris, funcId, *errCode);
    }

    void FfiPhotoAccessHelperUnRegisterChange(int64_t id, char* uri, int64_t funcId, int32_t *errCode)
    {
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
        photoAccessHelperImpl->UnRegisterChange(uri, funcId);
    }

    void FfiPhotoAccessHelperRelease(int64_t id, int32_t *errCode)
    {
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
        photoAccessHelperImpl->Release();
    }

    void FfiPhotoAccessHelperShowAssetsCreationDialog(int64_t id, CArrString srcFileUris,
        PhotoCreationConfigs photoCreationConfigs, int64_t funcId, FfiBundleInfo cBundleInfo, int32_t *errCode)
    {
        auto photoAccessHelperImpl = FFIData::GetData<PhotoAccessHelperImpl>(id);
        if (photoAccessHelperImpl == nullptr) {
            LOGE("Invalid object photoAccessHelperImpl");
            *errCode = JS_ERR_PARAMETER_INVALID;
            return;
        }
        photoAccessHelperImpl->ShowAssetsCreationDialog(srcFileUris,
            photoCreationConfigs, funcId, cBundleInfo, *errCode);
    }

    PhotoSelectResult FfiPhotoAccessHelperStartPhotoPicker(int64_t id,
        PhotoSelectOptions option, int32_t *errCode)
    {
        return PhotoAccessHelperImpl::StartPhotoPicker(id, option, *errCode);
    }

    FFI_EXPORT PhotoSelectResult FfiPhotoAccessHelperStartPhotoPickerV2(
        int64_t context, PhotoSelectOptions option, int32_t* errCode)
    {
        return PhotoAccessHelperImpl::StartPhotoPickerV2(context, option, *errCode);
    }

    char* FfiMovingPhotoGetUri(int64_t id, int32_t* errCode)
    {
        auto ffiMovingPhotoImpl = FFIData::GetData<FfiMovingPhotoImpl>(id);
        if (ffiMovingPhotoImpl == nullptr) {
            LOGE("Invalid object movingPhotoImpl");
            *errCode = JS_INNER_FAIL;
            return nullptr;
        }
        auto movingPhotoUri = ffiMovingPhotoImpl->GetUri();
        char* uri = MallocCString(movingPhotoUri);
        return uri;
    }

    void FfiMovingPhotoRequestContentUri(int64_t id,
        char* imageFileUri, char* videoFileUri, int32_t* errCode)
    {
        auto ffiMovingPhotoImpl = FFIData::GetData<FfiMovingPhotoImpl>(id);
        if (ffiMovingPhotoImpl == nullptr) {
            LOGE("Invalid object movingPhotoImpl");
            *errCode = JS_INNER_FAIL;
            return;
        }
        ffiMovingPhotoImpl->RequestContent(imageFileUri, videoFileUri, *errCode);
    }

    void FfiMovingPhotoRequestContentResourceType(int64_t id,
        int32_t resourceType, char* fileUri, int32_t* errCode)
    {
        auto ffiMovingPhotoImpl = FFIData::GetData<FfiMovingPhotoImpl>(id);
        if (ffiMovingPhotoImpl == nullptr) {
            LOGE("Invalid object movingPhotoImpl");
            *errCode = JS_INNER_FAIL;
            return;
        }
        ffiMovingPhotoImpl->RequestContent(resourceType, fileUri, *errCode);
    }

    CArrUI8 FfiMovingPhotoRequestContentArrayBuffer(int64_t id,
        int32_t resourceType, int32_t* errCode)
    {
        CArrUI8 result = { .head = nullptr, .size = 0};
        auto ffiMovingPhotoImpl = FFIData::GetData<FfiMovingPhotoImpl>(id);
        if (ffiMovingPhotoImpl == nullptr) {
            LOGE("Invalid object movingPhotoImpl");
            *errCode = JS_INNER_FAIL;
            return result;
        }
        return ffiMovingPhotoImpl->RequestContent(resourceType, *errCode);
    }

    char* FfiMediaAssetManagerRequestImage(int64_t contextId, int64_t photoAssetId,
        RequestOptions requestOptions, int64_t funcId, int32_t* errCode)
    {
        return MediaAssetManagerImpl::RequestImage(contextId, photoAssetId,
            requestOptions, funcId, *errCode);
    }

    char* FfiMediaAssetManagerRequestImageData(int64_t contextId, int64_t photoAssetId,
        RequestOptions requestOptions, int64_t funcId, int32_t* errCode)
    {
        return MediaAssetManagerImpl::RequestImageData(contextId, photoAssetId,
            requestOptions, funcId, *errCode);
    }

    char* FfiMediaAssetManagerRequestMovingPhoto(int64_t contextId, int64_t photoAssetId,
        RequestOptions requestOptions, int64_t funcId, int32_t* errCode)
    {
        return MediaAssetManagerImpl::RequestMovingPhoto(contextId, photoAssetId,
            requestOptions, funcId, *errCode);
    }

    char* FfiMediaAssetManagerRequestVideoFile(int64_t contextId, int64_t photoAssetId,
        RequestOptions requestOptions, char* fileUri, int64_t funcId, int32_t* errCode)
    {
        return MediaAssetManagerImpl::RequestVideoFile(contextId, photoAssetId,
            requestOptions, fileUri, funcId, *errCode);
    }

    void FfiMediaAssetManagerCancelRequest(int64_t contextId, char* cRequestId, int32_t* errCode)
    {
        MediaAssetManagerImpl::CancelRequest(contextId, cRequestId, *errCode);
    }

    int64_t FfiMediaAssetManagerLoadMovingPhoto(int64_t contextId, char* cImageFileUri,
        char* cVideoFileUri, int32_t* errCode)
    {
        return MediaAssetManagerImpl::LoadMovingPhoto(contextId, cImageFileUri, cVideoFileUri, *errCode);
    }

    // MediaAssetChangeRequest

    int64_t FfiMediaAssetChangeRequestImplConstructor(int64_t id, int32_t* errCode)
    {
        auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(id);
        if (!photoAssetImpl) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        auto fileAssetPtr = photoAssetImpl->GetFileAssetInstance();
        if (fileAssetPtr == nullptr) {
            *errCode = OHOS_INVALID_PARAM_CODE;
            LOGE("fileAsset is null");
            return 0;
        }
        if (fileAssetPtr->GetMediaType() != MEDIA_TYPE_IMAGE && fileAssetPtr->GetMediaType() != MEDIA_TYPE_VIDEO) {
            LOGE("Unsupported type of fileAsset");
            *errCode = OHOS_INVALID_PARAM_CODE;
            return 0;
        }
        auto changeRequest = FFIData::Create<MediaAssetChangeRequestImpl>(fileAssetPtr);
        if (!changeRequest) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        return changeRequest->GetID();
    }

    int64_t FfiMediaAssetChangeRequestImplCreateImageAssetRequest(int64_t id, char* fileUri, int32_t* errCode)
    {
        std::string filePath(fileUri);
        auto changeRequest =
            FFIData::Create<MediaAssetChangeRequestImpl>(id, filePath, MediaType::MEDIA_TYPE_IMAGE, errCode);
        if (!changeRequest) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        return changeRequest->GetID();
    }

    int64_t FfiMediaAssetChangeRequestImplCreateVideoAssetRequest(int64_t id, char* fileUri, int32_t* errCode)
    {
        std::string filePath(fileUri);
        auto changeRequest =
            FFIData::Create<MediaAssetChangeRequestImpl>(id, filePath, MediaType::MEDIA_TYPE_VIDEO, errCode);
        if (!changeRequest) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        return changeRequest->GetID();
    }

    RetDataI64 FfiMediaAssetChangeRequestImplCreateAssetRequest(
        int64_t id, int32_t photoType, char* extension, char* title, int32_t subType)
    {
        RetDataI64 ret = {};
        auto changeRequest =
            FFIData::Create<MediaAssetChangeRequestImpl>(id, photoType, extension, title, subType, &ret.code);
        if (!changeRequest) {
            ret.code = JS_INNER_FAIL;
            return ret;
        }
        ret.data = changeRequest->GetID();
        return ret;
    }

    static std::string GetUriFromAsset(const OHOS::sptr<PhotoAssetImpl> obj)
    {
        std::string displayName = obj->GetFileDisplayName();
        // todo
        std::string filePath = obj->GetFileAssetInstance()->GetFilePath();
        return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, std::to_string(obj->GetFileId()),
            MediaFileUtils::GetExtraUri(displayName, filePath));
    }

    static int32_t ParseAssetArray(CArrI64& assets, std::vector<std::string>& uriArray)
    {
        if (assets.size == 0) {
            LOGE("array is empty");
            return OHOS_INVALID_PARAM_CODE;
        }
        for (size_t i = 0; i < static_cast<size_t>(assets.size); i++) {
            auto photoAssetImpl = FFIData::GetData<PhotoAssetImpl>(assets.head[i]);
            if (!photoAssetImpl) {
                return JS_ERR_PARAMETER_INVALID;
            }
            if ((photoAssetImpl->GetMediaType() != MEDIA_TYPE_IMAGE &&
                    photoAssetImpl->GetMediaType() != MEDIA_TYPE_VIDEO)) {
                LOGE("Skip invalid asset, mediaType: %{public}d", photoAssetImpl->GetMediaType());
                continue;
            }
            uriArray.push_back(GetUriFromAsset(photoAssetImpl));
        }
        return 0;
    }

    int32_t FfiMediaAssetChangeRequestImplDeleteAssetsByObject(int64_t id, CArrI64 assets)
    {
        if (assets.size == 0) {
            LOGE("array is empty");
            return OHOS_INVALID_PARAM_CODE;
        }
        std::vector<std::string> uris;
        if (ParseAssetArray(assets, uris) != 0) {
            return OHOS_INVALID_PARAM_CODE;
        }
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJDeleteAssets(id, uris);
    }

    int32_t FfiMediaAssetChangeRequestImplDeleteAssetsByString(int64_t id, CArrString assets)
    {
        if (assets.size == 0) {
            LOGE("array is empty");
            return OHOS_INVALID_PARAM_CODE;
        }
        std::vector<std::string> uris;
        for (int64_t i = 0; i < assets.size; i++) {
            uris.push_back(assets.head[i]);
        }
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJDeleteAssets(id, uris);
    }

    int64_t FfiMediaAssetChangeRequestImplGetAsset(int64_t id, int32_t* errCode)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJGetAsset(errCode);
    }

    int32_t FfiMediaAssetChangeRequestImplSetTitle(int64_t id, char* title)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJSetTitle(title);
    }

    int32_t FfiMediaAssetChangeRequestImplGetWriteCacheHandler(int64_t id, int32_t* errCode)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJGetWriteCacheHandler(errCode);
    }

    int32_t FfiMediaAssetChangeRequestImplAddResourceByString(int64_t id, int32_t resourceType, char* fileUri)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJAddResource(resourceType, fileUri);
    }

    int32_t FfiMediaAssetChangeRequestImplAddResourceByBuffer(int64_t id, int32_t resourceType, CArrUI8 data)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJAddResource(resourceType, data.head, data.size);
    }

    int32_t FfiMediaAssetChangeRequestImplSaveCameraPhoto(int64_t id)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJSaveCameraPhoto();
    }

    int32_t FfiMediaAssetChangeRequestImplDiscardCameraPhoto(int64_t id)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJDiscardCameraPhoto();
    }

    int32_t FfiMediaAssetChangeRequestImplSetOrientation(int64_t id, int32_t orientation)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJSetOrientation(orientation);
    }

    int32_t FfiMediaAssetChangeRequestImplApplyChanges(int64_t id)
    {
        auto changeRequest = FFIData::GetData<MediaAssetChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->ApplyChanges();
    }

    // MediaAlbumChangeRequest

    int64_t FfiMediaAlbumChangeRequestImplConstructor(int64_t id, int32_t* errCode)
    {
        auto photoAlbumImpl = FFIData::GetData<PhotoAlbumImpl>(id);
        if (!photoAlbumImpl) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        auto photoAlbumPtr = photoAlbumImpl->GetPhotoAlbumInstance();
        if (photoAlbumPtr == nullptr) {
            LOGE("photoAlbum is null");
            *errCode = OHOS_INVALID_PARAM_CODE;
            return 0;
        }
        if (!(photoAlbumPtr->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER &&
                PhotoAlbum::CheckPhotoAlbumType(photoAlbumPtr->GetPhotoAlbumType()) &&
                PhotoAlbum::CheckPhotoAlbumSubType(photoAlbumPtr->GetPhotoAlbumSubType()))) {
            LOGE("Unsupported type of photoAlbum");
            *errCode = OHOS_INVALID_PARAM_CODE;
            return 0;
        }
        auto changeRequest = FFIData::Create<MediaAlbumChangeRequestImpl>(photoAlbumPtr);
        if (!changeRequest) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        return changeRequest->GetID();
    }

    int64_t FfiMediaAlbumChangeRequestImplGetAlbum(int64_t id, int32_t* errCode)
    {
        auto changeRequest = FFIData::GetData<MediaAlbumChangeRequestImpl>(id);
        if (!changeRequest) {
            *errCode = OHOS_INVALID_PARAM_CODE;
            return 0;
        }
        return changeRequest->CJGetAlbum(errCode);
    }

    int32_t FfiMediaAlbumChangeRequestImplSetAlbumName(int64_t id, char* albumName)
    {
        auto changeRequest = FFIData::GetData<MediaAlbumChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJSetAlbumName(albumName);
    }

    int32_t FfiMediaAlbumChangeRequestImplAddAssets(int64_t id, CArrI64 assets)
    {
        auto changeRequest = FFIData::GetData<MediaAlbumChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        vector<string> assetUriArray;
        if (ParseAssetArray(assets, assetUriArray) != 0) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJAddAssets(assetUriArray);
    }

    int32_t FfiMediaAlbumChangeRequestImplRemoveAssets(int64_t id, CArrI64 assets)
    {
        auto changeRequest = FFIData::GetData<MediaAlbumChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        vector<string> assetUriArray;
        if (ParseAssetArray(assets, assetUriArray) != 0) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->CJRemoveAssets(assetUriArray);
    }

    int32_t FfiMediaAlbumChangeRequestImplApplyChanges(int64_t id)
    {
        auto changeRequest = FFIData::GetData<MediaAlbumChangeRequestImpl>(id);
        if (!changeRequest) {
            return OHOS_INVALID_PARAM_CODE;
        }
        return changeRequest->ApplyChanges();
    }
}

enum class CJCameraShotType : int32_t {
    IMAGE = 0,
    VIDEO,
    MOVING_PHOTO,
    BURST,
};

int64_t CreatePhotoAssetImpl(const std::string &uri, int32_t cameraShotType, const std::string &burstKey)
{
    if (uri.empty()) {
        return 0;
    }
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    fileAsset->SetUri(uri);
    std::string fileId = MediaFileUtils::GetIdFromUri(uri);
    if (MediaFileUtils::IsValidInteger(fileId)) {
        fileAsset->SetId(stoi(fileId));
    }

    fileAsset->SetDisplayName(MediaFileUtils::GetFileName(uri));
    if (cameraShotType == static_cast<int32_t>(CJCameraShotType::IMAGE)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CJCameraShotType::MOVING_PHOTO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CJCameraShotType::BURST)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::BURST));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_IMAGE);
        fileAsset->SetBurstKey(burstKey);
    } else if (cameraShotType == static_cast<int32_t>(CJCameraShotType::VIDEO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(MediaType::MEDIA_TYPE_VIDEO);
    }
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    auto photoAssetImpl = FFIData::Create<PhotoAssetImpl>(fileAsset);
    return photoAssetImpl->GetID();
}
}
}